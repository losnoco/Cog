//
//  PlaylistModel.swift
//  CogPlaylist
//
//  The playlists without their table: the one shown and its entries in
//  order, the entry playing (in whichever playlist), the queue, shuffle,
//  repeat and stop-after. What plays next is PlaybackOrder's, which the
//  macOS app's PlaylistController uses too; the AppKit parts (the table,
//  sorting by column, drag and drop, undo, trash) stay there.
//

import Combine
import CoreData
import Foundation

/// As PlaylistControllerEnums.h has them, and stored under the same
/// defaults keys ("repeat", "shuffle"), which decoders read too.
@objc public enum PlaylistRepeatMode: Int {
	case none = 0
	case one
	case album
	case all
}

@objc public enum PlaylistShuffleMode: Int {
	case off = 0
	case albums
	case all
}

@MainActor
public final class PlaylistModel: ObservableObject {
	public let store: PlaylistStore
	private let defaults: UserDefaults
	public let order: PlaybackOrder

	/// Every playlist, in their order.
	@Published public private(set) var playlists: [Playlist] = []
	/// The playlist on screen, which is not necessarily the one playing.
	@Published public private(set) var shownPlaylist: Playlist?
	/// The shown playlist's entries in order; each one's `index` is its
	/// position.
	@Published public private(set) var entries: [PlaylistEntry] = []
	/// The entry playing (or last played), in any playlist. A removed one
	/// stays current, with a negative index, until playback moves on.
	@Published public private(set) var currentEntry: PlaylistEntry?

	/// Entries to play before the playlist goes on, from any playlist, in
	/// order.
	public var queue: [PlaylistEntry] { order.queue }
	/// The playlist playback goes on in: the current entry's.
	public var playingPlaylist: Playlist? { currentEntry?.playlist }
	/// The playing playlist's entries in order, or the shown one's when
	/// nothing has played.
	public var playingEntries: [PlaylistEntry] {
		guard let playing = playingPlaylist, playing != shownPlaylist else { return entries }
		return playing.orderedEntries
	}

	/// Shuffle or repeat changed, or entries were removed: what plays next
	/// may have changed (PlaybackController's playlistDidChange:).
	public var onPlaylistChange: (() -> Void)?
	/// The entry playback moves to, for a selection that follows playback
	/// (`selectionFollowsPlayback`).
	public var onFollowPlayback: ((PlaylistEntry?) -> Void)? {
		didSet { order.onFollow = onFollowPlayback }
	}

	public init(store: PlaylistStore, defaults: UserDefaults = .standard) {
		self.store = store
		self.defaults = defaults
		order = PlaybackOrder(defaults: defaults, save: { [weak store] in store?.save() })
		defaults.register(defaults: ["repeat": PlaylistRepeatMode.all.rawValue, "shuffle": PlaylistShuffleMode.off.rawValue])
		load()
	}

	private var context: NSManagedObjectContext { store.viewContext }

	// MARK: - Loading

	/// Reads the playlists, dropping entries that were removed or have no
	/// URL, as the macOS app does on launch, and shows the playlist shown
	/// last.
	public func load() {
		Playlist.adoptOrphans(in: context)
		let request = NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")
		var loaded = (try? context.fetch(request)) ?? []
		loaded.removeAll { entry in
			guard entry.deLeted || (entry.urlString ?? "").isEmpty else { return false }
			context.delete(entry)
			return true
		}
		// Art no entry of any playlist names any more, as macOS drops it on
		// quitting.
		let named = Set(loaded.compactMap(\.artHash))
		let artwork = (try? context.fetch(NSFetchRequest<AlbumArtwork>(entityName: "AlbumArtwork"))) ?? []
		for art in artwork where !named.contains(art.artHash ?? "") {
			context.delete(art)
		}
		// The art store took in every picture as the store opened; it must not
		// hand out the ones just dropped.
		try? ArtworkStore.shared?.loadAll()
		playlists = Playlist.all(in: context)
		currentEntry = loaded.first { $0.current }
		let saved = defaults.string(forKey: "shownPlaylist").flatMap(UUID.init(uuidString:))
		show(playlists.first { $0.uuid == saved } ?? currentEntry?.playlist ?? playlists.first)
		order.loadQueue(in: context)
		store.save()
	}

	// MARK: - Playlists

	/// Puts `playlist` on screen; playback goes on where it was.
	public func show(_ playlist: Playlist?) {
		shownPlaylist = playlist
		entries = playlist?.orderedEntries ?? []
		updateIndexes()
		if let uuid = playlist?.uuid {
			defaults.set(uuid.uuidString, forKey: "shownPlaylist")
		}
	}

