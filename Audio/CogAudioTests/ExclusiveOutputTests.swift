//
//  ExclusiveOutputTests.swift
//  CogAudioTests
//
//  Created by Marko Jurkovic on 9/30/26.
//

@testable import CogAudio
import XCTest

/// Integer PCM of 16, 24 or 32 bits, as a lossless decoder produces it.
final class IntegerMemoryDecoder: NSObject, CogDecoder {
	let samples: [Int32]
	let bits: Int
	let sampleRate: Double
	let channels: Int
	private var position = 0

	init(samples: [Int32], bits: Int, sampleRate: Double, channels: Int = 2) {
		self.samples = samples
		self.bits = bits
		self.sampleRate = sampleRate
		self.channels = channels
	}

	static func mimeTypes() -> [Any]! { [] }
	static func fileTypes() -> [Any]! { [] }
	static func fileTypeAssociations() -> [Any]! { [] }
	static func priority() -> Float { 1 }

	func properties() -> [AnyHashable: Any]! {
		["sampleRate": sampleRate, "channels": channels, "bitsPerSample": bits, "floatingPoint": false,
		 "totalFrames": samples.count / channels, "seekable": true, "encoding": "lossless", "codec": "Test"]
	}

	func metadata() -> [AnyHashable: Any]! { [:] }

	func readAudio() -> AudioChunk! {
		let frames = samples.count / channels
		guard position < frames else { return AudioChunk() }
		let count = min(4096, frames - position)
		let chunk = Self.chunk(Array(samples[(position * channels)..<((position + count) * channels)]), bits: bits, sampleRate: sampleRate, channels: channels)
		position += count
		return chunk
	}

	/// `samples` packed as `bits`-bit signed integers (24 in three bytes).
	static func chunk(_ samples: [Int32], bits: Int, sampleRate: Double, channels: Int) -> AudioChunk {
		let bytes = bits == 24 ? 3 : bits / 8
		var data = Data(capacity: samples.count * bytes)
		for sample in samples {
			withUnsafeBytes(of: sample.littleEndian) { data.append(contentsOf: $0.prefix(bytes)) }
		}
		let chunk = AudioChunk()
		let bytesPerFrame = UInt32(bytes * channels)
		chunk.format = AudioStreamBasicDescription(mSampleRate: sampleRate, mFormatID: kAudioFormatLinearPCM,
		                                           mFormatFlags: kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked,
		                                           mBytesPerPacket: bytesPerFrame, mFramesPerPacket: 1,
		                                           mBytesPerFrame: bytesPerFrame, mChannelsPerFrame: UInt32(channels),
		                                           mBitsPerChannel: UInt32(bits), mReserved: 0)
		data.withUnsafeBytes { chunk.assignSamples($0.baseAddress!, frameCount: samples.count / channels) }
		return chunk
	}

	func open(_ source: CogSource!) -> Bool { true }

	func seek(_ frame: Int) -> Int {
		position = min(max(0, frame), samples.count / channels)
		return position
	}

	func close() {}
}

final class ExclusiveOutputTests: XCTestCase {
	// MARK: - Integers in, the same integers out

	/// Integer PCM made float the way the feeder does it, through ChunkList
	/// (with the HDCD decoder engaged for lossless 16-bit, 44.1 kHz stereo,
	/// as it is by default).
	private func floats(_ samples: [Int32], bits: Int, lossless: Bool = true) -> [Float] {
		let list = ChunkList(maximumDuration: 100)
		let chunk = IntegerMemoryDecoder.chunk(samples, bits: bits, sampleRate: 44100, channels: 2)
		chunk.lossless = lossless
		list.add(chunk)
		var floats: [Float] = []
		while !list.isEmpty() {
			let converted = list.removeSamples(asFloat32: 65536)
			guard converted.frameCount() > 0 else { break }
			let data = converted.removeSamples(converted.frameCount())
			data.withUnsafeBytes { floats += $0.bindMemory(to: Float.self) }
		}
		return floats
	}

	private func convert(_ floats: [Float], to format: CogSampleFormat) -> [UInt8] {
		var out = [UInt8](repeating: 0, count: floats.count * cog_sample_format_bytes(format))
		floats.withUnsafeBufferPointer { cog_convert_samples(&out, format, $0.baseAddress!, floats.count) }
		return out
	}

