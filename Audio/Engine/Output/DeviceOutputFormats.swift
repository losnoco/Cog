//
//  DeviceOutputFormats.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import AudioToolbox
import Foundation

/// What `DeviceOutput` shares between macOS (AUHAL, `DeviceOutput.swift`) and
/// iOS (RemoteIO, `DeviceOutput+iOS.swift`): formats, rates, layouts and the
/// settings both read.
extension DeviceOutput {
	// MARK: - Spatial audio

	/// What the engine renders for the spatial mixer: a 7.1 bed (WAVE order:
	/// L R C LFE, back L R, side L R), which every surround layout up to
	/// eight channels fits into, so tracks of different layouts share it.
	public static func spatialFormat(sampleRate: Double) -> StreamFormat {
		StreamFormat(sampleRate: sampleRate, channels: 8, channelConfig: UInt32(AudioConfig7Point1))
	}

	/// How a device's spatial audio is rendered: for headphones, for
	/// speakers (the Mac's own, or external ones), or not at all (a plain
	/// downmix). Chosen per device in `spatialDevicesKey`, by its UID;
	/// otherwise `automatic` guesses.
	public enum SpatialOutput: String {
		case headphones
		case speakers
		case off
	}

	/// Per-device choices, `[device UID: SpatialOutput.rawValue]`; a device
	/// missing from it is `automatic`.
	public static let spatialDevicesKey = "spatialAudioDevices"

	/// The setting `setHeadTracking` follows, read when the mixer is set up.
	public static let headTrackingKey = "enableHeadTracking"

	static func planarFloatASBD(sampleRate: Double, channels: Int) -> AudioStreamBasicDescription {
		AudioStreamBasicDescription(mSampleRate: sampleRate, mFormatID: kAudioFormatLinearPCM,
		                            mFormatFlags: kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved,
		                            mBytesPerPacket: 4, mFramesPerPacket: 1, mBytesPerFrame: 4, mChannelsPerFrame: UInt32(channels),
		                            mBitsPerChannel: 32, mReserved: 0)
	}

	// MARK: - Render formats

	/// The I/O buffer to ask for, in milliseconds (hidden setting
	/// `outputBufferMilliseconds`).
	static var bufferMilliseconds: Double {
		let setting = UserDefaults.standard.double(forKey: "outputBufferMilliseconds")
		return setting > 0 ? setting : 20
	}

	/// 24-bit samples high-aligned in 32-bit words, as DoP DACs expect: no
	/// float conversion can then disturb the carrier.
	static func integerASBD(_ format: StreamFormat) -> AudioStreamBasicDescription {
		let bytesPerFrame = UInt32(MemoryLayout<Int32>.size * format.channels)
		return AudioStreamBasicDescription(mSampleRate: format.sampleRate, mFormatID: kAudioFormatLinearPCM,
		                                   mFormatFlags: kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsAlignedHigh | kAudioFormatFlagsNativeEndian,
		                                   mBytesPerPacket: bytesPerFrame, mFramesPerPacket: 1,
		                                   mBytesPerFrame: bytesPerFrame, mChannelsPerFrame: UInt32(format.channels),
		                                   mBitsPerChannel: 24, mReserved: 0)
	}

	/// The renderer's sample format for a stream format, if it can write it:
	/// interleaved, native-endian Float32 or signed integer of 16, 24 or 32
	/// bits.
	static func sampleFormat(of format: AudioStreamBasicDescription) -> CogSampleFormat? {
		let flags = format.mFormatFlags
		guard format.mFormatID == kAudioFormatLinearPCM, format.mChannelsPerFrame > 0, format.mFramesPerPacket <= 1,
		      flags & kAudioFormatFlagIsNonInterleaved == 0,
		      flags & kAudioFormatFlagIsBigEndian == kAudioFormatFlagsNativeEndian & kAudioFormatFlagIsBigEndian else {
			return nil
		}
		let bytes = format.mBytesPerFrame / format.mChannelsPerFrame
		if flags & kAudioFormatFlagIsFloat != 0 {
			return format.mBitsPerChannel == 32 && bytes == 4 ? .float32 : nil
		}
		guard flags & kAudioFormatFlagIsSignedInteger != 0 else { return nil }
		switch (format.mBitsPerChannel, bytes) {
		case (32, 4): return .int32
		case (24, 4): return flags & kAudioFormatFlagIsAlignedHigh != 0 ? .int24High : .int24Low
		case (24, 3): return .int24Packed
		case (16, 2): return .int16
		default: return nil
		}
	}

