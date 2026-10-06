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

	/// A 24-bit stereo FLAC of one second of a 1 kHz sine at 48 kHz.
	private func writeFLAC(named name: String) throws -> URL {
		let url = directory.appendingPathComponent(name)
		let settings: [String: Any] = [AVFormatIDKey: kAudioFormatFLAC, AVSampleRateKey: 48000.0, AVNumberOfChannelsKey: 2,
		                               AVLinearPCMBitDepthKey: 24]
		let file = try AVAudioFile(forWriting: url, settings: settings, commonFormat: .pcmFormatFloat32, interleaved: false)
		let buffer = try XCTUnwrap(AVAudioPCMBuffer(pcmFormat: file.processingFormat, frameCapacity: 48000))
		buffer.frameLength = 48000
		for channel in 0..<2 {
			for frame in 0..<48000 {
				buffer.floatChannelData![channel][frame] = 0.25 * sin(2 * .pi * 1000 * Float(frame) / 48000)
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
		XCTAssertTrue(names(plugins.sources()["https"]).contains("HTTPSource"))
		XCTAssertTrue(names(plugins.containers()["cue"]).contains("CueSheetContainer"))
		XCTAssertTrue(names(plugins.containers()["m3u"]).contains("M3uContainer"))
		XCTAssertTrue(names(plugins.containers()["pls"]).contains("PlsContainer"))
	}

	/// Every plugin linked into CogPlugins registers what it reads.
	func testEveryPluginRegisters() {
		let registered = [plugins.decodersByExtension(), plugins.decodersByMimeType(), plugins.containers(), plugins.sources(),
		                  plugins.metadataReaders(), plugins.propertiesReadersByExtension()].map { names($0) }.joined()
		let classes = ["AdPlugDecoder", "APLDecoder", "ArchiveContainer", "ArchiveSource", "CoreAudioDecoder", "CueSheetDecoder",
		               "FFMPEGDecoder", "FileSource", "FlacDecoder", "GameDecoder", "HCDecoder", "HLSDecoder", "HTTPSource",
		               "HVLDecoder", "jxsDecoder", "libvgmDecoder", "M3uContainer", "MIDIDecoder", "MP3Decoder", "MusepackDecoder",
		               "OMPTDecoder", "OpusFile", "OrganyaDecoder", "PlsContainer", "ShortenDecoder", "SidDecoder", "SilenceDecoder",
		               "TagLibMetadataReader", "VGMDecoder", "VorbisDecoder", "WavPackDecoder"]
		for name in classes {
			XCTAssertTrue(registered.contains(name), "\(name) is not registered")
		}
	}

	// MARK: - Decoding

	/// A MIDI file through the MIDI plugin's FM synthesizer (OPL3), which
	/// needs no SoundFont: one note, then silence.
	func testAMIDIFileDecodesThroughTheMIDIPlugin() throws {
		let defaults = UserDefaults.standard
		let settings: [String: Any] = ["midiPlugin": "OPL3W0", "synthSampleRate": 44100, "synthDefaultSeconds": 150,
		                               "synthDefaultFadeSeconds": 0, "synthDefaultLoopCount": 1]
		for (key, value) in settings { defaults.set(value, forKey: key) }
		defer { for key in settings.keys { defaults.removeObject(forKey: key) } }

		// Format 0, 96 ticks a quarter note at 120 bpm: middle C for a quarter.
		let track: [UInt8] = [0x00, 0xC0, 0x00, 0x00, 0x90, 60, 100, 0x60, 0x80, 60, 0, 0x00, 0xFF, 0x2F, 0x00]
		var smf = Data("MThd".utf8) + Data([0, 0, 0, 6, 0, 0, 0, 1, 0, 96])
		smf += Data("MTrk".utf8) + Data([0, 0, 0, UInt8(track.count)]) + Data(track)
		let url = directory.appendingPathComponent("note.mid")
		try smf.write(to: url)

		let source = try XCTUnwrap(plugins.audioSource(for: url))
		XCTAssertTrue(source.open(url))
		let decoder = try XCTUnwrap((NSClassFromString("MIDIDecoder") as? NSObject.Type)?.init() as? CogDecoder)
		guard decoder.open(source) else {
			return XCTFail("the MIDI plugin would not open the file")
		}
		var frames = 0
		var peak: Float = 0
		while frames < 44100 * 10, let chunk = decoder.readAudio(), chunk.frameCount() > 0 {
			frames += Int(chunk.frameCount())
			if chunk.format.mFormatFlags & kAudioFormatFlagIsFloat != 0 {
				chunk.removeSamples(chunk.frameCount()).withUnsafeBytes { peak = max(peak, $0.bindMemory(to: Float.self).map(abs).max() ?? 0) }
			}
		}
		decoder.close()
		XCTAssertGreaterThan(frames, 44100 / 4, "at least the note")
		XCTAssertLessThan(frames, 44100 * 10, "and it ends")
		XCTAssertGreaterThan(peak, 0.01, "the note is heard")
	}


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

	/// libFLAC through the Flac plugin, not Core Audio (which reads FLAC too).
	func testAFLACFileDecodesThroughTheFlacPlugin() throws {
		XCTAssertTrue(names(plugins.decodersByExtension()["flac"]).contains("FlacDecoder"))
		let url = try writeFLAC(named: "sine.flac")

		let source = try XCTUnwrap(plugins.audioSource(for: url))
		XCTAssertTrue(source.open(url))
		let decoderClass = try XCTUnwrap(NSClassFromString("FlacDecoder") as? NSObject.Type)
		let decoder = try XCTUnwrap(decoderClass.init() as? CogDecoder)
		guard decoder.open(source) else {
			return XCTFail("the Flac plugin would not open the file")
		}
		let properties = decoder.properties() ?? [:]
		XCTAssertEqual((properties["sampleRate"] as? NSNumber)?.doubleValue, 48000)
		XCTAssertEqual((properties["bitsPerSample"] as? NSNumber)?.intValue, 24)
		XCTAssertEqual(properties["codec"] as? String, "FLAC")
		var frames = 0
		while let chunk = decoder.readAudio(), chunk.frameCount() > 0 {
			frames += Int(chunk.frameCount())
		}
		decoder.close()
		XCTAssertEqual(frames, 48000)
	}

	/// FFmpeg (with fdk-aac) through the FFMPEG plugin, on AAC in MP4.
	func testAnAACFileDecodesThroughTheFFmpegPlugin() throws {
		XCTAssertTrue(names(plugins.decodersByExtension()["m4a"]).contains("FFMPEGDecoder"))
		let url = directory.appendingPathComponent("sine.m4a")
		do {
			let settings: [String: Any] = [AVFormatIDKey: kAudioFormatMPEG4AAC, AVSampleRateKey: 44100.0, AVNumberOfChannelsKey: 2,
			                               AVEncoderBitRateKey: 128000]
			let file = try AVAudioFile(forWriting: url, settings: settings, commonFormat: .pcmFormatFloat32, interleaved: false)
			let buffer = try XCTUnwrap(AVAudioPCMBuffer(pcmFormat: file.processingFormat, frameCapacity: 44100))
			buffer.frameLength = 44100
			for channel in 0..<2 {
				for frame in 0..<44100 {
					buffer.floatChannelData![channel][frame] = 0.25 * sin(2 * .pi * 440 * Float(frame) / 44100)
				}
			}
			try file.write(from: buffer)
		}

		let source = try XCTUnwrap(plugins.audioSource(for: url))
		XCTAssertTrue(source.open(url))
		let decoder = try XCTUnwrap((NSClassFromString("FFMPEGDecoder") as? NSObject.Type)?.init() as? CogDecoder)
		guard decoder.open(source) else {
			return XCTFail("the FFMPEG plugin would not open the file")
		}
		let properties = decoder.properties() ?? [:]
		XCTAssertEqual((properties["sampleRate"] as? NSNumber)?.doubleValue, 44100)
		XCTAssertEqual((properties["channels"] as? NSNumber)?.intValue, 2)
		XCTAssertEqual((properties["codec"] as? String)?.lowercased(), "aac", names(properties))
		var frames = 0
		var peak: Float = 0
		while let chunk = decoder.readAudio(), chunk.frameCount() > 0 {
			frames += Int(chunk.frameCount())
			if chunk.format.mFormatFlags & kAudioFormatFlagIsFloat != 0 {
				let data = chunk.removeSamples(chunk.frameCount())
				data.withUnsafeBytes { peak = max(peak, $0.bindMemory(to: Float.self).map(abs).max() ?? 0) }
			}
		}
		decoder.close()
		// AAC trims its priming, so the length is the original's, near enough.
		XCTAssertEqual(Double(frames), 44100, accuracy: 2048)
		XCTAssertEqual(peak, 0.25, accuracy: 0.05, "the sine, not silence")
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

	// MARK: - Tags

	/// Puts a Vorbis comment block with `comments` ("TITLE=…") into a FLAC
	/// file, straight after its STREAMINFO block.
	private func addVorbisComments(_ comments: [String], to url: URL) throws {
		var data = try Data(contentsOf: url)
		XCTAssertEqual(data.prefix(4), Data("fLaC".utf8))
		func le32(_ value: Int) -> Data { withUnsafeBytes(of: UInt32(value).littleEndian) { Data($0) } }
		var body = le32(3) + Data("Cog".utf8) + le32(comments.count)
		for comment in comments {
			body += le32(comment.utf8.count) + Data(comment.utf8)
		}
		// STREAMINFO is 4 + 34 bytes in; if it was the last block, ours is.
		let streamInfoLast = data[4] & 0x80 != 0
		data[4] &= 0x7F
		let header = Data([(streamInfoLast ? 0x80 : 0) | 4, UInt8(body.count >> 16 & 0xFF), UInt8(body.count >> 8 & 0xFF), UInt8(body.count & 0xFF)])
		data.insert(contentsOf: header + body, at: 4 + 4 + 34)
		try data.write(to: url)
	}

	func testTagLibReadsTags() throws {
		let url = try writeFLAC(named: "tagged.flac")
		try addVorbisComments(["TITLE=Sine", "ARTIST=Cog"], to: url)
		let reader = try XCTUnwrap(NSClassFromString("TagLibMetadataReader") as? CogMetadataReader.Type)
		let metadata = reader.metadata(for: url) ?? [:]
		XCTAssertTrue(names(metadata["title"]).contains("Sine"), names(metadata))
		XCTAssertTrue(names(metadata["artist"]).contains("Cog"), names(metadata))
		// The file still decodes: the block went in whole.
		let source = try XCTUnwrap(plugins.audioSource(for: url))
		XCTAssertTrue(source.open(url))
		XCTAssertTrue(try XCTUnwrap(plugins.audioDecoder(for: source, skipCue: false)).open(source))
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
