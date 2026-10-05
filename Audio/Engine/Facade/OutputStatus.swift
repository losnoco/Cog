//
//  OutputStatus.swift
//  CogAudio
//
//  Created by Marko Jurkovic on 9/30/26.
//

import AudioToolbox
import CoreAudio
import Foundation

/// A track's samples as its decoder produces them.
struct SourceFormat: Equatable {
	let asbd: AudioStreamBasicDescription
	/// The decoder's names for its codec and its encoding ("lossless",
	/// "lossy" or "synthesized"), if it gives them.
	let codec: String?
	let encoding: String?

	init?(properties: [AnyHashable: Any]?) {
		guard let properties else { return nil }
		let asbd = propertiesToASBD(properties)
		guard asbd.mSampleRate > 0, asbd.mBitsPerChannel > 0, asbd.mChannelsPerFrame > 0 else { return nil }
		self.asbd = asbd
		codec = properties["codec"] as? String
		encoding = properties["encoding"] as? String
	}

	var isDSD: Bool { asbd.mBitsPerChannel == 1 }
	var isFloat: Bool { asbd.mFormatFlags & kAudioFormatFlagIsFloat != 0 }

	static func == (lhs: Self, rhs: Self) -> Bool {
		lhs.codec == rhs.codec && lhs.encoding == rhs.encoding &&
			withUnsafeBytes(of: lhs.asbd) { lhs in withUnsafeBytes(of: rhs.asbd) { rhs in lhs.elementsEqual(rhs) } }
	}
}

/// What playback does between the decoder and Core Audio for the audio being
/// heard, as `CogAudioOutputStatusDidChangeNotification` reports it. The
/// engine sends it again only when this changes.
struct OutputStatus: Equatable {
	/// The heard track as decoded, if its decoder described it.
	var source: SourceFormat?
	/// HDCD was found in the track, and decoding it is on.
	var decodesHDCD = false
	/// What the DSP thread did to it.
	var processing = Pump.Processing()
	/// The modifications of the DSP stages that changed it, in chain order.
	var stageModifications: [String] = []
	/// The track's linear gain (ReplayGain or volume scaling).
	var trackGain: Float = 1
	/// Where that gain came from, and whether the peak limited it.
	var gainSource: ReplayGain.Source?
	var gainPeakLimited = false
	/// Cog's volume, in percent.
	var volume: Double = 100
	var deviceID = AudioDeviceID(kAudioObjectUnknown)
	var followsSystemDefault = false
	/// The format rendered for the device, and the sample words Core Audio
	/// takes it in.
	var render = StreamFormat(sampleRate: 0, channels: 0)
	var renderFormat = AudioStreamBasicDescription()
	var exclusive = false
	/// While spatial, what the mixer renders for, and whether it follows
	/// the listener's head.
	var spatialRendering: AUSpatialMixerOutputType?
	var headTracking = false
	/// Spatial audio is on for the device, but the mixer could not be set up.
	var spatialRefused = false
	/// The device's I/O buffer, and the frames from Cog to the listener.
	var bufferFrames = 0
	var latencyFrames = 0

	/// As the renderer applies it.
	var unityVolume: Bool { Float(volume * 0.01) == 1 }

	/// Whether Cog hands Core Audio integers rather than float.
	var integerRender: Bool {
		renderFormat.mFormatID == kAudioFormatLinearPCM && renderFormat.mFormatFlags & kAudioFormatFlagIsFloat == 0
	}

	static func == (lhs: Self, rhs: Self) -> Bool {
		lhs.source == rhs.source && lhs.decodesHDCD == rhs.decodesHDCD && lhs.processing == rhs.processing &&
			lhs.stageModifications == rhs.stageModifications && lhs.trackGain == rhs.trackGain && lhs.volume == rhs.volume &&
			lhs.deviceID == rhs.deviceID && lhs.followsSystemDefault == rhs.followsSystemDefault && lhs.render == rhs.render &&
			lhs.exclusive == rhs.exclusive && lhs.gainSource == rhs.gainSource && lhs.gainPeakLimited == rhs.gainPeakLimited &&
			lhs.spatialRendering == rhs.spatialRendering && lhs.headTracking == rhs.headTracking && lhs.spatialRefused == rhs.spatialRefused &&
			lhs.bufferFrames == rhs.bufferFrames && lhs.latencyFrames == rhs.latencyFrames &&
			withUnsafeBytes(of: lhs.renderFormat) { lhs in withUnsafeBytes(of: rhs.renderFormat) { rhs in lhs.elementsEqual(rhs) } }
	}

