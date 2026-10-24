//
//  PlaylistTests.swift
//  CogPlaylistTests
//
//  Playlists: their names and order, copying entries between them, and
//  stores from before there were any.
//

@testable import CogPlaylist
import CoreData
import XCTest

@MainActor
final class PlaylistTests: XCTestCase {
	private var store: PlaylistStore!
	private var context: NSManagedObjectContext { store.viewContext }

	override func setUp() async throws {
		store = try PlaylistStore(inMemory: true)
	}

	private func entry(_ name: String) -> PlaylistEntry {
		let entry = PlaylistEntry(context: context)
		entry.url = URL(fileURLWithPath: "/music/\(name).flac")
		entry.metadataBlob = ["title": [name]] as NSDictionary
		return entry
	}

	// MARK: - Names and order

	func testThereIsAlwaysAPlaylist() {
		Playlist.adoptOrphans(in: context)
		let playlists = Playlist.all(in: context)
		XCTAssertEqual(playlists.map(\.name), ["Playlist"])
		XCTAssertNotNil(playlists[0].uuid)
		Playlist.adoptOrphans(in: context)
		XCTAssertEqual(Playlist.all(in: context).count, 1, "only when there is none")
	}

	func testNamesAreUniqueIgnoringCase() {
		let first = Playlist.insert(named: "Chiptunes", at: -1, in: context)
		let second = Playlist.insert(named: "chiptunes", at: -1, in: context)
		let third = Playlist.insert(named: "Chiptunes", at: -1, in: context)
		XCTAssertEqual([first, second, third].map(\.name), ["Chiptunes", "chiptunes 2", "Chiptunes 3"])
		XCTAssertEqual(Playlist.uniqueName("Chiptunes", in: context, excluding: first), "Chiptunes",
		               "renaming keeps its own name")
		XCTAssertEqual(Playlist.uniqueName("  ", in: context), "Playlist")
	}

	func testInsertingKeepsTheOrderAndIdentity() {
		let a = Playlist.insert(named: "A", at: -1, in: context)
		let b = Playlist.insert(named: "B", at: -1, in: context)
		let c = Playlist.insert(named: "C", at: 0, in: context)
		XCTAssertEqual(Playlist.all(in: context).map(\.name), ["C", "A", "B"])
		let uuid = b.uuid!
		Playlist.renumber([b, c, a])
		XCTAssertEqual(Playlist.all(in: context).map(\.name), ["B", "C", "A"])
		XCTAssertEqual(Playlist.with(uuid: uuid, in: context), b)
	}

	// MARK: - Entries

	func testOrderedEntriesAreThePlaylistsAlone() {
		let a = Playlist.insert(named: "A", at: -1, in: context)
		let b = Playlist.insert(named: "B", at: -1, in: context)
		a.append([entry("A1"), entry("A2")])
		b.append([entry("B1")])
		let removed = entry("A3")
		a.append([removed])
		removed.deLeted = true
		XCTAssertEqual(a.orderedEntries.map(\.title), ["A1", "A2"])
		XCTAssertEqual(a.orderedEntries.map(\.index), [0, 1])
		XCTAssertEqual(b.orderedEntries.map(\.title), ["B1"])
	}

	func testCopiesKeepTheFileAndTagsButNotThePlaybackState() {
		let a = Playlist.insert(named: "A", at: -1, in: context)
		let b = Playlist.insert(named: "B", at: -1, in: context)
		let original = entry("A1")
		original.artHash = "hash"
		original.queued = true
		original.queuePosition = 0
		original.current = true
		original.stopAfter = true
		a.append([original])

		let copy = original.copyForPlaylist()
		b.append([copy])
		XCTAssertEqual(copy.urlString, original.urlString)
		XCTAssertEqual(copy.title, "A1")
		XCTAssertEqual(copy.artHash, "hash")
		XCTAssertFalse(copy.queued)
		XCTAssertEqual(copy.queuePosition, -1)
		XCTAssertFalse(copy.current)
		XCTAssertFalse(copy.stopAfter)
		XCTAssertEqual(copy.playlist, b)
		XCTAssertEqual(original.playlist, a)
	}

	// MARK: - Stores from before playlists

	/// A store written by the first model, which had no playlists, opens on
	/// the current one with every entry in one playlist, as it was.
	func testAStoreWithoutPlaylistsMovesIntoOne() throws {
		let url = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString + ".sqlite")
		defer {
			for suffix in ["", "-shm", "-wal"] {
				try? FileManager.default.removeItem(at: URL(fileURLWithPath: url.path + suffix))
			}
		}

		let bundle = Bundle(for: PlaylistStore.self)
		let firstModelURL = try XCTUnwrap(bundle.url(forResource: "DataModel", withExtension: "mom", subdirectory: "DataModel.momd"))
		let firstModel = try XCTUnwrap(NSManagedObjectModel(contentsOf: firstModelURL))
		XCTAssertNil(firstModel.entitiesByName["Playlist"])
		do {
			PlaylistStore.registerTransformers()
			let container = NSPersistentContainer(name: "DataModel", managedObjectModel: firstModel)
			container.persistentStoreDescriptions = [NSPersistentStoreDescription(url: url)]
			var loadError: Error?
			container.loadPersistentStores { _, error in loadError = error }
			if let loadError { throw loadError }
			for (index, name) in ["A1", "A2", "B1"].enumerated() {
				let entry = NSEntityDescription.insertNewObject(forEntityName: "PlaylistEntry", into: container.viewContext)
				entry.setValue(URL(fileURLWithPath: "/music/\(name).flac").absoluteString, forKey: "urlString")
				entry.setValue(["title": [name]] as NSDictionary, forKey: "metadataBlob")
				entry.setValue(Int64(2 - index), forKey: "index")
				entry.setValue(name == "A2", forKey: "current")
				entry.setValue(name == "B1", forKey: "queued")
				entry.setValue(Int64(name == "B1" ? 0 : -1), forKey: "queuePosition")
			}
			try container.viewContext.save()
			for store in container.persistentStoreCoordinator.persistentStores {
				try container.persistentStoreCoordinator.remove(store)
			}
		}

		let reopened = try PlaylistStore(url: url)
		Playlist.adoptOrphans(in: reopened.viewContext)
		let playlists = Playlist.all(in: reopened.viewContext)
		XCTAssertEqual(playlists.map(\.name), ["Playlist"])
		let entries = playlists[0].orderedEntries
		XCTAssertEqual(entries.map(\.title), ["B1", "A2", "A1"], "in the order they had")
		XCTAssertEqual(entries.map(\.index), [0, 1, 2])
		XCTAssertEqual(entries.filter(\.current).map(\.title), ["A2"])
		XCTAssertEqual(entries.filter(\.queued).map(\.title), ["B1"])
	}
}
