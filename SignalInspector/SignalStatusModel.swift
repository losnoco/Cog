//
//  SignalStatusModel.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-05.
//

import Cocoa
import CogAudio

/// What reaches the output device, as `CogAudioOutputStatusDidChangeNotification`
/// last described it: the status bar's summary and tooltip, and the Signal
/// Inspector's chain, from one reading so they cannot disagree.
@MainActor
@objc final class SignalStatusModel: NSObject, ObservableObject {
	@objc static let shared = SignalStatusModel()

	/// Nil while nothing plays.
	@Published private(set) var chain: SignalChain?

	/// For the status bar: "Bit perfect · Float32 · 48 kHz".
	@objc private(set) var summary = NSLocalizedString("Audio: —", comment: "No active audio output information")
	@objc private(set) var toolTip = NSLocalizedString("No active audio output.", comment: "No active audio output tooltip")

	/// Posted on the main thread after each change.
	@objc static let didChangeNotification = Notification.Name("SignalStatusModelDidChange")

	private override init() {
		super.init()
		// Posted on the main thread.
		NotificationCenter.default.addObserver(self, selector: #selector(outputStatusDidChange(_:)), name: .CogAudioOutputStatusDidChange, object: nil)
	}

	@objc private func outputStatusDidChange(_ notification: Notification) {
		update(notification.userInfo)
	}

	func update(_ status: [AnyHashable: Any]?) {
		chain = status.flatMap(SignalChain.init(status:))
		if let chain {
			summary = chain.summary
			toolTip = chain.toolTip + "\n" + NSLocalizedString("Click to open the Signal Inspector.", comment: "Tooltip note: clicking the audio output status opens the Signal Inspector")
		} else {
			summary = NSLocalizedString("Audio: —", comment: "No active audio output information")
			toolTip = NSLocalizedString("No active audio output.", comment: "No active audio output tooltip")
		}
		NotificationCenter.default.post(name: Self.didChangeNotification, object: self)
	}
}

/// One step of the signal path.
struct SignalStage: Identifiable, Equatable {
	enum State {
		/// Hands the samples on as they came.
		case passes
		/// Changes them.
		case modifies
		/// Enabled, but out of the way for this audio.
		case bypassed
	}

	let id: String
	let name: String
	let state: State
	/// Its settings, or why it is bypassed.
	var detail: String?
	/// The format leaving it, where that is worth showing.
	var format: String?
}

/// The whole path from the decoder to the device.
struct SignalChain: Equatable {
	enum Verdict {
		case bitPerfect
		case modified
		case unknown
	}

	let verdict: Verdict
	let stages: [SignalStage]
	let summary: String
	let toolTip: String