	/// How the decoded samples are changed before Core Audio has them, in
	/// signal order; empty if they are not, nil if that cannot be told.
	///
	/// Transport fades are not counted: they only shape the start, a pause
	/// and a seek, and the gain rests at exactly 1 in between.
	var modifications: [String]? {
		guard let source else { return nil }
		// Passed through by the pump and the renderer alike, volume and all.
		if processing.passesDoP { return [] }

		var modifications: [String] = []
		if source.isDSD {
			modifications.append(CogAudioOutputModificationDSDToPCM)
		} else {
			if decodesHDCD {
				modifications.append(CogAudioOutputModificationHDCD)
			} else if losesPrecision(source) {
				modifications.append(CogAudioOutputModificationPrecision)
			}
			// The feeder's resampler is bypassed, bit-exact, at equal rates.
			if abs(source.asbd.mSampleRate - render.sampleRate) >= 0.5 {
				modifications.append(CogAudioOutputModificationResampling)
			}
		}
		if processing.appliesGain {
			modifications.append(CogAudioOutputModificationTrackGain)
		}
		modifications += stageModifications
		// Surround fitted into the spatial mixer's 7.1 bed loses nothing;
		// the spatialization is the modification.
		if processing.fitsChannels && !stageModifications.contains(CogAudioOutputModificationSpatialAudio) {
			modifications.append(CogAudioOutputModificationChannelLayout)
		}
		if !unityVolume {
			modifications.append(CogAudioOutputModificationVolume)
		}
		return modifications
	}

	/// Everything is converted to Float32, which holds integers of up to 24
	/// bits exactly but rounds anything wider. Integer output then holds as
	/// many bits as it has (24 for a DoP carrier; a device held exclusively
	/// may run at 16) and rounds any other float.
	private func losesPrecision(_ source: SourceFormat) -> Bool {
		if source.isFloat {
			return source.asbd.mBitsPerChannel > 32 || integerRender
		}
		let kept = integerRender ? min(24, renderFormat.mBitsPerChannel) : 24
		return source.asbd.mBitsPerChannel > kept
	}

	/// The notification's userInfo, with the device's name and stream formats
	/// as Core Audio reports them now.
	func userInfo(deviceName: String?, virtualFormats: [AudioStreamBasicDescription], physicalFormats: [AudioStreamBasicDescription]) -> [AnyHashable: Any] {
		var userInfo: [AnyHashable: Any] = [
			CogAudioOutputRenderFormatKey: Self.value(renderFormat),
			CogAudioOutputDoPKey: processing.passesDoP,
			CogAudioOutputSystemDefaultKey: followsSystemDefault,
			CogAudioOutputExclusiveKey: exclusive,
			CogAudioOutputVirtualFormatsKey: virtualFormats.map(Self.value),
			CogAudioOutputPhysicalFormatsKey: physicalFormats.map(Self.value),
		]
		if let deviceName {
			userInfo[CogAudioOutputDeviceNameKey] = deviceName
		}
		if let source {
			userInfo[CogAudioOutputSourceFormatKey] = Self.value(source.asbd)
			userInfo[CogAudioOutputSourceCodecKey] = source.codec
			userInfo[CogAudioOutputSourceEncodingKey] = source.encoding
		}
		if let modifications {
			userInfo[CogAudioOutputModificationsKey] = modifications
			if modifications.contains(CogAudioOutputModificationTrackGain), trackGain > 0 {
				userInfo[CogAudioOutputTrackGainKey] = 20 * log10(Double(trackGain))
			}
			if modifications.contains(CogAudioOutputModificationChannelLayout) {
				userInfo[CogAudioOutputFittedChannelsKey] = processing.output.channels
			}
			if modifications.contains(CogAudioOutputModificationVolume) {
				userInfo[CogAudioOutputVolumeKey] = volume
			}
		}
		addStages(to: &userInfo)
		return userInfo
	}

	/// The resampler's input: the track's rate, or for DSD the PCM rate the
	/// feeder decimates it to.
	var resamplerInputRate: Double? {
		guard let source, !processing.passesDoP else { return nil }
		return source.isDSD ? source.asbd.mSampleRate / 8 : source.asbd.mSampleRate
	}

