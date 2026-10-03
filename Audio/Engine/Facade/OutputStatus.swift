//
//  OutputStatus.swift
//  CogAudio
//
//  Created by Marko Jurkovic on 9/30/26.
//

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
	/// Cog's volume, in percent.
	var volume: Double = 100
	var deviceID = AudioDeviceID(kAudioObjectUnknown)
	var followsSystemDefault = false
	/// The format rendered for the device, and the sample words Core Audio
	/// takes it in.
	var render = StreamFormat(sampleRate: 0, channels: 0)
	var renderFormat = AudioStreamBasicDescription()
	var exclusive = false

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
			lhs.exclusive == rhs.exclusive &&
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
		return userInfo
	}

	/// As `[NSValue valueWithBytes:&format objCType:@encode(AudioStreamBasicDescription)]`.
	static func value(_ format: AudioStreamBasicDescription) -> NSValue {
		withUnsafePointer(to: format) { NSValue(bytes: $0, objCType: "{AudioStreamBasicDescription=dIIIIIIII}") }
	}
}
