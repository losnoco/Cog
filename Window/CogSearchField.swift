//
//  CogSearchField.swift
//  Cog
//
//  Created by Christopher Snowhill on 9/30/25.
//  Copyright 2025 Christopher Snowhill. All rights reserved.
//

import AppKit

@objc(CogSearchField)
class CogSearchField: NSSearchField, NSSearchFieldDelegate {
	@IBOutlet var mainWindow: NSWindow?

	private var borrowedWidth: NSLayoutConstraint?

	/// Customize Toolbar's palette takes this very field out of the toolbar to
	/// draw it, in windows of AppKit's own. If its width changes there, the search
	/// item lays out the toolbar while the item's view is away, and the toolbar
	/// raises an exception as it constrains that view to its place. That happens
	/// only with the field expanded, in a window wide enough for it. So the field
	/// keeps its width until it is back in the main window.
	override func viewWillMove(toWindow newWindow: NSWindow?) {
		if newWindow === mainWindow {
			borrowedWidth?.isActive = false
			borrowedWidth = nil
		} else if window === mainWindow && borrowedWidth == nil {
			let constraint = widthAnchor.constraint(equalToConstant: frame.width)
			constraint.isActive = true
			borrowedWidth = constraint
		}
		super.viewWillMove(toWindow: newWindow)
	}

	override func cancelOperation(_ sender: Any?) {
		mainWindow?.makeFirstResponder(nil)
		mainWindow?.resignFirstResponder()
	}
}