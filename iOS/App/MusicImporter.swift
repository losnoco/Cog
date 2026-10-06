//
//  MusicImporter.swift
//  Cog (iOS)
//
//  Music picked in Files comes in as copies in the app's Documents folder,
//  under Music/, which Files and Finder also show (UIFileSharingEnabled):
//  a picked file's access lasts only as long as its security scope, and
//  copies keep playing from one launch to the next.
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

	/// Copies picked files and folders into Music/, keeping each folder's
	/// layout (a cue sheet's files stay together), and returns the copies.
	/// Off the main thread: a folder can take a while.
	static func importItems(_ urls: [URL]) async -> [URL] {
		await Task.detached(priority: .userInitiated) {
			urls.compactMap { url in
				let scoped = url.startAccessingSecurityScopedResource()
				defer { if scoped { url.stopAccessingSecurityScopedResource() } }
				let destination = uniqueDestination(for: url.lastPathComponent)
				do {
					try FileManager.default.copyItem(at: url, to: destination)
					return destination
				} catch {
					NSLog("Could not import \(url.lastPathComponent): \(error)")
					return nil
				}
			}
		}.value
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
			      let relative = pathInsideAppContainer(url.path) else { continue }
			let current = URL(fileURLWithPath: home + relative)
			guard FileManager.default.fileExists(atPath: current.path) else { continue }
			var components = URLComponents(url: current, resolvingAgainstBaseURL: false)
			components?.fragment = url.fragment
			entry.url = components?.url ?? current
			entry.error = false
			moved += 1
		}
		if moved > 0 { model.store.save() }
		return moved
	}

	/// "/Documents/Music/a.flac" from ".../Containers/Data/Application/<UUID>/Documents/Music/a.flac".
	private static func pathInsideAppContainer(_ path: String) -> String? {
		guard let range = path.range(of: #"/Containers/Data/Application/[0-9A-Fa-f-]{36}"#, options: .regularExpression) else {
			return nil
		}
		return String(path[range.upperBound...])
	}

	/// Music/name, or "name 2", "name 3"… if taken.
	private static func uniqueDestination(for name: String) -> URL {
		let folder = musicFolder
		var candidate = folder.appendingPathComponent(name)
		let base = (name as NSString).deletingPathExtension
		let ext = (name as NSString).pathExtension
		var number = 2
		while FileManager.default.fileExists(atPath: candidate.path) {
			let numbered = ext.isEmpty ? "\(base) \(number)" : "\(base) \(number).\(ext)"
			candidate = folder.appendingPathComponent(numbered)
			number += 1
		}
		return candidate
	}
}
