//
//  PumpTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import CogAudio
import XCTest

/// Feeder → pump → renderer, with the test pulling from the renderer in
/// place of a device.
final class PumpTests: XCTestCase {
	private struct Played {
		var samples: [Float] = []
		var events: [(position: UInt64, event: PresentationEvent)] = []
		var ended: Bool {
			if case .endOfStream? = events.last?.event { return true }
			return false
		}
	}

	private func makeFeeder(outputRate: Double, tracks: [(EngineTrack, MemoryDecoder)]) -> Feeder {
		let byURL = Dictionary(tracks.map { ($0.0.url, $0.1) }, uniquingKeysWith: { a, _ in a })
		return Feeder(outputRate: outputRate) { track in
			guard let source = byURL[track.url] else { return nil }
			return MemoryDecoder(samples: source.samples, sampleRate: source.sampleRate, channels: source.channels)
		}!
	}

	/// Pulls device-sized buffers until `until` says stop, keeping only real
	/// audio (not the silence rendered while the ring is empty).
	private func play(_ pump: Pump, renderer: OpaquePointer, into played: inout Played, timeout: TimeInterval = 30, until: (Played) -> Bool) {
		let channels = pump.outputFormat.channels
		var buffer = [Float](repeating: 0, count: 512 * channels)
		let deadline = Date().addingTimeInterval(timeout)
		while !until(played) {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			played.samples.append(contentsOf: buffer[0..<(got * channels)])
			played.events.append(contentsOf: pump.presentation.take(through: cog_ring_read_position(pump.ring)))
			if got == 0 {
				if Date() > deadline {
					XCTFail("playback did not finish")
					return
				}
				Thread.sleep(forTimeInterval: 0.001)
			}
		}
	}

	private func trackStarts(_ played: Played) -> [(UInt64, URL, Double)] {
		played.events.compactMap { entry in
			if case let .trackStart(track, offset) = entry.event { return (entry.position, track.url, offset) }
			return nil
		}
	}

