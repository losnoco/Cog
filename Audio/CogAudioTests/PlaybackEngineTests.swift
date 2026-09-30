//
//  PlaybackEngineTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

@testable import CogAudio
import XCTest

/// Records what the engine tells `AudioPlayer`, and plays the playlist's part
/// in handing out the next track.
final class RecordingHost: NSObject, PlaybackEngineHost {
	var queue: [EngineTrack] = []
	var log: [String] = []
	var statuses: [CogStatus] = []
	var stopped = false

	private func name(_ userInfo: Any?) -> String { userInfo as? String ?? "nil" }

	func playbackEngineNextTrack(after userInfo: Any?) -> EngineTrack? {
		DispatchQueue.main.sync {
			log.append("next after \(name(userInfo))")
			return queue.isEmpty ? nil : queue.removeFirst()
		}
	}

	func playbackEngineDidBeginTrack(_ userInfo: Any?) { log.append("begin \(name(userInfo))") }
	func playbackEngineDidChangeStatus(_ status: CogStatus, userInfo: Any?) { statuses.append(status) }
	func playbackEngineDidStopNaturally(_ userInfo: Any?) {
		log.append("stopped after \(name(userInfo))")
		stopped = true
	}
	func playbackEngineReportPlayCount(_ userInfo: Any?) { log.append("played \(name(userInfo))") }
	func playbackEngineReportScrobble(_ userInfo: Any?) { log.append("scrobble \(name(userInfo))") }
	func playbackEngineSetError(_ error: Bool, forTrack userInfo: Any?) { log.append("error \(name(userInfo))") }
	func playbackEngineRestartAtCurrentPosition(_ userInfo: Any?) { log.append("restart") }
	func playbackEngineBeginEqualizer(_ equalizer: CogEqualizer) { log.append("eq on") }
	func playbackEngineEndEqualizer(_ equalizer: CogEqualizer) { log.append("eq off") }
}

/// Plays through the machine's real default output at volume zero.
final class PlaybackEngineTests: XCTestCase {
	private func runMainLoop(until condition: () -> Bool, timeout: TimeInterval) {
		let deadline = Date().addingTimeInterval(timeout)
		while !condition() && Date() < deadline {
			RunLoop.main.run(mode: .default, before: Date(timeIntervalSinceNow: 0.01))
		}
	}

	func testTwoTracksPlayThroughAndAreAnnouncedAndCounted() throws {
		let samples = SeamSignal.loopable(frames: 24000, sampleRate: 48000) // 0.5 s
		let decoders = ["a", "b"].reduce(into: [URL: MemoryDecoder]()) { map, name in
			map[URL(string: "memory://\(name)")!] = MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2)
		}
		let host = RecordingHost()
		host.queue = [EngineTrack(url: URL(string: "memory://b")!, userInfo: "b", gain: 1)]

		let engine = PlaybackEngine()
		engine.host = host
		engine.opener = { track in
			decoders[track.url].map { MemoryDecoder(samples: $0.samples, sampleRate: $0.sampleRate, channels: $0.channels) }
		}
		engine.volume = 0
		engine.setScrobbleThreshold(0.25)

		XCTAssertTrue(engine.play(URL(string: "memory://a")!, userInfo: "a", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { host.stopped }, timeout: 10)

		XCTAssertTrue(host.stopped)
		XCTAssertEqual(host.statuses.first, .playing)
		XCTAssertEqual(host.statuses.last, .stopped)
		XCTAssertEqual(host.log.filter { !$0.hasPrefix("next") }, [
			"scrobble a",
			"played a",
			"begin b",
			"scrobble b",
			"played b",
			"stopped after b",
		])
	}

	func testPlaybackPositionAdvancesAndSeeks() throws {
		let samples = SeamSignal.loopable(frames: 480000, sampleRate: 48000) // 10 s
		let host = RecordingHost()
		let engine = PlaybackEngine()
		engine.host = host
		engine.opener = { _ in MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2) }
		engine.volume = 0

		XCTAssertTrue(engine.play(URL(string: "memory://long")!, userInfo: "long", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { engine.amountPlayed > 0.3 }, timeout: 5)
		XCTAssertGreaterThan(engine.amountPlayed, 0.3)

		engine.seek(to: 7)
		XCTAssertEqual(engine.amountPlayed, 7)
		runMainLoop(until: { engine.amountPlayed > 7.2 }, timeout: 5)
		XCTAssertGreaterThan(engine.amountPlayed, 7.2)
		XCTAssertLessThan(engine.amountPlayed, 9)

		engine.pause()
		XCTAssertEqual(host.statuses.last, .paused)
		runMainLoop(until: { false }, timeout: 0.4)
		let paused = engine.amountPlayed
		runMainLoop(until: { false }, timeout: 0.3)
		XCTAssertEqual(engine.amountPlayed, paused, accuracy: 0.01, "position holds while paused")

		engine.resume()
		runMainLoop(until: { engine.amountPlayed > paused + 0.2 }, timeout: 5)
		XCTAssertGreaterThan(engine.amountPlayed, paused + 0.2)

		engine.stop()
		XCTAssertEqual(host.statuses.last, .stopped)
		XCTAssertFalse(host.stopped, "a user stop is not a natural stop")
	}

