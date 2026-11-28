//
//  FileTreeController.swift
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

import AppKit

@objc(FileTreeController)
@MainActor
class FileTreeController: NSObject, NSMenuItemValidation {
	@IBOutlet var controller: FileTreeViewController?
	@IBOutlet var outlineView: NSOutlineView?
	@IBOutlet var dataSource: FileTreeDataSource?

	@IBAction func addToPlaylist(_ sender: Any?) {
		doAddToPlaylist(sender, origin: .internal)
	}

	private func doAddToPlaylist(_ sender: Any?, origin: URLOrigin) {
		guard let outlineView else { return }
		let selectedIndexes = outlineView.selectedRowIndexes
		var urls: [URL] = []

		var index = selectedIndexes.first
		while let i = index {
			if let node = outlineView.item(atRow: i) as? PathNode, let url = node.URL {
				urls.append(url)
			}
			index = selectedIndexes.integerGreaterThan(i)
		}

		controller?.doAddToPlaylist(urls, origin: origin)
	}

	@IBAction func addToPlaylistExternal(_ sender: Any?) {
		doAddToPlaylist(sender, origin: .external)
	}

	@IBAction func setAsPlaylist(_ sender: Any?) {
		controller?.clear(sender)
		addToPlaylist(sender)
	}

	@IBAction func playPauseResume(_ id: NSObject?) {
		controller?.playPauseResume(id)
	}

	@IBAction func showEntryInFinder(_ sender: Any?) {
		guard let outlineView else { return }
		let ws = NSWorkspace.shared
		let selectedIndexes = outlineView.selectedRowIndexes

		var index = selectedIndexes.first
		while let i = index {
			if let node = outlineView.item(atRow: i) as? PathNode, let url = node.URL {
				ws.selectFile(url.path, inFileViewerRootedAtPath: url.path)
			}
			index = selectedIndexes.integerGreaterThan(i)
		}
	}

	@IBAction func setAsRoot(_ sender: Any?) {
		guard let outlineView else { return }
		let index = outlineView.selectedRowIndexes.first

		if let index,
		   let node = outlineView.item(atRow: index) as? PathNode {
			dataSource?.changeURL(node.URL)
		}
	}

	func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
		let action = menuItem.action
		guard let outlineView else { return false }

		if outlineView.numberOfSelectedRows == 0 {
			return false
		}

		if action == #selector(setAsRoot(_:)) {
			let row = outlineView.selectedRow

			if outlineView.numberOfSelectedRows > 1 {
				return false
			}

			// Only let directories be Set as Root
			var isDir: ObjCBool = false
			if let node = outlineView.item(atRow: row) as? PathNode, let url = node.URL {
				FileManager.default.fileExists(atPath: url.path, isDirectory: &isDir)
			}
			return isDir.boolValue
		}

		return true
	}
}