	private func words(_ bytes: [UInt8], _ format: CogSampleFormat) -> [Int32] {
		switch format {
		case .int16:
			return stride(from: 0, to: bytes.count, by: 2).map { Int32(Int16(bitPattern: UInt16(bytes[$0]) | UInt16(bytes[$0 + 1]) << 8)) }
		case .int24Packed:
			return stride(from: 0, to: bytes.count, by: 3).map {
				Int32(bitPattern: UInt32(bytes[$0]) << 8 | UInt32(bytes[$0 + 1]) << 16 | UInt32(bytes[$0 + 2]) << 24) >> 8
			}
		default:
			return stride(from: 0, to: bytes.count, by: 4).map {
				Int32(bitPattern: UInt32(bytes[$0]) | UInt32(bytes[$0 + 1]) << 8 | UInt32(bytes[$0 + 2]) << 16 | UInt32(bytes[$0 + 3]) << 24)
			}
		}
	}

	/// Every 16-bit value comes back exactly in every layout, whether or not
	/// the HDCD decoder saw it.
	func testEvery16BitSampleComesBackExactly() {
		let samples = (Int32(Int16.min)...Int32(Int16.max)).map { $0 }
		for lossless in [true, false] {
			let floats = floats(samples, bits: 16, lossless: lossless)
			XCTAssertEqual(floats.count, samples.count)
			XCTAssertEqual(words(convert(floats, to: .int16), .int16), samples, "Int16, lossless \(lossless)")
			XCTAssertEqual(words(convert(floats, to: .int32), .int32), samples.map { $0 << 16 }, "Int32, lossless \(lossless)")
			XCTAssertEqual(words(convert(floats, to: .int24High), .int24High), samples.map { $0 << 16 }, "Int24 high, lossless \(lossless)")
			XCTAssertEqual(words(convert(floats, to: .int24Low), .int24Low), samples.map { $0 << 8 }, "Int24 low, lossless \(lossless)")
			XCTAssertEqual(words(convert(floats, to: .int24Packed), .int24Packed), samples.map { $0 << 8 }, "Int24 packed, lossless \(lossless)")
		}
	}

	/// 24-bit values come back exactly in every layout that can hold them.
	func testEvery24BitSampleComesBackExactly() {
		var samples = Array(stride(from: Int32(-(1 << 23)), through: Int32((1 << 23) - 1), by: 97))
		samples += [-(1 << 23), -(1 << 23) + 1, -1, 0, 1, (1 << 23) - 2, (1 << 23) - 1]
		if samples.count % 2 == 1 { samples.append(0) }
		let floats = floats(samples, bits: 24)
		XCTAssertEqual(floats.count, samples.count)
		XCTAssertEqual(words(convert(floats, to: .int32), .int32), samples.map { $0 << 8 })
		XCTAssertEqual(words(convert(floats, to: .int24High), .int24High), samples.map { $0 << 8 })
		XCTAssertEqual(words(convert(floats, to: .int24Low), .int24Low), samples)
		XCTAssertEqual(words(convert(floats, to: .int24Packed), .int24Packed), samples)
	}

	/// Processed audio at or past full scale clips instead of wrapping to the
	/// other extreme, and anything between the steps rounds to the nearest.
	func testOutOfRangeSamplesClipAndOthersRound() {
		let justBelowOne = Float(1).nextDown
		let input: [Float] = [2, 1, justBelowOne, -1, -2, .nan, 0.5 / 32768, 1.5 / 32768, -0.75 / 32768]
		XCTAssertEqual(words(convert(input, to: .int16), .int16), [32767, 32767, 32767, -32768, -32768, 0, 0, 2, -1])
		XCTAssertEqual(words(convert(input, to: .int24Low), .int24Low).prefix(6), [8_388_607, 8_388_607, 8_388_607, -8_388_608, -8_388_608, 0])
		XCTAssertEqual(words(convert(input, to: .int24High), .int24High).prefix(3), [0x7FFF_FF00, 0x7FFF_FF00, 0x7FFF_FF00])
		XCTAssertEqual(words(convert(input, to: .int32), .int32).prefix(6), [Int32.max, Int32.max, 0x7FFF_FF80, Int32.min, Int32.min, 0])
	}

	// MARK: - Callbacks

	private func renderer(channels: UInt32, samples: [Float], format: CogSampleFormat, scratch: Int) -> (OpaquePointer, OpaquePointer) {
		let ring = cog_ring_create(4096, channels)!
		samples.withUnsafeBufferPointer { _ = cog_ring_write(ring, $0.baseAddress!, samples.count / Int(channels)) }
		let renderer = cog_renderer_create(ring)!
		XCTAssertTrue(cog_renderer_set_output_format(renderer, format, scratch))
		return (ring, renderer)
	}

