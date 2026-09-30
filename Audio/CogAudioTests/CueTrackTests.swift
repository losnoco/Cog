//
//  CueTrackTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

@testable import CogAudio
import XCTest

/// One file holding several tracks, addressed as `file#n` like a cue sheet,
/// which moves on to the next with `setTrack:` rather than being reopened.
final class CueMemoryDecoder: NSObject, CogDecoder {
	let samples: [Float]
	let boundaries: [Int]
	private var position = 0
	private var end = 0

	init(samples: [Float], boundaries: [Int], track: Int) {
		self.samples = samples
		self.boundaries = boundaries
		super.init()
		select(track)
	}

	private func select(_ track: Int) {
		position = track == 0 ? 0 : boundaries[track - 1]
		end = track < boundaries.count ? boundaries[track] : samples.count / 2
	}

	static func mimeTypes() -> [Any]! { [] }
	static func fileTypes() -> [Any]! { [] }
	static func fileTypeAssociations() -> [Any]! { [] }
	static func priority() -> Float { 1 }

	func properties() -> [AnyHashable: Any]! {
		["sampleRate": 48000.0, "channels": 2, "bitsPerSample": 32, "floatingPoint": true, "encoding": "lossless"]
	}

	func metadata() -> [AnyHashable: Any]! { [:] }

	func readAudio() -> AudioChunk! {
		let chunk = AudioChunk()
		guard position < end else { return chunk }
		let count = min(777, end - position)
		chunk.format = AudioStreamBasicDescription(mSampleRate: 48000, mFormatID: kAudioFormatLinearPCM, mFormatFlags: kAudioFormatFlagsNativeFloatPacked,
		                                           mBytesPerPacket: 8, mFramesPerPacket: 1, mBytesPerFrame: 8, mChannelsPerFrame: 2,
		                                           mBitsPerChannel: 32, mReserved: 0)
		samples.withUnsafeBufferPointer { chunk.assignSamples($0.baseAddress! + position * 2, frameCount: count) }
		position += count
		return chunk
	}

	/// The next track carries straight on: only the end moves.
	func setTrack(_ track: URL!) -> Bool {
		guard let number = Int(track.fragment ?? ""), position == end else { return false }
		end = number < boundaries.count ? boundaries[number] : samples.count / 2
		return true
	}

	func open(_ source: CogSource!) -> Bool { true }
	func seek(_ frame: Int) -> Int { position = frame; return frame }
	func close() {}
}

final class CueTrackTests: XCTestCase {
	func testTheNextTrackOfTheSameFileReusesTheDecoder() throws {
		let samples = SeamSignal.loopable(frames: 30000, sampleRate: 48000)
		let first = EngineTrack(url: URL(string: "file:///album.flac#0")!)
		let second = EngineTrack(url: URL(string: "file:///album.flac#1")!)
		var opened = 0
		let feeder = try XCTUnwrap(Feeder(outputRate: 48000, opener: { track in
			opened += 1
			return CueMemoryDecoder(samples: samples, boundaries: [12345], track: Int(track.url.fragment!)!)
		}))
		let delegate = ScriptedTracks([second])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: first)
		pump.start()
		var played: [Float] = []
		var starts: [(UInt64, EngineTrack)] = []
		var ended = false
		var buffer = [Float](repeating: 0, count: 512 * 2)
		let deadline = Date().addingTimeInterval(10)
		while !ended && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			played += buffer[0..<(got * 2)]
			for entry in pump.presentation.take(through: cog_ring_read_position(pump.ring)) {
				switch entry.event {
				case let .trackStart(track, _): starts.append((entry.position, track))
				case .endOfStream: ended = true
				default: break
				}
			}
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()

		XCTAssertEqual(opened, 1, "the second track reused the first's decoder")
		XCTAssertEqual(played, samples, "one unbroken stream")
		XCTAssertEqual(starts.map(\.0), [0, 12345])
		XCTAssertTrue(starts.last?.1 === second)
	}

	func testSameFileIgnoresTheFragmentOnly() {
		let a = URL(string: "file:///music/album.flac#01")!
		XCTAssertTrue(Feeder.sameFile(a, URL(string: "file:///music/album.flac#02")!))
		XCTAssertFalse(Feeder.sameFile(a, URL(string: "file:///music/other.flac#02")!))
		XCTAssertFalse(Feeder.sameFile(a, URL(string: "http:///music/album.flac#02")!))
	}
}