	init?(status: [AnyHashable: Any]) {
		guard let renderFormat = SignalFormatting.format(from: status[CogAudioOutputRenderFormatKey]) else { return nil }
		let isDoP = status[CogAudioOutputDoPKey] as? Bool ?? false
		let exclusive = status[CogAudioOutputExclusiveKey] as? Bool ?? false
		let sourceFormat = SignalFormatting.format(from: status[CogAudioOutputSourceFormatKey])
		let modifications = status[CogAudioOutputModificationsKey] as? [String]
		let unavailable = NSLocalizedString("Unavailable", comment: "A format Core Audio or the decoder did not report")

		let integrity: String
		var lines: [String] = []
		if let modifications {
			if modifications.isEmpty {
				verdict = .bitPerfect
				integrity = NSLocalizedString("Bit perfect", comment: "Bit-perfect Cog signal-integrity state")
				lines.append(NSLocalizedString("Bit perfect: Cog passes the decoded samples on unchanged.", comment: "Tooltip heading when Cog does not change the samples"))
			} else {
				verdict = .modified
				integrity = NSLocalizedString("Modified", comment: "Modified Cog signal-integrity state")
				lines.append(NSLocalizedString("Modified by Cog:", comment: "Tooltip heading before the ways Cog changes the samples"))
				for modification in modifications {
					lines.append("• " + SignalFormatting.modification(modification, status: status, sourceRate: sourceFormat?.mSampleRate ?? 0, renderFormat: renderFormat))
				}
			}
		} else {
			verdict = .unknown
			integrity = NSLocalizedString("Unknown", comment: "Unknown Cog signal-integrity state")
			lines.append(NSLocalizedString("Unknown: the decoder did not describe its samples.", comment: "Tooltip heading when Cog cannot tell whether it changes the samples"))
		}
		lines.append("")

		let source = sourceFormat.map {
			SignalFormatting.source($0, codec: status[CogAudioOutputSourceCodecKey] as? String, encoding: status[CogAudioOutputSourceEncodingKey] as? String)
		}
		lines.append(String(format: NSLocalizedString("Source: %@", comment: "Tooltip line: the track as decoded"), source ?? unavailable))
		let cogOutput = SignalFormatting.format(renderFormat, isDoP: isDoP)
		lines.append(String(format: NSLocalizedString("Cog output: %@", comment: "Tooltip line: what Cog renders for Core Audio"), cogOutput))

		// Core Audio's formats only where they differ from what they follow:
		// usually they are the same throughout.
		var previous = cogOutput
		let coreAudio = SignalFormatting.streamFormats(status[CogAudioOutputVirtualFormatsKey] as? [Any])
		let showsCoreAudio = coreAudio != nil && coreAudio != cogOutput
		if let coreAudio, showsCoreAudio {
			lines.append(String(format: NSLocalizedString("Core Audio: %@", comment: "Tooltip line: the formats the system mixes in"), coreAudio))
			previous = coreAudio
		}
		var qualifiers: [String] = []
		if status[CogAudioOutputSystemDefaultKey] as? Bool ?? false {
			qualifiers.append(NSLocalizedString("system default", comment: "The output device is the system's default"))
		}
		qualifiers.append(exclusive ? NSLocalizedString("exclusive", comment: "Cog holds the output device for itself") :
			NSLocalizedString("shared", comment: "The output device plays other apps' audio too"))
		let deviceName = status[CogAudioOutputDeviceNameKey] as? String ?? NSLocalizedString("Unnamed device", comment: "An output device without a name")
		var device = "\(deviceName) (\(qualifiers.joined(separator: ", ")))"
		let physical = SignalFormatting.streamFormats(status[CogAudioOutputPhysicalFormatsKey] as? [Any])
		let physicalMatches = physical == previous
		if physicalMatches {
			device = String(format: NSLocalizedString("%@, same format", comment: "An output device whose format matches the line above"), device)
		} else {
			device = "\(device) · \(physical ?? unavailable)"
		}
		lines.append(String(format: NSLocalizedString("Device: %@", comment: "Tooltip line: the output device and the format it runs at"), device))
		lines.append("")
		lines.append(NSLocalizedString("Covers Cog only; macOS and the device may still change the sound.", comment: "Tooltip note on what the signal-integrity verdict covers"))

		summary = String(format: NSLocalizedString("%@ · %@%@", comment: "Signal integrity, Cog output format, and exclusive transport status"),
		                 integrity, SignalFormatting.shortFormat(renderFormat, isDoP: isDoP),
		                 exclusive ? NSLocalizedString(" · Exclusive", comment: "Exclusive audio transport status") : "")
		toolTip = lines.joined(separator: "\n")

		stages = Self.stages(status: status, modifications: modifications ?? [], source: sourceFormat, sourceDescription: source,
		                     renderFormat: renderFormat, isDoP: isDoP, cogOutput: cogOutput,
		                     coreAudio: showsCoreAudio ? coreAudio : nil,
		                     device: (deviceName, qualifiers, physicalMatches ? nil : physical ?? unavailable))
	}

	// MARK: - Stages