	/// The device's IOProc fills the whole buffer in the stream's own words,
	/// however much more that is than the scratch buffer holds.
	func testTheIOProcFillsTheStreamPastTheScratchBuffer() {
		let samples = (0..<600).map { Float($0 - 300) / 32768 }
		let (ring, renderer) = renderer(channels: 2, samples: samples, format: .int16, scratch: 64)
		defer {
			cog_renderer_destroy(renderer)
			cog_ring_destroy(ring)
		}
		var bytes = [UInt8](repeating: 0xAA, count: 300 * 2 * 2)
		bytes.withUnsafeMutableBytes { raw in
			var list = AudioBufferList(mNumberBuffers: 1, mBuffers: AudioBuffer(mNumberChannels: 2, mDataByteSize: UInt32(raw.count), mData: raw.baseAddress))
			var input = AudioBufferList()
			var (now, inputTime, outputTime) = (AudioTimeStamp(), AudioTimeStamp(), AudioTimeStamp())
			_ = cog_renderer_device_io_proc(0, &now, &input, &inputTime, &list, &outputTime, UnsafeMutableRawPointer(renderer))
			XCTAssertEqual(list.mBuffers.mDataByteSize, UInt32(raw.count), "an IOProc's buffer is not resized")
		}
		XCTAssertEqual(words(bytes, .int16), (0..<600).map { Int32($0 - 300) })
		XCTAssertEqual(cog_renderer_frames_rendered(renderer), 300)
	}

	/// A stream of another channel count gets silence, not misaligned words.
	func testTheIOProcSilencesAStreamItWasNotSetUpFor() {
		let (ring, renderer) = renderer(channels: 2, samples: [Float](repeating: 0.5, count: 200), format: .int32, scratch: 64)
		defer {
			cog_renderer_destroy(renderer)
			cog_ring_destroy(ring)
		}
		var bytes = [UInt8](repeating: 0xAA, count: 50 * 6 * 4)
		bytes.withUnsafeMutableBytes { raw in
			var list = AudioBufferList(mNumberBuffers: 1, mBuffers: AudioBuffer(mNumberChannels: 6, mDataByteSize: UInt32(raw.count), mData: raw.baseAddress))
			var input = AudioBufferList()
			var (now, inputTime, outputTime) = (AudioTimeStamp(), AudioTimeStamp(), AudioTimeStamp())
			_ = cog_renderer_device_io_proc(0, &now, &input, &inputTime, &list, &outputTime, UnsafeMutableRawPointer(renderer))
		}
		XCTAssertTrue(bytes.allSatisfy { $0 == 0 })
		XCTAssertEqual(cog_ring_readable(ring), 100, "nothing was taken from the ring")
	}

	/// The unit's callback renders all it is asked for through a small
	/// scratch buffer too, in packed 24-bit words.
	func testTheUnitCallbackRendersPastTheScratchBuffer() {
		let samples = (0..<400).map { Float($0) / 8_388_608 }
		let (ring, renderer) = renderer(channels: 2, samples: samples, format: .int24Packed, scratch: 32)
		defer {
			cog_renderer_destroy(renderer)
			cog_ring_destroy(ring)
		}
		var bytes = [UInt8](repeating: 0, count: 200 * 2 * 3)
		bytes.withUnsafeMutableBytes { raw in
			var list = AudioBufferList(mNumberBuffers: 1, mBuffers: AudioBuffer(mNumberChannels: 2, mDataByteSize: UInt32(raw.count), mData: raw.baseAddress))
			var flags = AudioUnitRenderActionFlags()
			var time = AudioTimeStamp()
			_ = cog_renderer_audio_unit_render(UnsafeMutableRawPointer(renderer), &flags, &time, 0, 200, &list)
			XCTAssertEqual(list.mBuffers.mDataByteSize, UInt32(raw.count))
		}
		XCTAssertEqual(words(bytes, .int24Packed), (0..<400).map { Int32($0) })
	}

