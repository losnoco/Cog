//
//  CogPluginsTests.swift
//  CogPluginsTests
//
//  The plugins as iOS has them: linked into CogPlugins.framework, found by
//  PluginController without loading bundles, and played by the engine
//  through RemoteIO.
//

import AVFoundation
@testable import CogAudio
import XCTest

final class CogPluginsTests: XCTestCase {
	private var directory: URL!

	override func setUpWithError() throws {
		directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
		try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
	}

	override func tearDownWithError() throws {
		try? FileManager.default.removeItem(at: directory)
	}

	private var plugins: CogPluginController {
		PluginController.shared()
	}

	/// A 16-bit stereo WAV of `seconds` of a 440 Hz sine at `sampleRate`.
	private func writeWAV(named name: String, seconds: Double, sampleRate: Double = 44100) throws -> URL {
		let url = directory.appendingPathComponent(name)
		let settings: [String: Any] = [AVFormatIDKey: kAudioFormatLinearPCM, AVSampleRateKey: sampleRate, AVNumberOfChannelsKey: 2,
		                               AVLinearPCMBitDepthKey: 16, AVLinearPCMIsFloatKey: false, AVLinearPCMIsBigEndianKey: false]
		let file = try AVAudioFile(forWriting: url, settings: settings, commonFormat: .pcmFormatFloat32, interleaved: false)
		let frames = AVAudioFrameCount(seconds * sampleRate)
		let buffer = try XCTUnwrap(AVAudioPCMBuffer(pcmFormat: file.processingFormat, frameCapacity: frames))
		buffer.frameLength = frames
		for channel in 0..<2 {
			let samples = buffer.floatChannelData![channel]
			for frame in 0..<Int(frames) {
				samples[frame] = 0.5 * sin(2 * .pi * 440 * Float(frame) / Float(sampleRate))
			}
		}
		try file.write(from: buffer)
		return url
	}

	private func names(_ value: Any?) -> String {
		String(describing: value ?? "nil")
	}

	// MARK: - Discovery

	func testThePluginsAreFoundWithoutBundles() {
		XCTAssertTrue(names(plugins.decodersByExtension()["wav"]).contains("CoreAudioDecoder"))
		XCTAssertTrue(names(plugins.decodersByExtension()["m4a"]).contains("CoreAudioDecoder"))
		XCTAssertTrue(names(plugins.sources()["file"]).contains("FileSource"))
		XCTAssertTrue(names(plugins.sources()["silence"]).contains("SilenceSource"))
		XCTAssertTrue(names(plugins.containers()["cue"]).contains("CueSheetContainer"))
		XCTAssertTrue(names(plugins.containers()["m3u"]).contains("M3uContainer"))
		XCTAssertTrue(names(plugins.containers()["pls"]).contains("PlsContainer"))
	}

	// MARK: - Decoding

	func testAWAVFileDecodesThroughThePlugins() throws {
		let url = try writeWAV(named: "sine.wav", seconds: 1)
		let source = try XCTUnwrap(plugins.audioSource(for: url))
		XCTAssertTrue(source.open(url))
		let decoder = try XCTUnwrap(plugins.audioDecoder(for: source, skipCue: false))
		guard decoder.open(source) else {
			return XCTFail("the decoder would not open the file")
		}
		let properties = decoder.properties() ?? [:]
		XCTAssertEqual((properties["sampleRate"] as? NSNumber)?.doubleValue, 44100)
		XCTAssertEqual((properties["channels"] as? NSNumber)?.intValue, 2)
		XCTAssertEqual((properties["bitsPerSample"] as? NSNumber)?.intValue, 16)

		var frames = 0
		while let chunk = decoder.readAudio(), chunk.frameCount() > 0 {
			frames += Int(chunk.frameCount())
		}
		decoder.close()
		XCTAssertEqual(frames, 44100)
	}

	func testACueSheetListsItsTracks() throws {
		let audio = try writeWAV(named: "album.wav", seconds: 2)
		let cue = directory.appendingPathComponent("album.cue")
		try """
		FILE "album.wav" WAVE
		  TRACK 01 AUDIO
		    TITLE "One"
		    INDEX 01 00:00:00
		  TRACK 02 AUDIO
		    TITLE "Two"
		    INDEX 01 00:01:00

		""".write(to: cue, atomically: true, encoding: .utf8)
		let tracks = plugins.urls(forContainerURL: cue) as? [URL] ?? []
		XCTAssertEqual(tracks.count, 2)
		XCTAssertEqual(tracks.first?.fragment, "01")
		XCTAssertTrue(plugins.dependencyUrls(forContainerURL: cue).contains { ($0 as? URL)?.lastPathComponent == audio.lastPathComponent })
	}

	// MARK: - Playing

	/// The whole path: FileSource and CoreAudio from CogPlugins, the engine,
	/// and RemoteIO, at volume zero.
	func testAWAVFilePlaysToTheEnd() throws {
		let url = try writeWAV(named: "short.wav", seconds: 0.5)
		let host = Host()
		let engine = PlaybackEngine()
		engine.host = host
		engine.volume = 0
		XCTAssertTrue(engine.play(url, userInfo: "short", rgInfo: nil, startPaused: false, seekTo: 0))
		let deadline = Date().addingTimeInterval(5)
		while !host.stopped && Date() < deadline {
			RunLoop.main.run(until: Date().addingTimeInterval(0.02))
		}
		XCTAssertTrue(host.stopped, "played to the end")
		XCTAssertFalse(host.errors.contains("short"))
		XCTAssertGreaterThan(engine.amountPlayed, 0.4)
	}
}

private final class Host: NSObject, PlaybackEngineHost {
	var stopped = false
	var errors: [String] = []

	func playbackEngineNextTrack(after userInfo: Any?) -> EngineTrack? { nil }
	func playbackEngineDidBeginTrack(_ userInfo: Any?) {}
	func playbackEngineDidChangeStatus(_ status: CogStatus, userInfo: Any?) {}
	func playbackEngineDidStopNaturally(_ userInfo: Any?) { stopped = true }
	func playbackEngineReportPlayCount(_ userInfo: Any?) {}
	func playbackEngineReportScrobble(_ userInfo: Any?) {}
	func playbackEngineSetError(_ error: Bool, forTrack userInfo: Any?) {
		if error { errors.append(userInfo as? String ?? "?") }
	}
	func playbackEnginePushInfo(_ info: [AnyHashable: Any], toTrack userInfo: Any?) {}
	func playbackEngineRestartAtCurrentPosition(_ userInfo: Any?) {}
	func playbackEngineBeginEqualizer(_ equalizer: CogEqualizer) {}
	func playbackEngineEndEqualizer(_ equalizer: CogEqualizer) {}
}
