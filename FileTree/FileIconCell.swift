//
//  FileIconCell.swift
//  Cog
//
//  Created by Vincent Spader on 8/20/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

import AppKit

@objc(FileIconCell)
class FileIconCell: ImageTextCell {
	override var objectValue: Any? {
		get { super.objectValue }
		set {
			if let node = newValue as? PathNode {
				super.objectValue = node.display
				super.setImage(node.icon)
			} else {
				super.objectValue = newValue
			}
		}
	}
}