	/// Two formats laid out alike, rate aside.
	static func sameRepresentation(_ a: AudioStreamBasicDescription, _ b: AudioStreamBasicDescription) -> Bool {
		a.mFormatID == b.mFormatID && a.mFormatFlags == b.mFormatFlags && a.mBytesPerFrame == b.mBytesPerFrame &&
			a.mChannelsPerFrame == b.mChannelsPerFrame && a.mBitsPerChannel == b.mBitsPerChannel
	}

	/// "sint24/32 non-mixable at 96000 Hz", for the log.
	static func describe(_ format: AudioStreamBasicDescription) -> String {
		let flags = format.mFormatFlags
		let kind = flags & kAudioFormatFlagIsFloat != 0 ? "float" : (flags & kAudioFormatFlagIsSignedInteger != 0 ? "sint" : "uint")
		let word = format.mChannelsPerFrame > 0 ? format.mBytesPerFrame / format.mChannelsPerFrame * 8 : 0
		let alignment = kind != "float" && format.mBitsPerChannel < word ? (flags & kAudioFormatFlagIsAlignedHigh != 0 ? " high" : " low") : ""
		return "\(kind)\(format.mBitsPerChannel)/\(word)\(alignment)\(flags & kAudioFormatFlagIsNonMixable != 0 ? " non-mixable" : ""), \(format.mChannelsPerFrame) ch at \(Int(format.mSampleRate)) Hz"
	}

	static let fullVolumeKey = "setDeviceVolumeTo100ForExclusiveOutput"

	// MARK: - Device rate

	static func supports(_ rate: Double, among ranges: [AudioValueRange]) -> Bool {
		ranges.isEmpty || ranges.contains { rate >= $0.mMinimum - 1 && rate <= $0.mMaximum + 1 }
	}

	/// The rate to run a device held exclusively at for audio at `rate`: that
	/// rate if the device offers it, so nothing is resampled. Otherwise, of
	/// the rates it offers, the lowest above that is a whole multiple of
	/// `rate` (22.05 kHz plays at 44.1), else the highest below that divides
	/// it (DSD64 as PCM, 352.8 kHz, plays at 176.4 on a 192 kHz device),
	/// else the nearest above, else the highest.
	static func deviceRate(for rate: Double, among ranges: [AudioValueRange]) -> Double {
		guard rate > 0, !supports(rate, among: ranges) else { return rate }
		let common: [Double] = [8000, 11025, 16000, 22050, 32000, 44100, 48000, 64000, 88200, 96000, 128000,
		                        176400, 192000, 352800, 384000, 705600, 768000, 1411200, 1536000]
		let offered = Set(common.filter { supports($0, among: ranges) } + ranges.flatMap { [$0.mMinimum, $0.mMaximum] }).filter { $0 > 0 }
		func related(_ a: Double, _ b: Double) -> Bool {
			let ratio = a / b
			return abs(ratio - ratio.rounded()) < 1e-6
		}
		let above = offered.filter { $0 > rate }.sorted()
		let below = offered.filter { $0 < rate }.sorted(by: >)
		return above.first { related($0, rate) } ?? below.first { related(rate, $0) } ?? above.first ?? below.first ?? rate
	}

	// MARK: - Channel layouts

	/// The layouts Cog has always used for 1–8 device channels.
	static func layoutTag(channels: Int) -> AudioChannelLayoutTag {
		switch channels {
		case 1: return kAudioChannelLayoutTag_Mono
		case 2: return kAudioChannelLayoutTag_Stereo
		case 3: return kAudioChannelLayoutTag_DVD_4
		case 4: return kAudioChannelLayoutTag_Quadraphonic
		case 5: return kAudioChannelLayoutTag_MPEG_5_0_A
		case 6: return kAudioChannelLayoutTag_MPEG_5_1_A
		case 7: return kAudioChannelLayoutTag_MPEG_6_1_A
		default: return kAudioChannelLayoutTag_MPEG_7_1_A
		}
	}

	/// Cog's channel-flag configuration for the same layouts.
	static func channelConfig(channels: Int) -> UInt32 {
		switch channels {
		case 1: return UInt32(AudioConfigMono)
		case 2: return UInt32(AudioConfigStereo)
		case 3: return UInt32(AudioConfig3Point0)
		case 4: return UInt32(AudioConfig4Point0)
		case 5: return UInt32(AudioConfig5Point0)
		case 6: return UInt32(AudioConfig5Point1)
		case 7: return UInt32(AudioConfig6Point1)
		default: return UInt32(AudioConfig7Point1)
		}
	}
}
