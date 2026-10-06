//
//  PlaylistModelTests.swift
//  CogPlaylistTests
//
//  The playlist's rules (what plays next, the queue, shuffle, repeat,
//  removing what plays), as macOS's PlaylistController has them.
//

@testable import CogPlaylist
import CoreData
import XCTest

@MainActor
final class PlaylistModelTests: XCTestCase {
	private var store: PlaylistStore!
	private var defaults: UserDefaults!
	private var model: PlaylistModel!

	override func setUp() async throws {
		store = try PlaylistStore(inMemory: true)
		defaults = UserDefaults(suiteName: "PlaylistModelTests-\(UUID().uuidString)")
		defaults.set(PlaylistRepeatMode.none.rawValue, forKey: "repeat")
		model = PlaylistModel(store: store, defaults: defaults)
	}

	/// Entries named "A1", "A2", "B1", ...: album, then track.
	@discardableResult
	private func add(_ names: [String]) -> [PlaylistEntry] {
		let entries = names.map { name -> PlaylistEntry in
			let entry = PlaylistEntry(context: store.viewContext)
			entry.url = URL(fileURLWithPath: "/music/\(name).flac")
			entry.metadataBlob = ["album": [String(name.prefix(1))], "tracknumber": [String(name.dropFirst())], "title": [name]] as NSDictionary
			return entry
		}
		model.insert(entries)
		return entries
	}

	private func names(_ entries: [PlaylistEntry?]) -> [String] {
		entries.map { $0?.title ?? "nil" }
	}

	/// What plays from `start` on, `count` tracks, as the engine asks.
	private func playOrder(from start: PlaylistEntry, count: Int) -> [String] {
		var order: [PlaylistEntry?] = [start]
		var entry: PlaylistEntry? = start
		for _ in 1..<count {
			entry = model.nextEntry(after: entry)
			order.append(entry)
			if entry == nil { break }
		}
		return names(order)
	}

	// MARK: - Order

	func testEntriesAreIndexedInOrder() {
		add(["A1", "A2", "B1"])
		XCTAssertEqual(model.entries.map(\.index), [0, 1, 2])
		model.move(fromOffsets: IndexSet(integer: 2), toOffset: 0)
		XCTAssertEqual(names(model.entries), ["B1", "A1", "A2"])
		XCTAssertEqual(model.entries.map(\.index), [0, 1, 2])
	}

	func testPlaysInOrderAndStopsAtTheEnd() {
		let entries = add(["A1", "A2", "B1"])
		XCTAssertEqual(playOrder(from: entries[0], count: 4), ["A1", "A2", "B1", "nil"])
	}

	func testRepeatAllGoesAround() {
		let entries = add(["A1", "A2"])
		model.repeatMode = .all
		XCTAssertEqual(playOrder(from: entries[0], count: 5), ["A1", "A2", "A1", "A2", "A1"])
		XCTAssertEqual(names([model.previousEntry(before: entries[0])]), ["A2"])
	}

	func testRepeatOneRepeatsUnlessSkipped() {
		let entries = add(["A1", "A2"])
		model.repeatMode = .one
		XCTAssertEqual(model.nextEntry(after: entries[0]), entries[0])
		XCTAssertEqual(model.nextEntry(after: entries[0], ignoreRepeatOne: true), entries[1])
	}

	func testRepeatAlbumStaysOnTheAlbum() {
		let entries = add(["A1", "A2", "B1", "B2"])
		model.repeatMode = .album
		XCTAssertEqual(playOrder(from: entries[0], count: 5), ["A1", "A2", "A1", "A2", "A1"])
		XCTAssertEqual(playOrder(from: entries[2], count: 4), ["B1", "B2", "B1", "B2"])
	}

	func testAlwaysStopAfterCurrent() {
		let entries = add(["A1", "A2"])
		defaults.set(true, forKey: "alwaysStopAfterCurrent")
		XCTAssertNil(model.nextEntry(after: entries[0]))
		// Skipping still moves.
		model.setCurrent(entries[0])
		XCTAssertTrue(model.next())
		XCTAssertEqual(model.currentEntry, entries[1])
	}

	func testNextAndPreviousMoveTheCurrentEntry() {
		let entries = add(["A1", "A2", "B1"])
		XCTAssertTrue(model.next(), "from nothing, the first")
		XCTAssertEqual(model.currentEntry, entries[0])
		XCTAssertTrue(entries[0].current)
		XCTAssertTrue(model.next())
		XCTAssertEqual(model.currentEntry, entries[1])
		XCTAssertFalse(entries[0].current)
		XCTAssertTrue(model.previous())
		XCTAssertEqual(model.currentEntry, entries[0])
		XCTAssertFalse(model.previous(), "nothing before the first")
	}

	// MARK: - Queue

	func testTheQueuePlaysFirstAndEmpties() {
		let entries = add(["A1", "A2", "B1", "B2"])
		model.addToQueue([entries[3], entries[2]])
		XCTAssertEqual(entries[3].queuePosition, 0)
		XCTAssertEqual(entries[2].queuePosition, 1)
		XCTAssertEqual(playOrder(from: entries[0], count: 4), ["A1", "B2", "B1", "B2"],
		               "after the queue, on from the last queued entry")
		XCTAssertTrue(model.queue.isEmpty)
		XCTAssertFalse(entries[3].queued)
	}

