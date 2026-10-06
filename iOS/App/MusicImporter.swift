//
//  MusicImporter.swift
//  Cog (iOS)
//
//  Cog's own Music folder, in its Documents, which Files and Finder show
//  (UIFileSharingEnabled); and keeping the playlist's paths pointing where
//  the files are when their folder moves. Music elsewhere plays in place
//  (MusicLocations).
//

import CogPlaylist
import Foundation

enum MusicImporter {
	static var musicFolder: URL {
		let documents = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
		let folder = documents.appendingPathComponent("Music", isDirectory: true)
		try? FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
		return folder
	}

	/// Entries stored with their absolute path into the app's data
	/// container, which iOS moves when the app is reinstalled (as every build
	/// from Xcode does): points those at the container as it is now, where
	/// the file is. Returns how many moved.
	@MainActor
	@discardableResult
	static func relocateMovedContainer(in model: PlaylistModel) -> Int {
		let home = URL(fileURLWithPath: NSHomeDirectory()).standardizedFileURL.path
		var moved = 0
		for entry in model.entries {
			guard let url = entry.url, url.isFileURL, !FileManager.default.fileExists(atPath: url.path),
			      let range = url.path.range(of: #"/Containers/Data/Application/[0-9A-Fa-f-]{36}"#, options: .regularExpression),
			      FileManager.default.fileExists(atPath: home + url.path[range.upperBound...]) else { continue }
			moved += repoint([entry], from: String(url.path[..<range.upperBound]), to: home)
		}
		if moved > 0 { model.store.save() }
		return moved
	}

	/// Points the entries under `old` at the same place under `new`, keeping
	/// what follows the path (a cue sheet's track). Returns how many moved.
	@MainActor
	static func repoint(_ entries: [PlaylistEntry], from old: String, to new: String) -> Int {
		let prefix = old.hasSuffix("/") ? old : old + "/"
		var moved = 0
		for entry in entries {
			guard let url = entry.url, url.isFileURL else { continue }
			let path = url.path
			guard path == old || path.hasPrefix(prefix),
			      var components = URLComponents(url: url, resolvingAgainstBaseURL: false) else { continue }
			components.path = new + path.dropFirst(old.count)
			guard let repointed = components.url else { continue }
			entry.url = repointed
			entry.error = false
			moved += 1
		}
		return moved
	}
}
