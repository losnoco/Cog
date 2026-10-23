//
//  VUMeterView.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-22.
//

import CogAudio
import SwiftUI

/// What the VU meter shows from the output's levels: a needle each for left
/// and right that moves like a real VU meter's, and the Signal Inspector's
/// bars.
@MainActor
final class VUMeterModel: ObservableObject {
	/// A needle with VU ballistics: a damped spring that reaches 99% of a
	/// step in about 300 ms and overshoots it by about 1.5%.
	struct Needle {
		/// Along the scale: 0 at rest, 1 at +3 VU.
		var position = 0.0
		var velocity = 0.0

		private static let damping = 0.8
		/// Radians per second.
		private static let frequency = 19.0

		mutating func advance(toward target: Double, by seconds: Double) {
			// Short steps keep the spring steady when frames come late.
			var remaining = min(seconds, 0.1)
			while remaining > 0 {
				let step = min(remaining, 1.0 / 240)
				let acceleration = Self.frequency * Self.frequency * (target - position) - 2 * Self.damping * Self.frequency * velocity
				velocity += acceleration * step
				position += velocity * step
				remaining -= step
			}
			// The needle stops against its pins.
			if position < -0.02 {
				position = -0.02
				velocity = max(0, velocity)
			} else if position > 1.1 {
				position = 1.1
				velocity = min(0, velocity)
			}
		}
	}

	let bars = SignalMetricsModel()
	@Published private(set) var labels = ["L", "R"]
	/// Levels are coming in, or the needles have yet to come to rest.
	@Published private(set) var isMoving = false

	private var needles = [Needle(), Needle()]
	/// Linear RMS for each needle in the latest reading.
	private var levels = [0.0, 0.0]
	private var lastFrame: Date?
	/// When each needle's channel last came within 1 dB of full scale.
	private var peakTimes = [Date.distantPast, Date.distantPast]

	private static let peakLampLevel = pow(10, -1.0 / 20)
	private static let peakLampSeconds = 1.0

	func update(_ metrics: SignalMetrics?) {
		bars.update(metrics)
		let rms = metrics?.rms.map(\.doubleValue) ?? []
		let peaks = metrics?.peaks.map(\.doubleValue) ?? []
		// Mono moves both needles; beyond stereo, they show the front pair.
		let sources = rms.count == 1 ? [0, 0] : [0, 1]
		levels = sources.map { $0 < rms.count ? rms[$0] : 0 }
		let now = Date()
		for (needle, source) in sources.enumerated() where source < peaks.count && peaks[source] >= Self.peakLampLevel {
			peakTimes[needle] = now
		}
		if let metrics, !rms.isEmpty {
			let names = SignalMetricsModel.channelLabels(count: min(metrics.channels, Int(COG_METER_CHANNELS)), config: metrics.channelConfig)
			let newLabels = sources.map { $0 < names.count ? names[$0] : "" }
			if labels != newLabels {
				labels = newLabels
			}
		}
		let moving = levels.contains { $0 > 0 } || needles.contains { abs($0.position) > 0.001 || abs($0.velocity) > 0.001 }
		if isMoving != moving {
			isMoving = moving
			lastFrame = nil
		}
	}

	/// Moves the needles on to `date`, with 0 VU at `reference` dBFS, and
	/// says where they are.
	func needlePositions(at date: Date, reference: Double) -> [Double] {
		let elapsed = lastFrame.map { date.timeIntervalSince($0) } ?? 0
		lastFrame = date
		let fullScale = pow(10, (reference + 3) / 20)
		for index in needles.indices {
			needles[index].advance(toward: levels[index] / fullScale, by: elapsed)
		}
		return needles.map(\.position)
	}

	func isPeakLit(_ needle: Int, at date: Date) -> Bool {
		date.timeIntervalSince(peakTimes[needle]) < Self.peakLampSeconds
	}
}

enum VUMeterStyle: String {
	case needles, bars
}

struct VUMeterView: View {
	@ObservedObject var model: VUMeterModel
	@AppStorage("vuMeterStyle") private var style = VUMeterStyle.needles
	/// The level, in dBFS, that reads 0 VU.
	@AppStorage("vuMeterReference") private var reference = -18.0

	private static let references = [-12.0, -14.0, -18.0, -20.0]

