//
//  PlaylistStoreTests.swift
//  CogPlaylistTests
//

@testable import CogPlaylist
import CoreData
import XCTest

final class PlaylistStoreTests: XCTestCase {
	func testStoreOpensOnTheModel() throws {
		let store = try PlaylistStore(inMemory: true)
		XCTAssertTrue(store.container.managedObjectModel === PlaylistStore.model)
		XCTAssertEqual(Set(PlaylistStore.model.entitiesByName.keys),
		               ["AlbumArtwork", "LyricsCache", "PendingListen", "PlayCount", "PlaylistEntry", "SandboxToken"])
	}

	/// The model's entities are this framework's classes, which the macOS app
	/// and its category extend.
	func testEntitiesAreThisFrameworksClasses() throws {
		let store = try PlaylistStore(inMemory: true)
		let entry = NSEntityDescription.insertNewObject(forEntityName: "PlaylistEntry", into: store.viewContext)
		XCTAssertTrue(entry is PlaylistEntry)
		XCTAssertTrue(Bundle(for: type(of: entry)) == Bundle(for: PlaylistStore.self))
	}

	func testMetadataBlobRoundTripsThroughTheTransformer() throws {
		let url = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString + ".sqlite")
		defer { try? FileManager.default.removeItem(at: url) }
		do {
			let store = try PlaylistStore(url: url)
			let entry = PlaylistEntry(context: store.viewContext)
			entry.metadataBlob = ["artist": ["A"]] as NSDictionary
			store.save()
		}
		let reopened = try PlaylistStore(url: url)
		let entries = try reopened.viewContext.fetch(NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry"))
		XCTAssertEqual((entries.first?.metadataBlob as? NSDictionary)?["artist"] as? [String], ["A"])
	}
}
