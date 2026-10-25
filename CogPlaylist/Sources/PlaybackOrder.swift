//
//  PlaybackOrder.swift
//  CogPlaylist
//
//  What plays next, for both apps: repeat, shuffle, the queue and stop-after,
//  within the playlist of the entry playing. The queue is one for all
//  playlists; once it empties, playback goes on in the playlist of the last
//  entry it played (or stops, with `stopWhenQueueEmpties`). The playlist on
//  screen only matters when nothing has played yet.
//
//  It works on entries and their stored order, not on a table, so a search
//  or a sorted view does not change what plays next. Callers save the store
//  and keep their own selection, through `save` and `onFollow`.
//

import CoreData
import Foundation

@objc public final class PlaybackOrder: NSObject {
	private let defaults: UserDefaults
	private let save: () -> Void

	/// Entries to play before the playlist goes on, from any playlist, in
	/// order; each one's `queuePosition` is its position.
	@objc public private(set) var queue: [PlaylistEntry] = []
	/// Where playback goes on from after the current entry was removed.
	@objc public var nextEntryAfterDeleted: PlaylistEntry?
	/// The entry playback moves to, for a selection that follows playback
	/// (`selectionFollowsPlayback`).
	@objc public var onFollow: ((PlaylistEntry?) -> Void)?

	/// The last entry the queue handed out: when it ends with the queue
	/// empty, `stopWhenQueueEmpties` stops playback.
	private var lastFromQueue: PlaylistEntry?
	/// The order shuffle plays in, for one playlist; each entry's
	/// `shuffleIndex` is its position. Extended at either end as repeat-all
	/// runs past it.
	private var shuffleList: [PlaylistEntry] = []
	private var shufflePlaylist: Playlist?

	@objc public init(defaults: UserDefaults = .standard, save: @escaping () -> Void) {
		self.defaults = defaults
		self.save = save
		super.init()
	}

	private var repeatMode: PlaylistRepeatMode {
		PlaylistRepeatMode(rawValue: defaults.integer(forKey: "repeat")) ?? .none
	}

	private var shuffleMode: PlaylistShuffleMode {
		PlaylistShuffleMode(rawValue: defaults.integer(forKey: "shuffle")) ?? .off
	}

	// MARK: - What plays next

	/// The entry at `position` in `playlist`; past either end, with repeat
	/// all, around again; otherwise nil.
	@objc(entryAtPosition:inPlaylist:)
	public func entry(at position: Int, in playlist: Playlist?) -> PlaylistEntry? {
		entry(at: position, of: playlist?.orderedEntries ?? [])
	}

	private func entry(at position: Int, of entries: [PlaylistEntry]) -> PlaylistEntry? {
		guard !entries.isEmpty else { return nil }
		var i = position
		if i < 0 || i >= entries.count {
			guard repeatMode == .all else { return nil }
			while i < 0 { i += entries.count }
			i %= entries.count
		}
		return entries[i]
	}

	/// The entry to play after `entry`: itself with repeat one, then the
	/// queue's first, then the shuffle order's or the playlist's next, with
	/// repeat album held to the album. nil ends playback, as does the
	/// `alwaysStopAfterCurrent` setting. `ignoreRepeatOne` is for skipping,
	/// which leaves even a repeated entry. With no entry, from the start of
	/// `shown`.
	@objc(nextEntryAfter:ignoreRepeatOne:startingIn:)
	public func nextEntry(after entry: PlaylistEntry?, ignoreRepeatOne: Bool = false, startingIn shown: Playlist? = nil) -> PlaylistEntry? {
		if !ignoreRepeatOne && defaults.bool(forKey: "alwaysStopAfterCurrent") {
			follow(nextEntry(after: entry, ignoreRepeatOne: true, startingIn: shown))
			return nil
		}

		if !ignoreRepeatOne && repeatMode == .one {
			follow(entry)
			return entry
		}

		if !queue.isEmpty {
			let next = queue.removeFirst()
			next.queued = false
			next.queuePosition = -1
			renumberQueue()
			lastFromQueue = next
			save()
			follow(next)
			return next
		}

		if !ignoreRepeatOne, let entry, entry == lastFromQueue, defaults.bool(forKey: "stopWhenQueueEmpties") {
			return nil
		}

		guard let entry else {
			let first = firstEntry(in: shown)
			follow(first)
			return first
		}
		// Its playlist was deleted.
		guard let playlist = entry.playlist, !entry.isDeleted else { return nil }

		if shuffleMode != .off {
			let next = shuffledEntry(at: Int(entry.shuffleIndex) + 1, in: playlist, current: entry)
			if next != nil { follow(next) }
			return next
		}

		let entries = playlist.orderedEntries
		var i: Int
		if entry.deLeted {
			// The current entry, removed: on from what followed it.
			i = nextEntryAfterDeleted.map { position(of: $0, in: entries) } ?? 0
			nextEntryAfterDeleted = nil
		} else {
			i = position(of: entry, in: entries) + 1
		}

		if repeatMode == .album {
			let next = self.entry(at: i, of: entries)
			let sameAlbum = next?.album.map { $0.caseInsensitiveCompare(entry.album ?? "") == .orderedSame } ?? false
			if i > entries.count - 1 || !sameAlbum {
				let album = albumEntries(entry.album, of: entries)
				if entry.album == nil || album.isEmpty {
					i -= 1
				} else {
					i = position(of: album[0], in: entries)
				}
			}
		}

		let next = self.entry(at: i, of: entries)
		follow(next)
		return next
	}

