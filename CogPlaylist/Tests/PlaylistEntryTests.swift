//
//  PlaylistEntryTests.swift
//  CogPlaylistTests
//
//  What PlaylistEntry+Extension promises, as the macOS app's category did.
//

@testable import CogPlaylist
import CoreData
import XCTest

final class PlaylistEntryTests: XCTestCase {
	private var store: PlaylistStore!

	override func setUpWithError() throws {
		store = try PlaylistStore(inMemory: true)
	}

	private func makeEntry(url: String? = nil, tags: [String: [String]] = [:]) -> PlaylistEntry {
		let entry = PlaylistEntry(context: store.viewContext)
		entry.urlString = url
		entry.metadataBlob = tags as NSDictionary
		return entry
	}

	// MARK: - URL

	func testURLFromStoredString() {
		XCTAssertEqual(makeEntry(url: "file:///Music/a%20b.flac").url?.path, "/Music/a b.flac")
		XCTAssertEqual(makeEntry(url: "http://example.com/stream").url?.absoluteString, "http://example.com/stream")
	}

	func testURLFromBarePathKeepsCueFragment() {
		let url = makeEntry(url: "/Music/album.cue#03").url
		XCTAssertEqual(url?.path, "/Music/album.cue")
		XCTAssertEqual(url?.fragment, "03")
	}

	func testURLFromBarePathWithHashInsideName() {
		// A '#' not followed by digits up to the end is part of the name.
		XCTAssertEqual(makeEntry(url: "/Music/#1 hits.mp3").url?.path, "/Music/#1 hits.mp3")
	}

	func testNoURLIsSilence() {
		XCTAssertEqual(makeEntry(url: nil).url?.absoluteString, "silence://10")
		XCTAssertEqual(makeEntry(url: "").url?.absoluteString, "silence://10")
	}

	func testSettingURLStoresItsString() {
		let entry = makeEntry()
		entry.url = URL(fileURLWithPath: "/Music/x.ogg")
		XCTAssertEqual(entry.urlString, "file:///Music/x.ogg")
		entry.url = nil
		XCTAssertNil(entry.urlString)
	}

	func testFilenameAndFragment() {
		let entry = makeEntry(url: "file:///Music/album.flac#02")
		XCTAssertEqual(entry.filename, "album.flac")
		XCTAssertEqual(entry.filenameFragment, "album.flac#02")
		XCTAssertEqual(makeEntry(url: "file:///Music/a.mp3").filenameFragment, "a.mp3")
	}

	func testPathAbbreviatesHomeAndKeepsOtherSchemes() {
		let home = NSHomeDirectory()
		XCTAssertEqual(makeEntry(url: URL(fileURLWithPath: home + "/x.mp3").absoluteString).path, "~/x.mp3")
		XCTAssertEqual(makeEntry(url: "http://example.com/a.mp3").path, "http://example.com/a.mp3")
	}

	// MARK: - Tags

	func testTagsReadFromTheBlob() {
		let entry = makeEntry(tags: ["artist": ["A", "B"], "album": ["Album"], "title": ["Song"]])
		XCTAssertEqual(entry.artist, "A, B")
		XCTAssertEqual(entry.album, "Album")
		XCTAssertEqual(entry.rawTitle, "Song")
		XCTAssertNil(entry.genre)
	}

	func testTagNamesWithPeriodsAreStoredWithOnePointLeaders() {
		let entry = makeEntry(tags: [:])
		entry.setValue("x.y", fromString: "1")
		XCTAssertEqual((entry.metadataBlob as? NSDictionary)?["x\u{2024}y"] as? [String], ["1"])
		XCTAssertEqual(entry.readAllValuesAsString("x.y"), "1")
		XCTAssertEqual(PlaylistEntry.metaTag(forKey: "x\u{2024}y"), "x.y")
	}

	func testSettingValuesSplitsOnCommaSpaceAndNilRemoves() {
		let entry = makeEntry(tags: [:])
		entry.genre = "Rock, Pop"
		XCTAssertEqual((entry.metadataBlob as? NSDictionary)?["genre"] as? [String], ["Rock", "Pop"])
		entry.genre = nil
		XCTAssertNil(entry.genre)
	}

	func testAddValueAppends() {
		let entry = makeEntry(tags: ["artist": ["A"]])
		entry.addValue("artist", fromString: "B")
		entry.addValue("composer", fromString: "C")
		XCTAssertEqual(entry.artist, "A, B")
		XCTAssertEqual(entry.composer, "C")
	}

	func testNothingIsSetWithoutABlob() {
		let entry = makeEntry()
		entry.metadataBlob = nil
		entry.artist = "A"
		XCTAssertNil(entry.artist)
	}

	func testAlbumArtistFallsBackAndSettingClearsTheOthers() {
		let entry = makeEntry(tags: ["album artist": ["Spaced"]])
		XCTAssertEqual(entry.albumartist, "Spaced")
		entry.albumartist = "Plain"
		XCTAssertEqual(entry.readAllValuesAsString("albumartist"), "Plain")
		XCTAssertNil(entry.readAllValuesAsString("album artist"))
	}

