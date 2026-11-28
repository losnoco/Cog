//
//  FileTreeOutlineView.swift
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

import AppKit

@objc(FileTreeOutlineView)
class FileTreeOutlineView: NSOutlineView {
	override func awakeFromNib() {
		doubleAction = #selector(FileTreeController.addToPlaylistExternal(_:))
		target = delegate
	}

	override func keyDown(with event: NSEvent) {
		let modifiers = event.modifierFlags.intersection([.command, .shift, .control, .option])
		let characters = event.characters ?? ""

		if characters.count == 1 {
			let c = characters.utf16.first!

			if modifiers.isEmpty && (c == UInt16(NSEnterCharacter) || c == UInt16(NSCarriageReturnCharacter)) {
				(delegate as? FileTreeController)?.addToPlaylistExternal(self)
				return
			} else if modifiers.isEmpty && c == UInt16(UnicodeScalar(" ").value) {
				(delegate as? FileTreeController)?.playPauseResume(self)
				return
			}
		}

		super.keyDown(with: event)
	}

	// enables right-click selection for "Show in Finder" contextual menu
	override func menu(for event: NSEvent) -> NSMenu? {
		// Find which row is under the cursor
		window?.makeFirstResponder(self)
		let menuPoint = convert(event.locationInWindow, from: nil)
		let iRow = row(at: menuPoint)
		let contextMenu = menu

		/* Update the file tree selection before showing menu
		 Preserves the selection if the row under the mouse is selected (to allow for
		 multiple items to be selected), otherwise selects the row under the mouse */
		let currentRowIsSelected = selectedRowIndexes.contains(iRow)

		if iRow == -1 {
			deselectAll(self)
		} else if !currentRowIsSelected {
			selectRowIndexes(IndexSet(integer: iRow), byExtendingSelection: false)
		}

		return contextMenu
	}
}