	/// 16-bit output dithers what processing left between its steps, and
	/// nothing else: steps stay exact and silence silent.
	func testSixteenBitOutputDithersOnlyBetweenSteps() {
		let between = [Float](repeating: 0.3 / 32768, count: 3000)
		let onSteps = (0..<200).map { Float($0 - 100) / 32768 }
		let silence = [Float](repeating: 0, count: 200)
		let (ring, renderer) = renderer(channels: 1, samples: between + onSteps + silence, format: .int16, scratch: 256)
		defer {
			cog_renderer_destroy(renderer)
			cog_ring_destroy(ring)
		}
		var bytes = [UInt8](repeating: 0, count: 3400 * 2)
		bytes.withUnsafeMutableBytes { raw in
			var list = AudioBufferList(mNumberBuffers: 1, mBuffers: AudioBuffer(mNumberChannels: 1, mDataByteSize: UInt32(raw.count), mData: raw.baseAddress))
			var input = AudioBufferList()
			var (now, inputTime, outputTime) = (AudioTimeStamp(), AudioTimeStamp(), AudioTimeStamp())
			_ = cog_renderer_device_io_proc(0, &now, &input, &inputTime, &list, &outputTime, UnsafeMutableRawPointer(renderer))
		}
		let out = words(bytes, .int16)
		let dithered = out[0..<3000]
		XCTAssertTrue(dithered.allSatisfy { (-1...1).contains($0) }, "within a step either way")
		XCTAssertGreaterThan(Set(dithered).count, 1, "not merely rounded")
		XCTAssertEqual(Double(dithered.reduce(0, +)) / 3000, 0.3, accuracy: 0.05, "the mean kept")
		XCTAssertEqual(Array(out[3000..<3200]), (0..<200).map { Int32($0 - 100) }, "steps exact")
		XCTAssertTrue(out[3200...].allSatisfy { $0 == 0 }, "silence silent")
	}

	// MARK: - Choosing the device's format

	private func ranged(_ rate: Double, bits: UInt32, bytes: UInt32, flags: AudioFormatFlags, channels: UInt32 = 2) -> AudioStreamRangedDescription {
		let format = AudioStreamBasicDescription(mSampleRate: rate, mFormatID: kAudioFormatLinearPCM, mFormatFlags: flags,
		                                         mBytesPerPacket: bytes * channels, mFramesPerPacket: 1, mBytesPerFrame: bytes * channels,
		                                         mChannelsPerFrame: channels, mBitsPerChannel: bits, mReserved: 0)
		return AudioStreamRangedDescription(mFormat: format, mSampleRateRange: AudioValueRange(mMinimum: rate, mMaximum: rate))
	}

	private let int32Flags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked
	private let int24HighFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsAlignedHigh
	private let floatFlags = kAudioFormatFlagsNativeFloatPacked

	/// As a USB DAC lists them (an SMSL: mixable first, both widths, every
	/// rate), plus a float format and a six-channel one.
	private var dacFormats: [AudioStreamRangedDescription] {
		var formats: [AudioStreamRangedDescription] = []
		for nonMixable in [false, true] {
			let extra = nonMixable ? kAudioFormatFlagIsNonMixable : 0
			for rate in [44100.0, 96000] {
				formats.append(ranged(rate, bits: 32, bytes: 4, flags: int32Flags | extra))
			}
			for rate in [44100.0, 96000] {
				formats.append(ranged(rate, bits: 24, bytes: 4, flags: int24HighFlags | extra))
			}
		}
		formats.append(ranged(44100, bits: 32, bytes: 4, flags: floatFlags))
		formats.append(ranged(44100, bits: 16, bytes: 2, flags: kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked))
		formats.append(ranged(44100, bits: 32, bytes: 4, flags: int32Flags | kAudioFormatFlagIsNonMixable, channels: 6))
		return formats
	}

	func testTheWidestIntegerTheSystemCannotMixComesFirst() {
		let candidates = DeviceOutput.exclusiveCandidates(dacFormats, rate: 44100, channels: 2, integerBits: 0)
		XCTAssertEqual(candidates.map(DeviceOutput.sampleFormat(of:)), [.int32, .int24High, .int32, .int24High, .int16, .float32])
		XCTAssertEqual(candidates.map { $0.mFormatFlags & kAudioFormatFlagIsNonMixable != 0 }, [true, true, false, false, false, false])
		XCTAssertTrue(candidates.allSatisfy { $0.mSampleRate == 44100 && $0.mChannelsPerFrame == 2 })

		// DoP needs 24-bit integer words.
		let dop = DeviceOutput.exclusiveCandidates(dacFormats, rate: 44100, channels: 2, integerBits: 24)
		XCTAssertEqual(dop.map(DeviceOutput.sampleFormat(of:)), [.int32, .int24High, .int32, .int24High])

		XCTAssertEqual(DeviceOutput.exclusiveCandidates(dacFormats, rate: 96000, channels: 2, integerBits: 0).count, 4)
		XCTAssertTrue(DeviceOutput.exclusiveCandidates(dacFormats, rate: 48000, channels: 2, integerBits: 0).isEmpty)

		// A format given as a range of rates is taken at the one asked for.
		var ranged = ranged(0, bits: 24, bytes: 3, flags: kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked)
		ranged.mSampleRateRange = AudioValueRange(mMinimum: 32000, mMaximum: 192_000)
		let fromRange = DeviceOutput.exclusiveCandidates([ranged], rate: 88200, channels: 2, integerBits: 0)
		XCTAssertEqual(fromRange.first?.mSampleRate, 88200)
		XCTAssertEqual(fromRange.first.flatMap(DeviceOutput.sampleFormat(of:)), .int24Packed)
	}

