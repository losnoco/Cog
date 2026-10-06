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
	/// their tags load.
	@discardableResult
	public func add(_ urls: [URL], at position: Int? = nil) async -> [PlaylistEntry] {
		let expanded = await Task.detached(priority: .userInitiated) { Self.expand(urls) }.value
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
				group.addTask(priority: .utility) { (id, Self.entryInfo(for: url)) }
			}
			for _ in 0..<width { addNext() }
			for await (id, info) in group {
				(try? context.existingObject(with: id) as? PlaylistEntry)?.setMetadata(info)
				addNext()
			}
		}
		model.store.save()
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

	// MARK: - Entry info

	/// Properties and tags for one entry, merged, as entryInfoForURL() makes
	/// them; nil if the file cannot be read.
	nonisolated static func entryInfo(for url: URL) -> [String: Any]? {
		let cueSheetTrack = isCueSheetTrack(url)
		// A cue track's own tags first: the decoder's properties can carry the
		// shared album file's.
		var metadata = cueSheetTrack ? AudioMetadataReader.metadata(for: url) as? [String: Any] : nil
		guard let properties = AudioPropertiesReader.properties(for: url) as? [String: Any] else { return nil }
		if metadata == nil {
			metadata = AudioMetadataReader.metadata(for: url) as? [String: Any] ?? [:]
		}
		if cueSheetTrack {
			return properties.merging(metadata ?? [:]) { _, cue in cue }
		}
		return merge(properties, with: metadata ?? [:])
	}

	/// Whether a URL is one track of a cue sheet: a fragment of a .cue file,
	/// or of an audio file with one embedded.
	nonisolated static func isCueSheetTrack(_ url: URL) -> Bool {
		guard url.isFileURL, let fragment = url.fragment, !fragment.isEmpty else { return false }
		if url.pathExtension.caseInsensitiveCompare("cue") == .orderedSame { return true }
		var components = URLComponents(url: url, resolvingAgainstBaseURL: false)
		components?.fragment = nil
		guard let base = components?.url else { return false }
		if hasContent(AudioMetadataReader.metadata(for: base, skipCue: true)?["cuesheet"]) { return true }
		return hasContent(AudioPropertiesReader.properties(for: base, skipCue: true)?["cuesheet"])
	}

	nonisolated private static func hasContent(_ value: Any?) -> Bool {
		if let string = value as? String { return !string.isEmpty }
		if let array = value as? [Any] { return array.contains { ($0 as? String)?.isEmpty == false } }
		return false
	}

	/// The first dictionary, with what the second has that it lacks or has
	/// empty (as +[NSDictionary dictionaryByMerging:with:]).
	nonisolated static func merge(_ first: [String: Any], with second: [String: Any]) -> [String: Any] {
		var result = first
		for (key, value) in second {
			guard let existing = first[key] else {
				result[key] = value
				continue
			}
			if let existingDictionary = existing as? [String: Any], let valueDictionary = value as? [String: Any] {
				result[key] = merge(existingDictionary, with: valueDictionary)
			} else if isEmpty(existing) {
				result[key] = value
			}
		}
		return result
	}

	nonisolated private static func isEmpty(_ value: Any) -> Bool {
		switch value {
		case let string as String: return string.isEmpty
		case let number as NSNumber: return number == 0
		case let data as Data: return data.isEmpty
		default: return false
		}
	}
}