	private static func stages(status: [AnyHashable: Any], modifications: [String], source: AudioStreamBasicDescription?, sourceDescription: String?,
	                           renderFormat: AudioStreamBasicDescription, isDoP: Bool, cogOutput: String, coreAudio: String?,
	                           device: (name: String, qualifiers: [String], format: String?)) -> [SignalStage] {
		func modifies(_ modification: String) -> Bool { modifications.contains(modification) }
		func describe(_ modification: String) -> String {
			SignalFormatting.modification(modification, status: status, sourceRate: source?.mSampleRate ?? 0, renderFormat: renderFormat)
		}
		var stages: [SignalStage] = []

		stages.append(SignalStage(id: "decoder", name: NSLocalizedString("Decoder", comment: "Signal Inspector stage: the track's decoder"), state: .passes,
		                          detail: sourceDescription ?? NSLocalizedString("The decoder did not describe its samples.", comment: "Signal Inspector: the source format is unknown")))

		if let source, source.mBitsPerChannel == 1 {
			let dsd = SignalFormatting.dsdName(source.mSampleRate)
			if isDoP {
				stages.append(SignalStage(id: "dsd", name: NSLocalizedString("DSD over PCM", comment: "Signal Inspector stage: DSD packed into PCM words for the DAC"),
				                          state: .passes,
				                          detail: String(format: NSLocalizedString("%@ packed for the DAC, untouched", comment: "Signal Inspector: DoP; the DSD rate name, e.g. DSD64"), dsd)))
			} else {
				let pcmRate = (status[CogAudioOutputResamplerKey] as? [String: Any])?[CogAudioOutputStageInputRateKey] as? Double ?? source.mSampleRate / 8
				stages.append(SignalStage(id: "dsd", name: NSLocalizedString("DSD to PCM", comment: "Signal Inspector stage: DSD decimated to PCM"),
				                          state: .modifies,
				                          detail: String(format: NSLocalizedString("%@ to %@ PCM", comment: "Signal Inspector: DSD rate name to a PCM sample rate"),
				                                         dsd, SignalFormatting.sampleRate(pcmRate))))
			}
		}
		if modifies(CogAudioOutputModificationHDCD) {
			stages.append(SignalStage(id: "hdcd", name: NSLocalizedString("HDCD", comment: "Signal Inspector stage: HDCD decoding"), state: .modifies,
			                          detail: describe(CogAudioOutputModificationHDCD)))
		}
		if source != nil && !isDoP {
			let precision = modifies(CogAudioOutputModificationPrecision)
			stages.append(SignalStage(id: "precision", name: NSLocalizedString("Sample conversion", comment: "Signal Inspector stage: samples converted to Float32 for processing"),
			                          state: precision ? .modifies : .passes,
			                          detail: precision ? describe(CogAudioOutputModificationPrecision) :
			                          	NSLocalizedString("Float32, exact", comment: "Signal Inspector: the samples convert to Float32 without loss")))
		}

		if let resampler = status[CogAudioOutputResamplerKey] as? [String: Any],
		   let input = resampler[CogAudioOutputStageInputRateKey] as? Double, let output = resampler[CogAudioOutputStageOutputRateKey] as? Double {
			let active = resampler[CogAudioOutputStageActiveKey] as? Bool ?? false
			let quality = resampler[CogAudioOutputStageQualityKey] as? String ?? ""
			stages.append(SignalStage(id: "resampler", name: NSLocalizedString("Resampler", comment: "Signal Inspector stage: sample rate conversion"),
			                          state: active ? .modifies : .bypassed,
			                          detail: active ? String(format: NSLocalizedString("soxr %@ · %@ to %@", comment: "Signal Inspector: resampler quality, input and output rates"),
			                                                  quality, SignalFormatting.sampleRate(input), SignalFormatting.sampleRate(output)) :
			                          	String(format: NSLocalizedString("Bypassed at %@, bit-exact", comment: "Signal Inspector: the resampler is not needed at equal rates"),
			                          	       SignalFormatting.sampleRate(output))))
		}

		if let gainSource = status[CogAudioOutputTrackGainSourceKey] as? String {
			let applied = modifies(CogAudioOutputModificationTrackGain)
			var parts = [SignalFormatting.gainSource(gainSource)]
			if let gain = status[CogAudioOutputTrackGainKey] as? Double {
				parts.append(SignalFormatting.signedDecibels(gain) + " dB")
			} else if !applied {
				parts.append(NSLocalizedString("unity", comment: "Signal Inspector: a gain of exactly 0 dB"))
			}
			if status[CogAudioOutputTrackGainPeakLimitedKey] as? Bool ?? false {
				parts.append(NSLocalizedString("limited by the peak", comment: "Signal Inspector: the gain was lowered so the track cannot clip"))
			}
			stages.append(SignalStage(id: "gain", name: NSLocalizedString("Track gain", comment: "Signal Inspector stage: ReplayGain or tagged volume"),
			                          state: applied ? .modifies : .bypassed, detail: parts.joined(separator: " · ")))
		}

		if let stretch = status[CogAudioOutputTimeStretchKey] as? [String: Any] {
			stages.append(SignalStage(id: "timeStretch", name: NSLocalizedString("Time stretch", comment: "Signal Inspector stage: tempo and pitch"), state: .modifies,
			                          detail: SignalFormatting.timeStretch(stretch)))
		}

		if let surround = status[CogAudioOutputFreeSurroundKey] as? [String: Any] {
			let active = surround[CogAudioOutputStageActiveKey] as? Bool ?? false
			let channels = surround[CogAudioOutputStageChannelsKey] as? Int ?? 0
			stages.append(SignalStage(id: "freeSurround", name: "FreeSurround", state: active ? .modifies : .bypassed,
			                          detail: active ? String(format: NSLocalizedString("Stereo upmixed to %@", comment: "Signal Inspector: FreeSurround's output, e.g. 6 ch"),
			                                                  SignalFormatting.channels(UInt32(channels))) :
			                          	NSLocalizedString("Upmixes stereo only; passes this through", comment: "Signal Inspector: FreeSurround is on but the audio is not stereo")))
		}

		if let equalizer = status[CogAudioOutputEqualizerKey] as? [String: Any] {
			stages.append(SignalStage(id: "equalizer", name: NSLocalizedString("Equalizer", comment: "Signal Inspector stage: the graphic equalizer"), state: .modifies,
			                          detail: SignalFormatting.equalizer(equalizer)))
		}

		if modifies(CogAudioOutputModificationChannelLayout) {
			stages.append(SignalStage(id: "channels", name: NSLocalizedString("Channel mapping", comment: "Signal Inspector stage: channels fitted to the device"),
			                          state: .modifies, detail: describe(CogAudioOutputModificationChannelLayout)))
		}

		if let spatial = status[CogAudioOutputSpatialKey] as? [String: Any] {
			var parts = [SignalFormatting.spatialOutput(spatial[CogAudioOutputStageSpatialOutputKey] as? String)]
			parts.append(spatial[CogAudioOutputStageHeadTrackingKey] as? Bool ?? false ?
				NSLocalizedString("head tracking on", comment: "Signal Inspector: the spatial mixer follows the listener's head") :
				NSLocalizedString("head tracking off", comment: "Signal Inspector: the spatial mixer does not follow the listener's head"))
			stages.append(SignalStage(id: "spatial", name: NSLocalizedString("Spatial audio", comment: "Signal Inspector stage: Apple's spatial mixer"), state: .modifies,
			                          detail: parts.joined(separator: " · ")))
		} else if status[CogAudioOutputSpatialRefusedKey] as? Bool ?? false {
			stages.append(SignalStage(id: "spatial", name: NSLocalizedString("Spatial audio", comment: "Signal Inspector stage: Apple's spatial mixer"), state: .bypassed,
			                          detail: NSLocalizedString("Could not be set up; downmixed instead", comment: "Signal Inspector: the spatial mixer failed, so surround is downmixed")))
		}

		let volume = modifies(CogAudioOutputModificationVolume)
		stages.append(SignalStage(id: "volume", name: NSLocalizedString("Volume", comment: "Signal Inspector stage: Cog's volume"),
		                          state: isDoP ? .bypassed : volume ? .modifies : .passes,
		                          detail: isDoP ? NSLocalizedString("Not applied to DoP", comment: "Signal Inspector: volume cannot change DSD over PCM") :
		                          	volume ? describe(CogAudioOutputModificationVolume) :
		                          	NSLocalizedString("100%, unity", comment: "Signal Inspector: Cog's volume does not change the samples")))

		stages.append(SignalStage(id: "output", name: NSLocalizedString("Cog output", comment: "Signal Inspector stage: what Cog hands Core Audio"), state: .passes,
		                          detail: cogOutput))
		if let coreAudio {
			stages.append(SignalStage(id: "coreAudio", name: "Core Audio", state: .passes, detail: coreAudio))
		}
		var deviceDetail = [device.qualifiers.joined(separator: ", ")]
		if let format = device.format {
			deviceDetail.append(format)
		}
		if let latency = SignalFormatting.latency(status, renderFormat: renderFormat) {
			deviceDetail.append(latency)
		}
		stages.append(SignalStage(id: "device", name: device.name, state: .passes, detail: deviceDetail.joined(separator: " · ")))
		return stages
	}
}

// MARK: - Formatting

/// Descriptions of formats and settings, for the status bar and the Signal
/// Inspector.
enum SignalFormatting {
	/// "44.1 kHz", or "2.8224 MHz" for DSD, in the user's number format.
	static func sampleRate(_ sampleRate: Double) -> String {
		if sampleRate >= 1_000_000 {
			return String.localizedStringWithFormat("%.6g MHz", sampleRate / 1_000_000)
		}
		if sampleRate >= 1000 {
			return String.localizedStringWithFormat("%.6g kHz", sampleRate / 1000)
		}
		return String.localizedStringWithFormat("%.0f Hz", sampleRate)
	}

