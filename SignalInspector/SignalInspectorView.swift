//
//  SignalInspectorView.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-05.
//

import CogAudio
import SwiftUI

/// The live side of the inspector, refreshed from `SignalMetrics` while the
/// panel is visible.
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
		let labels = SignalFormatting.channelLabels(count: min(metrics.channels, Int(COG_METER_CHANNELS)), config: metrics.channelConfig)
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

	static func decibels(_ linear: Double) -> Double {
		linear > 0 ? max(floor, 20 * log10(linear)) : floor
	}
}

struct SignalInspectorView: View {
	@ObservedObject var status: SignalStatusModel
	@ObservedObject var meters: SignalMetricsModel

	var body: some View {
		ScrollView {
			VStack(alignment: .leading, spacing: 12) {
				if let chain = status.chain {
					VerdictHeader(verdict: chain.verdict)
					SectionTitle("Signal path")
					ChainView(stages: chain.stages)
					SectionTitle("Live")
					LiveView(model: meters)
				} else {
					Text("Nothing playing")
						.foregroundColor(.secondary)
						.frame(maxWidth: .infinity, minHeight: 120)
				}
			}
			.padding(12)
		}
		.font(.system(size: NSFont.smallSystemFontSize))
		.frame(minWidth: 300, maxWidth: .infinity, minHeight: 360, maxHeight: .infinity, alignment: .top)
	}
}

private struct SectionTitle: View {
	let title: LocalizedStringKey

	init(_ title: LocalizedStringKey) {
		self.title = title
	}

	var body: some View {
		Text(title)
			.font(.system(size: NSFont.smallSystemFontSize, weight: .semibold))
			.foregroundColor(.secondary)
			.textCase(.uppercase)
	}
}

private struct VerdictHeader: View {
	let verdict: SignalChain.Verdict

	var body: some View {
		HStack(alignment: .top, spacing: 8) {
			Image(systemName: symbol)
				.font(.system(size: 20))
				.foregroundColor(color)
			VStack(alignment: .leading, spacing: 2) {
				Text(title)
					.font(.system(size: NSFont.systemFontSize, weight: .semibold))
				Text(detail)
					.foregroundColor(.secondary)
					.fixedSize(horizontal: false, vertical: true)
				Text("Covers Cog only; macOS and the device may still change the sound.")
					.foregroundColor(.secondary)
					.fixedSize(horizontal: false, vertical: true)
			}
		}
	}

	private var symbol: String {
		switch verdict {
		case .bitPerfect: return "checkmark.seal.fill"
		case .modified: return "waveform.path.ecg"
		case .unknown: return "questionmark.circle"
		}
	}

	private var color: Color {
		switch verdict {
		case .bitPerfect: return .green
		case .modified: return .orange
		case .unknown: return .secondary
		}
	}

	private var title: LocalizedStringKey {
		switch verdict {
		case .bitPerfect: return "Bit perfect"
		case .modified: return "Modified"
		case .unknown: return "Unknown"
		}
	}

	private var detail: LocalizedStringKey {
		switch verdict {
		case .bitPerfect: return "Cog passes the decoded samples on unchanged."
		case .modified: return "Cog changes the samples at the highlighted stages."
		case .unknown: return "The decoder did not describe its samples."
		}
	}
}

/// The stages top to bottom, joined by a line, as the audio flows.
private struct ChainView: View {
	let stages: [SignalStage]

	var body: some View {
		VStack(alignment: .leading, spacing: 0) {
			ForEach(stages) { stage in
				StageRow(stage: stage, isLast: stage.id == stages.last?.id)
			}
		}
	}
}

private struct StageRow: View {
	let stage: SignalStage
	let isLast: Bool

	var body: some View {
		HStack(alignment: .top, spacing: 8) {
			VStack(spacing: 0) {
				Image(systemName: symbol)
					.foregroundColor(color)
					.frame(width: 14, height: 14)
				if !isLast {
					Rectangle()
						.fill(Color.secondary.opacity(0.35))
						.frame(width: 1)
						.frame(maxHeight: .infinity)
				}
			}
			VStack(alignment: .leading, spacing: 1) {
				Text(stage.name)
					.fontWeight(stage.state == .modifies ? .semibold : .regular)
				if let detail = stage.detail {
					Text(detail)
						.foregroundColor(.secondary)
						.textSelection(.enabled)
						.fixedSize(horizontal: false, vertical: true)
				}
			}
			.padding(.bottom, isLast ? 0 : 8)
			.opacity(stage.state == .bypassed ? 0.55 : 1)
			Spacer(minLength: 0)
		}
		.accessibilityElement(children: .combine)
		.accessibilityValue(Text(accessibilityState))
	}

	private var symbol: String {
		switch stage.state {
		case .passes: return "circle"
		case .modifies: return "circle.fill"
		case .bypassed: return "circle.dashed"
		}
	}

