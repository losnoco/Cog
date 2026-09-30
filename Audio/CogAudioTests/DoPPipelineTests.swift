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
	private var position = 0

	init(bytes: [UInt8], bitRate: Double = 2_822_400, channels: Int = 2) {
		self.bytes = bytes
		self.bitRate = bitRate
		self.channels = channels
	}

	static func mimeTypes() -> [Any]! { [] }
	static func fileTypes() -> [Any]! { [] }
	static func fileTypeAssociations() -> [Any]! { [] }
	static func priority() -> Float { 1 }

	func properties() -> [AnyHashable: Any]! {
		["sampleRate": bitRate, "channels": channels, "bitsPerSample": 1, "floatingPoint": false,
		 "totalFrames": bytes.count / channels * 8, "seekable": true, "encoding": "lossless"]
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