	static func channels(_ channels: UInt32) -> String {
		switch channels {
		case 1:
			return NSLocalizedString("Mono", comment: "One audio channel")
		case 2:
			return NSLocalizedString("Stereo", comment: "Two audio channels")
		default:
			return String(format: NSLocalizedString("%u ch", comment: "A number of audio channels, e.g. 6 ch"), channels)
		}
	}

	/// "Float32", "Int16", "DoP", or the format's four-character code if it
	/// is not PCM.
	static func sampleFormatName(_ format: AudioStreamBasicDescription, isDoP: Bool) -> String {
		if isDoP {
			return "DoP"
		}
		if format.mFormatID != kAudioFormatLinearPCM {
			let code = format.mFormatID.bigEndian
			return withUnsafeBytes(of: code) { String(bytes: $0, encoding: .macOSRoman) } ?? "?"
		}
		if format.mFormatFlags & kAudioFormatFlagIsFloat != 0 {
			return "Float\(format.mBitsPerChannel)"
		}
		if format.mFormatFlags & kAudioFormatFlagIsSignedInteger != 0 {
			return "Int\(format.mBitsPerChannel)"
		}
		return "UInt\(format.mBitsPerChannel)"
	}

	private static func nonMixableSuffix(_ format: AudioStreamBasicDescription) -> String {
		format.mFormatFlags & kAudioFormatFlagIsNonMixable != 0 ? NSLocalizedString(" · Non-mixable", comment: "Non-mixable Core Audio stream format") : ""
	}

