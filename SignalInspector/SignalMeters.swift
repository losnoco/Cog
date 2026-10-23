//
//  SignalMeters.swift
//  Cog
//
//  The output's levels as the Signal Inspector and the VU meter show them.
//  iOS's Now Playing shares them.
//

import CogAudio
import SwiftUI

/// The output's levels for each channel, refreshed from `SignalMetrics`
/// while the inspector or a meter is on screen.
@MainActor
final class SignalMetricsModel: ObservableObject {
	struct Channel: Identifiable, Equatable {
		let id: Int
		let label: String
		/// dBFS, from `floor` up.
		var peak: Double
		var rms: Double
		/// The highest peak lately, held a while before it falls.
		var hold: Double
		/// A peak reached full scale since the lamp was last cleared.
		var clipped: Bool
	}

	/// Where the meters bottom out.
	static let floor = -60.0
	/// How long a peak is held, and how fast the meters fall after it.
	private static let holdSeconds = 1.5
	private static let fallDecibelsPerSecond = 24.0

	@Published private(set) var channels: [Channel] = []
	@Published private(set) var metrics: SignalMetrics?

	private var holdTimes: [Date] = []
	private var lastUpdate = Date()

	func update(_ metrics: SignalMetrics?) {
		self.metrics = metrics
		let now = Date()
		let elapsed = min(now.timeIntervalSince(lastUpdate), 0.5)
		lastUpdate = now
		guard let metrics else {
			channels = []
			return
		}
		let labels = Self.channelLabels(count: min(metrics.channels, Int(COG_METER_CHANNELS)), config: metrics.channelConfig)
		if channels.count != labels.count || channels.map(\.label) != labels {
			channels = labels.enumerated().map { Channel(id: $0.offset, label: $0.element, peak: Self.floor, rms: Self.floor, hold: Self.floor, clipped: false) }
			holdTimes = Array(repeating: now, count: labels.count)
		}
		let fall = Self.fallDecibelsPerSecond * elapsed
		for index in channels.indices {
			// Nothing metered (paused, dry, DoP): the meters fall to rest.
			let peak = index < metrics.peaks.count ? Self.decibels(metrics.peaks[index].doubleValue) : Self.floor
			let rms = index < metrics.rms.count ? Self.decibels(metrics.rms[index].doubleValue) : Self.floor
			var channel = channels[index]
			channel.peak = max(peak, channel.peak - fall)
			channel.rms = max(rms, channel.rms - fall)
			if peak >= channel.hold {
				channel.hold = peak
				holdTimes[index] = now
			} else if now.timeIntervalSince(holdTimes[index]) > Self.holdSeconds {
				channel.hold = max(channel.peak, channel.hold - fall)
			}
			if index < metrics.peaks.count && metrics.peaks[index].doubleValue >= 1 {
				channel.clipped = true
			}
			channels[index] = channel
		}
	}

	func clearClipping() {
		for index in channels.indices {
			channels[index].clipped = false
		}
	}

	/// "L", "R", "C", "LFE"…, from the channel config's bits in interleaving
	/// order; numbers where the layout is unknown.
	static func channelLabels(count: Int, config: UInt32) -> [String] {
		let names = ["L", "R", "C", "LFE", "Ls", "Rs", "Lc", "Rc", "Cs", "Lsd", "Rsd", "Tc", "Tfl", "Tfc", "Tfr", "Tbl", "Tbc", "Tbr"]
		var labels: [String] = []
		for bit in 0..<names.count where config & (1 << UInt32(bit)) != 0 {
			labels.append(names[bit])
		}
		if count == 1 && config == 0 {
			return ["M"]
		}
		guard labels.count == count else {
			return (1...max(count, 1)).map(String.init)
		}
		return labels
	}

	static func decibels(_ linear: Double) -> Double {
		linear > 0 ? max(floor, 20 * log10(linear)) : floor
	}
}

struct MeterRow: View {
	let channel: SignalMetricsModel.Channel

	var body: some View {
		HStack(spacing: 6) {
			Text(channel.label)
				.font(.system(size: 9, design: .monospaced))
				.frame(width: 28, alignment: .trailing)
			GeometryReader { proxy in
				let width = proxy.size.width
				ZStack(alignment: .leading) {
					Rectangle().fill(Color.secondary.opacity(0.15))
					Rectangle().fill(barColor.opacity(0.45))
						.frame(width: width * fraction(channel.peak))
					Rectangle().fill(barColor)
						.frame(width: width * fraction(channel.rms))
					Rectangle().fill(holdColor)
						.frame(width: 2)
						.offset(x: max(0, width * fraction(channel.hold) - 2))
				}
			}
			.frame(height: 8)
			Circle()
				.fill(channel.clipped ? Color.red : Color.secondary.opacity(0.2))
				.frame(width: 7, height: 7)
				.accessibilityLabel(Text(channel.clipped ? "Clipped" : "Not clipped"))
		}
		.accessibilityElement(children: .ignore)
		.accessibilityLabel(Text(channel.label))
		.accessibilityValue(Text(String.localizedStringWithFormat("%.1f dBFS", channel.peak)))
	}

	private func fraction(_ decibels: Double) -> CGFloat {
		CGFloat(max(0, min(1, (decibels - SignalMetricsModel.floor) / -SignalMetricsModel.floor)))
	}

	private var barColor: Color {
		channel.peak > -1 ? .red : channel.peak > -6 ? .yellow : .green
	}

	private var holdColor: Color {
		channel.hold > -1 ? .red : .primary.opacity(0.7)
	}
}

/// dBFS marks under the meters.
struct MeterScale: View {
	private let marks: [Double] = [-60, -40, -20, -10, -6, -3, 0]

	var body: some View {
		HStack(spacing: 6) {
			Color.clear.frame(width: 28, height: 1)
			GeometryReader { proxy in
				ZStack(alignment: .topLeading) {
					ForEach(marks, id: \.self) { mark in
						Text(mark == 0 ? "0" : String(format: "%.0f", mark).replacingOccurrences(of: "-", with: "−"))
							.font(.system(size: 8))
							.foregroundColor(.secondary)
							.fixedSize()
							.position(x: proxy.size.width * CGFloat((mark - SignalMetricsModel.floor) / -SignalMetricsModel.floor), y: 5)
					}
				}
			}
			.frame(height: 10)
			Color.clear.frame(width: 7, height: 1)
		}
	}
}
