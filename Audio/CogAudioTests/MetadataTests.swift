//
//  MetadataTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

import CogAudio
import XCTest

/// A stream whose title changes partway through, announced through KVO as
/// decoders do (an internet radio station's ICY title, say).
final class RetitlingDecoder: NSObject, CogDecoder {
	private let frames: Int
	private let changeAt: Int
	private var position = 0
	private var title = "first"

	init(frames: Int, changeAt: Int) {
		self.frames = frames
		self.changeAt = changeAt
	}

	static func mimeTypes() -> [Any]! { [] }
	static func fileTypes() -> [Any]! { [] }
	static func fileTypeAssociations() -> [Any]! { [] }
	static func priority() -> Float { 1 }

	func properties() -> [AnyHashable: Any]! {
		["sampleRate": 48000.0, "channels": 2, "bitsPerSample": 32, "floatingPoint": true, "totalFrames": frames, "encoding": "lossy"]
	}

	@objc func metadata() -> [AnyHashable: Any]! { ["title": title] }

	func readAudio() -> AudioChunk! {
		let chunk = AudioChunk()
		guard position < frames else { return chunk }
		if position >= changeAt && title == "first" {
			willChangeValue(forKey: "metadata")
			title = "second"
			didChangeValue(forKey: "metadata")
		}
		let count = min(1000, frames - position)
		chunk.format = AudioStreamBasicDescription(mSampleRate: 48000, mFormatID: kAudioFormatLinearPCM, mFormatFlags: kAudioFormatFlagsNativeFloatPacked,
		                                           mBytesPerPacket: 8, mFramesPerPacket: 1, mBytesPerFrame: 8, mChannelsPerFrame: 2,
		                                           mBitsPerChannel: 32, mReserved: 0)
		let samples = [Float](repeating: 0.25, count: count * 2)
		samples.withUnsafeBufferPointer { chunk.assignSamples($0.baseAddress!, frameCount: count) }
		position += count
		return chunk
	}

	func open(_ source: CogSource!) -> Bool { true }
	func seek(_ frame: Int) -> Int { position = frame; return frame }
	func close() {}
}

final class MetadataTests: XCTestCase {
	/// The change is heard where the audio decoded after it starts, not when
	/// the decoder, seconds ahead, reports it.
	func testAMetadataChangeArrivesWithTheAudioAfterIt() throws {
		let track = EngineTrack(url: URL(string: "memory://radio")!, userInfo: "radio", gain: 1)
		let feeder = try XCTUnwrap(Feeder(outputRate: 48000, opener: { _ in RetitlingDecoder(frames: 48000, changeAt: 20000) }))
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: track)
		pump.start()
		var events: [(position: UInt64, event: PresentationEvent)] = []
		var ended = false
		var buffer = [Float](repeating: 0, count: 512 * 2)
		let deadline = Date().addingTimeInterval(10)
		while !ended && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			for entry in pump.presentation.take(through: cog_ring_read_position(pump.ring)) {
				events.append(entry)
				if case .endOfStream = entry.event { ended = true }
			}
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()

		let infos = events.compactMap { entry -> (UInt64, [AnyHashable: Any], EngineTrack)? in
			if case let .info(info, track) = entry.event { return (entry.position, info, track) }
			return nil
		}
		XCTAssertEqual(infos.count, 1)
		let (position, info, owner) = try XCTUnwrap(infos.first)
		XCTAssertEqual(position, 20000, "where the first frame decoded after the change plays")
		XCTAssertEqual(info["title"] as? String, "second")
		XCTAssertEqual((info["sampleRate"] as? NSNumber)?.doubleValue, 48000, "properties merged in, as InputNode did")
		XCTAssertTrue(owner === track)
	}
}