	private var color: Color {
		switch stage.state {
		case .passes: return .secondary
		case .modifies: return .orange
		case .bypassed: return .secondary
		}
	}

	private var accessibilityState: LocalizedStringKey {
		switch stage.state {
		case .passes: return "Passes the samples unchanged"
		case .modifies: return "Changes the samples"
		case .bypassed: return "Bypassed"
		}
	}
}

private struct LiveView: View {
	@ObservedObject var model: SignalMetricsModel

	var body: some View {
		VStack(alignment: .leading, spacing: 8) {
			if model.channels.isEmpty {
				Text("No levels while paused")
					.foregroundColor(.secondary)
			} else {
				VStack(alignment: .leading, spacing: 3) {
					ForEach(model.channels) { channel in
						MeterRow(channel: channel)
					}
					MeterScale()
				}
				.onTapGesture { model.clearClipping() }
				.help(Text("Peak and RMS of what Cog hands Core Audio, after every gain. Click to clear the clip lamps."))
			}
			if let metrics = model.metrics {
				StatsView(metrics: metrics)
			}
		}
	}
}

private struct MeterRow: View {
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
private struct MeterScale: View {
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

private struct StatsView: View {
	let metrics: SignalMetrics

	var body: some View {
		VStack(alignment: .leading, spacing: 2) {
			stat("Buffered", String(format: NSLocalizedString("%@ ms processed, %@ s decoded", comment: "Signal Inspector: shallow and deep buffer levels"),
			                        SignalFormatting.milliseconds(metrics.shallowBufferSeconds),
			                        String.localizedStringWithFormat("%.1f", metrics.deepBufferSeconds)))
			stat("Output latency", String(format: NSLocalizedString("%@ ms", comment: "A duration in milliseconds"), SignalFormatting.milliseconds(metrics.outputLatencySeconds)))
			stat("Underruns", "\(metrics.underruns)", warns: metrics.underruns > 0)
			stat("Device glitches", "\(metrics.discontinuities)", warns: metrics.discontinuities > 0)
			stat("Clipped samples", "\(metrics.clippedSamples)", warns: metrics.clippedSamples > 0)
		}
		.help(Text("Counts are for the track playing; clipped samples are counted only while this panel is open."))
	}

	private func stat(_ label: LocalizedStringKey, _ value: String, warns: Bool = false) -> some View {
		HStack {
			Text(label)
				.foregroundColor(.secondary)
			Spacer()
			Text(value)
				.monospacedDigit()
				.foregroundColor(warns ? .orange : .primary)
		}
	}
}

private func previewValue(_ format: AudioStreamBasicDescription) -> NSValue {
	withUnsafePointer(to: format) { NSValue(bytes: $0, objCType: "{AudioStreamBasicDescription=dIIIIIIII}") }
}

#Preview {
	let status = SignalStatusModel.shared
	status.update([
		CogAudioOutputRenderFormatKey: previewValue(AudioStreamBasicDescription(
			mSampleRate: 48000, mFormatID: kAudioFormatLinearPCM, mFormatFlags: kAudioFormatFlagsNativeFloatPacked,
			mBytesPerPacket: 8, mFramesPerPacket: 1, mBytesPerFrame: 8, mChannelsPerFrame: 2, mBitsPerChannel: 32, mReserved: 0)),
		CogAudioOutputSourceFormatKey: previewValue(AudioStreamBasicDescription(
			mSampleRate: 44100, mFormatID: kAudioFormatLinearPCM, mFormatFlags: kAudioFormatFlagIsSignedInteger,
			mBytesPerPacket: 4, mFramesPerPacket: 1, mBytesPerFrame: 4, mChannelsPerFrame: 2, mBitsPerChannel: 16, mReserved: 0)),
		CogAudioOutputSourceCodecKey: "FLAC",
		CogAudioOutputSourceEncodingKey: "lossless",
		CogAudioOutputDeviceNameKey: "MacBook Pro Speakers",
		CogAudioOutputModificationsKey: [CogAudioOutputModificationResampling, CogAudioOutputModificationTrackGain, CogAudioOutputModificationEqualizer],
		CogAudioOutputTrackGainKey: -7.4,
		CogAudioOutputTrackGainSourceKey: "album",
		CogAudioOutputResamplerKey: [CogAudioOutputStageActiveKey: true, CogAudioOutputStageInputRateKey: 44100.0,
		                             CogAudioOutputStageOutputRateKey: 48000.0, CogAudioOutputStageQualityKey: "HQ"],
		CogAudioOutputEqualizerKey: [CogAudioOutputStagePreampKey: -3.0, CogAudioOutputStageBandGainsKey: [2.0, 4.5, 0, 0, -6.0]],
		CogAudioOutputBufferFramesKey: 960,
		CogAudioOutputLatencyFramesKey: 1500,
	])
	return SignalInspectorView(status: status, meters: SignalMetricsModel())
		.frame(width: 320, height: 640)
}