	func testTheRendererWritesOnlyFormatsItKnows() {
		let bigEndian = ranged(44100, bits: 16, bytes: 2, flags: kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagIsBigEndian).mFormat
		let unsigned = ranged(44100, bits: 16, bytes: 2, flags: kAudioFormatFlagIsPacked).mFormat
		let int24Low = ranged(44100, bits: 24, bytes: 4, flags: kAudioFormatFlagIsSignedInteger).mFormat
		let float64 = ranged(44100, bits: 64, bytes: 8, flags: kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked).mFormat
		var nonInterleaved = ranged(44100, bits: 32, bytes: 4, flags: floatFlags | kAudioFormatFlagIsNonInterleaved).mFormat
		nonInterleaved.mBytesPerFrame = 4
		XCTAssertNil(DeviceOutput.sampleFormat(of: bigEndian))
		XCTAssertNil(DeviceOutput.sampleFormat(of: unsigned))
		XCTAssertNil(DeviceOutput.sampleFormat(of: float64))
		XCTAssertNil(DeviceOutput.sampleFormat(of: nonInterleaved))
		XCTAssertEqual(DeviceOutput.sampleFormat(of: int24Low), .int24Low)
	}

	// MARK: - The device's rate

	private let smslRates = [44100.0, 48000, 88200, 96000, 176_400, 192_000, 352_800, 384_000, 705_600, 768_000].map {
		AudioValueRange(mMinimum: $0, mMaximum: $0)
	}

	func testTheDeviceFollowsTheTracksRateWhenItCan() {
		XCTAssertEqual(DeviceOutput.deviceRate(for: 44100, among: smslRates), 44100)
		XCTAssertEqual(DeviceOutput.deviceRate(for: 352_800, among: smslRates), 352_800)
		XCTAssertEqual(DeviceOutput.deviceRate(for: 22050, among: smslRates), 44100, "a whole multiple above")
		XCTAssertEqual(DeviceOutput.deviceRate(for: 32000, among: smslRates), 96000, "a whole multiple above, before nearer rates that are not")
		XCTAssertEqual(DeviceOutput.deviceRate(for: 50000, among: smslRates), 88200, "else the nearest above")
		let upTo192k = Array(smslRates.prefix(6))
		XCTAssertEqual(DeviceOutput.deviceRate(for: 352_800, among: upTo192k), 176_400, "DSD64 as PCM: a whole fraction below")
		XCTAssertEqual(DeviceOutput.deviceRate(for: 768_000, among: upTo192k), 192_000)
		XCTAssertEqual(DeviceOutput.deviceRate(for: 50000, among: []), 50000, "a device that does not say gets the benefit of the doubt")
		let continuous = [AudioValueRange(mMinimum: 8000, mMaximum: 96000)]
		XCTAssertEqual(DeviceOutput.deviceRate(for: 50000, among: continuous), 50000)
		XCTAssertEqual(DeviceOutput.deviceRate(for: 192_000, among: continuous), 96000)
	}

	// MARK: - Planning

	private let cd: [AnyHashable: Any] = ["bitsPerSample": 16, "sampleRate": 44100.0, "channels": 2]
	private let hiRes: [AnyHashable: Any] = ["bitsPerSample": 24, "sampleRate": 96000.0, "channels": 2]
	private let cd24: [AnyHashable: Any] = ["bitsPerSample": 24, "sampleRate": 44100.0, "channels": 2]
	private let float44k: [AnyHashable: Any] = ["bitsPerSample": 32, "floatingPoint": true, "sampleRate": 44100.0, "channels": 2]
	private let pcm176k: [AnyHashable: Any] = ["bitsPerSample": 16, "sampleRate": 176_400.0, "channels": 2]
	private let dsd64: [AnyHashable: Any] = ["bitsPerSample": 1, "sampleRate": 2_822_400.0, "channels": 2]

