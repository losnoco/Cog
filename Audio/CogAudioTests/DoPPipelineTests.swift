//
//  DoPPipelineTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

@testable import CogAudio
import XCTest

/// DSD as a decoder hands it over: one byte (eight 1-bit samples) per
/// channel per frame, at the bit rate.
final class DSDMemoryDecoder: NSObject, CogDecoder {
	let bytes: [UInt8]
	let bitRate: Double
	let channels: Int
	/// Bytes are least significant bit first, as a DSF holds them, and the
	/// chunks say so.
	let reverseBits: Bool
	private var position = 0

	init(bytes: [UInt8], bitRate: Double = 2_822_400, channels: Int = 2, reverseBits: Bool = false) {
		self.bytes = bytes
		self.bitRate = bitRate
		self.channels = channels
		self.reverseBits = reverseBits
	}

	static func mimeTypes() -> [Any]! { [] }
	static func fileTypes() -> [Any]! { [] }
	static func fileTypeAssociations() -> [Any]! { [] }
	static func priority() -> Float { 1 }

	func properties() -> [AnyHashable: Any]! {
		["sampleRate": bitRate, "channels": channels, "bitsPerSample": 1, "floatingPoint": false,
		 "totalFrames": bytes.count / channels * 8, "seekable": true, "encoding": "lossless", "dsdDoPReverseBits": reverseBits]
	}

	func metadata() -> [AnyHashable: Any]! { [:] }

	func readAudio() -> AudioChunk! {
		let chunk = AudioChunk()
		let frames = bytes.count / channels
		guard position < frames else { return chunk }
		// An odd count, so DoP pairs straddle chunks.
		let count = min(1001, frames - position)
		chunk.format = AudioStreamBasicDescription(mSampleRate: bitRate, mFormatID: kAudioFormatLinearPCM, mFormatFlags: 0,
		                                           mBytesPerPacket: UInt32(channels), mFramesPerPacket: 1,
		                                           mBytesPerFrame: UInt32(channels), mChannelsPerFrame: UInt32(channels),
		                                           mBitsPerChannel: 1, mReserved: 0)
		bytes.withUnsafeBufferPointer { buffer in
			chunk.assignSamples(buffer.baseAddress! + position * channels, frameCount: count)
		}
		chunk.dsdDoPReverseBits = reverseBits
		position += count
		return chunk
	}

	func open(_ source: CogSource!) -> Bool { true }

	func seek(_ frame: Int) -> Int {
		position = min(max(0, frame / 8), bytes.count / channels)
		return position * 8
	}

	func close() {}
}

final class DoPPipelineTests: XCTestCase {
	private let stereo = StreamFormat(sampleRate: 176_400, channels: 2, channelConfig: UInt32(AudioConfigStereo))

	private func dsd(frames: Int, seed: UInt32 = 5) -> [UInt8] {
		(0..<(frames * 2)).map { UInt8(truncatingIfNeeded: (UInt32($0) &* 2654435761 &+ seed) >> 13) }
	}

	/// The DoP words `bytes` should become: two DSD bytes per carrier frame,
	/// first in the middle byte, markers alternating from 0x05.
	private func expectedDoP(_ bytes: [UInt8]) -> [Float] {
		var out: [Float] = []
		var marker: UInt32 = 0x05
		let frames = bytes.count / 2
		for pair in stride(from: 0, to: frames - 1, by: 2) {
			for channel in 0..<2 {
				let word = (marker << 24) | (UInt32(bytes[pair * 2 + channel]) << 16) | (UInt32(bytes[(pair + 1) * 2 + channel]) << 8)
				out.append(Float(Double(Int32(bitPattern: word)) / 2147483648.0))
			}
			marker = marker == 0x05 ? 0xFA : 0x05
		}
		return out
	}