	var body: some View {
		ZStack {
			switch style {
			case .needles:
				needles
			case .bars:
				VUMeterBars(model: model.bars)
			}
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.contentShape(Rectangle())
		// A click switches between the needles and the bars.
		.onTapGesture { style = style == .needles ? .bars : .needles }
		.contextMenu {
			Picker("Show", selection: $style) {
				Text("Needles").tag(VUMeterStyle.needles)
				Text("Bars").tag(VUMeterStyle.bars)
			}
			Picker("0 VU At", selection: $reference) {
				ForEach(Self.references, id: \.self) { level in
					Text(verbatim: String(format: "%.0f dBFS", level).replacingOccurrences(of: "-", with: "−"))
				}
			}
		}
		.help(Text("Click to switch between needles and bars."))
	}

	private var needles: some View {
		TimelineView(.animation(paused: !model.isMoving)) { context in
			let positions = model.needlePositions(at: context.date, reference: reference)
			HStack(spacing: 8) {
				ForEach(positions.indices, id: \.self) { index in
					VUFace(position: positions[index], label: model.labels[index], isPeakLit: model.isPeakLit(index, at: context.date))
				}
			}
		}
		.padding(8)
	}
}

/// One meter: the VU scale on a lit card, its needle and its peak lamp.
private struct VUFace: View {
	let position: Double
	let label: String
	let isPeakLit: Bool

	private static let card = Gradient(colors: [Color(red: 0.99, green: 0.94, blue: 0.80), Color(red: 0.93, green: 0.82, blue: 0.58)])
	private static let ink = Color(white: 0.13)
	private static let red = Color(red: 0.80, green: 0.14, blue: 0.10)
	private static let marks: [Double] = [-20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3]
	private static let minorMarks: [Double] = [-15, -6, -4, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5]
	/// Half the scale's sweep, in degrees either side of upright.
	private static let sweep = 46.0

	var body: some View {
		Canvas { context, size in
			let w = size.width
			let h = size.height
			let card = Path(roundedRect: CGRect(origin: .zero, size: size), cornerRadius: h * 0.06)
			context.fill(card, with: .linearGradient(Self.card, startPoint: .zero, endPoint: CGPoint(x: 0, y: h)))
			context.clip(to: card)

			let pivot = CGPoint(x: w / 2, y: h * 1.08)
			let radius = h * 0.86
			func point(_ fraction: Double, _ distance: CGFloat) -> CGPoint {
				let angle = (-Self.sweep + 2 * Self.sweep * fraction) * .pi / 180
				return CGPoint(x: pivot.x + distance * sin(angle), y: pivot.y - distance * cos(angle))
			}
			func arc(from: Double, to: Double, at distance: CGFloat) -> Path {
				Path { path in
					path.move(to: point(from, distance))
					for step in 1...48 {
						path.addLine(to: point(from + (to - from) * Double(step) / 48, distance))
					}
				}
			}

			// The scale, its red zone above 0 VU, its marks and their numbers.
			context.stroke(arc(from: Self.fraction(-20), to: Self.fraction(0), at: radius), with: .color(Self.ink), lineWidth: h * 0.012)
			context.stroke(arc(from: Self.fraction(0), to: Self.fraction(3), at: radius + h * 0.018), with: .color(Self.red), lineWidth: h * 0.036)
			for mark in Self.minorMarks {
				var tick = Path()
				tick.move(to: point(Self.fraction(mark), radius))
				tick.addLine(to: point(Self.fraction(mark), radius + h * 0.035))
				context.stroke(tick, with: .color(mark > 0 ? Self.red : Self.ink), lineWidth: h * 0.008)
			}
			for mark in Self.marks {
				let color = mark >= 0 ? Self.red : Self.ink
				var tick = Path()
				tick.move(to: point(Self.fraction(mark), radius))
				tick.addLine(to: point(Self.fraction(mark), radius + h * 0.065))
				context.stroke(tick, with: .color(color), lineWidth: h * 0.014)
				context.draw(Text(verbatim: String(format: "%.0f", abs(mark)))
					.font(.system(size: h * 0.085, weight: .medium))
					.foregroundColor(color),
					at: point(Self.fraction(mark), radius + h * 0.13))
			}
			context.draw(Text(verbatim: "−").font(.system(size: h * 0.1, weight: .medium)).foregroundColor(Self.ink),
			             at: point(0.06, radius * 0.82))
			context.draw(Text(verbatim: "+").font(.system(size: h * 0.1, weight: .medium)).foregroundColor(Self.red),
			             at: point(0.94, radius * 0.82))
			context.draw(Text(verbatim: "VU").font(.system(size: h * 0.16, weight: .semibold, design: .serif)).foregroundColor(Self.ink),
			             at: CGPoint(x: w / 2, y: h * 0.62))

			// The peak lamp, lit for a while when the channel nears full scale.
			let lamp = CGPoint(x: w - h * 0.14, y: h * 0.13)
			let lampRadius = h * 0.04
			context.fill(Path(ellipseIn: CGRect(x: lamp.x - lampRadius, y: lamp.y - lampRadius, width: lampRadius * 2, height: lampRadius * 2)),
			             with: .color(isPeakLit ? Color(red: 1, green: 0.2, blue: 0.1) : Color(red: 0.45, green: 0.12, blue: 0.08)))
			context.draw(Text("PEAK", comment: "VU meter: the lamp that lights near full scale")
				.font(.system(size: h * 0.055, weight: .semibold))
				.foregroundColor(Self.ink),
				at: CGPoint(x: lamp.x, y: lamp.y + lampRadius + h * 0.05))

			// The needle, under the housing that hides its pivot.
			var needle = Path()
			needle.move(to: pivot)
			needle.addLine(to: point(position, radius + h * 0.09))
			context.stroke(needle, with: .color(Self.ink), style: StrokeStyle(lineWidth: h * 0.013, lineCap: .round))
			let housing = CGRect(x: 0, y: h * 0.84, width: w, height: h * 0.16)
			context.fill(Path(housing), with: .linearGradient(Gradient(colors: [Color(white: 0.22), Color(white: 0.08)]),
			                                                   startPoint: CGPoint(x: 0, y: housing.minY), endPoint: CGPoint(x: 0, y: housing.maxY)))
			context.draw(Text(verbatim: label).font(.system(size: h * 0.075, weight: .semibold)).foregroundColor(Color(white: 0.85)),
			             at: CGPoint(x: housing.minX + h * 0.14, y: housing.midY))
		}
		.aspectRatio(1.55, contentMode: .fit)
		.accessibilityElement(children: .ignore)
		.accessibilityLabel(Text(verbatim: label))
		.accessibilityValue(Text(String.localizedStringWithFormat(NSLocalizedString("%.1f VU", comment: "VU meter: a needle's reading"), Self.vu(position))))
	}

