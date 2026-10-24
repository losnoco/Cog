//
//  Playlist+Extension.swift
//  CogPlaylist
//
//  The playlists entries belong to: each has a stable identifier, a name of
//  its own and a place in the playlists' order. One plays (the current
//  entry's), and any one may be shown. Both apps work on these through their
//  own contexts, so everything here takes one.
//

import CoreData
import Foundation

extension Playlist {
	/// What a new playlist is called, numbered from the second on.
	@objc public static var defaultName: String {
		Bundle.main.localizedString(forKey: "Playlist", value: "Playlist", table: nil)
	}

	/// The playlists, in their order.
	@objc(playlistsInContext:) public static func all(in context: NSManagedObjectContext) -> [Playlist] {
		let request = NSFetchRequest<Playlist>(entityName: "Playlist")
		request.sortDescriptors = [NSSortDescriptor(key: "order", ascending: true)]
		return (try? context.fetch(request)) ?? []
	}

	/// The playlist with `uuid`, if there still is one.
	@objc(playlistWithUUID:inContext:) public static func with(uuid: UUID, in context: NSManagedObjectContext) -> Playlist? {
		let request = NSFetchRequest<Playlist>(entityName: "Playlist")
		request.predicate = NSPredicate(format: "uuid == %@", uuid as CVarArg)
		request.fetchLimit = 1
		return try? context.fetch(request).first
	}

	/// A new playlist named `name` (made unique), placed at `position` in the
	/// playlists' order, the end by default.
	@objc(insertPlaylistNamed:atPosition:inContext:)
	public static func insert(named name: String?, at position: Int, in context: NSManagedObjectContext) -> Playlist {
		var playlists = all(in: context)
		let playlist = Playlist(context: context)
		playlist.uuid = UUID()
		playlist.name = uniqueName(name ?? defaultName, in: context)
		playlists.insert(playlist, at: position < 0 ? playlists.count : min(position, playlists.count))
		renumber(playlists)
		return playlist
	}

	/// `name` if no other playlist has it (ignoring case), or else with the
	/// first number after it that none has: "Playlist 2", "Playlist 3".
	@objc(uniqueName:inContext:excluding:)
	public static func uniqueName(_ name: String, in context: NSManagedObjectContext, excluding playlist: Playlist? = nil) -> String {
		let base = name.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty ? defaultName : name.trimmingCharacters(in: .whitespacesAndNewlines)
		let taken = Set(all(in: context).filter { $0 != playlist }.compactMap { $0.name?.lowercased() })
		guard taken.contains(base.lowercased()) else { return base }
		var number = 2
		while taken.contains("\(base) \(number)".lowercased()) {
			number += 1
		}
		return "\(base) \(number)"
	}

	/// Each playlist's `order` to its position in `playlists`.
	@objc public static func renumber(_ playlists: [Playlist]) {
		for (position, playlist) in playlists.enumerated() where playlist.order != Int64(position) {
			playlist.order = Int64(position)
		}
	}

	/// Readies a store for playlists: there is always one, and entries from
	/// before there were any (or that lost theirs) go into the first, in the
	/// order they had. Cheap when there is nothing to do.
	@objc(adoptOrphansInContext:) public static func adoptOrphans(in context: NSManagedObjectContext) {
		var playlists = all(in: context)
		if playlists.isEmpty {
			playlists = [insert(named: nil, at: 0, in: context)]
		}
		let request = NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")
		request.predicate = NSPredicate(format: "playlist == nil")
		request.sortDescriptors = [NSSortDescriptor(key: "index", ascending: true)]
		guard let orphans = try? context.fetch(request), !orphans.isEmpty else { return }
		playlists[0].append(orphans)
	}

	// MARK: - Entries

	/// The entries in playlist order, without removed ones. Each one's
	/// `index` is its position, as both apps keep it.
	@objc public var orderedEntries: [PlaylistEntry] {
		guard let context = managedObjectContext else { return [] }
		let request = NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")
		request.predicate = NSPredicate(format: "playlist == %@ AND (deLeted == NO OR deLeted == nil)", self)
		request.sortDescriptors = [NSSortDescriptor(key: "index", ascending: true)]
		return (try? context.fetch(request)) ?? []
	}

	/// Adds `added` after the playlist's entries, taking them from any other.
	@objc(appendEntries:) public func append(_ added: [PlaylistEntry]) {
		var position = Int64(orderedEntries.count)
		for entry in added {
			entry.playlist = self
			entry.index = position
			position += 1
		}
	}
}

extension PlaylistEntry {
	/// Attributes a copy starts afresh on: where the entry is, and its
	/// playback state.
	private static let uncopiedAttributes: Set<String> = [
		"index", "shuffleIndex", "current", "currentPosition", "countAdded", "stopAfter",
		"queued", "queuePosition", "deLeted", "removed", "dbIndex", "entryId",
	]

	/// A new entry with this one's file, tags and properties, in no
	/// playlist yet. Art is shared by hash, so the copy has it too.
	@objc public func copyForPlaylist() -> PlaylistEntry {
		let copy = PlaylistEntry(context: managedObjectContext!)
		for name in entity.attributesByName.keys where !Self.uncopiedAttributes.contains(name) {
			copy.setValue(value(forKey: name), forKey: name)
		}
		copy.queuePosition = -1
		return copy
	}
}