	/// A new, empty playlist after the others, shown.
	@discardableResult
	public func newPlaylist(named name: String? = nil, with added: [PlaylistEntry] = []) -> Playlist {
		let playlist = Playlist.insert(named: name, at: -1, in: context)
		playlist.append(added.map { $0.copyForPlaylist() })
		playlists = Playlist.all(in: context)
		store.save()
		show(playlist)
		return playlist
	}

	/// Renames `playlist`, keeping names unique; returns the name it got.
	@discardableResult
	public func rename(_ playlist: Playlist, to name: String) -> String {
		let unique = Playlist.uniqueName(name, in: context, excluding: playlist)
		playlist.name = unique
		objectWillChange.send()
		store.save()
		return unique
	}

	/// Deletes `playlist` and its entries, unless it is the last one. The
	/// one playing stops playback (through `onPlaylistChange`, as nothing
	/// plays next).
	public func delete(_ playlist: Playlist) {
		guard playlists.count > 1, let position = playlists.firstIndex(of: playlist) else { return }
		order.willDelete(playlist)
		let wasPlaying = playlist == playingPlaylist
		if wasPlaying { currentEntry = nil }
		context.delete(playlist)
		playlists.remove(at: position)
		Playlist.renumber(playlists)
		store.save()
		if playlist == shownPlaylist {
			show(playlists[min(position, playlists.count - 1)])
		}
		if wasPlaying { onPlaylistChange?() }
	}

	public func movePlaylists(fromOffsets source: IndexSet, toOffset destination: Int) {
		var reordered = playlists
		reordered.move(fromOffsets: source, toOffset: destination)
		Playlist.renumber(reordered)
		playlists = reordered
		store.save()
	}

	/// Copies `copied` to the end of `playlist`; the originals stay where
	/// they are.
	public func copy(_ copied: [PlaylistEntry], to playlist: Playlist) {
		let copies = copied.map { $0.copyForPlaylist() }
		if playlist == shownPlaylist {
			insert(copies)
		} else {
			playlist.append(copies)
			store.save()
			if playlist == playingPlaylist { onPlaylistChange?() }
		}
	}

	// MARK: - Modes

	public var repeatMode: PlaylistRepeatMode {
		get { PlaylistRepeatMode(rawValue: defaults.integer(forKey: "repeat")) ?? .none }
		set {
			defaults.set(newValue.rawValue, forKey: "repeat")
			objectWillChange.send()
			onPlaylistChange?()
		}
	}

	public var shuffleMode: PlaylistShuffleMode {
		get { PlaylistShuffleMode(rawValue: defaults.integer(forKey: "shuffle")) ?? .off }
		set {
			defaults.set(newValue.rawValue, forKey: "shuffle")
			objectWillChange.send()
			if newValue != .off { order.resetShuffle(in: playingPlaylist ?? shownPlaylist, current: currentEntry) }
			onPlaylistChange?()
		}
	}

	/// Off, albums, all, off.
	public func toggleShuffle() {
		switch shuffleMode {
		case .off: shuffleMode = .albums
		case .albums: shuffleMode = .all
		case .all: shuffleMode = .off
		}
	}

	/// None, one, album, all, none.
	public func toggleRepeat() {
		switch repeatMode {
		case .none: repeatMode = .one
		case .one: repeatMode = .album
		case .album: repeatMode = .all
		case .all: repeatMode = .none
		}
	}

	// MARK: - Editing the shown playlist

	/// Adds entries at `position` (the end by default) of the shown playlist.
	public func insert(_ newEntries: [PlaylistEntry], at position: Int? = nil) {
		for entry in newEntries {
			entry.deLeted = false
			entry.playlist = shownPlaylist
		}
		let at = min(max(position ?? entries.count, 0), entries.count)
		entries.insert(contentsOf: newEntries, at: at)
		updateIndexes()
		store.save()
		shownPlaylistChanged()
	}

	/// Removes the entries at `offsets`. Removed entries are only marked so
	/// (and dropped on the next load), as on macOS. The current entry may be
	/// removed while playing: playback goes on from the entry after it.
	public func remove(at offsets: IndexSet) {
		let removed = offsets.filter { $0 < entries.count }.map { entries[$0] }
		guard !removed.isEmpty else { return }
		order.willRemove(removed, current: currentEntry)
		for entry in removed {
			entry.deLeted = true
		}
		let removedSet = Set(removed)
		entries.removeAll { removedSet.contains($0) }
		updateIndexes()
		store.save()
		objectWillChange.send()
		shownPlaylistChanged()
		onPlaylistChange?()
	}

	public func removeAll() {
		remove(at: IndexSet(integersIn: 0..<entries.count))
	}