	/// Where `vu` sits along the scale, which like an analog meter's is even
	/// in voltage: +3 VU at the end, 0 VU at 71%.
	private static func fraction(_ vu: Double) -> Double {
		pow(10, (vu - 3) / 20)
	}

	private static func vu(_ fraction: Double) -> Double {
		fraction > 0 ? max(-20, 20 * log10(fraction) + 3) : -20
	}
}

/// The Signal Inspector's bars, for every channel.
private struct VUMeterBars: View {
	@ObservedObject var model: SignalMetricsModel

	var body: some View {
		VStack(alignment: .leading, spacing: 3) {
			if model.channels.isEmpty {
				Text("No levels while paused")
					.foregroundColor(.secondary)
					.frame(maxWidth: .infinity, maxHeight: .infinity)
			} else {
				ForEach(model.channels) { channel in
					MeterRow(channel: channel)
				}
				MeterScale()
			}
		}
		.padding(12)
	}
}

/// Two small bars for the toolbar: left and right, RMS over peak.
struct CompactVUMeter: View {
	@ObservedObject var model: SignalMetricsModel

	var body: some View {
		Canvas { context, size in
			let channels = Array(model.channels.prefix(2))
			let rows = channels.count == 1 ? [channels[0], channels[0]] : channels
			let rowHeight = (size.height - 4) / 2
			for row in 0..<2 {
				let frame = CGRect(x: 0, y: 1 + CGFloat(row) * (rowHeight + 2), width: size.width, height: rowHeight)
				context.fill(Path(roundedRect: frame, cornerRadius: 2), with: .color(.secondary.opacity(0.15)))
				guard row < rows.count else { continue }
				let channel = rows[row]
				let color: Color = channel.peak > -1 ? .red : channel.peak > -6 ? .yellow : .green
				var peak = frame
				peak.size.width *= Self.fraction(channel.peak)
				context.fill(Path(roundedRect: peak, cornerRadius: 2), with: .color(color.opacity(0.45)))
				var rms = frame
				rms.size.width *= Self.fraction(channel.rms)
				context.fill(Path(roundedRect: rms, cornerRadius: 2), with: .color(color))
			}
		}
		.accessibilityElement(children: .ignore)
		.accessibilityLabel(Text("VU Meter"))
	}

	private static func fraction(_ decibels: Double) -> CGFloat {
		CGFloat(max(0, min(1, (decibels - SignalMetricsModel.floor) / -SignalMetricsModel.floor)))
	}
}

#Preview("Needles") {
	HStack(spacing: 8) {
		VUFace(position: pow(10, -3.0 / 20), label: "L", isPeakLit: false)
		VUFace(position: 0.97, label: "R", isPeakLit: true)
	}
	.padding(8)
	.frame(width: 400, height: 150)
	.background(Color.black)
}
