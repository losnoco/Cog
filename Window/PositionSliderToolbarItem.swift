//
//  PositionSliderToolbarItem.swift
//  Cog
//
//  Created by Dzmitry Neviadomski on 7.01.21.
//

import Cocoa

class PositionSliderToolbarItem: NSToolbarItem {
    override var minSize: NSSize{
        get {
            return NSSize(width: 100, height: 28)
        }
        set {}
    }
	override var maxSize: NSSize{
		get {
			return NSSize(width: 1024, height: 28)
		}
		set {}
	}
}