	private var exclusive: PlaybackEngine.DevicePlanning {
		PlaybackEngine.DevicePlanning(channels: 2, rates: smslRates, exclusive: true)
	}

	func testExclusiveOutputRunsTheDeviceAtEachTracksRate() {
		typealias Plan = PlaybackEngine.OutputPlan
		XCTAssertEqual(PlaybackEngine.plan(for: cd, device: exclusive), Plan(deviceRate: 44100, exclusive: true))
		XCTAssertEqual(PlaybackEngine.plan(for: hiRes, device: exclusive), Plan(deviceRate: 96000, exclusive: true))
		XCTAssertEqual(PlaybackEngine.plan(for: float44k, device: exclusive), Plan(deviceRate: 44100, exclusive: true))
		XCTAssertEqual(PlaybackEngine.plan(for: dsd64, device: exclusive), Plan(deviceRate: 352_800, exclusive: true), "DSD made PCM")

		var dop = exclusive
		dop.dop = true
		XCTAssertEqual(PlaybackEngine.plan(for: dsd64, device: dop), Plan(deviceRate: 176_400, dop: true, exclusive: true))

		var off = exclusive
		off.exclusive = false
		XCTAssertEqual(PlaybackEngine.plan(for: hiRes, device: off), .shared)
	}

	func testTracksAtTheStreamsRateJoinItWhateverTheirDepth() {
		let at44k = PlaybackEngine.OutputPlan(deviceRate: 44100, exclusive: true)
		XCTAssertTrue(PlaybackEngine.admits(cd, into: at44k, device: exclusive))
		XCTAssertTrue(PlaybackEngine.admits(cd24, into: at44k, device: exclusive), "gapless into 24 bits")
		XCTAssertTrue(PlaybackEngine.admits(float44k, into: at44k, device: exclusive))
		XCTAssertFalse(PlaybackEngine.admits(hiRes, into: at44k, device: exclusive), "the device changes rate")
		XCTAssertFalse(PlaybackEngine.admits(cd, into: .shared, device: exclusive), "a shared stream gives way to a held one")

		var dop = exclusive
		dop.dop = true
		let carrier = PlaybackEngine.OutputPlan(deviceRate: 176_400, dop: true, exclusive: true)
		XCTAssertTrue(PlaybackEngine.admits(pcm176k, into: carrier, device: dop), "PCM at the carrier rate")
		XCTAssertFalse(PlaybackEngine.admits(cd, into: carrier, device: dop), "PCM at its own rate, rather than resampled")
		XCTAssertFalse(PlaybackEngine.admits(dsd64, into: PlaybackEngine.OutputPlan(deviceRate: 176_400, exclusive: true), device: dop),
		               "DSD wanting DoP needs a stream packing it")
	}

	// MARK: - Status

	private func status(bits: Int, float: Bool = false, render: AudioStreamBasicDescription) -> OutputStatus {
		let source = SourceFormat(properties: ["sampleRate": 44100.0, "bitsPerSample": bits, "floatingPoint": float, "channels": 2])
		return OutputStatus(source: source, render: StreamFormat(sampleRate: 44100, channels: 2, channelConfig: UInt32(AudioConfigStereo)),
		                    renderFormat: render, exclusive: true)
	}

	func testIntegerOutputIsBitPerfectForWhatItHolds() {
		let int32 = ranged(44100, bits: 32, bytes: 4, flags: int32Flags).mFormat
		let int16 = ranged(44100, bits: 16, bytes: 2, flags: kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked).mFormat
		XCTAssertEqual(status(bits: 16, render: int32).modifications, [])
		XCTAssertEqual(status(bits: 24, render: int32).modifications, [])
		XCTAssertEqual(status(bits: 32, render: int32).modifications, [CogAudioOutputModificationPrecision], "carried as Float32")
		XCTAssertEqual(status(bits: 32, float: true, render: int32).modifications, [CogAudioOutputModificationPrecision])
		XCTAssertEqual(status(bits: 16, render: int16).modifications, [])
		XCTAssertEqual(status(bits: 24, render: int16).modifications, [CogAudioOutputModificationPrecision])
	}
}
