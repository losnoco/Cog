//
//  PlaylistController+Playlists.swift
//  Cog
//
//  The playlists themselves: making, naming, ordering and deleting them,
//  and copying or moving entries between them, for the Playlist menu, the
//  table's menu and the tabs above it.
//

import Cocoa
import CogPlaylist

extension Notification.Name {
	/// Playlists were added, renamed, reordered or deleted.
	static let cogPlaylistsDidChange = Notification.Name("CogPlaylistsDidChange")
	/// Asks the tabs to rename the playlist given as the object in place.
	static let cogRenamePlaylist = Notification.Name("CogRenamePlaylist")
}

@MainActor
extension PlaylistController {
	private var playlistsContext: NSManagedObjectContext { persistentContainer.viewContext }

	/// Every playlist, in their order.
	var playlists: [Playlist] { Playlist.all(in: playlistsContext) }

	private func playlistsChanged() {
		commitPersistentStoreAsync()
		NotificationCenter.default.post(name: .cogPlaylistsDidChange, object: self)
	}

	// MARK: - Playlists

	/// A new playlist after the others, with copies of `entries`, shown.
	@discardableResult
	func makePlaylist(named name: String? = nil, copying entries: [PlaylistEntry] = []) -> Playlist {
		let playlist = Playlist.insert(named: name, at: -1, in: playlistsContext)
		playlist.append(entries.map { $0.copyForPlaylist() })
		playlistsChanged()
		show(playlist)
		return playlist
	}

	/// Renames `playlist`; a name another has gets a number.
	func rename(_ playlist: Playlist, to name: String) {
		let unique = Playlist.uniqueName(name, in: playlistsContext, excluding: playlist)
		guard unique != playlist.name else { return }
		playlist.name = unique
		playlistsChanged()
	}

	/// Moves `playlist` to `position` among the playlists.
	func move(_ playlist: Playlist, to position: Int) {
		var all = playlists
		guard let from = all.firstIndex(of: playlist) else { return }
		all.remove(at: from)
		all.insert(playlist, at: min(max(position, 0), all.count))
		Playlist.renumber(all)
		playlistsChanged()
	}

	/// Deletes `playlist` with its entries, unless it is the only one. The
	/// one playing stops; the one shown gives way to its neighbor.
	func delete(_ playlist: Playlist) {
		let all = playlists
		guard all.count > 1, let position = all.firstIndex(of: playlist) else { return }
		if playlist == playingPlaylist {
			stopPlayback()
			currentEntry = nil
		}
		playbackOrder.willDelete(playlist)
		let rest = all.filter { $0 != playlist }
		if playlist == shownPlaylist {
			show(rest[min(position, rest.count - 1)])
		}
		playlistsContext.delete(playlist)
		Playlist.renumber(rest)
		playlistsChanged()
	}

	/// Asks first when `playlist` has entries, as a sheet on `window`.
	func confirmDelete(_ playlist: Playlist, in window: NSWindow?) {
		guard playlists.count > 1 else { return }
		let count = playlist.orderedEntries.count
		guard count > 0, let window else {
			delete(playlist)
			return
		}
		let alert = NSAlert()
		alert.messageText = String(format: NSLocalizedString("Delete “%@”?", comment: "Deleting a playlist: the alert's title, with its name"), playlist.name ?? "")
		alert.informativeText = String.localizedStringWithFormat(NSLocalizedString("Its %lld tracks are removed with it.", comment: "Deleting a playlist: what goes with it"), count)
		alert.addButton(withTitle: NSLocalizedString("Delete", comment: "Deleting a playlist: the button that does it"))
		alert.addButton(withTitle: NSLocalizedString("Cancel", comment: "Deleting a playlist: the button that keeps it"))
		alert.buttons[0].hasDestructiveAction = true
		alert.beginSheetModal(for: window) { [weak self] response in
			if response == .alertFirstButtonReturn { self?.delete(playlist) }
		}
	}

	// MARK: - Entries between playlists

