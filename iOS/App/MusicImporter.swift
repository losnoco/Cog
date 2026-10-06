//
//  MusicImporter.swift
//  Cog (iOS)
//
//  Music picked in Files comes in as copies in the app's Documents folder,
//  under Music/, which Files and Finder also show (UIFileSharingEnabled):
//  a picked file's access lasts only as long as its security scope, and
//  copies keep playing from one launch to the next.
//

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
