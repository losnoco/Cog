//
//  WaveformView.swift
//  Cog
//
//  The audio the engine posts to VisualizationController, folded to mono,
//  as it is heard: an oscilloscope trace held still on rising zero
//  crossings, or the last few seconds' envelope scrolling past. iOS's Now
//  Playing and the macOS waveform pane share it.
//

import Accelerate
import CogAudio
import SwiftUI

/// The samples and timing kept from one frame to the next.
private final class WaveformSampler {
	private let controller = VisualizationController.shared()
	private var samples: [Float] = []
	/// How far past the newest audio posted the display has got: the
	/// engine posts in blocks, and frames fall between them.
	private var latencyOffset = 0.0
	private var lastPosted: UInt64 = 0
	private var lastFrame: Date?

	/// What the oscilloscope reads, and how much of it it shows: about 46 ms.
	static let scopeWindow = 4096
	static let scopeSpan = 2048
	/// The envelope's length.
	static let envelopeSeconds = 4.0

	func draw(in context: inout GraphicsContext, size: CGSize, at date: Date, style: WaveformView.Style) {
		let posted = controller.samplesPosted()
		if posted != lastPosted {
			lastPosted = posted
			latencyOffset = 0
		} else if let lastFrame {
			latencyOffset -= date.timeIntervalSince(lastFrame)
		}
		lastFrame = date

		let mid = size.height / 2
		if style.showsCenterLine {
			var line = Path()
			line.move(to: CGPoint(x: 0, y: mid))
			line.addLine(to: CGPoint(x: size.width, y: mid))
			context.stroke(line, with: .color(.gray.opacity(0.4)), lineWidth: 1)
		}

		switch style.mode {
		case .oscilloscope:
			read(Self.scopeWindow)
			context.stroke(scopePath(size: size), with: .color(style.lineColor), lineWidth: 1.5)
		case .envelope:
			read(Int(controller.readSampleRate() * Self.envelopeSeconds))
			context.fill(envelopePath(size: size), with: .color(style.lineColor))
		}
	}

	/// The `count` samples ending at what is heard, oldest first, with
	/// silence before any the controller doesn't have.
	private func read(_ count: Int) {
		if samples.count != count {
			samples = [Float](repeating: 0, count: count)
		}
		let copied = samples.withUnsafeMutableBufferPointer { buffer in
			Int(controller.copyHeardPCM(buffer.baseAddress!, count: Int32(count), latencyOffset: latencyOffset))
		}
		if copied < count {
			samples.withUnsafeMutableBufferPointer { buffer in
				let base = buffer.baseAddress!
				if copied > 0 {
					(base + (count - copied)).update(from: base, count: copied)
				}
				base.update(repeating: 0, count: count - copied)
			}
		}
	}

	/// Where the trace starts: the latest rising zero crossing that leaves a
	/// whole span after it, coming up from below a small threshold so noise
	/// around zero doesn't move it. Without one, the span that ends now.
	private func trigger() -> Int {
		let latest = samples.count - Self.scopeSpan
		let threshold: Float = 0.01
		var index = latest
		while index > 0 {
			if samples[index - 1] < 0, samples[index] >= 0 {
				// Was it below the threshold within the last few hundred samples?
				let from = max(0, index - 512)
				if samples[from..<index].contains(where: { $0 < -threshold }) {
					return index
				}
			}
			index -= 1
		}
		return latest
	}