	func testDiscTrackAndYearReadTheirLeadingNumbers() {
		let entry = makeEntry(tags: ["discnumber": ["2/3"], "tracknumber": ["05/12"], "date": ["1999-04-01"]])
		XCTAssertEqual(entry.disc, 2)
		XCTAssertEqual(entry.track, 5)
		XCTAssertEqual(entry.year, 1999)
		XCTAssertEqual(entry.trackText, "2.05")
		XCTAssertEqual(entry.yearText, "1999")
	}

	func testTrackTextWithoutDiscOrTrack() {
		XCTAssertEqual(makeEntry(tags: ["track": ["7"]]).trackText, "07")
		XCTAssertEqual(makeEntry(tags: [:]).trackText, "")
		XCTAssertEqual(makeEntry(tags: [:]).yearText, "")
	}

	func testSettingYearDiscAndTrack() {
		let entry = makeEntry(tags: ["year": ["1980"], "disc": ["9"]])
		entry.year = 2001
		XCTAssertEqual(entry.date, "2001")
		XCTAssertNil(entry.readAllValuesAsString("year"))
		entry.year = 0
		XCTAssertNil(entry.date)
		entry.disc = 3
		XCTAssertEqual(entry.readAllValuesAsString("discnumber"), "3")
		XCTAssertNil(entry.readAllValuesAsString("disc"))
		entry.setValue("track", fromString: "4")
		entry.track = 6
		XCTAssertEqual(entry.readAllValuesAsString("tracknumber"), "6")
		XCTAssertNil(entry.readAllValuesAsString("track"))
	}

	// MARK: - What is shown

	func testTitleFallsBackToTheFileName() {
		XCTAssertEqual(makeEntry(url: "file:///Music/song.mp3", tags: [:]).title, "song.mp3")
		XCTAssertEqual(makeEntry(url: "file:///Music/song.mp3", tags: ["title": ["Named"]]).title, "Named")
	}

	func testDisplay() {
		XCTAssertEqual(makeEntry(url: "file:///a.mp3", tags: ["artist": ["A"], "title": ["T"]]).display, "A - T")
		XCTAssertEqual(makeEntry(url: "file:///a.mp3", tags: ["title": ["T"]]).display, "T")
	}

	func testLengthOnceLoaded() {
		let entry = makeEntry()
		entry.totalFrames = 88200
		entry.sampleRate = 44100
		XCTAssertEqual(entry.length, 0)
		entry.metadataLoaded = true
		XCTAssertEqual(entry.length.doubleValue, 2, accuracy: 1e-9)
	}

	func testStatusPrecedence() {
		let entry = makeEntry()
		XCTAssertNil(entry.status)
		entry.error = true
		XCTAssertEqual(entry.status, "error")
		entry.queued = true
		XCTAssertEqual(entry.status, "queued")
		entry.current = true
		XCTAssertEqual(entry.status, "playing")
		entry.stopAfter = true
		XCTAssertEqual(entry.status, "stopAfter")
	}

	func testCuesheetPresent() {
		let entry = makeEntry()
		XCTAssertEqual(entry.cuesheetPresent, "no")
		entry.cuesheet = "FILE x"
		XCTAssertEqual(entry.cuesheetPresent, "yes")
	}

	func testUppercaseAliases() {
		let entry = makeEntry()
		entry.setValue(true, forKey: "Unsigned")
		XCTAssertTrue(entry.unSigned)
		entry.setValue(URL(fileURLWithPath: "/x.wav"), forKey: "URL")
		XCTAssertEqual(entry.urlString, "file:///x.wav")
	}

	// MARK: - Sound Check

	func testSoundcheck() {
		// 0x7D0 = 2000 is -3.01 dB; 0x1F4 = 500 is +3.01 dB; the lower wins.
		let tag = " 000007D0 000001F4 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000"
		XCTAssertEqual(PlaylistEntry.calculateSoundcheck(tag), -3.0103, accuracy: 1e-4)
		XCTAssertEqual(PlaylistEntry.calculateSoundcheck("too short"), 1)
		let entry = makeEntry()
		entry.soundcheck = tag
		XCTAssertEqual(entry.soundcheckDisplay, "-3.010300 dB")
		XCTAssertEqual(entry.soundcheckVolume, 0.7071, accuracy: 1e-4)
	}

	// MARK: - Bindings

	/// The window binds to the named tags and texts; they have to announce
	/// changes to what they are made of.
	func testDependentKeysNotify() {
		let entry = makeEntry(url: "file:///a.mp3", tags: [:])
		var changed = Set<String>()
		let keys = ["artist", "title", "display", "trackText", "yearText", "url", "filename", "status", "length"]
		let observer = KeyObserver { changed.insert($0) }
		keys.forEach { entry.addObserver(observer, forKeyPath: $0, options: [], context: nil) }
		defer { keys.forEach { entry.removeObserver(observer, forKeyPath: $0) } }

		entry.artist = "A"
		entry.rawTitle = "T"
		entry.setValue("tracknumber", fromString: "1")
		entry.date = "2000"
		entry.url = URL(fileURLWithPath: "/b.mp3")
		entry.queued = true
		entry.metadataLoaded = true

		XCTAssertEqual(changed, Set(keys))
	}
}

private final class KeyObserver: NSObject {
	let onChange: (String) -> Void

	init(_ onChange: @escaping (String) -> Void) {
		self.onChange = onChange
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?,
	                           context: UnsafeMutableRawPointer?) {
		if let keyPath { onChange(keyPath) }
	}
}
