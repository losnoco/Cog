//
//  PlaylistEntryMetadataTests.swift
//  CogPlaylistTests
//
//  Reading an entry's info, storing it, and its album art, as the macOS
//  app's loader and category did.
//

@testable import CogPlaylist
import CoreData
import XCTest

final class PlaylistEntryMetadataTests: XCTestCase {
	private var store: PlaylistStore!

	override func setUpWithError() throws {
		store = try PlaylistStore(inMemory: true)
	}

	private func makeEntry() -> PlaylistEntry {
		let entry = PlaylistEntry(context: store.viewContext)
		entry.urlString = "file:///Music/a.flac"
		entry.metadataBlob = NSDictionary()
		return entry
	}

	// MARK: - Merging properties and tags

	func testMergeKeepsWhatTheFirstHasAndFillsWhatItLacksOrHasEmpty() {
		let merged = PlaylistEntryInfo.merge(
			["title": "Kept", "album": "", "track": NSNumber(value: 0), "art": Data(), "codec": "FLAC"],
			with: ["title": "Other", "album": "Filled", "track": NSNumber(value: 3), "art": Data([1]), "artist": "Added"])
		XCTAssertEqual(merged["title"] as? String, "Kept")
		XCTAssertEqual(merged["album"] as? String, "Filled")
		XCTAssertEqual(merged["track"] as? NSNumber, 3)
		XCTAssertEqual(merged["art"] as? Data, Data([1]))
		XCTAssertEqual(merged["artist"] as? String, "Added")
		XCTAssertEqual(merged["codec"] as? String, "FLAC")
	}

	func testMergeMergesNestedDictionaries() {
		let merged = PlaylistEntryInfo.merge(["inner": ["a": "1", "b": ""]], with: ["inner": ["b": "2", "c": "3"]])
		let inner = merged["inner"] as? [String: Any]
		XCTAssertEqual(inner?["a"] as? String, "1")
		XCTAssertEqual(inner?["b"] as? String, "2")
		XCTAssertEqual(inner?["c"] as? String, "3")
	}

	func testCueSheetTracksNeedAFileAndAFragment() {
		XCTAssertTrue(PlaylistEntryInfo.isCueSheetTrack(URL(string: "file:///Music/album.cue#03")!))
		XCTAssertFalse(PlaylistEntryInfo.isCueSheetTrack(URL(string: "file:///Music/album.cue")!))
		XCTAssertFalse(PlaylistEntryInfo.isCueSheetTrack(URL(string: "http://example.com/a.cue#03")!))
	}

	// MARK: - Storing it

	func testSetMetadataStoresPropertiesInAttributesAndTagsInTheBlob() {
		let entry = makeEntry()
		entry.volume = 0.5
		entry.setMetadata(["bitrate": NSNumber(value: 320), "samplerate": NSNumber(value: 44100), "channels": "2",
		                   "totalFrames": NSNumber(value: 88200), "codec": "MP3", "floatingPoint": "1",
		                   "replaygain_track_gain": "-6.5", "artist": ["A", "B"], "title": "Song",
		                   // Plugins name tags with periods with U+2024, as the blob keeps them.
		                   "x\u{2024}y": "1"])
		XCTAssertEqual(entry.bitrate, 320)
		XCTAssertEqual(entry.sampleRate, 44100)
		XCTAssertEqual(entry.channels, 2)
		XCTAssertEqual(entry.totalFrames, 88200)
		XCTAssertEqual(entry.codec, "MP3")
		XCTAssertTrue(entry.floatingPoint)
		XCTAssertEqual(entry.replayGainTrackGain, -6.5)
		XCTAssertEqual(entry.volume, 1, "the volume resets unless the info names one")
		XCTAssertEqual(entry.artist, "A, B")
		XCTAssertEqual(entry.rawTitle, "Song")
		XCTAssertEqual(entry.readAllValuesAsString("x.y"), "1")
		XCTAssertNil((entry.metadataBlob as? NSDictionary)?["bitrate"], "properties are attributes, not tags")
		XCTAssertTrue(entry.metadataLoaded)
		XCTAssertFalse(entry.error)
		XCTAssertEqual(entry.length.doubleValue, 2, accuracy: 1e-9)
	}

	func testSetMetadataKeepsTagsAlreadyStored() {
		let entry = makeEntry()
		entry.metadataBlob = ["comment": ["Kept"]] as NSDictionary
		entry.setMetadata(["artist": "A"])
		XCTAssertEqual(entry.comment, "Kept")
		XCTAssertEqual(entry.artist, "A")
	}

	func testNoMetadataMarksTheEntryUnreadable() {
		let entry = makeEntry()
		entry.setMetadata(nil)
		XCTAssertTrue(entry.error)
		XCTAssertNotNil(entry.errorMessage)
		XCTAssertTrue(entry.metadataLoaded)
	}

	func testSetMetadataAnnouncesIt() {
		let entry = makeEntry()
		let announced = expectation(forNotification: CogPlaylistEntryMetadataLoadedNotification, object: entry)
		entry.setMetadata([:])
		wait(for: [announced], timeout: 1)
	}

	// MARK: - Album art

	func testArtIsStoredOnceByItsHash() throws {
		let picture = Data([0xFF, 0xD8, 0xFF, 0xE0, 1, 2, 3])
		let first = makeEntry()
		let second = makeEntry()
		first.setMetadata(["albumart": picture])
		second.albumArtInternal = picture

		XCTAssertEqual(first.artHash, second.artHash)
		XCTAssertEqual(first.artHash?.count, 64)
		XCTAssertEqual(first.artHash, first.artHash?.lowercased())
		XCTAssertEqual(first.albumArtInternal, picture)
		let stored = try store.viewContext.fetch(NSFetchRequest<AlbumArtwork>(entityName: "AlbumArtwork"))
		XCTAssertEqual(stored.count, 1)
	}

	func testEmptyArtChangesNothing() {
		let entry = makeEntry()
		entry.albumArtInternal = Data([1])
		let hash = entry.artHash
		entry.albumArtInternal = Data()
		entry.albumArtInternal = nil
		XCTAssertEqual(entry.artHash, hash)
	}

	func testStoredArtIsFoundAfterReopening() throws {
		let url = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString + ".sqlite")
		defer { try? FileManager.default.removeItem(at: url) }
		do {
			let store = try PlaylistStore(url: url)
			let entry = PlaylistEntry(context: store.viewContext)
			entry.albumArtInternal = Data([9, 9, 9])
			store.save()
		}
		let reopened = try PlaylistStore(url: url)
		let entry = try XCTUnwrap(reopened.viewContext.fetch(NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")).first)
		XCTAssertEqual(entry.albumArtInternal, Data([9, 9, 9]))
	}
}