	private func scopePath(size: CGSize) -> Path {
		let start = trigger()
		let mid = size.height / 2
		let columns = max(Int(size.width), 1)
		let perColumn = Double(Self.scopeSpan) / Double(columns)
		var path = Path()
		if perColumn <= 1 {
			// Fewer samples than pixels: a line through each.
			let step = size.width / CGFloat(Self.scopeSpan - 1)
			for i in 0..<Self.scopeSpan {
				let point = CGPoint(x: CGFloat(i) * step, y: y(samples[start + i], mid: mid))
				i == 0 ? path.move(to: point) : path.addLine(to: point)
			}
		} else {
			// More: each column's lowest and highest, in the order they came,
			// so loud high notes draw solid rather than aliased.
			samples.withUnsafeBufferPointer { buffer in
				for column in 0..<columns {
					let lower = start + Int(Double(column) * perColumn)
					let upper = start + min(Int(Double(column + 1) * perColumn), Self.scopeSpan)
					guard upper > lower else { continue }
					let slice = UnsafeBufferPointer(rebasing: buffer[lower..<upper])
					var minimum: Float = 0, minIndex: vDSP_Length = 0, maximum: Float = 0, maxIndex: vDSP_Length = 0
					vDSP_minvi(slice.baseAddress!, 1, &minimum, &minIndex, vDSP_Length(slice.count))
					vDSP_maxvi(slice.baseAddress!, 1, &maximum, &maxIndex, vDSP_Length(slice.count))
					let x = CGFloat(column) + 0.5
					let first = CGPoint(x: x, y: y(minIndex < maxIndex ? minimum : maximum, mid: mid))
					let second = CGPoint(x: x, y: y(minIndex < maxIndex ? maximum : minimum, mid: mid))
					column == 0 ? path.move(to: first) : path.addLine(to: first)
					path.addLine(to: second)
				}
			}
		}
		return path
	}

	/// Each column's highest along the top, its lowest back along the bottom.
	private func envelopePath(size: CGSize) -> Path {
		let mid = size.height / 2
		let columns = max(Int(size.width), 1)
		let perColumn = Double(samples.count) / Double(columns)
		var highs = [CGFloat](repeating: mid, count: columns)
		var lows = [CGFloat](repeating: mid, count: columns)
		samples.withUnsafeBufferPointer { buffer in
			for column in 0..<columns {
				let lower = Int(Double(column) * perColumn)
				let upper = min(Int(Double(column + 1) * perColumn), buffer.count)
				guard upper > lower else { continue }
				var minimum: Float = 0, maximum: Float = 0
				vDSP_minv(buffer.baseAddress! + lower, 1, &minimum, vDSP_Length(upper - lower))
				vDSP_maxv(buffer.baseAddress! + lower, 1, &maximum, vDSP_Length(upper - lower))
				// At least a hairline, so silence still shows where it is.
				highs[column] = min(y(maximum, mid: mid), mid - 0.5)
				lows[column] = max(y(minimum, mid: mid), mid + 0.5)
			}
		}
		var path = Path()
		path.move(to: CGPoint(x: 0, y: highs[0]))
		for column in 0..<columns {
			path.addLine(to: CGPoint(x: CGFloat(column) + 1, y: highs[column]))
		}
		for column in stride(from: columns - 1, through: 0, by: -1) {
			path.addLine(to: CGPoint(x: CGFloat(column), y: lows[column]))
		}
		path.closeSubpath()
		return path
	}

	/// Full scale reaches the edges; louder is clipped.
	private func y(_ sample: Float, mid: CGFloat) -> CGFloat {
		mid - CGFloat(min(max(sample, -1), 1)) * mid
	}
}

struct WaveformView: View {
	enum Mode: String, CaseIterable {
		/// A trace of the last 46 ms, held still on rising zero crossings.
		case oscilloscope
		/// The last few seconds' peaks, scrolling left.
		case envelope
	}

	struct Style {
		var lineColor: Color
		var mode: Mode
		/// A faint line at zero, for the pane.
		var showsCenterLine: Bool
	}

	/// Still while paused: no frames are drawn then.
	var isPlaying: Bool
	var style: Style
	@State private var sampler = WaveformSampler()

	init(isPlaying: Bool, lineColor: Color, mode: Mode = .oscilloscope, showsCenterLine: Bool = false) {
		self.isPlaying = isPlaying
		style = Style(lineColor: lineColor, mode: mode, showsCenterLine: showsCenterLine)
	}

	var body: some View {
		TimelineView(.animation(minimumInterval: 1.0 / 60.0, paused: !isPlaying)) { timeline in
			Canvas { context, size in
				sampler.draw(in: &context, size: size, at: timeline.date, style: style)
			}
		}
		.accessibilityHidden(true)
	}
}
