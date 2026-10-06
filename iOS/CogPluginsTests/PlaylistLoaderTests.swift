//
//  PlaylistLoaderTests.swift
//  CogPluginsTests
//
//  The shared playlist's loader with the plugins: folders and containers
//  expanded, and each entry's properties and tags read.
//

import AVFoundation
import CogAudio
import CogPlaylist
import XCTest

@MainActor
final class PlaylistLoaderTests: XCTestCase {
	private var directory: URL!

	override func setUpWithError() throws {
		directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
		try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
	}

	override func tearDownWithError() throws {
		try? FileManager.default.removeItem(at: directory)
	}

	private func writeWAV(named name: String, seconds: Double) throws {
		let url = directory.appendingPathComponent(name)
		let settings: [String: Any] = [AVFormatIDKey: kAudioFormatLinearPCM, AVSampleRateKey: 44100.0, AVNumberOfChannelsKey: 2,
		                               AVLinearPCMBitDepthKey: 16, AVLinearPCMIsFloatKey: false, AVLinearPCMIsBigEndianKey: false]
		let file = try AVAudioFile(forWriting: url, settings: settings, commonFormat: .pcmFormatFloat32, interleaved: false)
		let frames = AVAudioFrameCount(seconds * 44100)
		let buffer = try XCTUnwrap(AVAudioPCMBuffer(pcmFormat: file.processingFormat, frameCapacity: frames))
		buffer.frameLength = frames
		try file.write(from: buffer)
	}

	func testAFolderBecomesItsTracks() async throws {
		try writeWAV(named: "album.wav", seconds: 2)
		try writeWAV(named: "single.wav", seconds: 1)
		try """
		PERFORMER "Cog"
		TITLE "Album"
		FILE "album.wav" WAVE
		  TRACK 01 AUDIO
		    TITLE "One"
		    INDEX 01 00:00:00
		  TRACK 02 AUDIO
		    TITLE "Two"
		    INDEX 01 00:01:00

		""".write(to: directory.appendingPathComponent("album.cue"), atomically: true, encoding: .utf8)
		try "not audio".write(to: directory.appendingPathComponent("notes.txt"), atomically: true, encoding: .utf8)

		let model = PlaylistModel(store: try PlaylistStore(inMemory: true),
		                          defaults: try XCTUnwrap(UserDefaults(suiteName: "PlaylistLoaderTests-\(UUID().uuidString)")))
		let loader = PlaylistLoader(model: model)
		let added = await loader.add([directory])

		XCTAssertEqual(added.map(\.filename), ["album.cue", "album.cue", "single.wav"],
		               "the cue's tracks, not the file they play from; no text file")
		XCTAssertEqual(model.entries, added)
		XCTAssertEqual(added.map(\.url?.fragment), ["01", "02", nil])
		XCTAssertTrue(added.allSatisfy(\.metadataLoaded))
		XCTAssertEqual(added.map(\.title), ["One", "Two", "single.wav"])
		XCTAssertEqual(added[0].artist, "Cog")
		XCTAssertEqual(added[0].album, "Album")
		XCTAssertEqual(added[0].length.doubleValue, 1, accuracy: 0.05)
		XCTAssertEqual(added[2].length.doubleValue, 1, accuracy: 0.05)
		XCTAssertEqual(added[2].sampleRate, 44100)
		XCTAssertFalse(added.contains(where: \.error))
	}

	func testAddingAFolderAgainAddsOnlyWhatIsNew() async throws {
		let album = directory.appendingPathComponent("Album", isDirectory: true)
		try FileManager.default.createDirectory(at: album, withIntermediateDirectories: true)
		try writeWAV(named: "Album/one.wav", seconds: 0.5)
		try writeWAV(named: "top.wav", seconds: 0.5)
		let model = PlaylistModel(store: try PlaylistStore(inMemory: true),
		                          defaults: try XCTUnwrap(UserDefaults(suiteName: "PlaylistLoaderTests-\(UUID().uuidString)")))
		let loader = PlaylistLoader(model: model)
		let first = await loader.add([directory], skippingExisting: true)
		XCTAssertEqual(first.map(\.filename), ["one.wav", "top.wav"], "subfolders walked")

		try writeWAV(named: "Album/two.wav", seconds: 0.5)
		let second = await loader.add([directory], skippingExisting: true)
		XCTAssertEqual(second.map(\.filename), ["two.wav"])
		XCTAssertEqual(model.entries.count, 3)
		let third = await loader.add([directory], skippingExisting: true)
		XCTAssertTrue(third.isEmpty)
	}

	func testAnUnreadableFileIsMarked() async throws {
		try "not audio either".write(to: directory.appendingPathComponent("broken.wav"), atomically: true, encoding: .utf8)
		let model = PlaylistModel(store: try PlaylistStore(inMemory: true),
		                          defaults: try XCTUnwrap(UserDefaults(suiteName: "PlaylistLoaderTests-\(UUID().uuidString)")))
		let added = await PlaylistLoader(model: model).add([directory.appendingPathComponent("broken.wav")])
		XCTAssertEqual(added.count, 1)
		XCTAssertTrue(added[0].error)
		XCTAssertTrue(added[0].metadataLoaded)
	}
}
