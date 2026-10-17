//
//  PlayCounts.swift
//  Cog (iOS)
//
//  How often each track has been played, in the store's PlayCount table,
//  as the macOS app keeps it (PlaylistEntry.m's playCountItem and
//  PlaylistController's updatePlayCountForTrack:): found by album, artist
//  and title, else by file name.
//

import CogPlaylist
import CoreData
import Foundation

@MainActor
enum PlayCounts {
	private static var context: NSManagedObjectContext {
		PlaylistController.sharedPersistentContainer().viewContext
	}

	/// The entry's row, if it has been played or seen.
	static func item(for entry: PlaylistEntry) -> NSManagedObject? {
		let byTags = NSCompoundPredicate(andPredicateWithSubpredicates: [
			NSPredicate(format: "album == %@", argumentArray: [entry.album as Any]),
			NSPredicate(format: "artist == %@", argumentArray: [entry.artist as Any]),
			NSPredicate(format: "title == %@", entry.title),
		])
		for predicate in [byTags, NSPredicate(format: "filename == %@", entry.filenameFragment), NSPredicate(format: "filename == %@", entry.filename)] {
			let request = NSFetchRequest<NSManagedObject>(entityName: "PlayCount")
			request.predicate = predicate
			request.fetchLimit = 1
			if let item = try? context.fetch(request).first {
				return item
			}
		}
		return nil
	}

	/// Counts a play of the entry, once for each time it is in the playlist
	/// and played through, as macOS does.
	static func record(_ entry: PlaylistEntry) {
		guard !entry.deLeted, !entry.countAdded else { return }
		entry.countAdded = true
		let now = Date()
		if let item = item(for: entry) {
			item.setValue((item.value(forKey: "count") as? Int64 ?? 0) + 1, forKey: "count")
			item.setValue(now, forKey: "lastPlayed")
			if item.value(forKey: "firstSeen") == nil {
				item.setValue(now, forKey: "firstSeen")
			}
		} else {
			let item = NSEntityDescription.insertNewObject(forEntityName: "PlayCount", into: context)
			item.setValue(Int64(1), forKey: "count")
			item.setValue(now, forKey: "firstSeen")
			item.setValue(now, forKey: "lastPlayed")
			item.setValue(entry.album, forKey: "album")
			item.setValue(entry.artist, forKey: "artist")
			item.setValue(entry.title, forKey: "title")
			item.setValue(entry.filenameFragment, forKey: "filename")
		}
		do {
			try context.save()
		} catch {
			NSLog("Could not save a play count: %@", error.localizedDescription)
		}
	}
}