	/// "Float32 · 88.2 kHz · Stereo", with the word size when samples are
	/// stored wider than they are ("Int24 in 32-bit words").
	static func format(_ format: AudioStreamBasicDescription, isDoP: Bool) -> String {
		var sampleFormat = sampleFormatName(format, isDoP: isDoP)
		if !isDoP && format.mFormatID == kAudioFormatLinearPCM {
			let nonInterleaved = format.mFormatFlags & kAudioFormatFlagIsNonInterleaved != 0
			let bytesPerSample = nonInterleaved ? format.mBytesPerFrame : (format.mChannelsPerFrame > 0 ? format.mBytesPerFrame / format.mChannelsPerFrame : 0)
			let wordBits = bytesPerSample * 8
			if wordBits > format.mBitsPerChannel {
				sampleFormat = String(format: NSLocalizedString("%@ in %u-bit words", comment: "A sample format stored in wider words, e.g. Int24 in 32-bit words"), sampleFormat, wordBits)
			}
		}
		return "\(sampleFormat) · \(sampleRate(format.mSampleRate)) · \(channels(format.mChannelsPerFrame))\(nonMixableSuffix(format))"
	}

	/// "Float32 · 48 kHz", for the status bar itself.
	static func shortFormat(_ format: AudioStreamBasicDescription, isDoP: Bool) -> String {
		"\(sampleFormatName(format, isDoP: isDoP)) · \(sampleRate(format.mSampleRate))\(nonMixableSuffix(format))"
	}

	/// "DSD64", after its rate as a multiple of 44.1 or 48 kHz.
	static func dsdName(_ sampleRate: Double) -> String {
		for base in [44100.0, 48000.0] {
			let multiple = sampleRate / base
			if multiple >= 1, abs(multiple - multiple.rounded()) < 1e-6 {
				return String(format: "DSD%.0f", multiple)
			}
		}
		return "DSD"
	}

	private static func encoding(_ encoding: String?) -> String? {
		switch encoding {
		case "lossless":
			return NSLocalizedString("lossless", comment: "How a track is encoded")
		case "lossy":
			return NSLocalizedString("lossy", comment: "How a track is encoded")
		case "synthesized":
			return NSLocalizedString("synthesized", comment: "How a track is encoded: rendered while playing, as MIDI or chiptunes are")
		default:
			// Decoders that cannot tell say "lossy/lossless".
			return nil
		}
	}

