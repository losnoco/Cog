//
//  TagWriterTests.swift
//  CogPluginsTests
//
//  Tags, covers and chapters written through the TagLib writer and read
//  back, both through the writer and through every reader the playlist
//  merges, so a format whose decoder reads its own tags is shown to see
//  the save too.
//

import AVFoundation
@testable import CogAudio
import CogPlaylist
import XCTest

final class TagWriterTests: XCTestCase {
	private var directory: URL!

	override func setUpWithError() throws {
		_ = PluginController.shared()
		directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
		try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
	}

	override func tearDownWithError() throws {
		try? FileManager.default.removeItem(at: directory)
	}

	private var plugins: CogPluginController { PluginController.shared() }

	// MARK: - Fixtures

	/// One second of a sine through AVAudioFile, in whatever format the
	/// settings and extension pick.
	private func writeAudio(named name: String, settings extra: [String: Any]) throws -> URL {
		let url = directory.appendingPathComponent(name)
		var settings: [String: Any] = [AVSampleRateKey: 44100.0, AVNumberOfChannelsKey: 2]
		settings.merge(extra) { $1 }
		let file = try AVAudioFile(forWriting: url, settings: settings, commonFormat: .pcmFormatFloat32, interleaved: false)
		let buffer = try XCTUnwrap(AVAudioPCMBuffer(pcmFormat: file.processingFormat, frameCapacity: 44100))
		buffer.frameLength = 44100
		for channel in 0..<2 {
			for frame in 0..<44100 {
				buffer.floatChannelData![channel][frame] = 0.25 * sin(2 * .pi * 440 * Float(frame) / 44100)
			}
		}
		try file.write(from: buffer)
		return url
	}

	private func writeFLAC() throws -> URL {
		try writeAudio(named: "tags.flac", settings: [AVFormatIDKey: kAudioFormatFLAC, AVLinearPCMBitDepthKey: 16])
	}

	private func writeWAV() throws -> URL {
		try writeAudio(named: "tags.wav", settings: [AVFormatIDKey: kAudioFormatLinearPCM, AVLinearPCMBitDepthKey: 16,
		                                             AVLinearPCMIsFloatKey: false, AVLinearPCMIsBigEndianKey: false])
	}

	private func writeM4A() throws -> URL {
		try writeAudio(named: "tags.m4a", settings: [AVFormatIDKey: kAudioFormatMPEG4AAC, AVEncoderBitRateKey: 128000])
	}

	/// About a second of silent MPEG-1 Layer III frames (128 kbit/s, 44.1
	/// kHz): headers and zeroed side information, which decode as silence.
	/// AVAudioFile can't write MP3.
	private func writeMP3() throws -> URL {
		let url = directory.appendingPathComponent("tags.mp3")
		var frame = Data([0xFF, 0xFB, 0x90, 0x00])
		frame.append(Data(count: 417 - 4))
		var data = Data()
		for _ in 0..<40 { data.append(frame) }
		try data.write(to: url)
		return url
	}

	/// A 2×2 PNG.
	private let png = Data(base64Encoded: "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAFklEQVR4nGP8z8DAwMDAxMDAwMDAAAANHQEDasKb6QAAAABJRU5ErkJggg==")!

	// MARK: - Reading back

	/// What the writer itself reads.
	private func written(_ url: URL) throws -> [AnyHashable: Any] {
		try XCTUnwrap(plugins.writableTags(for: url))
	}

	/// What the playlist stores: the properties and every reader's tags,
	/// merged as PlaylistEntryInfo merges them for an entry. Some formats'
	/// tags come only from their decoder (MP4, and FFmpeg's chapters).
	private func merged(_ url: URL) -> [AnyHashable: Any] {
		PlaylistEntryInfo.info(for: url) ?? [:]
	}

	private func values(_ tags: [AnyHashable: Any], _ name: String) -> [String] {
		(tags[name] as? [Any])?.map { "\($0)" } ?? (tags[name] as? String).map { [$0] } ?? []
	}

	private func write(_ tags: [String: Any], to url: URL, replace: Bool) throws {
		try plugins.writeTags(tags, to: url, replace: replace)
	}

	// MARK: - Tags

