//
//  TagSaver.swift
//  Cog
//
//  Saving tags from the playlist: which file a row's tags live in, writing
//  each file once however many rows show it, and bringing every row of a
//  saved file, in every playlist, back in line with the file.
//
//  Not undoable: rewriting the old tags would be wrong once anything else
//  has touched the file.
//

import Cocoa
import CogAudio
// Only these: CogPlaylist's own PlaylistLoader would hide the app's.
import class CogPlaylist.Playlist
import class CogPlaylist.PlaylistEntry
import class CogPlaylist.PlaylistEntryInfo

@MainActor @objc final class TagSaver: NSObject {
	/// One file's save: `tags` in the writer's terms (Plugin.h), the whole
	/// set when `replace`, otherwise only the tags it names.
	/// Built once and only read after, so it may cross to the save queue.
	struct FileTags: @unchecked Sendable {
		let url: URL
		let tags: [String: Any]
		let replace: Bool
	}

	struct Outcome {
		var saved: [URL] = []
		var failures: [(url: URL, error: Error)] = []
		/// Saved files whose rows no longer match what the file lists (its
		/// chapters were added or removed), in some playlist.
		var rowsChanged: [URL] = []
	}

	private let playlistController: PlaylistController
	private let playlistLoader: PlaylistLoader
	private let queue = DispatchQueue(label: "org.cogx.cog.tag-saver", qos: .userInitiated)
	private var plugins: CogPluginController { PluginController.shared() }

	/// Probes by file, for menu validation, which runs often. Dropped by a
	/// save.
	private var probes: [URL: (stamp: Date?, writable: Bool)] = [:]

	@objc init(playlistController: PlaylistController, playlistLoader: PlaylistLoader) {
		self.playlistController = playlistController
		self.playlistLoader = playlistLoader
	}

	// MARK: - Which file

	/// The file whose tags an entry shows, or nil when they can't be saved
	/// there: not a local file, or a cue sheet's track, whose "tags" are the
	/// cue sheet's. A chapter's row ("book.mp3#2") gives its file.
	@objc static func tagFileURL(for entry: PlaylistEntry) -> URL? {
		guard let url = entry.url, url.isFileURL else { return nil }
		guard let fragment = url.fragment, !fragment.isEmpty else { return url }
		if PlaylistEntryInfo.isCueSheetTrack(url) { return nil }
		var components = URLComponents(url: url, resolvingAgainstBaseURL: false)
		components?.fragment = nil
		return components?.url
	}

	/// The chapter a row plays, or nil for a whole file.
	static func chapterIndex(for entry: PlaylistEntry) -> Int? {
		guard let fragment = entry.url?.fragment, !fragment.isEmpty else { return nil }
		return Int(fragment)
	}

	/// Whether the entries' files can have their tags saved: every entry
	/// names a file, and the first few files probe writable. Menus validate
	/// often, so a large selection isn't probed whole; the editor leaves out
	/// whatever turns out not to be writable.
	@objc func canEditTags(of entries: [PlaylistEntry]) -> Bool {
		guard !entries.isEmpty else { return false }
		var probed = Set<URL>()
		for entry in entries {
			guard let file = Self.tagFileURL(for: entry) else { return false }
			guard probed.count < 16, probed.insert(file).inserted else { continue }
			if !isWritable(file) { return false }
		}
		return true
	}

	private func isWritable(_ file: URL) -> Bool {
		let stamp = (try? file.resourceValues(forKeys: [.contentModificationDateKey]))?.contentModificationDate
		if let probe = probes[file], probe.stamp == stamp { return probe.writable }
		let writable = (plugins.tagWritingCapabilities(for: file)?[CogTagWriterWritable] as? Bool) ?? false
		probes[file] = (stamp, writable)
		return writable
	}