	func testDSDReachesTheDeviceAsBitExactDoP() throws {
		let bytes = dsd(frames: 40000)
		let track = EngineTrack(url: URL(string: "memory://dsd")!, userInfo: nil, gain: 0.5)
		let feeder = try XCTUnwrap(Feeder(outputRate: 176_400, opener: { _ in DSDMemoryDecoder(bytes: bytes) }, dsdAsDoP: true))
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: stereo, stages: [EqualizerStage()], carrier: true))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }
		cog_gain_ramp_to(cog_renderer_volume(renderer), 0.3, 0)

		feeder.start(with: track)
		pump.start()
		var played: [Float] = []
		var buffer = [Float](repeating: 0, count: 512 * 2)
		let deadline = Date().addingTimeInterval(10)
		while played.count < 20000 * 2 && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			played += buffer[0..<(got * 2)]
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()

		XCTAssertEqual(played.count, 20000 * 2)
		XCTAssertEqual(played, expectedDoP(bytes), "no gain, EQ or resampler touched it")
	}

	// MARK: - Bit order

	/// A 1 kHz sine at half scale as DSD64 from a second-order sigma-delta
	/// modulator, stereo, packed with the earliest bit in the most
	/// significant place (DSDIFF order; DSF reverses it).
	private func sigmaDelta(seconds: Double) -> [UInt8] {
		let rate = 2_822_400.0
		let bits = Int(rate * seconds) / 8 * 8
		var bytes = [UInt8](repeating: 0, count: bits / 8 * 2)
		var first = 0.0, second = 0.0, output = 1.0
		for n in 0..<bits {
			let input = 0.5 * sin(2 * .pi * 1000 * Double(n) / rate)
			first += input - output
			second += first - output
			output = second >= 0 ? 1 : -1
			if output > 0 {
				let byte = n / 8
				let mask = UInt8(0x80) >> UInt8(n % 8)
				bytes[byte * 2] |= mask
				bytes[byte * 2 + 1] |= mask
			}
		}
		return bytes
	}

	private func reversed(_ bytes: [UInt8]) -> [UInt8] {
		bytes.map { byte in
			var value = byte, result: UInt8 = 0
			for _ in 0..<8 {
				result = (result << 1) | (value & 1)
				value >>= 1
			}
			return result
		}
	}

	/// Decimates `bytes` to PCM through the feeder, and returns the noise
	/// left in the audio band once the 1 kHz sine is taken out, relative to
	/// the sine: the second-order noise shaping only works if the decimator
	/// reads the bits in the right order.
	private func inBandNoise(_ bytes: [UInt8], reverseBits: Bool) throws -> Double {
		let rate = 352_800.0
		let feeder = try XCTUnwrap(Feeder(outputRate: rate, opener: { _ in DSDMemoryDecoder(bytes: bytes, reverseBits: reverseBits) }))
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: rate, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }
		feeder.start(with: EngineTrack(url: URL(string: "memory://dsd")!))
		pump.start()
		var left: [Double] = []
		var buffer = [Float](repeating: 0, count: 4096 * 2)
		let wanted = Int(rate * 0.4)
		let deadline = Date().addingTimeInterval(20)
		while left.count < wanted && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 4096) }
			for frame in 0..<got { left.append(Double(buffer[frame * 2])) }
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()

		// A tenth of a second (exactly 100 periods), past the filter's start.
		let segment = Array(left[8820..<(8820 + 35280)])
		let phase = { (n: Int) in 2 * Double.pi * 1000 * Double(n) / rate }
		var sine = 0.0, cosine = 0.0
		for (n, value) in segment.enumerated() {
			sine += value * sin(phase(n))
			cosine += value * cos(phase(n))
		}
		sine *= 2 / Double(segment.count)
		cosine *= 2 / Double(segment.count)
		var residual = segment.enumerated().map { n, value in value - sine * sin(phase(n)) - cosine * cos(phase(n)) }
		// Four 16-sample moving averages: nulls from 22 kHz up, and deep
		// rejection of the ultrasonic noise the modulator shapes there.
		for _ in 0..<4 {
			var running = 0.0
			var smoothed = [Double](repeating: 0, count: residual.count)
			for n in residual.indices {
				running += residual[n]
				if n >= 16 { running -= residual[n - 16] }
				smoothed[n] = running / 16
			}
			residual = smoothed
		}
		let settled = residual[100...]
		let noise = (settled.map { $0 * $0 }.reduce(0, +) / Double(settled.count)).squareRoot()
		return noise / (sine * sine + cosine * cosine).squareRoot()
	}

	func testTheDecimatorReadsTheEarliestBitFromTheTop() throws {
		let bytes = sigmaDelta(seconds: 0.5)
		let right = try inBandNoise(bytes, reverseBits: false)
		let wrong = try inBandNoise(reversed(bytes), reverseBits: false)
		print("in-band noise relative to the sine: MSB first \(right), bits reversed \(wrong)")
		XCTAssertLessThan(right, 0.001, "MSB-first decimates cleanly")
		XCTAssertGreaterThan(wrong, right * 10, "the measurement tells the orders apart")
	}

	func testTheDecimatorHonoursTheReverseFlag() throws {
		let bytes = sigmaDelta(seconds: 0.5)
		let flagged = try inBandNoise(reversed(bytes), reverseBits: true)
		XCTAssertLessThan(flagged, 0.001, "LSB-first bytes that say so decimate cleanly")
	}

	func testDoPHonoursTheReverseFlag() throws {
		let bytes = dsd(frames: 4000)
		let track = EngineTrack(url: URL(string: "memory://dsd")!)
		let feeder = try XCTUnwrap(Feeder(outputRate: 176_400, opener: { _ in DSDMemoryDecoder(bytes: self.reversed(bytes), reverseBits: true) }, dsdAsDoP: true))
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: stereo, carrier: true))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }
		feeder.start(with: track)
		pump.start()
		var played: [Float] = []
		var buffer = [Float](repeating: 0, count: 512 * 2)
		let deadline = Date().addingTimeInterval(10)
		while played.count < 2000 * 2 && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			played += buffer[0..<(got * 2)]
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()
		XCTAssertEqual(played, expectedDoP(bytes), "carried MSB first, as DoP requires")
	}

	func testTheCarrierATrackWants() {
		let yes: (Double) -> Bool = { _ in true }
		let dsd64: [AnyHashable: Any] = ["bitsPerSample": 1, "sampleRate": 2_822_400.0, "channels": 2]
		XCTAssertEqual(PlaybackEngine.carrierRate(for: dsd64, deviceChannels: 2, supports: yes), 176_400)
		XCTAssertNil(PlaybackEngine.carrierRate(for: dsd64, deviceChannels: 6, supports: yes), "DoP cannot be remapped")
		XCTAssertNil(PlaybackEngine.carrierRate(for: dsd64, deviceChannels: 2) { _ in false }, "the device must take the rate")

		let hiRes: [AnyHashable: Any] = ["bitsPerSample": 24, "sampleRate": 176_400.0, "channels": 2, "floatingPoint": false]
		XCTAssertEqual(PlaybackEngine.carrierRate(for: hiRes, deviceChannels: 2, supports: yes), 176_400, "may be DoP already")
		var cd = hiRes
		cd["sampleRate"] = 44100.0
		XCTAssertNil(PlaybackEngine.carrierRate(for: cd, deviceChannels: 2, supports: yes))
		var float = hiRes
		float["floatingPoint"] = true
		XCTAssertNil(PlaybackEngine.carrierRate(for: float, deviceChannels: 2, supports: yes))
	}

	func testWhatMayJoinAStream() {
		let yes: (Double) -> Bool = { _ in true }
		let dsd64: [AnyHashable: Any] = ["bitsPerSample": 1, "sampleRate": 2_822_400.0, "channels": 2]
		let dsd128: [AnyHashable: Any] = ["bitsPerSample": 1, "sampleRate": 5_644_800.0, "channels": 2]
		let cd: [AnyHashable: Any] = ["bitsPerSample": 16, "sampleRate": 44100.0, "channels": 2]
		XCTAssertTrue(PlaybackEngine.admits(dsd64, into: 176_400, deviceChannels: 2, supports: yes))
		XCTAssertFalse(PlaybackEngine.admits(dsd128, into: 176_400, deviceChannels: 2, supports: yes), "another carrier rate")
		XCTAssertTrue(PlaybackEngine.admits(cd, into: 176_400, deviceChannels: 2, supports: yes), "PCM is resampled into a carrier stream")
		XCTAssertFalse(PlaybackEngine.admits(dsd64, into: nil, deviceChannels: 2, supports: yes), "PCM stream, DSD wanting a carrier")
		XCTAssertTrue(PlaybackEngine.admits(dsd64, into: nil, deviceChannels: 2) { _ in false }, "DSD the device cannot carry becomes PCM")
		XCTAssertFalse(PlaybackEngine.admits(dsd128, into: 176_400, deviceChannels: 2) { $0 == 176_400 }, "but not inside a carrier stream")
	}

	func testATrackNeedingAnotherFormatEndsTheStreamAndWaits() throws {
		let first = EngineTrack(url: URL(string: "memory://pcm")!)
		let second = EngineTrack(url: URL(string: "memory://dsd")!)
		let feeder = try XCTUnwrap(Feeder(outputRate: 48000, opener: { track in
			track.url.host == "dsd" ? DSDMemoryDecoder(bytes: [UInt8](repeating: 0x69, count: 2000))
				: MemoryDecoder(samples: [Float](repeating: 0.1, count: 9600), sampleRate: 48000, channels: 2)
		}))
		feeder.admits = { decoder in (decoder.properties()["bitsPerSample"] as? Int) != 1 }
		let delegate = ScriptedTracks([second])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: first)
		pump.start()
		var ended = false
		var buffer = [Float](repeating: 0, count: 512 * 2)
		let deadline = Date().addingTimeInterval(10)
		while !ended && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			for entry in pump.presentation.take(through: cog_ring_read_position(pump.ring)) {
				if case .endOfStream = entry.event { ended = true }
			}
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		XCTAssertTrue(ended, "the stream ends before the DSD track")
		let handoff = try XCTUnwrap(feeder.takeHandoff())
		XCTAssertTrue(handoff.track === second)
		XCTAssertEqual(handoff.decoder.properties()["bitsPerSample"] as? Int, 1, "handed over opened")
		XCTAssertNil(feeder.takeHandoff())
		pump.stop()
		feeder.stop()
	}
}
