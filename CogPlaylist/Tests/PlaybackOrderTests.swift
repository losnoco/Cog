//
//  PlaybackOrderTests.swift
//  CogPlaylistTests
//
//  What plays next with more than one playlist: the playing one, not the
//  shown one; the queue across them; and deleting what plays.
//

@testable import CogPlaylist
import CoreData
import XCTest

@MainActor
final class PlaybackOrderTests: XCTestCase {
	private var store: PlaylistStore!
	private var defaults: UserDefaults!
	private var model: PlaylistModel!

	override func setUp() async throws {
		store = try PlaylistStore(inMemory: true)
		defaults = UserDefaults(suiteName: "PlaybackOrderTests-\(UUID().uuidString)")
		defaults.set(PlaylistRepeatMode.none.rawValue, forKey: "repeat")
		model = PlaylistModel(store: store, defaults: defaults)
	}

	/// Entries named "A1", "A2", "B1", ... into the shown playlist.
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

	/// Two playlists: "First" with A1-A3 (shown first), and "Second" with
	/// B1-B3, which ends up shown.
	private func twoPlaylists() -> (a: [PlaylistEntry], b: [PlaylistEntry]) {
		let a = add(["A1", "A2", "A3"])
		model.rename(model.shownPlaylist!, to: "First")
		model.newPlaylist(named: "Second")
		let b = add(["B1", "B2", "B3"])
		return (a, b)
	}

	func testPlaybackStaysInThePlayingPlaylist() {
		let (a, b) = twoPlaylists()
		model.setCurrent(a[0])
		XCTAssertEqual(model.shownPlaylist?.name, "Second")
		XCTAssertEqual(model.playingPlaylist?.name, "First")
		XCTAssertEqual(model.nextEntry(after: a[0]), a[1])
		XCTAssertEqual(model.previousEntry(before: a[1]), a[0])
		XCTAssertEqual(model.playingEntries, a)
		XCTAssertEqual(model.entries, b)
		XCTAssertEqual(a.map(\.index), [0, 1, 2], "each playlist numbers its own")
		XCTAssertEqual(b.map(\.index), [0, 1, 2])
	}

	func testWithNothingPlayingTheShownPlaylistStarts() {
		let (_, b) = twoPlaylists()
		XCTAssertEqual(model.nextEntry(after: nil), b[0])
	}

	func testTheQueueTakesFromAnyPlaylistAndPlaybackGoesOnInTheLastQueued() {
		let (a, b) = twoPlaylists()
		model.setCurrent(a[0])
		model.addToQueue([b[1]])
		let queued = model.nextEntry(after: a[0])
		XCTAssertEqual(queued, b[1])
		model.setCurrent(queued)
		XCTAssertEqual(model.playingPlaylist?.name, "Second")
		XCTAssertEqual(model.nextEntry(after: b[1]), b[2], "on in the queued entry's playlist")
	}

	func testStopWhenTheQueueEmpties() {
		let (a, b) = twoPlaylists()
		defaults.set(true, forKey: "stopWhenQueueEmpties")
		model.addToQueue([b[1]])
		XCTAssertEqual(model.nextEntry(after: a[0]), b[1])
		XCTAssertNil(model.nextEntry(after: b[1]), "ends after the last queued entry")
		XCTAssertEqual(model.nextEntry(after: b[1], ignoreRepeatOne: true), b[2], "skipping still moves")
		XCTAssertEqual(model.nextEntry(after: b[2]), nil, "the end of the playlist")
		defaults.set(false, forKey: "stopWhenQueueEmpties")
		model.addToQueue([a[1]])
		XCTAssertEqual(model.nextEntry(after: b[0]), a[1])
		XCTAssertEqual(model.nextEntry(after: a[1]), a[2])
	}

	func testRemovingThePlayingEntryGoesOnInItsPlaylistWhileAnotherIsShown() {
		let a = add(["A1", "A2", "A3", "A4"])
		model.setCurrent(a[1])
		model.remove(at: IndexSet([1, 2]))
		XCTAssertEqual(a[1].index, -2, "keeps its old place")
		model.newPlaylist(named: "Second")
		add(["B1"])
		XCTAssertEqual(model.nextEntry(after: a[1]), a[3])
		XCTAssertEqual(model.previousEntry(before: a[1]), a[0])
	}

	func testDeletingThePlayingPlaylistEndsPlayback() {
		let (a, b) = twoPlaylists()
		var changed = false
		model.onPlaylistChange = { changed = true }
		let first = model.playlists[0]
		model.setCurrent(a[0])
		model.addToQueue([a[2], b[0]])
		model.delete(first)
		XCTAssertNil(model.currentEntry)
		XCTAssertTrue(changed)
		XCTAssertEqual(model.queue, [b[0]], "its entries leave the queue")
		XCTAssertEqual(model.playlists.map(\.name), ["Second"])
		model.delete(model.playlists[0])
		XCTAssertEqual(model.playlists.count, 1, "the last one stays")
	}

	func testDeletingTheShownPlaylistShowsTheNextOne() {
		_ = twoPlaylists()
		model.newPlaylist(named: "Third")
		model.show(model.playlists[1])
		model.delete(model.playlists[1])
		XCTAssertEqual(model.shownPlaylist?.name, "Third")
		model.delete(model.playlists[1])
		XCTAssertEqual(model.shownPlaylist?.name, "First", "the last one's neighbor before it")
	}

	func testShuffleStaysInThePlayingPlaylist() {
		let (a, _) = twoPlaylists()
		model.setCurrent(a[0])
		model.shuffleMode = .all
		var played = [a[0]]
		var entry: PlaylistEntry? = a[0]
		while let next = model.nextEntry(after: entry) {
			played.append(next)
			entry = next
		}
		XCTAssertEqual(Set(names(played)), ["A1", "A2", "A3"])
		XCTAssertEqual(played.count, 3)
	}

	func testCopiesGoToTheEndOfAnotherPlaylist() {
		let (a, _) = twoPlaylists()
		let first = model.playlists[0]
		model.copy([a[0]], to: model.shownPlaylist!)
		XCTAssertEqual(names(model.entries), ["B1", "B2", "B3", "A1"])
		XCTAssertEqual(first.orderedEntries, a, "the original stays")
	}

	func testPlaylistsAreReorderedAndTheShownOneRemembered() {
		_ = twoPlaylists()
		model.newPlaylist(named: "Third")
		model.movePlaylists(fromOffsets: IndexSet(integer: 2), toOffset: 0)
		XCTAssertEqual(model.playlists.map(\.name), ["Third", "First", "Second"])
		model.show(model.playlists[2])
		let reopened = PlaylistModel(store: store, defaults: defaults)
		XCTAssertEqual(reopened.playlists.map(\.name), ["Third", "First", "Second"])
		XCTAssertEqual(reopened.shownPlaylist?.name, "Second")
		XCTAssertEqual(names(reopened.entries), ["B1", "B2", "B3"])
	}
}