	func testRepeatOneReachesTheRendererSeamlessly() throws {
		let samples = SeamSignal.loopable(frames: 96000, sampleRate: 48000)
		let lap = (EngineTrack(url: URL(string: "memory://lap")!), MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2))
		let feeder = makeFeeder(outputRate: 384000, tracks: [lap])
		let delegate = ScriptedTracks([lap.0])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: 384000, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: lap.0)
		pump.start()
		var played = Played()
		play(pump, renderer: renderer, into: &played) { $0.ended }
		pump.stop()
		feeder.stop()

		XCTAssertEqual(played.samples.count / 2, 1_536_000)
		XCTAssertEqual(trackStarts(played).map(\.0), [0, 768000])
		XCTAssertEqual(played.events.last?.position, 1_536_000)

		let reference = SeamSignal.reference(samples + samples, inputRate: 48000, outputRate: 384000)
		var worst: Float = 0
		for i in (768000 - 20000) * 2..<(768000 + 20000) * 2 {
			worst = max(worst, abs(played.samples[i] - reference[i]))
		}
		XCTAssertLessThan(worst, 1e-5, "seam at the renderer")
	}

	func testMonoIsFittedToAStereoDevice() throws {
		let mono = SeamSignal.loopable(frames: 48000, sampleRate: 48000).enumerated().filter { $0.offset % 2 == 0 }.map(\.element)
		let track = (EngineTrack(url: URL(string: "memory://mono")!), MemoryDecoder(samples: mono, sampleRate: 48000, channels: 1))
		let feeder = makeFeeder(outputRate: 48000, tracks: [track])
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: track.0)
		pump.start()
		var played = Played()
		play(pump, renderer: renderer, into: &played) { $0.ended }
		pump.stop()
		feeder.stop()

		XCTAssertEqual(played.samples.count, 48000 * 2)
		for frame in stride(from: 0, to: 48000, by: 997) {
			XCTAssertEqual(played.samples[frame * 2], played.samples[frame * 2 + 1], "both channels carry the mono signal")
		}
		XCTAssertNotEqual(played.samples[1000 * 2], 0)
	}

	func testASeekFlushesTheDeviceSideAndMarksTheNewPosition() throws {
		let samples = SeamSignal.loopable(frames: 480000, sampleRate: 48000)
		let track = (EngineTrack(url: URL(string: "memory://long")!), MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2))
		let feeder = makeFeeder(outputRate: 48000, tracks: [track])
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: track.0)
		pump.start()
		var played = Played()
		play(pump, renderer: renderer, into: &played) { $0.samples.count >= 2000 * 2 }
		feeder.seek(to: 9, in: track.0)
		var afterSeek = Played()
		play(pump, renderer: renderer, into: &afterSeek) { $0.ended }
		pump.stop()
		feeder.stop()

		// Whatever was queued before the seek is gone; after the new track
		// start, the audio is exactly the last second of the track.
		let starts = trackStarts(afterSeek)
		let seekStart = try XCTUnwrap(starts.last)
		XCTAssertEqual(seekStart.2, 9)
		let startPosition = Int(seekStart.0)
		let readBefore = Int(cog_ring_read_position(pump.ring)) - afterSeek.samples.count / 2
		let offsetInAfter = (startPosition - readBefore) * 2
		XCTAssertGreaterThanOrEqual(offsetInAfter, 0)
		let heard = Array(afterSeek.samples[offsetInAfter...])
		XCTAssertEqual(heard.count, 48000 * 2)
		XCTAssertEqual(Array(heard.prefix(8)), Array(samples[(432000 * 2)..<(432000 * 2 + 8)]))
	}

	// MARK: - ReplayGain

	/// A constant signal makes the applied gain directly visible.
	private func constant(_ frames: Int, _ value: Float = 0.5) -> [Float] {
		[Float](repeating: value, count: frames * 2)
	}

	private func makePump(_ tracks: [(EngineTrack, MemoryDecoder)], next: [EngineTrack]) throws -> (Feeder, Pump, OpaquePointer, ScriptedTracks) {
		let feeder = makeFeeder(outputRate: 48000, tracks: tracks)
		let delegate = ScriptedTracks(next)
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		return (feeder, pump, renderer, delegate)
	}

	func testEachTracksGainAppliesFromItsFirstFrame() throws {
		let a = (EngineTrack(url: URL(string: "memory://a")!, gain: 0.5), MemoryDecoder(samples: constant(4800), sampleRate: 48000, channels: 2))
		let b = (EngineTrack(url: URL(string: "memory://b")!, gain: 2), MemoryDecoder(samples: constant(4800), sampleRate: 48000, channels: 2))
		let (feeder, pump, renderer, delegate) = try makePump([a, b], next: [b.0])
		defer { cog_renderer_destroy(renderer) }
		_ = delegate

		feeder.start(with: a.0)
		pump.start()
		var played = Played()
		play(pump, renderer: renderer, into: &played) { $0.ended }
		pump.stop()
		feeder.stop()

		XCTAssertEqual(played.samples.count, 9600 * 2)
		XCTAssertTrue(played.samples[0..<(4800 * 2)].allSatisfy { $0 == 0.25 }, "track A at gain 0.5 throughout")
		XCTAssertTrue(played.samples[(4800 * 2)...].allSatisfy { $0 == 1.0 }, "track B at gain 2 from its first frame")
	}

	func testAGainChangeMidTrackRampsInsteadOfStepping() throws {
		let track = (EngineTrack(url: URL(string: "memory://long")!, gain: 1), MemoryDecoder(samples: constant(96000), sampleRate: 48000, channels: 2))
		let (feeder, pump, renderer, delegate) = try makePump([track], next: [])
		defer { cog_renderer_destroy(renderer) }
		_ = delegate

		feeder.start(with: track.0)
		pump.start()
		var played = Played()
		play(pump, renderer: renderer, into: &played) { $0.samples.count >= 1000 * 2 }
		// As a late ReplayGain update or a new volume scaling setting would.
		track.0.gain = 0.25
		play(pump, renderer: renderer, into: &played) { $0.ended }
		pump.stop()
		feeder.stop()

		let left = stride(from: 0, to: played.samples.count, by: 2).map { played.samples[$0] }
		XCTAssertEqual(left.first, 0.5)
		XCTAssertEqual(left.last!, 0.125, accuracy: 1e-6, "reaches the new gain")
		var largestStep: Float = 0
		for i in 1..<left.count {
			largestStep = max(largestStep, abs(left[i] - left[i - 1]))
		}
		// 0.5 -> 0.125 over 20 ms (960 frames) is about 3.9e-4 per frame.
		XCTAssertLessThan(largestStep, 1e-3, "no step")
		let rampStart = try XCTUnwrap(left.firstIndex { $0 < 0.5 })
		let rampEnd = try XCTUnwrap(left.firstIndex { abs($0 - 0.125) < 1e-6 })
		XCTAssertEqual(rampEnd - rampStart, 960, accuracy: 2, "ramps over 20 ms")
	}
}
