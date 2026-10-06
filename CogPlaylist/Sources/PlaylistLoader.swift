//
//  PlaylistLoader.swift
//  CogPlaylist
//
//  Adds files to a playlist: what they stand for found by PlaylistExpander
//  (folders walked, containers opened), and each entry's properties and tags
//  read in the background with PlaylistEntryInfo.
//
//  Files outside the app's container must be readable when added, which on
//  iOS means the app holds their security scope (a document picker's URLs).
//

import CoreData
import Foundation

@MainActor
public final class PlaylistLoader {
	public let model: PlaylistModel

	public init(model: PlaylistModel) {
		self.model = model
	}

	/// Adds `urls` (files, folders, containers, streams) at `position`, the
	/// end by default, and returns the entries made, which play at once while
	/// their tags load. With `skippingExisting`, tracks the playlist already
	/// has are left out, after folders and containers are opened, so adding a
	/// folder again adds only what is new in it.
	@discardableResult
	public func add(_ urls: [URL], at position: Int? = nil, skippingExisting: Bool = false) async -> [PlaylistEntry] {
		var expanded = await Task.detached(priority: .userInitiated) { () -> [URL] in
			// As the macOS app expands them, with what suits a library of its
			// own: cue sheets and playlists in folders read, and the files
			// they play from left to them.
			let expander = PlaylistExpander()
			expander.readsCueSheetsInFolders = true
			expander.readsPlaylistsInFolders = true
			expander.skipsContainerDependencies = true
			return expander.urls(for: urls, sort: true)
		}.value
		if skippingExisting {
			let existing = Set(model.entries.compactMap { $0.url.map(Self.identity) })
			expanded.removeAll { existing.contains(Self.identity($0)) }
		}
		guard !expanded.isEmpty else { return [] }
		let context = model.store.viewContext
		let entries = expanded.map { url -> PlaylistEntry in
			let entry = PlaylistEntry(context: context)
			entry.url = url
			entry.metadataBlob = NSDictionary()
			return entry
		}
		model.insert(entries, at: position)
		await loadInfo(for: entries)
		return entries
	}

	/// Reads the entries' properties and tags, a few at a time, and stores
	/// them as they arrive.
	public func loadInfo(for entries: [PlaylistEntry]) async {
		let work = entries.compactMap { entry in entry.url.map { (entry.objectID, $0) } }
		let context = model.store.viewContext
		await withTaskGroup(of: (NSManagedObjectID, [String: Any]?).self) { group in
			var pending = work[...]
			let width = max(2, ProcessInfo.processInfo.activeProcessorCount)
			func addNext() {
				guard let (id, url) = pending.popFirst() else { return }
				group.addTask(priority: .utility) { (id, PlaylistEntryInfo.info(for: url)) }
			}
			for _ in 0..<width { addNext() }
			for await (id, info) in group {
				(try? context.existingObject(with: id) as? PlaylistEntry)?.setMetadata(info)
				addNext()
			}
		}
		model.store.save()
	}

	/// What makes two URLs the same track: a file's resolved path (the
	/// same file reached by /var or /private/var is one) and its fragment
	/// (a cue sheet or subsong track); anything else as written.
	nonisolated static func identity(_ url: URL) -> String {
		guard url.isFileURL else { return url.absoluteString }
		return url.standardizedFileURL.resolvingSymlinksInPath().path + "#" + (url.fragment ?? "")
	}
}
