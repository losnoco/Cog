//
//  SyncedLyricsTests.swift
//  CogPlaylistTests
//

@testable import CogPlaylist
import XCTest

final class SyncedLyricsTests: XCTestCase {
	func testTimestampsInEachForm() throws {
		let lyrics = try XCTUnwrap(SyncedLyrics("[00:01]One\n[00:02.5]Two\n[00:03.25]Three\n[00:04.125]Four\n[01:05:50]Five"))
		XCTAssertEqual(lyrics.lines.map(\.text), ["One", "Two", "Three", "Four", "Five"])
		let expected = [1, 2.5, 3.25, 4.125, 65.5]
		for (line, time) in zip(lyrics.lines, expected) {
			XCTAssertEqual(line.time, time, accuracy: 0.0001)
		}
	}

	func testWordsWithoutATimeRunOnFromTheLineBefore() throws {
		let lyrics = try XCTUnwrap(SyncedLyrics("[00:03.49]知りたいその\n秘密ミステリアス\n[00:06.07]抜けてるとこさえ"))
		XCTAssertEqual(lyrics.lines.map(\.text), ["知りたいその\n秘密ミステリアス", "抜けてるとこさえ"])
	}

	func testALineWithSeveralTimesIsSungAtEach() throws {
		let lyrics = try XCTUnwrap(SyncedLyrics("[00:10.00][00:30.00]Chorus\n[00:20.00]Verse"))
		XCTAssertEqual(lyrics.lines.map(\.text), ["Chorus", "Verse", "Chorus"])
		XCTAssertEqual(lyrics.lines.map(\.id), [0, 1, 2])
		XCTAssertEqual(SyncedLyrics.plainText(from: "[00:10.00][00:30.00]Chorus\n[00:20.00]Verse"), "Chorus\nVerse")
	}

	func testHeadersAreDroppedAndTheOffsetApplies() throws {
		let lyrics = try XCTUnwrap(SyncedLyrics("[ar:Artist]\n[ti:Title]\n[offset:+500]\n[00:01.00]One\n[00:02.00]Two"))
		XCTAssertEqual(lyrics.lines.map(\.text), ["One", "Two"])
		XCTAssertEqual(lyrics.lines[0].time, 0.5, accuracy: 0.0001)
		XCTAssertEqual(lyrics.lines[1].time, 1.5, accuracy: 0.0001)
	}

	func testWordStampsAreRemoved() throws {
		let lyrics = try XCTUnwrap(SyncedLyrics("[00:01.00]<00:01.00>One <00:01.50>two\n[00:02.00]a <b> c"))
		XCTAssertEqual(lyrics.lines.map(\.text), ["One two", "a <b> c"])
	}

	func testGapsStayAsEmptyLines() throws {
		let lyrics = try XCTUnwrap(SyncedLyrics("[00:01.00]One\n[00:02.00]\n[00:03.00]Two"))
		XCTAssertEqual(lyrics.lines.map(\.text), ["One", "", "Two"])
		XCTAssertEqual(SyncedLyrics.plainText(from: "[00:01.00]One\n[00:02.00]\n[00:03.00]Two"), "One\n\nTwo")
	}

	func testTextThatIsNotTimedIsNotSynced() {
		XCTAssertNil(SyncedLyrics("Just\nsome words"))
		XCTAssertNil(SyncedLyrics("[Verse: Someone] Words\n[Chorus]\nMore"))
		XCTAssertNil(SyncedLyrics("[00:00.00]All\n[00:00.00]at zero"))
		XCTAssertNil(SyncedLyrics(""))
	}

	func testPlainTextIsLeftAlone() {
		let text = "[Chorus]\nJust  words\n\nAnd more"
		XCTAssertEqual(SyncedLyrics.plainText(from: text), text)
	}

	func testPlainTextDropsTimesAndHeaders() {
		XCTAssertEqual(SyncedLyrics.plainText(from: "[ar:Artist]\n[00:01.00]One\nmore\n[00:02.00]Two"), "One\nmore\nTwo")
	}

	func testTheLineAtATime() throws {
		let lyrics = try XCTUnwrap(SyncedLyrics("[00:02.00]One\n[00:04.00]Two\n[00:06.00]Three"))
		XCTAssertNil(lyrics.lineIndex(at: 1.99))
		XCTAssertEqual(lyrics.lineIndex(at: 2), 0)
		XCTAssertEqual(lyrics.lineIndex(at: 5.99), 1)
		XCTAssertEqual(lyrics.lineIndex(at: 600), 2)
	}
}