	/// Playing another track while one plays switches the running pipeline
	/// to it (and crossfades) instead of building a new one.
	func testANewTrackWhilePlayingSwitchesInPlace() throws {
		let long = SeamSignal.loopable(frames: 480000, sampleRate: 48000) // 10 s
		let short = SeamSignal.loopable(frames: 144000, sampleRate: 48000) // 3 s
		let host = RecordingHost()
		let engine = PlaybackEngine()
		engine.host = host
		engine.opener = { track in
			MemoryDecoder(samples: track.url.host == "b" ? short : long, sampleRate: 48000, channels: 2)
		}
		engine.volume = 0

		XCTAssertTrue(engine.play(URL(string: "memory://a")!, userInfo: "a", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { engine.amountPlayed > 0.3 }, timeout: 5)
		XCTAssertGreaterThan(engine.amountPlayed, 0.3)

		XCTAssertTrue(engine.play(URL(string: "memory://b")!, userInfo: "b", rgInfo: nil, startPaused: false, seekTo: 2))
		XCTAssertEqual(engine.pipelineBuilds, 1, "switched in place")
		XCTAssertEqual(engine.amountPlayed, 2)
		runMainLoop(until: { engine.amountPlayed > 2.2 }, timeout: 5)
		XCTAssertGreaterThan(engine.amountPlayed, 2.2)
		XCTAssertLessThan(engine.amountPlayed, 3)

		runMainLoop(until: { host.stopped }, timeout: 5)
		XCTAssertTrue(host.stopped, "the new track plays to its end")
		XCTAssertEqual(host.log.filter { !$0.hasPrefix("next") }, ["played b", "stopped after b"], "a is not counted, b is")

		// Paused, a new track is a fresh start, as before.
		XCTAssertTrue(engine.play(URL(string: "memory://a")!, userInfo: "a", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { engine.amountPlayed > 0.3 }, timeout: 5)
		engine.pause()
		XCTAssertTrue(engine.play(URL(string: "memory://b")!, userInfo: "b", rgInfo: nil, startPaused: false, seekTo: 0))
		XCTAssertEqual(engine.pipelineBuilds, 3)
		engine.stop()
	}

	/// A DSD track after a PCM one needs a DoP carrier: the stream ends
	/// before it and the engine rebuilds, announcing it once heard. The DSD
	/// is at 16 times the device's own rate, so the device keeps its rate,
	/// and its payload is DoP idle, as the carrier ignores the volume.
	func testADSDTrackAfterPCMIsHandedToANewPipeline() throws {
		UserDefaults.standard.set(true, forKey: "enableDoP")
		defer { UserDefaults.standard.removeObject(forKey: "enableDoP") }
		let device = try DeviceOutput()
		try device.selectDevice(nil)
		let rate = device.format.sampleRate
		let channels = device.format.channels

		let pcm = SeamSignal.loopable(frames: 24000, sampleRate: 48000) // 0.5 s
		let dsd = [UInt8](repeating: 0x69, count: Int(rate * 16 / 8 / 2) * channels) // 0.5 s
		let host = RecordingHost()
		host.queue = [EngineTrack(url: URL(string: "memory://dsd")!, userInfo: "dsd", gain: 1)]
		let engine = PlaybackEngine()
		engine.host = host
		engine.allowsDeviceRateChanges = false
		engine.opener = { track in
			track.url.host == "dsd" ? DSDMemoryDecoder(bytes: dsd, bitRate: rate * 16, channels: channels)
				: MemoryDecoder(samples: pcm, sampleRate: 48000, channels: 2)
		}
		engine.volume = 0

		XCTAssertTrue(engine.play(URL(string: "memory://pcm")!, userInfo: "pcm", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { host.stopped }, timeout: 10)

		XCTAssertTrue(host.stopped)
		XCTAssertEqual(engine.pipelineBuilds, 2, "rebuilt for the DSD track")

		// With DoP off, the DSD is converted to PCM in the same stream.
		UserDefaults.standard.set(false, forKey: "enableDoP")
		host.stopped = false
		host.log.removeAll()
		host.queue = [EngineTrack(url: URL(string: "memory://dsd")!, userInfo: "dsd", gain: 1)]
		XCTAssertTrue(engine.play(URL(string: "memory://pcm")!, userInfo: "pcm", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { host.stopped }, timeout: 10)
		XCTAssertEqual(engine.pipelineBuilds, 3, "one pipeline for both")
		XCTAssertEqual(host.log.filter { !$0.hasPrefix("next") }, ["played pcm", "begin dsd", "played dsd", "stopped after dsd"])
		XCTAssertEqual(host.log.filter { !$0.hasPrefix("next") }, [
			"played pcm",
			"begin dsd",
			"played dsd",
			"stopped after dsd",
		])
	}

	/// At tempo 2 the playback position runs at twice the wall clock: the
	/// stretch map, not the rendered frame count, gives track time.
	func testThePositionFollowsTheTempo() throws {
		UserDefaults.standard.set("faster", forKey: "rubberbandEngine")
		UserDefaults.standard.set(2.0, forKey: "tempo")
		defer {
			UserDefaults.standard.removeObject(forKey: "rubberbandEngine")
			UserDefaults.standard.removeObject(forKey: "tempo")
		}
		let samples = SeamSignal.loopable(frames: 480000, sampleRate: 48000) // 10 s
		let host = RecordingHost()
		let engine = PlaybackEngine()
		engine.host = host
		engine.opener = { _ in MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2) }
		engine.volume = 0

		XCTAssertTrue(engine.play(URL(string: "memory://fast")!, userInfo: "fast", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { engine.amountPlayed > 0.5 }, timeout: 5)
		let start = (Date(), engine.amountPlayed)
		runMainLoop(until: { false }, timeout: 1.5)
		let rate = (engine.amountPlayed - start.1) / Date().timeIntervalSince(start.0)
		engine.stop()
		XCTAssertEqual(rate, 2, accuracy: 0.25, "track seconds per wall-clock second")
	}
}
