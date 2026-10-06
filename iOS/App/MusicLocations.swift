//
//  MusicLocations.swift
//  Cog (iOS)
//
//  The files and folders picked in Files, played where they are: the picker
//  grants access to each (a folder's covers everything inside it, files
//  added later included), a bookmark keeps it from one launch to the next,
//  and the app holds it for as long as it runs, as the decoders open
//  files by path whenever they please.
//

import CogPlaylist
import Foundation

@MainActor
final class MusicLocations: ObservableObject {
	struct Location: Codable, Identifiable, Hashable {
		var bookmark: Data
		/// Where it was when last resolved, which the playlist's entries
		/// under it start with.
		var path: String
		var id: String { path }
		var name: String { (path as NSString).lastPathComponent }
	}

	@Published private(set) var locations: [Location] = []
	/// The resolved URLs being accessed, by path.
	private var accessed: [String: URL] = [:]

	private static var storeURL: URL {
		let support = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
		try? FileManager.default.createDirectory(at: support, withIntermediateDirectories: true)
		return support.appendingPathComponent("MusicLocations.plist")
	}

	init() {
		if let data = try? Data(contentsOf: Self.storeURL) {
			locations = (try? PropertyListDecoder().decode([Location].self, from: data)) ?? []
		}
	}

	/// Resolves every bookmark and starts accessing it. A location that has
	/// moved since (bookmarks follow renames and moves) takes the playlist's
	/// entries under it along.
	func restore(in model: PlaylistModel) {
		var moved = 0
		for (index, location) in locations.enumerated() {
			var stale = false
			guard let url = try? URL(resolvingBookmarkData: location.bookmark, options: .withoutUI, bookmarkDataIsStale: &stale) else {
				NSLog("Could not resolve the bookmark for \(location.path)")
				continue
			}
			_ = url.startAccessingSecurityScopedResource()
			accessed[url.path] = url
			if url.path != location.path {
				moved += MusicImporter.repoint(model.entries, from: location.path, to: url.path)
				locations[index].path = url.path
			}
			if stale, let fresh = try? url.bookmarkData(options: .minimalBookmark, includingResourceValuesForKeys: nil, relativeTo: nil) {
				locations[index].bookmark = fresh
			}
		}
		if moved > 0 { model.store.save() }
		save()
	}

	/// Keeps access to what was picked and returns it to add, all of it:
	/// what lies inside a location already held needs no bookmark of its
	/// own, nor does the app's own container.
	func add(_ urls: [URL]) -> [URL] {
		let home = URL(fileURLWithPath: NSHomeDirectory()).standardizedFileURL.path
		for url in urls {
			_ = url.startAccessingSecurityScopedResource()
			let path = url.path
			if Self.path(path, isInside: home) || locations.contains(where: { Self.path(path, isInside: $0.path) }) {
				continue
			}
			guard let bookmark = try? url.bookmarkData(options: .minimalBookmark, includingResourceValuesForKeys: nil, relativeTo: nil) else {
				NSLog("Could not bookmark \(path); it plays until Cog quits")
				continue
			}
			// A folder that holds locations already stands for them.
			for inner in locations where Self.path(inner.path, isInside: path) {
				stopAccessing(inner)
			}
			locations.removeAll { Self.path($0.path, isInside: path) }
			locations.append(Location(bookmark: bookmark, path: path))
			accessed[path] = url
		}
		save()
		return urls
	}

	/// Gives up a location: its entries stay in the playlist but no longer
	/// play.
	func remove(_ location: Location) {
		stopAccessing(location)
		locations.removeAll { $0 == location }
		save()
	}

	private func stopAccessing(_ location: Location) {
		accessed.removeValue(forKey: location.path)?.stopAccessingSecurityScopedResource()
	}

	private func save() {
		do {
			try PropertyListEncoder().encode(locations).write(to: Self.storeURL, options: .atomic)
		} catch {
			NSLog("Could not save the music locations: \(error)")
		}
	}

	/// The path itself, or anything under it.
	private static func path(_ path: String, isInside folder: String) -> Bool {
		path == folder || path.hasPrefix(folder.hasSuffix("/") ? folder : folder + "/")
	}
}
