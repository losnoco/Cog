//
//  TagLibChapterTests.swift
//  CogPluginsTests
//
//  The ID3v2 chapter reader the minimp3 decoder will read its spans from:
//  TagLibID3v2Reader's chaptersForTag:, over a tag block. It shares the
//  metadata writer's reader, so the chapters and their order must agree
//  with what the writer saves and reads back, and hold to the CHAP/CTOC
//  rule: the top-level ordered table of contents' order when it gives one,
//  else by start.
//

@testable import CogAudio
import XCTest

final class TagLibChapterTests: XCTestCase {
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

	private func chapters(forTag data: Data) -> [[String: Any]] {
		// The plugin's class method has no Swift protocol; call it by selector.
		guard let anyClass = NSClassFromString("TagLibID3v2Reader") else {
			XCTFail("TagLibID3v2Reader is not loaded")
			return []
		}
		let cls = anyClass as AnyObject
		let selector = NSSelectorFromString("chaptersForTag:")
		guard cls.responds(to: selector) else {
			XCTFail("TagLibID3v2Reader does not answer chaptersForTag:")
			return []
		}
		let result = cls.perform(selector, with: data as NSData)
		return (result?.takeUnretainedValue() as? [[String: Any]]) ?? []
	}

	private func titles(_ chapters: [[String: Any]]) -> [String] {
		chapters.map { $0[CogChapterTitle] as? String ?? "" }
	}

	private func starts(_ chapters: [[String: Any]]) -> [UInt32] {
		chapters.map { ($0[CogChapterStart] as? NSNumber)?.uint32Value ?? 0 }
	}

	// MARK: - Fixtures

	/// About a second of silent MPEG-1 Layer III frames, as TagWriterTests
	/// builds them: AVAudioFile can't write MP3.
	private func writeMP3() throws -> URL {
		let url = directory.appendingPathComponent("chapters.mp3")
		var frame = Data([0xFF, 0xFB, 0x90, 0x00])
		frame.append(Data(count: 417 - 4))
		var data = Data()
		for _ in 0..<40 { data.append(frame) }
		try data.write(to: url)
		return url
	}

	private func syncsafe(_ n: Int) -> Data {
		Data([UInt8((n >> 21) & 0x7F), UInt8((n >> 14) & 0x7F), UInt8((n >> 7) & 0x7F), UInt8(n & 0x7F)])
	}

	private func be32(_ n: UInt32) -> Data {
		Data([UInt8((n >> 24) & 0xFF), UInt8((n >> 16) & 0xFF), UInt8((n >> 8) & 0xFF), UInt8(n & 0xFF)])
	}

	/// One ID3v2.3/2.4 frame: identifier, size, two flag bytes, payload.
	private func frame(_ id: String, _ payload: Data, version: UInt8 = 4) -> Data {
		var d = Data(id.utf8)
		d.append(version == 4 ? syncsafe(payload.count) : be32(UInt32(payload.count)))
		d.append(Data([0, 0]))
		d.append(payload)
		return d
	}

	/// A whole tag: the ten-byte header and the frames after it.
	private func tag(_ frames: Data, version: UInt8 = 4) -> Data {
		var d = Data("ID3".utf8)
		d.append(Data([version, 0, 0]))
		d.append(syncsafe(frames.count))
		d.append(frames)
		return d
	}

	private func textPayload(_ s: String, encoding: UInt8) -> Data {
		var d = Data([encoding])
		switch encoding {
			case 1:
				d.append(Data([0xFF, 0xFE]))
				d.append(s.data(using: .utf16LittleEndian) ?? Data())
			case 2:
				d.append(s.data(using: .utf16BigEndian) ?? Data())
			default:
				d.append(Data(s.utf8))
		}
		d.append((encoding == 1 || encoding == 2) ? Data([0, 0]) : Data([0]))
		return d
	}

	/// A CHAP payload: element ID, start/end ms, the two ignored offsets,
	/// and an optional embedded TIT2 title.
	private func chap(_ elementID: String, start: UInt32, end: UInt32, title: String? = nil, encoding: UInt8 = 3) -> Data {
		var p = Data(elementID.utf8)
		p.append(0)
		p.append(be32(start))
		p.append(be32(end))
		p.append(be32(0xFFFF_FFFF))
		p.append(be32(0xFFFF_FFFF))
		if let title {
			p.append(frame("TIT2", textPayload(title, encoding: encoding)))
		}
		return p
	}

	/// A CTOC payload: element ID, flags, child count, the child element IDs.
	private func ctoc(_ elementID: String, topLevel: Bool, ordered: Bool, children: [String]) -> Data {
		var p = Data(elementID.utf8)
		p.append(0)
		var flags: UInt8 = 0
		if topLevel { flags |= 0x02 }
		if ordered { flags |= 0x01 }
		p.append(flags)
		p.append(UInt8(children.count))
		for child in children {
			p.append(Data(child.utf8))
			p.append(0)
		}
		return p
	}

	// MARK: - Agreement with the writer

