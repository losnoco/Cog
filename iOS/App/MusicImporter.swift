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

	/// Moves a file another app shared, which iOS copied into
	/// Documents/Inbox, into the Music folder, where it stays with the rest.
	/// Anything else, opened where it is, is returned as it is.
	static func keepInbox(_ url: URL) -> URL {
		let documents = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
		let inbox = documents.appendingPathComponent("Inbox", isDirectory: true).resolvingSymlinksInPath().path + "/"
		guard url.isFileURL, url.resolvingSymlinksInPath().path.hasPrefix(inbox) else { return url }
		let name = url.deletingPathExtension().lastPathComponent
		var destination = musicFolder.appendingPathComponent(url.lastPathComponent)
		var copy = 2
		while FileManager.default.fileExists(atPath: destination.path) {
			destination = musicFolder.appendingPathComponent("\(name) \(copy)").appendingPathExtension(url.pathExtension)
			copy += 1
		}
		do {
			try FileManager.default.moveItem(at: url, to: destination)
			return destination
		} catch {
			NSLog("Could not move \(url.path) into the Music folder: \(error)")
			return url
		}
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
		for entry in model.allEntries {
			guard let url = entry.url, url.isFileURL, !FileManager.default.fileExists(atPath: url.path),
			      let range = url.path.range(of: #"/Containers/Data/Application/[0-9A-Fa-f-]{36}"#, options: .regularExpression),
			      FileManager.default.fileExists(atPath: home + url.path[range.upperBound...]) else { continue }
			moved += repoint([entry], from: String(url.path[..<range.upperBound]), to: home)
		}
		if moved > 0 { model.store.save() }
		return moved
	}

	/// Takes out the "._" files macOS leaves beside what it copies to disks
	/// that cannot keep its metadata, which adding a folder used to add as
	/// tracks (they share the file's extension but hold no audio). Returns
	/// how many went.
	@MainActor
	@discardableResult
	static func removeAppleDoubleFiles(in model: PlaylistModel) -> Int {
		let appleDouble = model.allEntries.filter { entry in
			guard let url = entry.url, url.isFileURL else { return false }
			return url.lastPathComponent.hasPrefix("._")
		}
		if !appleDouble.isEmpty { model.remove(appleDouble) }
		return appleDouble.count
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