	/// "FLAC (lossless) · Int16 · 44.1 kHz · Stereo".
	static func source(_ format: AudioStreamBasicDescription, codec: String?, encoding encodingName: String?) -> String {
		var parts: [String] = []
		let encodingName = encoding(encodingName)
		if let codec, !codec.isEmpty {
			parts.append(encodingName.map { "\(codec) (\($0))" } ?? codec)
		} else if let encodingName {
			parts.append(encodingName)
		}
		parts.append(format.mBitsPerChannel == 1 ? dsdName(format.mSampleRate) : sampleFormatName(format, isDoP: false))
		parts.append(sampleRate(format.mSampleRate))
		parts.append(channels(format.mChannelsPerFrame))
		return parts.joined(separator: " · ")
	}

	static func format(from value: Any?) -> AudioStreamBasicDescription? {
		guard let value = value as? NSValue else { return nil }
		var format = AudioStreamBasicDescription()
		value.getValue(&format, size: MemoryLayout<AudioStreamBasicDescription>.size)
		return format
	}

	/// Each stream's format, identical streams counted ("Float32 · 48 kHz ·
	/// Stereo ×2"); nil if there are none.
	static func streamFormats(_ values: [Any]?) -> String? {
		var descriptions: [String] = []
		var counts: [String: Int] = [:]
		for value in values ?? [] {
			guard let format = format(from: value) else { continue }
			let description = self.format(format, isDoP: false)
			if counts[description] == nil {
				descriptions.append(description)
			}
			counts[description, default: 0] += 1
		}
		guard !descriptions.isEmpty else { return nil }
		return descriptions.map { description in
			let count = counts[description] ?? 1
			return count > 1 ? "\(description) ×\(count)" : description
		}.joined(separator: " / ")
	}

	/// With a real minus sign.
	static func signedDecibels(_ decibels: Double) -> String {
		String.localizedStringWithFormat("%+.1f", decibels).replacingOccurrences(of: "-", with: "−")
	}

	static func percentage(_ percent: Double) -> String {
		let formatter = NumberFormatter()
		formatter.numberStyle = .percent
		formatter.maximumFractionDigits = 1
		return formatter.string(from: NSNumber(value: percent / 100)) ?? "\(percent)%"
	}