	public func move(fromOffsets source: IndexSet, toOffset destination: Int) {
		entries.move(fromOffsets: source, toOffset: destination)
		updateIndexes()
		store.save()
		if shownPlaylist == playingPlaylist { onPlaylistChange?() }
	}

	/// Puts the playlist in the order `descriptors` give (the macOS table's
	/// column sorting), which becomes its order.
	public func sort(using descriptors: [NSSortDescriptor]) {
		entries = (entries as NSArray).sortedArray(using: descriptors) as? [PlaylistEntry] ?? entries
		updateIndexes()
		store.save()
		shownPlaylistChanged()
	}

	/// Puts the playlist in the order `areInIncreasingOrder` gives, which
	/// becomes its order (for keys KVC cannot reach, as the tags are).
	public func sort(by areInIncreasingOrder: (PlaylistEntry, PlaylistEntry) -> Bool) {
		entries.sort(by: areInIncreasingOrder)
		updateIndexes()
		store.save()
		shownPlaylistChanged()
	}

	/// Puts the playlist in a random order (not shuffle, which leaves it be).
	public func randomize() {
		entries.shuffle()
		updateIndexes()
		store.save()
		shownPlaylistChanged()
	}

	/// Each entry's `index` to its position.
	private func updateIndexes() {
		for (position, entry) in entries.enumerated() where entry.index != Int64(position) {
			entry.index = Int64(position)
		}
	}

	/// The shown playlist's entries or order changed: a new shuffle order
	/// for it, and if it plays, what plays next may have changed.
	private func shownPlaylistChanged() {
		if shuffleMode != .off { order.resetShuffle(in: shownPlaylist, current: currentEntry) }
		if shownPlaylist == playingPlaylist { onPlaylistChange?() }
	}

	// MARK: - Queue and stop after

	public func toggleQueued(_ toggled: [PlaylistEntry]) {
		objectWillChange.send()
		order.toggleQueued(toggled)
	}

	public func addToQueue(_ added: [PlaylistEntry]) {
		objectWillChange.send()
		order.addToQueue(added)
	}

	public func removeFromQueue(_ removed: [PlaylistEntry]) {
		objectWillChange.send()
		order.removeFromQueue(removed)
	}

	public func emptyQueue() {
		objectWillChange.send()
		order.emptyQueue()
	}

	/// Stops playback once the current entry ends (again: no longer).
	public func toggleStopAfterCurrent() {
		guard let currentEntry else { return }
		currentEntry.stopAfter.toggle()
		objectWillChange.send()
		store.save()
	}

	public func toggleStopAfter(_ toggled: [PlaylistEntry]) {
		for entry in toggled {
			entry.stopAfter.toggle()
		}
		objectWillChange.send()
		store.save()
	}

	// MARK: - What plays next

	/// The entry to play after `entry` (see PlaybackOrder); with none, the
	/// shown playlist's first.
	public func nextEntry(after entry: PlaylistEntry?, ignoreRepeatOne: Bool = false) -> PlaylistEntry? {
		objectWillChange.send()
		return order.nextEntry(after: entry, ignoreRepeatOne: ignoreRepeatOne, startingIn: shownPlaylist)
	}

	/// The entry to play before `entry`.
	public func previousEntry(before entry: PlaylistEntry?, ignoreRepeatOne: Bool = false) -> PlaylistEntry? {
		order.previousEntry(before: entry, ignoreRepeatOne: ignoreRepeatOne)
	}

	/// Skips to the next entry; false if there is none.
	@discardableResult
	public func next() -> Bool {
		guard let next = nextEntry(after: currentEntry, ignoreRepeatOne: true) else { return false }
		setCurrent(next)
		return true
	}

	/// Skips to the previous entry; false if there is none.
	@discardableResult
	public func previous() -> Bool {
		guard let previous = previousEntry(before: currentEntry, ignoreRepeatOne: true) else { return false }
		setCurrent(previous)
		return true
	}

	/// Makes `entry` the one playing.
	public func setCurrent(_ entry: PlaylistEntry?) {
		guard entry != currentEntry else { return }
		if let currentEntry {
			currentEntry.current = false
			currentEntry.stopAfter = false
			currentEntry.currentPosition = 0
			currentEntry.countAdded = false
		}
		entry?.current = true
		currentEntry = entry
		store.save()
	}
}

extension Array {
	/// As SwiftUI's, which this framework does without: the elements at
	/// `source`, in order, to before `destination`.
	mutating func move(fromOffsets source: IndexSet, toOffset destination: Int) {
		let moving = source.map { self[$0] }
		for index in source.reversed() { remove(at: index) }
		let insertAt = destination - source.filter { $0 < destination }.count
		insert(contentsOf: moving, at: Swift.max(0, Swift.min(insertAt, count)))
	}
}
