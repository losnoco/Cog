//
//  PlaylistModel.swift
//  CogPlaylist
//
//  The playlist without its table: the entries in order, the one playing,
//  the queue, shuffle, repeat and stop-after, and what plays next. Ported
//  from Playlist/PlaylistController.m, whose AppKit parts (the table,
//  sorting by column, drag and drop, undo, trash) stay there; keep the
//  rules in step.
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

	/// The entries in playlist order; each one's `index` is its position.
	@Published public private(set) var entries: [PlaylistEntry] = []
	/// Entries to play before the playlist goes on, in order.
	@Published public private(set) var queue: [PlaylistEntry] = []
	/// The entry playing (or last played). A removed one stays current, with
	/// a negative index, until playback moves on.
	@Published public private(set) var currentEntry: PlaylistEntry?

	/// The order shuffle plays in; each entry's `shuffleIndex` is its
	/// position. Extended at either end as repeat-all runs past it.
	private var shuffleList: [PlaylistEntry] = []
	/// Where playback goes on from after the current entry was removed.
	private var nextEntryAfterDeleted: PlaylistEntry?

	/// Shuffle or repeat changed, or entries were removed: what plays next
	/// may have changed (PlaybackController's playlistDidChange:).
	public var onPlaylistChange: (() -> Void)?
	/// The entry playback moves to, for a selection that follows playback
	/// (`selectionFollowsPlayback`).
	public var onFollowPlayback: ((PlaylistEntry?) -> Void)?

	public init(store: PlaylistStore, defaults: UserDefaults = .standard) {
		self.store = store
		self.defaults = defaults
		defaults.register(defaults: ["repeat": PlaylistRepeatMode.all.rawValue, "shuffle": PlaylistShuffleMode.off.rawValue])
		load()
	}

	private var context: NSManagedObjectContext { store.viewContext }

	// MARK: - Loading

	/// Reads the playlist, dropping what was removed or has no URL, as the
	/// macOS app does on launch.
	public func load() {
		let request = NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")
		request.sortDescriptors = [NSSortDescriptor(key: "index", ascending: true)]
		var loaded = (try? context.fetch(request)) ?? []
		loaded.removeAll { entry in
			guard entry.deLeted || (entry.urlString ?? "").isEmpty else { return false }
			context.delete(entry)
			return true
		}
		entries = loaded
		// Art no entry names any more, as macOS drops it on quitting.
		let named = Set(loaded.compactMap(\.artHash))
		let artwork = (try? context.fetch(NSFetchRequest<AlbumArtwork>(entityName: "AlbumArtwork"))) ?? []
		for art in artwork where !named.contains(art.artHash ?? "") {
			context.delete(art)
		}
		updateIndexes()
		currentEntry = entries.first { $0.current }
		queue = entries.filter(\.queued).sorted { $0.queuePosition < $1.queuePosition }
		if shuffleMode != .off {
			shuffleList = entries.sorted { $0.shuffleIndex < $1.shuffleIndex }
			if shuffleList.isEmpty { resetShuffleList() }
		}
		store.save()
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
			if newValue != .off { resetShuffleList() }
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

	// MARK: - Editing

	/// Adds entries at `position` (the end by default).
	public func insert(_ newEntries: [PlaylistEntry], at position: Int? = nil) {
		for entry in newEntries {
			entry.deLeted = false
		}
		let at = min(max(position ?? entries.count, 0), entries.count)
		entries.insert(contentsOf: newEntries, at: at)
		updateIndexes()
		store.save()
		if shuffleMode != .off { resetShuffleList() }
	}

	/// Removes the entries at `offsets`. Removed entries are only marked so
	/// (and dropped on the next load), as on macOS. The current entry may be
	/// removed while playing: playback goes on from the entry after it.
	public func remove(at offsets: IndexSet) {
		let removed = offsets.filter { $0 < entries.count }.map { entries[$0] }
		guard !removed.isEmpty else { return }
		let removedIndexes = IndexSet(removed.map { Int($0.index) })
		for entry in removed {
			entry.deLeted = true
		}

		if let current = currentEntry, current.index >= 0, offsets.contains(Int(current.index)) {
			updateNextAfterDeleted(current, removing: offsets)
		} else if let next = nextEntryAfterDeleted, offsets.contains(Int(next.index)) {
			updateNextAfterDeleted(next, removing: offsets)
		}

		// The current entry, removed, keeps its old position as a negative
		// index (-index - 1), moved down past whatever else went before it,
		// so previous() can still find the entry before it.
		if let current = currentEntry {
			if current.index >= 0 && removedIndexes.contains(Int(current.index)) {
				current.index = -current.index - 1
			}
			if current.index < 0 {
				var i = Int(-current.index - 1)
				for j in stride(from: i - 1, through: 0, by: -1) where removedIndexes.contains(j) {
					i -= 1
				}
				current.index = Int64(-i - 1)
			}
		}

		let removedSet = Set(removed.map(\.objectID))
		entries.removeAll { removedSet.contains($0.objectID) }
		// Unlike macOS, which plays a removed queued entry anyway.
		if queue.contains(where: { removedSet.contains($0.objectID) }) {
			for entry in queue where removedSet.contains(entry.objectID) {
				entry.queued = false
				entry.queuePosition = -1
			}
			queue.removeAll { removedSet.contains($0.objectID) }
			renumberQueue()
		}
		updateIndexes()
		store.save()
		if shuffleMode != .off { resetShuffleList() }
		onPlaylistChange?()
	}

	/// Which entry follows `last` once the entries at `offsets` are gone:
	/// the first one after it that is not among them.
	private func updateNextAfterDeleted(_ last: PlaylistEntry, removing offsets: IndexSet) {
		var next: PlaylistEntry?
		for range in offsets.rangeView {
			if range.lowerBound <= Int(last.index) && range.upperBound > Int(last.index) {
				next = range.upperBound < entries.count ? entries[range.upperBound] : nil
			} else if let pending = next, range.lowerBound <= Int(pending.index) && range.upperBound > Int(pending.index) {
				next = range.upperBound < entries.count ? entries[range.upperBound] : nil
			} else if let pending = next, range.lowerBound > Int(pending.index) {
				break
			}
		}
		nextEntryAfterDeleted = next
	}

	public func removeAll() {
		remove(at: IndexSet(integersIn: 0..<entries.count))
	}

	public func move(fromOffsets source: IndexSet, toOffset destination: Int) {
		let moving = source.map { entries[$0] }
		var remaining = entries
		for index in source.reversed() { remaining.remove(at: index) }
		let insertAt = destination - source.filter { $0 < destination }.count
		remaining.insert(contentsOf: moving, at: max(0, min(insertAt, remaining.count)))
		entries = remaining
		updateIndexes()
		store.save()
	}

	/// Puts the playlist in the order `descriptors` give (the macOS table's
	/// column sorting), which becomes its order.
	public func sort(using descriptors: [NSSortDescriptor]) {
		entries = (entries as NSArray).sortedArray(using: descriptors) as? [PlaylistEntry] ?? entries
		updateIndexes()
		store.save()
	}

	/// Puts the playlist in the order `areInIncreasingOrder` gives, which
	/// becomes its order (for keys KVC cannot reach, as the tags are).
	public func sort(by areInIncreasingOrder: (PlaylistEntry, PlaylistEntry) -> Bool) {
		entries.sort(by: areInIncreasingOrder)
		updateIndexes()
		store.save()
		if shuffleMode != .off { resetShuffleList() }
	}

	/// Puts the playlist in a random order (not shuffle, which leaves it be).
	public func randomize() {
		entries.shuffle()
		updateIndexes()
		store.save()
		if shuffleMode != .off { resetShuffleList() }
	}

	/// Each entry's `index` to its position.
	private func updateIndexes() {
		for (position, entry) in entries.enumerated() where entry.index != Int64(position) {
			entry.index = Int64(position)
		}
	}

	// MARK: - Queue and stop after

	public func toggleQueued(_ toggled: [PlaylistEntry]) {
		for entry in toggled {
			if entry.queued {
				queue.removeAll { $0 == entry }
				entry.queued = false
				entry.queuePosition = -1
			} else {
				entry.queued = true
				entry.queuePosition = Int64(queue.count)
				queue.append(entry)
			}
		}
		renumberQueue()
		store.save()
	}

	public func addToQueue(_ added: [PlaylistEntry]) {
		for entry in added {
			entry.queued = true
			entry.queuePosition = Int64(queue.count)
			queue.append(entry)
		}
		renumberQueue()
		store.save()
	}

	public func removeFromQueue(_ removed: [PlaylistEntry]) {
		for entry in removed {
			entry.queued = false
			entry.queuePosition = -1
			queue.removeAll { $0 == entry }
		}
		renumberQueue()
		store.save()
	}

	public func emptyQueue() {
		for entry in queue {
			entry.queued = false
			entry.queuePosition = -1
		}
		queue.removeAll()
		store.save()
	}

	private func renumberQueue() {
		for (position, entry) in queue.enumerated() {
			entry.queuePosition = Int64(position)
		}
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

	/// The entry at a playlist position; past either end, with repeat all,
	/// around again; otherwise nil.
	public func entry(at position: Int) -> PlaylistEntry? {
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
	/// which leaves even a repeated entry. With no entry, from the start.
	public func nextEntry(after entry: PlaylistEntry?, ignoreRepeatOne: Bool = false) -> PlaylistEntry? {
		if !ignoreRepeatOne && defaults.bool(forKey: "alwaysStopAfterCurrent") {
			followPlayback(nextEntry(after: entry, ignoreRepeatOne: true))
			return nil
		}

		if !ignoreRepeatOne && repeatMode == .one {
			followPlayback(entry)
			return entry
		}

		if !queue.isEmpty {
			let next = queue.removeFirst()
			next.queued = false
			next.queuePosition = -1
			renumberQueue()
			store.save()
			followPlayback(next)
			return next
		}

		guard let entry else {
			let first = shuffleMode != .off ? shuffledEntry(at: 0) : self.entry(at: 0)
			followPlayback(first)
			return first
		}

		if shuffleMode != .off {
			let next = shuffledEntry(at: Int(entry.shuffleIndex) + 1)
			if next != nil { followPlayback(next) }
			return next
		}

		var i: Int
		if entry.deLeted {
			// The current entry, removed: on from what followed it.
			i = nextEntryAfterDeleted.map { Int($0.index) } ?? 0
			nextEntryAfterDeleted = nil
		} else {
			i = Int(entry.index) + 1
		}

		if repeatMode == .album {
			let next = self.entry(at: i)
			let sameAlbum = next?.album.map { $0.caseInsensitiveCompare(entry.album ?? "") == .orderedSame } ?? false
			if i > entries.count - 1 || !sameAlbum {
				let album = albumEntries(entry.album)
				if entry.album == nil || album.isEmpty {
					i -= 1
				} else {
					i = Int(album[0].index)
				}
			}
		}

		let next = self.entry(at: i)
		followPlayback(next)
		return next
	}

	/// The entry to play before `entry`, as `nextEntry` goes forward (the
	/// queue aside).
	public func previousEntry(before entry: PlaylistEntry?, ignoreRepeatOne: Bool = false) -> PlaylistEntry? {
		if !ignoreRepeatOne && defaults.bool(forKey: "alwaysStopAfterCurrent") {
			followPlayback(previousEntry(before: entry, ignoreRepeatOne: true))
			return nil
		}

		if !ignoreRepeatOne && repeatMode == .one {
			followPlayback(entry)
			return entry
		}

		guard let entry else { return nil }

		if shuffleMode != .off {
			let previous = shuffledEntry(at: Int(entry.shuffleIndex) - 1)
			if previous != nil { followPlayback(previous) }
			return previous
		}

		// A removed current entry's negative index remembers where it was.
		let i = entry.index < 0 ? Int(-entry.index - 2) : Int(entry.index) - 1
		let previous = self.entry(at: i)
		followPlayback(previous)
		return previous
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

	private func followPlayback(_ entry: PlaylistEntry?) {
		if defaults.bool(forKey: "selectionFollowsPlayback") {
			onFollowPlayback?(entry)
		}
	}

	// MARK: - Shuffle

	/// The entries of an album, by exact name ("" and none alike), as
	/// filterPlaylistOnAlbum: has it.
	private func albumEntries(_ album: String?) -> [PlaylistEntry] {
		if let album, !album.isEmpty {
			return entries.filter { $0.album == album }
		}
		return entries.filter { ($0.album ?? "").isEmpty }
	}

	private func shuffledEntry(at position: Int) -> PlaylistEntry? {
		var i = position
		while i < 0 {
			guard repeatMode == .all else { return nil }
			addShuffledList(atFront: true)
			i += entries.count
		}
		while i >= shuffleList.count {
			guard repeatMode == .all, !entries.isEmpty else { return nil }
			addShuffledList(atFront: false)
		}
		return shuffleList[i]
	}

	/// A shuffled order: albums whole (each in disc and track order), or
	/// every entry.
	private func shuffledOrder() -> [PlaylistEntry] {
		guard shuffleMode == .albums else { return entries.shuffled() }
		var seen = Set<String>()
		var albums: [String] = []
		for entry in entries {
			let album = entry.album ?? ""
			if seen.insert(album).inserted { albums.append(album) }
		}
		return albums.shuffled().flatMap { album in
			albumEntries(album).sorted { ($0.disc, $0.track) < ($1.disc, $1.track) }
		}
	}

	private func addShuffledList(atFront: Bool) {
		let order = shuffledOrder()
		if atFront {
			shuffleList.insert(contentsOf: order, at: 0)
			renumberShuffle(from: 0)
		} else {
			let start = shuffleList.count
			shuffleList.append(contentsOf: order)
			renumberShuffle(from: start)
		}
		store.save()
	}

	private func renumberShuffle(from start: Int = 0) {
		for i in start..<shuffleList.count {
			shuffleList[i].shuffleIndex = Int64(i)
		}
	}

	/// A new shuffle order, starting with what plays now: its whole album
	/// when shuffling albums.
	private func resetShuffleList() {
		shuffleList = []
		addShuffledList(atFront: true)
		guard let current = currentEntry, current.index >= 0 else { return }
		if shuffleMode == .albums {
			let album = albumEntries(current.album)
			shuffleList.removeAll { album.contains($0) }
			shuffleList.insert(contentsOf: album, at: 0)
		} else {
			shuffleList.removeAll { $0 == current }
			shuffleList.insert(current, at: 0)
		}
		renumberShuffle()
		store.save()
	}
}
