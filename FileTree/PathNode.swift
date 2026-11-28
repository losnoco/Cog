//
//  PathNode.swift
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

import AppKit
import CogAudio

// From Apple's "Resolving Aliases" sample, updated 2018-06-28.
private func resolveAliases(_ url: URL) -> URL {
	if let bookmarkData = try? URL.bookmarkData(withContentsOf: url) {
		var isStale = false
		if let resolved = try? URL(resolvingBookmarkData: bookmarkData, options: .withSecurityScope, relativeTo: nil, bookmarkDataIsStale: &isStale), !isStale {
			return resolved
		}
	}
	return url
}

// Nodes are built on the search queue and handed to the main thread, and the
// FSEvents callback refreshes them from its run loop, as in the ObjC original;
// @unchecked Sendable describes that handoff.
@objc(PathNode)
class PathNode: NSObject, @unchecked Sendable {
	var dataSource: FileTreeDataSource?

	@objc var URL: URL? {
		didSet {
			guard let u = URL else { return }
			display = FileManager.default.displayName(atPath: u.path)
			lastPathComponent = (u.path as NSString).lastPathComponent
			let newIcon = NSWorkspace.shared.icon(forFile: u.path)
			newIcon.size = NSSize(width: 16.0, height: 16.0)
			icon = newIcon
		}
	}

	@objc var display: String?
	@objc var lastPathComponent: String?
	@objc var icon: NSImage?

	private var _subpaths: [Any]?
	private var _subpathsLookup: [String: PathNode]?

	@objc var subpaths: [Any]? {
		if _subpaths == nil {
			updatePath()
		}
		return _subpaths
	}

	@objc func setSubpaths(_ s: [Any]?) {
		_subpaths = s
		var lookup: [String: PathNode] = [:]
		for case let node as PathNode in s ?? [] {
			if let key = node.lastPathComponent {
				lookup[key] = node
			}
		}
		_subpathsLookup = lookup
	}

	@objc var subpathsLookup: [String: PathNode]? {
		_subpathsLookup
	}

	@objc init(dataSource ds: FileTreeDataSource?, url u: URL?) {
		super.init()
		dataSource = ds
		URL = u
	}

	@objc func updatePath() {
	}

	@objc func isLeaf() -> Bool {
		true
	}

	@objc func processPaths(_ contents: [Any]) {
		var newSubpathsDirs: [PathNode] = []
		var newSubpaths: [PathNode] = []

		for case let s as String in contents {
			guard let first = s.first, first != "." else {
				continue
			}

			var u = Foundation.URL(fileURLWithPath: s)
			let displayName = FileManager.default.displayName(atPath: u.path)

			u = resolveAliases(u)

			let newNode: PathNode
			var isDir: ObjCBool = false

			if (s as NSString).pathExtension.caseInsensitiveCompare("savedSearch") == .orderedSame {
				newNode = SmartFolderNode(dataSource: dataSource, url: u)
				isDir = false
			} else {
				FileManager.default.fileExists(atPath: u.path, isDirectory: &isDir)

				if !isDir.boolValue && !((AudioPlayer.fileTypes() as? [String]) ?? []).contains((u.pathExtension as NSString).lowercased) {
					continue
				}

				if isDir.boolValue {
					newNode = DirectoryNode(dataSource: dataSource, url: u)
				} else if ((AudioPlayer.containerTypes() as? [String]) ?? []).contains((u.pathExtension as NSString).lowercased) {
					newNode = ContainerNode(dataSource: dataSource, url: u)
				} else {
					newNode = FileNode(dataSource: dataSource, url: u)
				}
			}

			newNode.display = displayName

			if isDir.boolValue {
				newSubpathsDirs.append(newNode)
			} else {
				newSubpaths.append(newNode)
			}
		}

		setSubpaths(newSubpathsDirs + newSubpaths)
	}
}
