//
//  FileNode.swift
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

import Foundation

@objc(FileNode)
class FileNode: PathNode {
	override func isLeaf() -> Bool {
		true
	}
}