	/// Copies `entries` to the end of `playlist`, or moves them there from
	/// the playlist shown.
	func add(_ entries: [PlaylistEntry], to playlist: Playlist, moving: Bool = false) {
		guard !entries.isEmpty, !(moving && playlist == shownPlaylist) else { return }
		let copies = entries.map { $0.copyForPlaylist() }
		if playlist == shownPlaylist {
			clearFilterPredicate(self)
			let count = (arrangedObjects as? [Any])?.count ?? 0
			insert(contentsOf: copies, atArrangedObjectIndexes: IndexSet(integersIn: count..<count + copies.count))
		} else {
			playlist.append(copies)
			if playlist == playingPlaylist { playingPlaylistDidChange() }
		}
		if moving {
			let rows = IndexSet(entries.map { row(for: $0) }.filter { $0 != NSNotFound })
			if !rows.isEmpty { remove(atArrangedObjectIndexes: rows) }
		}
		commitPersistentStoreAsync()
	}

	/// Adds files to `playlist`, which is shown for them; they go there
	/// even if another is shown by the time they are ready.
	func add(_ urls: [URL], to playlist: Playlist) {
		guard !urls.isEmpty else { return }
		show(playlist)
		addURLs(inBackground: ["entries": urls, "sort": true, "origin": URLOrigin.internal.rawValue, "playlist": playlist])
	}

	// MARK: - Menu commands

	@IBAction func newPlaylist(_ sender: Any?) {
		makePlaylist()
		NotificationCenter.default.post(name: .cogRenamePlaylist, object: shownPlaylist)
	}

	@IBAction func newPlaylistFromSelection(_ sender: Any?) {
		guard let selected = selectedObjects as? [PlaylistEntry], !selected.isEmpty else { return }
		makePlaylist(copying: selected)
		NotificationCenter.default.post(name: .cogRenamePlaylist, object: shownPlaylist)
	}

	@IBAction func renamePlaylist(_ sender: Any?) {
		guard let shownPlaylist else { return }
		NotificationCenter.default.post(name: .cogRenamePlaylist, object: shownPlaylist)
	}

	@IBAction func deletePlaylist(_ sender: Any?) {
		guard let shownPlaylist else { return }
		confirmDelete(shownPlaylist, in: tableView?.window)
	}

	@IBAction func showNextPlaylist(_ sender: Any?) {
		showPlaylist(offsetBy: 1)
	}

	@IBAction func showPreviousPlaylist(_ sender: Any?) {
		showPlaylist(offsetBy: -1)
	}

	private func showPlaylist(offsetBy offset: Int) {
		let all = playlists
		guard all.count > 1, let shownPlaylist, let position = all.firstIndex(of: shownPlaylist) else { return }
		show(all[(position + offset + all.count) % all.count])
	}

	/// From the Add to Playlist menu: the selection, copied to the item's
	/// playlist, or to a new one.
	@IBAction func addSelectionToPlaylist(_ sender: NSMenuItem) {
		guard let selected = selectedObjects as? [PlaylistEntry], !selected.isEmpty else { return }
		if let playlist = sender.representedObject as? Playlist {
			add(selected, to: playlist)
		} else {
			makePlaylist(copying: selected)
			NotificationCenter.default.post(name: .cogRenamePlaylist, object: shownPlaylist)
		}
	}

	/// The Add to Playlist menu's items: every other playlist, then a new
	/// one.
	@objc func fillAddToPlaylistMenu(_ menu: NSMenu) {
		menu.removeAllItems()
		for playlist in playlists where playlist != shownPlaylist {
			let item = NSMenuItem(title: playlist.name ?? "", action: #selector(addSelectionToPlaylist(_:)), keyEquivalent: "")
			item.target = self
			item.representedObject = playlist
			menu.addItem(item)
		}
		if !menu.items.isEmpty { menu.addItem(.separator()) }
		let item = NSMenuItem(title: NSLocalizedString("New Playlist", comment: "Add to Playlist submenu: copies the selection into a new playlist"), action: #selector(addSelectionToPlaylist(_:)), keyEquivalent: "")
		item.target = self
		menu.addItem(item)
	}
}
