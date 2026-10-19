//
//  SpectrumView.swift
//  Cog
//
//  The spectrum, as SpectrumViewCG.m draws it: the audio the engine posts
//  to VisualizationController, through DeaDBeeF's analyzer, bars with
//  falling peaks. iOS's Now Playing and the macOS spectrum window share it.
//

import CogAudio
import SwiftUI

/// The analyzer's state from one frame to the next.
private final class Analyzer {
	private var analyzer = ddb_analyzer_t()
	private var drawData = ddb_analyzer_draw_data_t()
	private var pcm = [Float](repeating: 0, count: 4096)
	private var fft = [Float](repeating: 0, count: 2048)
	private var fftState: UnsafeMutableRawPointer?
	private let controller = VisualizationController.shared()
	/// How far past the newest audio posted the display has got: the
	/// engine posts in blocks, and frames fall between them.
	private var latencyOffset = 0.0
	private var lastPosted: UInt64 = 0
	private var lastFrame: Date?

	/// The analyzer's floor, in dB; the full view's grid steps down to it.
	static let lowerBound: Float = -80

	init() {
		ddb_analyzer_init(&analyzer)
		analyzer.db_lower_bound = Self.lowerBound
		analyzer.min_freq = 10
		analyzer.max_freq = 22000
		analyzer.peak_hold = 10
		analyzer.view_width = 64
		analyzer.fractional_bars = 1
		analyzer.octave_bars_step = 2
		analyzer.max_of_stereo_data = 1
		analyzer.freq_is_log = 0
		analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS
	}

	deinit {
		controller.freeFFTState(&fftState)
		ddb_analyzer_dealloc(&analyzer)
		ddb_analyzer_draw_data_dealloc(&drawData)
	}

	/// Advances to `date` and draws into `context`, bars from the bottom.
	func draw(in context: inout GraphicsContext, size: CGSize, at date: Date, style: SpectrumView.Style) {
		let mode = style.frequencyMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS
		if analyzer.mode != mode {
			analyzer.mode = mode
			analyzer.mode_did_change = 1
		}
		analyzer.freq_is_log = style.isFullView ? 1 : 0
		analyzer.view_width = style.isFullView ? Int32(size.width) : 64

		let posted = controller.samplesPosted()
		if posted != lastPosted {
			lastPosted = posted
			latencyOffset = 0
		} else if let lastFrame {
			latencyOffset -= date.timeIntervalSince(lastFrame)
		}
		lastFrame = date

		controller.copyVisPCM(&pcm, visFFT: &fft, visFFTState: &fftState, latencyOffset: latencyOffset)
		ddb_analyzer_process(&analyzer, Int32(controller.readSampleRate() / 2), 1, &fft, 2048)
		ddb_analyzer_tick(&analyzer)
		ddb_analyzer_get_draw_data(&analyzer, Int32(size.width), Int32(size.height), &drawData)

		if style.isFullView {
			drawGrid(in: &context, size: size)
			drawFrequencyLabels(in: &context)
		}

		guard let bars = drawData.bars else { return }
		let drawn = UnsafeBufferPointer(start: bars, count: Int(drawData.bar_count))
		var body = Path()
		var peaks = Path()
		if style.frequencyMode {
			// A line for each frequency, and a dot for its peak.
			for bar in drawn {
				let x = CGFloat(bar.xpos)
				body.move(to: CGPoint(x: x, y: size.height))
				body.addLine(to: CGPoint(x: x, y: size.height - CGFloat(bar.bar_height)))
				peaks.move(to: CGPoint(x: x - 0.5, y: size.height - CGFloat(bar.peak_ypos)))
				peaks.addLine(to: CGPoint(x: x + 0.5, y: size.height - CGFloat(bar.peak_ypos)))
			}
			context.stroke(body, with: .color(style.barColor), lineWidth: 1)
			context.stroke(peaks, with: .color(style.peakColor), lineWidth: 1)
		} else {
			let width = CGFloat(drawData.bar_width)
			for bar in drawn {
				let x = CGFloat(bar.xpos)
				body.addRect(CGRect(x: x, y: size.height - CGFloat(bar.bar_height), width: width, height: CGFloat(bar.bar_height)))
				peaks.addRect(CGRect(x: x, y: size.height - CGFloat(bar.peak_ypos) - 1, width: width, height: 1.5))
			}
			context.fill(body, with: .color(style.barColor))
			context.fill(peaks, with: .color(style.peakColor))
		}
	}

	/// Dotted lines every 10 dB, labelled, from the top down to the floor.
	private func drawGrid(in context: inout GraphicsContext, size: CGSize) {
		let lower = -Self.lowerBound.rounded(.down)
		var lines = Path()
		for db in stride(from: Float(10), to: lower, by: 10) {
			let y = CGFloat(db / lower) * size.height
			guard y < size.height else { break }
			lines.move(to: CGPoint(x: 0, y: y))
			lines.addLine(to: CGPoint(x: size.width - 1, y: y))
			context.draw(Text(verbatim: "\(-Int(db)) dB").font(.system(size: 10)).foregroundColor(.gray),
			             at: CGPoint(x: 0, y: y), anchor: .topLeading)
		}
		context.stroke(lines, with: .color(.gray), style: StrokeStyle(lineWidth: 1, dash: [1, 2]))
	}

	/// The octaves' frequencies, along the top.
	private func drawFrequencyLabels(in context: inout GraphicsContext) {
		let count = Int(drawData.label_freq_count)
		let positions = withUnsafeBytes(of: drawData.label_freq_positions) { Array($0.bindMemory(to: Float.self).prefix(count)) }
		let texts = withUnsafeBytes(of: drawData.label_freq_texts) { bytes in
			// Four bytes each, NUL-terminated within them.
			(0..<count).map { index in
				String(decoding: bytes[(index * 4)..<(index * 4 + 4)].prefix { $0 != 0 }, as: UTF8.self)
			}
		}
		for (position, text) in zip(positions, texts) where position >= 0 {
			context.draw(Text(verbatim: text).font(.system(size: 10)).foregroundColor(.gray),
			             at: CGPoint(x: CGFloat(position), y: 0), anchor: .topLeading)
		}
	}
}

struct SpectrumView: View {
	struct Style {
		var barColor: Color
		var peakColor: Color
		/// The spectrum window's: the whole width on a logarithmic scale,
		/// with the decibel grid and the octaves' frequencies.
		var isFullView: Bool
		/// Single frequencies as lines, instead of octave note bands.
		var frequencyMode: Bool
	}

	/// Still while paused: no frames are drawn then.
	var isPlaying: Bool
	var style: Style
	@State private var analyzer = Analyzer()

	init(isPlaying: Bool, barColor: Color, peakColor: Color = .white, isFullView: Bool = false, frequencyMode: Bool = false) {
		self.isPlaying = isPlaying
		style = Style(barColor: barColor, peakColor: peakColor, isFullView: isFullView, frequencyMode: frequencyMode)
	}

	var body: some View {
		TimelineView(.animation(minimumInterval: 1.0 / 60.0, paused: !isPlaying)) { timeline in
			Canvas { context, size in
				analyzer.draw(in: &context, size: size, at: timeline.date, style: style)
			}
		}
		.accessibilityHidden(true)
	}
}