	func testFLACRoundTripsTagsIncludingCustomAndRepeatedOnes() throws {
		let url = try writeFLAC()
		let capabilities = try XCTUnwrap(plugins.tagWritingCapabilities(for: url))
		XCTAssertEqual(capabilities[CogTagWriterWritable] as? Bool, true)
		XCTAssertEqual(capabilities[CogTagWriterArbitraryKeys] as? Bool, true)
		XCTAssertNil(capabilities[CogTagWriterChapters])

		try write(["title": ["Old"], "comment": ["gone after a replace"]], to: url, replace: true)
		try write(["title": ["Sine"], "artist": ["Cog"], "genre": ["Ambient", "Test"], "cog_custom": ["kept"], "track": ["3"]], to: url, replace: true)

		let tags = try written(url)
		XCTAssertEqual(values(tags, "title"), ["Sine"])
		XCTAssertEqual(values(tags, "genre"), ["Ambient", "Test"])
		XCTAssertEqual(values(tags, "cog_custom"), ["kept"])
		XCTAssertEqual(values(tags, "track"), ["3"])
		XCTAssertNil(tags["comment"], "a replace removes what the map doesn't name")

		let read = merged(url)
		XCTAssertEqual(values(read, "title"), ["Sine"], "\(read)")
		XCTAssertEqual(values(read, "artist"), ["Cog"])
		XCTAssertTrue(values(read, "genre").contains("Test"), "\(read)")
	}

	func testMergeChangesOnlyTheNamedTagsAndAnEmptyListRemovesOne() throws {
		let url = try writeFLAC()
		try write(["title": ["Sine"], "artist": ["Cog"], "album": ["First"]], to: url, replace: true)
		try write(["album": ["Second"], "artist": []], to: url, replace: false)
		let tags = try written(url)
		XCTAssertEqual(values(tags, "title"), ["Sine"])
		XCTAssertEqual(values(tags, "album"), ["Second"])
		XCTAssertNil(tags["artist"])
	}

	func testMergeTakesATagWhateverTheCaseOfItsName() throws {
		let url = try writeFLAC()
		try write(["ALBUM ARTIST": ["Old Spelling"]], to: url, replace: true)
		try write(["album artist": ["New"]], to: url, replace: false)
		XCTAssertEqual(values(try written(url), "album artist"), ["New"])
	}

	func testCoverIsAddedAndRemoved() throws {
		let url = try writeFLAC()
		try write([CogTagAlbumArt: png], to: url, replace: false)
		XCTAssertEqual(try written(url)[CogTagAlbumArt] as? Data, png)
		XCTAssertEqual(merged(url)["albumArt"] as? Data, png)

		try write(["title": ["Still there"]], to: url, replace: true)
		XCTAssertEqual(try written(url)[CogTagAlbumArt] as? Data, png, "a write without a cover leaves it alone")

		try write([CogTagAlbumArt: Data()], to: url, replace: false)
		XCTAssertNil(try written(url)[CogTagAlbumArt])
	}

	func testWAVRoundTripsTags() throws {
		let url = try writeWAV()
		try write(["title": ["Wave"], "artist": ["Cog"]], to: url, replace: true)
		let tags = try written(url)
		XCTAssertEqual(values(tags, "title"), ["Wave"])
		XCTAssertEqual(values(merged(url), "artist"), ["Cog"])
	}

	/// MP4 tags save; chapters aren't offered, since TagLib can't keep them.
	func testM4ARoundTripsTagsWithoutOfferingChapters() throws {
		let url = try writeM4A()
		let capabilities = try XCTUnwrap(plugins.tagWritingCapabilities(for: url))
		XCTAssertEqual(capabilities[CogTagWriterWritable] as? Bool, true)
		XCTAssertNil(capabilities[CogTagWriterChapters])
		try write(["title": ["Packed"], "album": ["Box"], "cog_custom": ["atom"]], to: url, replace: true)
		let tags = try written(url)
		XCTAssertEqual(values(tags, "title"), ["Packed"])
		XCTAssertEqual(values(tags, "cog_custom"), ["atom"])
		XCTAssertEqual(values(merged(url), "album"), ["Box"], "\(merged(url))")
	}

	/// ID3v2 keeps a name it has no frame for in a TXXX frame, which reads
	/// back under the same name.
	func testMP3KeepsACustomNameAndDropsAnOldID3v1Tag() throws {
		let url = try writeMP3()
		// An ID3v1 tag with a stale title, which the save must strip.
		var id3v1 = Data("TAG".utf8)
		id3v1.append("Stale".padding(toLength: 30, withPad: "\0", startingAt: 0).data(using: .isoLatin1)!)
		id3v1.append(Data(count: 128 - id3v1.count))
		let handle = try FileHandle(forWritingTo: url)
		handle.seekToEndOfFile()
		handle.write(id3v1)
		try handle.close()

		let capabilities = try XCTUnwrap(plugins.tagWritingCapabilities(for: url))
		XCTAssertEqual(capabilities[CogTagWriterFoldsKeys] as? Bool, true)
		try write(["title": ["Fresh"], "cog_custom": ["folded"]], to: url, replace: true)
		let tags = try written(url)
		XCTAssertEqual(values(tags, "title"), ["Fresh"])
		XCTAssertEqual(values(tags, "cog_custom"), ["folded"])
		let bytes = try Data(contentsOf: url)
		XCTAssertNotEqual(bytes.suffix(128).prefix(3), Data("TAG".utf8), "the ID3v1 tag should be gone")
	}