	func testTogglingTheQueueRenumbersIt() {
		let entries = add(["A1", "A2", "B1"])
		model.toggleQueued([entries[0], entries[1], entries[2]])
		model.toggleQueued([entries[1]])
		XCTAssertEqual(model.queue, [entries[0], entries[2]])
		XCTAssertEqual(model.queue.map(\.queuePosition), [0, 1])
		XCTAssertEqual(entries[1].queuePosition, -1)
	}

	// MARK: - Removing

	func testRemovingThePlayingEntryGoesOnFromTheNext() {
		let entries = add(["A1", "A2", "B1", "B2"])
		model.setCurrent(entries[1])
		model.remove(at: IndexSet([1, 2]))
		XCTAssertEqual(names(model.entries), ["A1", "B2"])
		XCTAssertTrue(entries[1].deLeted)
		XCTAssertLessThan(entries[1].index, 0, "remembered where it was")
		XCTAssertEqual(model.nextEntry(after: entries[1]), entries[3])
		XCTAssertEqual(model.previousEntry(before: entries[1]), entries[0])
	}

	func testRemovingTheLastWhilePlayingItGoesBackToTheStart() {
		let entries = add(["A1", "A2"])
		model.setCurrent(entries[1])
		model.remove(at: IndexSet(integer: 1))
		XCTAssertEqual(model.nextEntry(after: entries[1]), entries[0], "nothing after it: from the start, as macOS does")
	}

	func testRemovedEntriesLeaveTheQueue() {
		let entries = add(["A1", "A2", "B1"])
		model.addToQueue([entries[2], entries[1]])
		model.remove(at: IndexSet(integer: 2))
		XCTAssertEqual(model.queue, [entries[1]])
		XCTAssertEqual(entries[1].queuePosition, 0)
	}

	func testRemovedEntriesAreDroppedOnLoad() {
		add(["A1", "A2", "B1"])
		model.remove(at: IndexSet(integer: 0))
		let reloaded = PlaylistModel(store: store, defaults: defaults)
		XCTAssertEqual(names(reloaded.entries), ["A2", "B1"])
		XCTAssertEqual(reloaded.entries.map(\.index), [0, 1])
		let request = NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")
		XCTAssertEqual(try store.viewContext.count(for: request), 2)
	}

	// MARK: - Shuffle

	func testShufflePlaysEverythingOnce() {
		let entries = add((1...20).map { "A\($0)" })
		model.setCurrent(entries[0])
		model.shuffleMode = .all
		let order = playOrder(from: entries[0], count: 21)
		XCTAssertEqual(order.first, "A1", "starts from what plays")
		XCTAssertEqual(order.last, "nil", "and ends without repeat")
		XCTAssertEqual(Set(order.dropLast()).count, 20)
	}

	func testShuffleAlbumsKeepsAlbumsWholeAndInOrder() {
		let entries = add(["A1", "A2", "A3", "B1", "B2", "C1", "C2", "C3"])
		model.setCurrent(entries[3])
		model.shuffleMode = .albums
		let order = playOrder(from: entries[3], count: 9).dropLast()
		XCTAssertEqual(Array(order.prefix(2)), ["B1", "B2"], "the playing album first")
		var albums: [Character] = []
		for name in order where albums.last != name.first { albums.append(name.first!) }
		XCTAssertEqual(albums.count, 3, "each album played whole: \(order)")
		for album in "ABC" {
			let tracks = order.filter { $0.first == album }
			XCTAssertEqual(tracks, tracks.sorted())
		}
	}

	func testShuffleWithRepeatAllGoesOn() {
		let entries = add(["A1", "A2", "A3"])
		model.repeatMode = .all
		model.setCurrent(entries[0])
		model.shuffleMode = .all
		let order = playOrder(from: entries[0], count: 9)
		XCTAssertFalse(order.contains("nil"))
		XCTAssertEqual(Set(order[3..<6]).count, 3, "the next round is all of them too: \(order)")
	}

	// MARK: - Entries

	func testMetadataIsStoredAsMacOSStoresIt() {
		let entry = PlaylistEntry(context: store.viewContext)
		entry.url = URL(fileURLWithPath: "/music/song.flac")
		entry.metadataBlob = NSDictionary()
		entry.setMetadata(["title": "Song", "artist": ["Someone"], "bitrate": NSNumber(value: 900), "sampleRate": NSNumber(value: 44100),
		                   "totalFrames": NSNumber(value: 441000), "replaygain_track_gain": "-6.5 dB", "tracknumber": "3/12"])
		XCTAssertEqual(entry.title, "Song")
		XCTAssertEqual(entry.display, "Someone - Song")
		XCTAssertEqual(entry.bitrate, 900)
		XCTAssertEqual(entry.length, 10, accuracy: 0.001)
		XCTAssertEqual(entry.replayGainTrackGain, -6.5)
		XCTAssertEqual(entry.track, 3)
		XCTAssertTrue(entry.metadataLoaded)
		XCTAssertNil((entry.metadataBlob as? NSDictionary)?["bitrate"], "properties go to their attributes")

		entry.setMetadata(nil)
		XCTAssertTrue(entry.error)
	}

	func testUntitledEntriesShowTheirFileName() {
		let entry = PlaylistEntry(context: store.viewContext)
		entry.url = URL(fileURLWithPath: "/music/some file.flac")
		XCTAssertEqual(entry.title, "some file.flac")
		XCTAssertEqual(entry.display, "some file.flac")
	}
}
