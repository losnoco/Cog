//
//  SandboxPathBehaviorController.swift
//  Preferences
//
//  Created by Christopher Snowhill on 6/19/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

import AppKit
import CogPlaylist
import CoreData

@objc(SandboxPathBehaviorController)
class SandboxPathBehaviorController: NSArrayController {
	override func awakeFromNib() {
		refresh()
	}

	@objc func refresh() {
		remove(contentsOf: (arrangedObjects as? [Any]) ?? [])

		sortDescriptors = [NSSortDescriptor(key: "path", ascending: true)]

		let pc = PlaylistController.sharedPersistentContainer()
		let request = NSFetchRequest<NSManagedObject>(entityName: "SandboxToken")

		guard let results = try? pc.viewContext.fetch(request), !results.isEmpty else { return }

		for case let token as SandboxToken in results {
			var isDisconnected = false
			var isStale = true
			let bookmarkData = token.bookmark ?? Data()

			if let values = try? URL.resourceValues(forKeys: [.volumeURLKey], fromBookmarkData: bookmarkData),
			   let volumeUrl = values.volume, volumeUrl.isFileURL {
				if !FileManager.default.fileExists(atPath: volumeUrl.path) {
					isDisconnected = true
				}
			}

			let valid: String
			if !isDisconnected {
				let bookmarkUrl = try? URL(resolvingBookmarkData: bookmarkData, options: .withSecurityScope, relativeTo: nil, bookmarkDataIsStale: &isStale)
				valid = (bookmarkUrl == nil || isStale) ? NSLocalizedString("ValidNo", comment: "Negative answer to whether a path is valid.") : NSLocalizedString("ValidYes", comment: "Positive answer to whether a path is valid.")
			} else {
				valid = NSLocalizedString("ValidNo", comment: "Negative answer to whether a path is valid.")
			}

			addObject(["path": token.path, "valid": valid, "isFolder": token.folder, "token": token] as [String: Any])
		}
	}

	@objc func addUrl(_ url: URL) {
		guard let bookmark = try? url.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil) else {
			NSLog("Failed to add bookmark for URL: \(url)")
			return
		}

		let pc = PlaylistController.sharedPersistentContainer()

		pc.viewContext.performAndWait {
			let token = NSEntityDescription.insertNewObject(forEntityName: "SandboxToken", into: pc.viewContext) as! SandboxToken
			token.path = url.path
			token.bookmark = bookmark
			do {
				try pc.viewContext.save()
				addObject(["path": url.path,
				           "valid": NSLocalizedString("ValidYes", comment: "Positive answer to whether a path is valid."),
				           "isFolder": token.folder,
				           "token": token] as [String: Any])
				SandboxBroker.cleanupFolderAccess()
				refresh()
			} catch {
				NSLog("Error saving bookmark: \(error.localizedDescription)")
			}
		}
	}

	@objc func removeToken(_ token: Any) {
		let pc = PlaylistController.sharedPersistentContainer()
		let objects = arrangedObjects as? [Any] ?? []

		var updated = false
		for case let obj as [String: Any] in objects {
			if let t = obj["token"] as? NSObject, t.isEqual(token) {
				removeObject(obj)
				pc.viewContext.performAndWait {
					if let mo = token as? NSManagedObject { pc.viewContext.delete(mo) }
				}
				updated = true
				break
			}
		}

		if updated {
			pc.viewContext.performAndWait {
				do { try pc.viewContext.save() } catch {
					NSLog("Error deleting bookmark: \(error.localizedDescription)")
				}
			}
		}
	}

	@objc func removeStaleEntries() {
		var updated = false
		let pc = PlaylistController.sharedPersistentContainer()
		let no = NSLocalizedString("ValidNo", comment: "Negative answer to whether a path is valid.")
		for case let entry as [String: Any] in (arrangedObjects as? [Any] ?? []) {
			if (entry["valid"] as? String) == no {
				removeObject(entry)
				pc.viewContext.performAndWait {
					if let mo = entry["token"] as? NSManagedObject { pc.viewContext.delete(mo) }
				}
				updated = true
			}
		}
		if updated {
			pc.viewContext.performAndWait {
				do { try pc.viewContext.save() } catch {
					NSLog("Error saving after removing stale bookmarks: \(error.localizedDescription)")
				}
			}
		}
	}

	@objc func matchesPath(_ url: URL) -> Bool {
		let yes = NSLocalizedString("ValidYes", comment: "Positive answer to whether a path is valid.")
		for case let entry as [String: Any] in (arrangedObjects as? [Any] ?? []) {
			guard (entry["valid"] as? String) == yes, let path = entry["path"] as? String, !path.isEmpty else { continue }
			let isFolder = (entry["isFolder"] as? NSNumber)?.boolValue ?? false
			let testPath = URL(fileURLWithPath: path)
			if (isFolder && SandboxBroker.isPath(url, aSubdirectoryOf: testPath)) ||
				(!isFolder && url.path == path) {
				return true
			}
		}
		return false
	}
}