	// MARK: - Chapters

	func testMP3ChaptersRoundTripAndFFmpegListsThemInTheSameOrder() throws {
		let url = try writeMP3()
		XCTAssertEqual(try XCTUnwrap(plugins.tagWritingCapabilities(for: url))[CogTagWriterChapters] as? Bool, true)

		// Handed out of order: they're kept by start.
		let chapters: [[String: Any]] = [
			[CogChapterStart: 600, CogChapterTitle: "Third"],
			[CogChapterStart: 0, CogChapterTitle: "First"],
			[CogChapterStart: 300, CogChapterTitle: "Second"],
		]
		try write(["album": ["Book"], CogTagChapters: chapters], to: url, replace: true)

		let read = try XCTUnwrap(try written(url)[CogTagChapters] as? [[String: Any]])
		XCTAssertEqual(read.map { $0[CogChapterTitle] as? String }, ["First", "Second", "Third"])
		XCTAssertEqual(read.map { ($0[CogChapterStart] as? NSNumber)?.intValue }, [0, 300, 600])
		XCTAssertEqual(read.map { ($0[CogChapterEnd] as? NSNumber)?.intValue }.prefix(2), [300, 600])
		XCTAssertGreaterThan((read[2][CogChapterEnd] as? NSNumber)?.intValue ?? 0, 600, "the last one ends where the file does")

		// Playback's chapters come from FFmpeg: the same three, in the same
		// order, so an entry's #N is the editor's chapter N.
		let tracks = plugins.urls(forContainerURL: url) ?? []
		XCTAssertEqual(tracks.count, 3, "\(tracks)")
		if tracks.count == 3, let second = tracks[1] as? URL {
			XCTAssertEqual(values(merged(second), "title"), ["Second"], "\(merged(second))")
		}

		// A rename keeps the chapters' identities.
		var renamed = read
		renamed[1][CogChapterTitle] = "Middle"
		try write([CogTagChapters: renamed], to: url, replace: false)
		let again = try XCTUnwrap(try written(url)[CogTagChapters] as? [[String: Any]])
		XCTAssertEqual(again.map { $0[CogChapterTitle] as? String }, ["First", "Middle", "Third"])
		XCTAssertEqual(again.map { $0[CogChapterID] as? String }, read.map { $0[CogChapterID] as? String })
		XCTAssertEqual(values(try written(url), "album"), ["Book"], "a chapter write leaves the tags alone")

		try write([CogTagChapters: []], to: url, replace: false)
		XCTAssertEqual((try written(url)[CogTagChapters] as? [Any])?.count, 0)
	}

	/// The reader's title goes to the album when the file has chapters.
	func testAChapteredFileReadsItsTitleAsTheAlbum() throws {
		let url = try writeMP3()
		try write(["title": ["The Whole Book"], CogTagChapters: [[CogChapterStart: 0, CogChapterTitle: "One"]]], to: url, replace: true)
		let reader = try XCTUnwrap(NSClassFromString("TagLibMetadataReader") as? CogMetadataReader.Type)
		let tags = reader.metadata(for: url) ?? [:]
		XCTAssertEqual(tags["album"] as? String, "The Whole Book", "\(tags)")
	}

	// MARK: - Failures

	func testAReadOnlyFileIsNotWritableAndIsLeftAlone() throws {
		let url = try writeFLAC()
		try write(["title": ["Before"]], to: url, replace: true)
		let before = try Data(contentsOf: url)
		try FileManager.default.setAttributes([.posixPermissions: 0o444], ofItemAtPath: url.path)
		defer { try? FileManager.default.setAttributes([.posixPermissions: 0o644], ofItemAtPath: url.path) }

		XCTAssertNil(plugins.tagWritingCapabilities(for: url)?[CogTagWriterWritable])
		XCTAssertThrowsError(try write(["title": ["After"]], to: url, replace: true))
		XCTAssertEqual(try Data(contentsOf: url), before)
	}

	func testNoCopyIsLeftBesideTheFile() throws {
		let url = try writeFLAC()
		try write(["title": ["Tidy"]], to: url, replace: true)
		let names = try FileManager.default.contentsOfDirectory(atPath: directory.path)
		XCTAssertEqual(names, ["tags.flac"])
	}
}
