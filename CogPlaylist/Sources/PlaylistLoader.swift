//
//  PlaylistLoader.swift
//  CogPlaylist
//
//  Adds files to a playlist: folders walked, containers (cue sheets,
//  playlists, archives, multi-track formats) opened through the plugins, and
//  each entry's properties and tags read in the background. The plugin
//  dealing of Playlist/PlaylistLoader.m, without its macOS parts (sandbox
//  bookmarks, XML playlists, Spotlight).
//
//  Files outside the app's container must be readable when added, which on
//  iOS means the app holds their security scope (a document picker's URLs).
//

import CogAudio
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
		var expanded = await Task.detached(priority: .userInitiated) { Self.expand(urls) }.value
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

	// MARK: - Expanding

	/// The file extensions the plugins play, and those they open as
	/// containers of other entries.
	nonisolated static var playableTypes: Set<String> {
		Set(((AudioPlayer.fileTypes() as? [String]) ?? []).map { $0.lowercased() })
	}

	nonisolated static var containerTypes: Set<String> {
		Set(((AudioPlayer.containerTypes() as? [String]) ?? []).map { $0.lowercased() })
	}

	/// The entries `urls` stand for, in order, each once. Files a container
	/// plays from (a cue sheet's audio file) are its tracks, not entries of
	/// their own.
	nonisolated static func expand(_ urls: [URL]) -> [URL] {
		let playable = playableTypes
		let containers = containerTypes
		var result: [URL] = []
		var seen = Set<String>()
		var dependencies = Set<String>()
		func add(_ url: URL) {
			if seen.insert(url.absoluteString).inserted { result.append(url) }
		}
		func visit(_ url: URL) {
			let url = PlaylistEntry.normalize(url)
			var isDirectory: ObjCBool = false
			if url.isFileURL, FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory), isDirectory.boolValue {
				for file in files(in: url) { visit(file) }
				return
			}
			let type = url.pathExtension.lowercased()
			if containers.contains(type), url.fragment == nil,
			   let inner = AudioContainer.urls(forContainerURL: url) as? [URL], !inner.isEmpty {
				inner.forEach(add)
				for dependency in (AudioContainer.dependencyUrls(forContainerURL: url) as? [URL]) ?? [] {
					dependencies.insert(dependency.absoluteString)
				}
				return
			}
			if !url.isFileURL || playable.contains(type) {
				add(url)
			}
		}
		urls.forEach(visit)
		return result.filter { !dependencies.contains($0.absoluteString) }
	}

	/// A folder's files, depth first, in Finder's order.
	nonisolated static func files(in folder: URL) -> [URL] {
		guard let enumerator = FileManager.default.enumerator(at: folder, includingPropertiesForKeys: [.isRegularFileKey],
		                                                      options: [.skipsHiddenFiles]) else { return [] }
		let files = enumerator.compactMap { $0 as? URL }.filter { (try? $0.resourceValues(forKeys: [.isRegularFileKey]).isRegularFile) == true }
		return files.sorted { $0.path.localizedStandardCompare($1.path) == .orderedAscending }
	}
}