	/// Where playing `playlist` from nothing starts: its first entry, or
	/// shuffle's first. The queue is left be.
	@objc(firstEntryInPlaylist:) public func firstEntry(in playlist: Playlist?) -> PlaylistEntry? {
		shuffleMode != .off ? shuffledEntry(at: 0, in: playlist, current: nil) : entry(at: 0, in: playlist)
	}

	/// The entry to play before `entry`, as `nextEntry` goes forward (the
	/// queue aside).
	@objc(previousEntryBefore:ignoreRepeatOne:)
	public func previousEntry(before entry: PlaylistEntry?, ignoreRepeatOne: Bool = false) -> PlaylistEntry? {
		if !ignoreRepeatOne && defaults.bool(forKey: "alwaysStopAfterCurrent") {
			follow(previousEntry(before: entry, ignoreRepeatOne: true))
			return nil
		}

		if !ignoreRepeatOne && repeatMode == .one {
			follow(entry)
			return entry
		}

		guard let entry, let playlist = entry.playlist, !entry.isDeleted else { return nil }

		if shuffleMode != .off {
			let previous = shuffledEntry(at: Int(entry.shuffleIndex) - 1, in: playlist, current: entry)
			if previous != nil { follow(previous) }
			return previous
		}

		// A removed current entry's negative index remembers where it was.
		let entries = playlist.orderedEntries
		let i = entry.index < 0 ? Int(-entry.index - 2) : position(of: entry, in: entries) - 1
		let previous = self.entry(at: i, of: entries)
		follow(previous)
		return previous
	}

	/// Where `entry` is in `entries`: its index, unless a search on macOS
	/// has left the indexes behind for now.
	private func position(of entry: PlaylistEntry, in entries: [PlaylistEntry]) -> Int {
		let index = Int(entry.index)
		if index >= 0 && index < entries.count && entries[index] == entry { return index }
		return entries.firstIndex(of: entry) ?? index
	}

	private func follow(_ entry: PlaylistEntry?) {
		if defaults.bool(forKey: "selectionFollowsPlayback") {
			onFollow?(entry)
		}
	}

	// MARK: - Removing

	/// Notes that `removed`, all of one playlist, are about to go, before
	/// they are marked removed and the rest renumbered. The current entry
	/// may be among them while it plays: playback goes on from the entry
	/// after it, and it keeps its old position as a negative index
	/// (-index - 1), moved down past whatever else went before it, so the
	/// entry before it can still be found. Removed entries leave the queue.
	@objc(willRemoveEntries:current:)
	public func willRemove(_ removed: [PlaylistEntry], current: PlaylistEntry?) {
		guard let playlist = removed.first?.playlist else { return }
		let removedSet = Set(removed)
		dropFromQueue(removed)

		guard let current, current.playlist == playlist else { return }
		let entries = playlist.orderedEntries
		func firstKept(after last: PlaylistEntry) -> PlaylistEntry? {
			entries.first { $0.index > last.index && !removedSet.contains($0) }
		}
		if current.index >= 0 && removedSet.contains(current) {
			nextEntryAfterDeleted = firstKept(after: current)
			current.index = -current.index - 1
		} else if let next = nextEntryAfterDeleted, removedSet.contains(next) {
			nextEntryAfterDeleted = firstKept(after: next)
		}
		if current.index < 0 {
			let i = Int(-current.index - 1)
			let before = removed.filter { $0 != current && $0.index >= 0 && Int($0.index) < i }.count
			current.index = Int64(-(i - before) - 1)
		}
	}

	/// Lets go of a playlist about to be deleted: its entries leave the
	/// queue, and shuffle forgets its order.
	@objc(willDeletePlaylist:)
	public func willDelete(_ playlist: Playlist) {
		dropFromQueue(queue.filter { $0.playlist == playlist })
		if shufflePlaylist == playlist {
			shuffleList = []
			shufflePlaylist = nil
		}
		if nextEntryAfterDeleted?.playlist == playlist {
			nextEntryAfterDeleted = nil
		}
	}

	// MARK: - Queue

	/// Reads the queue from the store, across playlists.
	@objc(loadQueueInContext:) public func loadQueue(in context: NSManagedObjectContext) {
		let request = NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")
		request.predicate = NSPredicate(format: "queued == YES AND (deLeted == NO OR deLeted == nil) AND urlString != nil AND urlString != ''")
		request.sortDescriptors = [NSSortDescriptor(key: "queuePosition", ascending: true)]
		queue = (try? context.fetch(request)) ?? []
		renumberQueue()
	}