	/// One way Cog changes the samples, with how much where that is known.
	static func modification(_ modification: String, status: [AnyHashable: Any], sourceRate: Double, renderFormat: AudioStreamBasicDescription) -> String {
		switch modification {
		case CogAudioOutputModificationResampling:
			return String(format: NSLocalizedString("Resampled from %@ to %@", comment: "Cog signal-integrity reason: source and output sample rates"),
			              sampleRate(sourceRate), sampleRate(renderFormat.mSampleRate))
		case CogAudioOutputModificationTrackGain:
			if let gain = status[CogAudioOutputTrackGainKey] as? Double {
				return String(format: NSLocalizedString("ReplayGain or tagged volume: %@ dB", comment: "Cog signal-integrity reason: the track's gain"), signedDecibels(gain))
			}
			return NSLocalizedString("ReplayGain or tagged volume applied", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationChannelLayout:
			let to = channels(renderFormat.mChannelsPerFrame)
			if let fitted = status[CogAudioOutputFittedChannelsKey] as? UInt32 {
				return String(format: NSLocalizedString("Channels remapped from %@ to %@", comment: "Cog signal-integrity reason: channels before and after"), channels(fitted), to)
			}
			return String(format: NSLocalizedString("Channels remapped to %@", comment: "Cog signal-integrity reason: the device's channels"), to)
		case CogAudioOutputModificationVolume:
			if let volume = status[CogAudioOutputVolumeKey] as? Double {
				return String(format: NSLocalizedString("Cog volume at %@", comment: "Cog signal-integrity reason: Cog's volume in percent"), percentage(volume))
			}
			return NSLocalizedString("Cog volume is not 100%", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationDSDToPCM:
			return NSLocalizedString("DSD converted to PCM", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationHDCD:
			return NSLocalizedString("HDCD decoded", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationPrecision:
			return NSLocalizedString("Sample precision reduced", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationTimeStretch:
			return NSLocalizedString("Tempo or pitch changed", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationFreeSurround:
			return NSLocalizedString("Upmixed by FreeSurround", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationEqualizer:
			return NSLocalizedString("Equalizer applied", comment: "Cog signal-integrity reason")
		case CogAudioOutputModificationSpatialAudio:
			return NSLocalizedString("Spatialized for headphones by macOS", comment: "Cog signal-integrity reason")
		default:
			return modification
		}
	}

	static func gainSource(_ source: String) -> String {
		switch source {
		case "album":
			return NSLocalizedString("Album gain", comment: "Signal Inspector: the gain comes from the album's ReplayGain")
		case "track":
			return NSLocalizedString("Track gain", comment: "Signal Inspector: the gain comes from the track's ReplayGain")
		case "soundcheck":
			return NSLocalizedString("Sound Check", comment: "Signal Inspector: the gain comes from iTunes Sound Check")
		default:
			return NSLocalizedString("Tagged volume", comment: "Signal Inspector: the gain comes from a volume tag")
		}
	}

	/// "Rubber Band R3 · tempo 1.25× · pitch +2.0 st".
	static func timeStretch(_ stretch: [String: Any]) -> String {
		let tempo = stretch[CogAudioOutputStageTempoKey] as? Double ?? 1
		let pitch = stretch[CogAudioOutputStagePitchKey] as? Double ?? 1
		let engine: String
		switch stretch[CogAudioOutputStageEngineKey] as? String {
		case "varispeed":
			engine = NSLocalizedString("Varispeed", comment: "Time stretch engine: resampling, so the pitch follows the tempo")
		case "faster":
			engine = "Rubber Band R2"
		case "finer":
			engine = "Rubber Band R3"
		case "signalsmith":
			engine = "Signalsmith Stretch"
		default:
			engine = stretch[CogAudioOutputStageEngineKey] as? String ?? ""
		}
		var parts = [engine, String(format: NSLocalizedString("tempo %@×", comment: "Signal Inspector: a tempo ratio, e.g. tempo 1.25×"),
		                            String.localizedStringWithFormat("%.3g", tempo))]
		if abs(pitch - 1) > 1e-6 {
			let semitones = 12 * log2(pitch)
			parts.append(String(format: NSLocalizedString("pitch %@ st", comment: "Signal Inspector: a pitch shift in semitones, e.g. pitch +2.0 st"),
			                    signedDecibels(semitones)))
		}
		return parts.joined(separator: " · ")
	}

	/// "Preamp −3.0 dB · 4 bands, −6.0 to +4.5 dB", or flat.
	static func equalizer(_ equalizer: [String: Any]) -> String {
		let preamp = equalizer[CogAudioOutputStagePreampKey] as? Double ?? 0
		let gains = equalizer[CogAudioOutputStageBandGainsKey] as? [Double] ?? []
		var parts = [String(format: NSLocalizedString("Preamp %@ dB", comment: "Signal Inspector: the equalizer's preamp"), signedDecibels(preamp))]
		let adjusted = gains.filter { abs($0) >= 0.05 }
		if let low = adjusted.min(), let high = adjusted.max() {
			parts.append(String(format: NSLocalizedString("%ld of %ld bands, %@ to %@ dB", comment: "Signal Inspector: equalizer bands moved, and their range in dB"),
			                    adjusted.count, gains.count, signedDecibels(low), signedDecibels(high)))
		} else {
			parts.append(NSLocalizedString("bands flat", comment: "Signal Inspector: no equalizer band is moved"))
		}
		return parts.joined(separator: " · ")
	}

	static func spatialOutput(_ output: String?) -> String {
		switch output {
		case "builtInSpeakers":
			return NSLocalizedString("For built-in speakers", comment: "Signal Inspector: the spatial mixer renders for the Mac's speakers")
		case "externalSpeakers":
			return NSLocalizedString("For external speakers", comment: "Signal Inspector: the spatial mixer renders for external speakers")
		default:
			return NSLocalizedString("For headphones", comment: "Signal Inspector: the spatial mixer renders for headphones")
		}
	}

	/// "20 ms buffer, 31 ms to the ear".
	static func latency(_ status: [AnyHashable: Any], renderFormat: AudioStreamBasicDescription) -> String? {
		guard renderFormat.mSampleRate > 0, let buffer = status[CogAudioOutputBufferFramesKey] as? Int, let latency = status[CogAudioOutputLatencyFramesKey] as? Int,
		      buffer > 0 || latency > 0 else { return nil }
		return String(format: NSLocalizedString("%@ ms buffer, %@ ms latency", comment: "Signal Inspector: the device's I/O buffer and output latency"),
		              milliseconds(Double(buffer) / renderFormat.mSampleRate), milliseconds(Double(latency) / renderFormat.mSampleRate))
	}

	static func milliseconds(_ seconds: Double) -> String {
		String.localizedStringWithFormat("%.0f", seconds * 1000)
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
}
