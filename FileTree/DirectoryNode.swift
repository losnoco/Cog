//
//  DirectoryNode.swift
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

import AppKit

@objc(DirectoryNode)
class DirectoryNode: PathNode {
	override func isLeaf() -> Bool {
		false
	}

	override func updatePath() {
		guard let url = URL else { return }

		guard let enumerator = FileManager.default.enumerator(at: url,
		                                                      includingPropertiesForKeys: [.nameKey, .isDirectoryKey],
		                                                      options: [.skipsSubdirectoryDescendants, .skipsPackageDescendants, .skipsHiddenFiles],
		                                                      errorHandler: { _, _ in false }) else {
			return
		}
		var fullPaths: [String] = []
		for case let theURL as URL in enumerator {
			fullPaths.append(theURL.path)
		}

		processPaths(fullPaths.sorted { ($0 as NSString).finderCompare($1) == .orderedAscending })
	}
}