	func testChaptersForTagReadsWhatTheWriterWrote() throws {
		let url = try writeMP3()
		let input: [[String: Any]] = [
			[CogChapterStart: 600, CogChapterTitle: "Third"],
			[CogChapterStart: 0, CogChapterTitle: "First"],
			[CogChapterStart: 300, CogChapterTitle: "Second"],
		]
		try plugins.writeTags(["album": ["Book"], CogTagChapters: input], to: url, replace: true)

		// The tag the writer saved, read straight off the file's head.
		let file = try Data(contentsOf: url)
		XCTAssertEqual(file.prefix(3), Data("ID3".utf8))
		let size = Int(file[6]) << 21 | Int(file[7]) << 14 | Int(file[8]) << 7 | Int(file[9])
		let tagBlock = file.prefix(10 + size)

		let read = chapters(forTag: Data(tagBlock))
		let written = try XCTUnwrap(plugins.writableTags(for: url)?[CogTagChapters] as? [[String: Any]])
		XCTAssertEqual(titles(read), ["First", "Second", "Third"])
		XCTAssertEqual(starts(read), [0, 300, 600])
		XCTAssertEqual(titles(read), titles(written), "the reader and the writer agree by construction")
		XCTAssertEqual(starts(read), starts(written))
		XCTAssertEqual(read.map { $0[CogChapterID] as? String }, written.map { $0[CogChapterID] as? String })
	}

	// MARK: - The order rule

	func testTheTopLevelOrderedTableOfContentsOrderWins() {
		var frames = Data()
		frames.append(frame("CHAP", chap("a", start: 0, end: 300, title: "A")))
		frames.append(frame("CHAP", chap("b", start: 300, end: 600, title: "B")))
		frames.append(frame("CHAP", chap("c", start: 600, end: 900, title: "C")))
		frames.append(frame("CTOC", ctoc("toc", topLevel: true, ordered: true, children: ["c", "a", "b"])))
		let read = chapters(forTag: tag(frames))
		XCTAssertEqual(titles(read), ["C", "A", "B"], "the table's order, not the start order")
		XCTAssertEqual(starts(read), [600, 0, 300])
	}

	func testWithoutAnOrderedTableChaptersSortByStart() {
		var frames = Data()
		frames.append(frame("CHAP", chap("c", start: 600, end: 900, title: "C")))
		frames.append(frame("CHAP", chap("a", start: 0, end: 300, title: "A")))
		frames.append(frame("CHAP", chap("b", start: 300, end: 600, title: "B")))
		XCTAssertEqual(titles(chapters(forTag: tag(frames))), ["A", "B", "C"])

		// An unordered table gives no order, so the start sort stands.
		frames.append(frame("CTOC", ctoc("toc", topLevel: true, ordered: false, children: ["c", "a", "b"])))
		XCTAssertEqual(titles(chapters(forTag: tag(frames))), ["A", "B", "C"])
	}

	func testANonTopLevelTableIsIgnored() {
		var frames = Data()
		frames.append(frame("CHAP", chap("a", start: 0, end: 300, title: "A")))
		frames.append(frame("CHAP", chap("b", start: 300, end: 600, title: "B")))
		// Ordered, but not top-level: it does not speak for the whole file.
		frames.append(frame("CTOC", ctoc("nested", topLevel: false, ordered: true, children: ["b", "a"])))
		XCTAssertEqual(titles(chapters(forTag: tag(frames))), ["A", "B"])
	}

	// MARK: - Versions, encodings, robustness

	func testAnID3v22TagYieldsNoChapters() {
		// CHAP and CTOC are four-character frames, which v2.2 does not have.
		let frames = frame("TT2", textPayload("A title", encoding: 0), version: 2)
		XCTAssertTrue(chapters(forTag: tag(frames, version: 2)).isEmpty)
	}

	func testTitlesReadInEveryEncoding() {
		var frames = Data()
		frames.append(frame("CHAP", chap("a", start: 0, end: 100, title: "Alpha", encoding: 0)))
		frames.append(frame("CHAP", chap("b", start: 100, end: 200, title: "Beta", encoding: 1)))
		frames.append(frame("CHAP", chap("c", start: 200, end: 300, title: "Gamma", encoding: 2)))
		frames.append(frame("CHAP", chap("d", start: 300, end: 400, title: "Café", encoding: 3)))
		XCTAssertEqual(titles(chapters(forTag: tag(frames))), ["Alpha", "Beta", "Gamma", "Café"])
	}

	func testATruncatedChapterFrameIsDroppedAndTheRestRead() {
		var frames = Data()
		frames.append(frame("CHAP", chap("a", start: 0, end: 300, title: "Whole")))
		// A CHAP whose payload stops before the times are complete.
		var broken = Data("b".utf8)
		broken.append(0)
		broken.append(be32(300))
		frames.append(frame("CHAP", broken))
		let read = chapters(forTag: tag(frames))
		XCTAssertTrue(titles(read).contains("Whole"), "the intact chapter reads; the broken one does not crash the walk")
	}

	func testEmptyAndNonTagDataYieldNoChapters() {
		XCTAssertTrue(chapters(forTag: Data()).isEmpty)
		XCTAssertTrue(chapters(forTag: Data([0x00, 0x01, 0x02, 0x03, 0xFF, 0xFB, 0x90, 0x00])).isEmpty)
	}
}