	@objc(toggleQueued:) public func toggleQueued(_ toggled: [PlaylistEntry]) {
		for entry in toggled {
			if entry.queued {
				queue.removeAll { $0 == entry }
				entry.queued = false
				entry.queuePosition = -1
			} else {
				entry.queued = true
				queue.append(entry)
			}
		}
		renumberQueue()
		save()
	}

	@objc(addToQueue:) public func addToQueue(_ added: [PlaylistEntry]) {
		for entry in added where !entry.queued {
			entry.queued = true
			queue.append(entry)
		}
		renumberQueue()
		save()
	}

	@objc(removeFromQueue:) public func removeFromQueue(_ removed: [PlaylistEntry]) {
		dropFromQueue(removed)
		save()
	}

	@objc public func emptyQueue() {
		dropFromQueue(queue)
		save()
	}

	private func dropFromQueue(_ dropped: [PlaylistEntry]) {
		let droppedSet = Set(dropped)
		guard queue.contains(where: droppedSet.contains) else { return }
		for entry in queue where droppedSet.contains(entry) {
			entry.queued = false
			entry.queuePosition = -1
		}
		queue.removeAll { droppedSet.contains($0) }
		renumberQueue()
	}

	private func renumberQueue() {
		for (position, entry) in queue.enumerated() where entry.queuePosition != Int64(position) {
			entry.queuePosition = Int64(position)
		}
	}

	// MARK: - Shuffle

	/// The entries of an album, by exact name ("" and none alike), as
	/// filterPlaylistOnAlbum: has it.
	private func albumEntries(_ album: String?, of entries: [PlaylistEntry]) -> [PlaylistEntry] {
		if let album, !album.isEmpty {
			return entries.filter { $0.album == album }
		}
		return entries.filter { ($0.album ?? "").isEmpty }
	}

	/// `playlist`'s shuffle order at `position`, read back from the store
	/// when the order was last another playlist's.
	private func shuffledEntry(at position: Int, in playlist: Playlist?, current: PlaylistEntry?) -> PlaylistEntry? {
		guard let playlist else { return nil }
		let entries = playlist.orderedEntries
		if shufflePlaylist != playlist {
			let stored = entries.sorted { $0.shuffleIndex < $1.shuffleIndex }
			if Set(stored.map(\.shuffleIndex)).count == stored.count {
				shuffleList = stored
				shufflePlaylist = playlist
			} else {
				// Never shuffled: from the entry asked about.
				resetShuffle(in: playlist, current: current)
			}
		}
		var i = position
		while i < 0 {
			guard repeatMode == .all, !entries.isEmpty else { return nil }
			addShuffledList(of: entries, atFront: true)
			i += entries.count
		}
		while i >= shuffleList.count {
			guard repeatMode == .all, !entries.isEmpty else { return nil }
			addShuffledList(of: entries, atFront: false)
		}
		return shuffleList[i]
	}

	/// A shuffled order: albums whole (each in disc and track order), or
	/// every entry.
	private func shuffledOrder(of entries: [PlaylistEntry]) -> [PlaylistEntry] {
		guard shuffleMode == .albums else { return entries.shuffled() }
		var seen = Set<String>()
		var albums: [String] = []
		for entry in entries {
			let album = entry.album ?? ""
			if seen.insert(album).inserted { albums.append(album) }
		}
		return albums.shuffled().flatMap { album in
			albumEntries(album, of: entries).sorted { ($0.disc, $0.track) < ($1.disc, $1.track) }
		}
	}

	private func addShuffledList(of entries: [PlaylistEntry], atFront: Bool) {
		let order = shuffledOrder(of: entries)
		if atFront {
			shuffleList.insert(contentsOf: order, at: 0)
			renumberShuffle(from: 0)
		} else {
			let start = shuffleList.count
			shuffleList.append(contentsOf: order)
			renumberShuffle(from: start)
		}
		save()
	}

	private func renumberShuffle(from start: Int = 0) {
		for i in start..<shuffleList.count {
			shuffleList[i].shuffleIndex = Int64(i)
		}
	}

	/// A new shuffle order for `playlist`, starting with `current` when it
	/// is that playlist's: its whole album when shuffling albums. For when
	/// shuffle is turned on, or the playlist changed.
	@objc(resetShuffleInPlaylist:current:)
	public func resetShuffle(in playlist: Playlist?, current: PlaylistEntry?) {
		guard let playlist else { return }
		let entries = playlist.orderedEntries
		shufflePlaylist = playlist
		shuffleList = []
		addShuffledList(of: entries, atFront: true)
		guard let current, current.playlist == playlist, current.index >= 0, !current.deLeted else { return }
		if shuffleMode == .albums {
			let album = albumEntries(current.album, of: entries)
			shuffleList.removeAll { album.contains($0) }
			shuffleList.insert(contentsOf: album, at: 0)
		} else {
			shuffleList.removeAll { $0 == current }
			shuffleList.insert(current, at: 0)
		}
		renumberShuffle()
		save()
	}
}
