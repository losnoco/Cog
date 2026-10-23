//
//  SignalInspectorView.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-05.
//

import CogAudio
import SwiftUI

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
		// The panel sets its own smallest size; docked, the pane may be smaller.
		.frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
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