	/// Each stage's settings, and what surrounds them, for inspection.
	private func addStages(to userInfo: inout [AnyHashable: Any]) {
		userInfo[CogAudioOutputBufferFramesKey] = bufferFrames
		userInfo[CogAudioOutputLatencyFramesKey] = latencyFrames
		if let gainSource {
			userInfo[CogAudioOutputTrackGainSourceKey] = gainSource.rawValue
			userInfo[CogAudioOutputTrackGainPeakLimitedKey] = gainPeakLimited
		}
		if spatialRefused {
			userInfo[CogAudioOutputSpatialRefusedKey] = true
		}
		if let inputRate = resamplerInputRate {
			userInfo[CogAudioOutputResamplerKey] = [
				CogAudioOutputStageActiveKey: abs(inputRate - render.sampleRate) >= 0.5,
				CogAudioOutputStageInputRateKey: inputRate,
				CogAudioOutputStageOutputRateKey: render.sampleRate,
				CogAudioOutputStageQualityKey: "HQ",
			] as [String: Any]
		}
		for inspection in processing.inspections {
			switch inspection {
			case let .timeStretch(engine, tempo, pitch):
				userInfo[CogAudioOutputTimeStretchKey] = [
					CogAudioOutputStageActiveKey: true,
					CogAudioOutputStageEngineKey: engine,
					CogAudioOutputStageTempoKey: tempo,
					CogAudioOutputStagePitchKey: pitch,
				] as [String: Any]
			case let .freeSurround(upmixes, output):
				userInfo[CogAudioOutputFreeSurroundKey] = [
					CogAudioOutputStageActiveKey: upmixes,
					CogAudioOutputStageChannelsKey: output.channels,
					CogAudioOutputStageChannelConfigKey: output.channelConfig,
				] as [String: Any]
			case let .equalizer(preampDB, gainsDB):
				userInfo[CogAudioOutputEqualizerKey] = [
					CogAudioOutputStageActiveKey: true,
					CogAudioOutputStagePreampKey: Double(preampDB),
					CogAudioOutputStageBandFrequenciesKey: EqualizerStage.bands,
					CogAudioOutputStageBandGainsKey: gainsDB.map(Double.init),
				] as [String: Any]
			}
		}
		if stageModifications.contains(CogAudioOutputModificationSpatialAudio) {
			var spatial: [String: Any] = [CogAudioOutputStageActiveKey: true, CogAudioOutputStageHeadTrackingKey: headTracking]
			switch spatialRendering {
			case .spatialMixerOutputType_Headphones?:
				spatial[CogAudioOutputStageSpatialOutputKey] = "headphones"
			case .spatialMixerOutputType_BuiltInSpeakers?:
				spatial[CogAudioOutputStageSpatialOutputKey] = "builtInSpeakers"
			case .spatialMixerOutputType_ExternalSpeakers?:
				spatial[CogAudioOutputStageSpatialOutputKey] = "externalSpeakers"
			default:
				break
			}
			userInfo[CogAudioOutputSpatialKey] = spatial
		}
	}

	/// As `[NSValue valueWithBytes:&format objCType:@encode(AudioStreamBasicDescription)]`.
	static func value(_ format: AudioStreamBasicDescription) -> NSValue {
		withUnsafePointer(to: format) { NSValue(bytes: $0, objCType: "{AudioStreamBasicDescription=dIIIIIIII}") }
	}
}

/// The audio reaching the device lately, for a live view of the output:
/// levels since the last call, and counts since the heard track began.
/// Taken on the main thread by `PlaybackEngine.signalMetrics()` while
/// metering is on.
@objc public final class SignalMetrics: NSObject {
	/// Per channel of the render format (at most `COG_METER_CHANNELS`, any
	/// beyond in the last), linear: the largest sample, and the RMS. Empty
	/// if nothing was metered since the last call: paused, dry, or DoP.
	@objc public let peaks: [NSNumber]
	@objc public let rms: [NSNumber]
	/// The render format's channels and their layout, for labelling.
	@objc public let channels: Int
	@objc public let channelConfig: UInt32
	/// Since the heard track began: samples beyond full scale (counted only
	/// while metering), times the shallow ring ran dry, and cycles the
	/// device skipped or repeated.
	@objc public let clippedSamples: UInt64
	@objc public let underruns: UInt64
	@objc public let discontinuities: UInt64
	/// Seconds of audio waiting: DSP output for the device, and decoded
	/// audio for the DSP thread.
	@objc public let shallowBufferSeconds: Double
	@objc public let deepBufferSeconds: Double
	/// From Cog handing audio over to it being heard.
	@objc public let outputLatencySeconds: Double

	init(snapshot: CogMeterSnapshot, channels: Int, channelConfig: UInt32, clippedSamples: UInt64, underruns: UInt64, discontinuities: UInt64,
	     shallowBufferSeconds: Double, deepBufferSeconds: Double, outputLatencySeconds: Double) {
		let slots = snapshot.frames > 0 ? min(channels, Int(COG_METER_CHANNELS)) : 0
		var peaks: [NSNumber] = []
		var rms: [NSNumber] = []
		withUnsafeBytes(of: snapshot.peak) { peak in
			withUnsafeBytes(of: snapshot.sumOfSquares) { sums in
				let peak = peak.bindMemory(to: Float.self)
				let sums = sums.bindMemory(to: Double.self)
				for slot in 0..<slots {
					// The last slot holds every channel from there on.
					let sharing = slot == Int(COG_METER_CHANNELS) - 1 ? channels - slot : 1
					peaks.append(NSNumber(value: peak[slot]))
					rms.append(NSNumber(value: (sums[slot] / (Double(snapshot.frames) * Double(sharing))).squareRoot()))
				}
			}
		}
		self.peaks = peaks
		self.rms = rms
		self.channels = channels
		self.channelConfig = channelConfig
		self.clippedSamples = clippedSamples
		self.underruns = underruns
		self.discontinuities = discontinuities
		self.shallowBufferSeconds = shallowBufferSeconds
		self.deepBufferSeconds = deepBufferSeconds
		self.outputLatencySeconds = outputLatencySeconds
	}
}