	/// The distinct files behind `entries`, in their order, without the
	/// ones whose tags can't be saved.
	func files(of entries: [PlaylistEntry]) -> [URL] {
		var seen = Set<URL>()
		return entries.compactMap { Self.tagFileURL(for: $0) }.filter { seen.insert($0).inserted }
	}

	// MARK: - Saving

	/// Writes each file in the background, then reloads every row of the
	/// files that saved, in every playlist, replacing what they stored.
	/// `completion` runs on the main queue.
	func save(_ files: [FileTags], completion: @escaping @MainActor (Outcome) -> Void) {
		queue.async { [weak self] in
			let plugins: CogPluginController = PluginController.shared()
			var outcome = Outcome()
			var listed: [URL: [String]] = [:]
			for file in files {
				do {
					try plugins.writeTags(file.tags, to: file.url, replace: file.replace)
					outcome.saved.append(file.url)
					listed[file.url] = Self.rowURLStrings(of: file.url, plugins: plugins)
				} catch {
					NSLog("Tag editor: could not save tags to %@: %@", file.url.path, error.localizedDescription)
					outcome.failures.append((file.url, error))
				}
			}
			DispatchQueue.main.async { [outcome, listed] in
				MainActor.assumeIsolated {
					guard let self else { return }
					var outcome = outcome
				for url in outcome.saved + outcome.failures.map(\.url) {
					self.probes[url] = nil
				}
				for url in outcome.saved {
					let rows = self.rows(of: url)
					if let now = listed[url], self.rowsDiffer(rows, from: now) {
						outcome.rowsChanged.append(url)
					}
					self.reload(rows.flatMap(\.value))
				}
				completion(outcome)
				}
			}
		}
	}

	/// Replaces each playlist's rows of `file` with the rows it lists now,
	/// after its chapters changed.
	func rebuildRows(of file: URL) {
		for (playlist, entries) in rows(of: file) {
			playlistController.replaceEntries(entries, withRowsOf: file, in: playlist)
		}
	}

	// MARK: - Rows

	/// What the file expands to as a playlist adds it: its chapters'
	/// "#N" URLs, or the file alone.
	nonisolated private static func rowURLStrings(of file: URL, plugins: CogPluginController) -> [String] {
		let urls = (plugins.urls(forContainerURL: file) as? [URL]) ?? []
		return (urls.isEmpty ? [file] : urls).map(\.absoluteString)
	}

	/// Every live row showing `file` or one of its subsongs, by playlist.
	private func rows(of file: URL) -> [Playlist: [PlaylistEntry]] {
		let path = file.absoluteString
		let request = NSFetchRequest<PlaylistEntry>(entityName: "PlaylistEntry")
		request.predicate = NSPredicate(format: "(urlString == %@ OR urlString BEGINSWITH %@) AND (deLeted == NO OR deLeted == nil)", path, path + "#")
		request.sortDescriptors = [NSSortDescriptor(key: "index", ascending: true)]
		let entries = (try? playlistController.persistentContainer.viewContext.fetch(request)) ?? []
		var byPlaylist: [Playlist: [PlaylistEntry]] = [:]
		for entry in entries {
			guard let playlist = entry.playlist else { continue }
			byPlaylist[playlist, default: []].append(entry)
		}
		return byPlaylist
	}

	/// Whether some playlist's rows of the file aren't what it lists now. A
	/// cue sheet's tracks of an embedded sheet ("album.flac#01") are listed
	/// by it the same way before and after, so they never differ.
	private func rowsDiffer(_ rows: [Playlist: [PlaylistEntry]], from listed: [String]) -> Bool {
		let now = Set(listed)
		return rows.values.contains { Set($0.compactMap(\.urlString)) != now }
	}

	private func reload(_ entries: [PlaylistEntry]) {
		guard !entries.isEmpty else { return }
		for entry in entries {
			entry.metadataLoaded = false
		}
		playlistLoader.performSelector(inBackground: NSSelectorFromString("reloadInfoForEntries:"), with: entries)
	}
}
