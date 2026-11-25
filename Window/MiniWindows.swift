//
//  MiniWindows.swift
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//
//  The mini and mini-plus windows share their keyboard shortcuts and
//  drag-and-drop; only their sizing and frame autosave differ.
//

import AppKit
import Carbon

private let iTunesDrop = NSPasteboard.PasteboardType(iTunesDropType)

class MiniWindowBase: NSWindow, NSDraggingDestination {
	@IBOutlet var playbackController: PlaybackController?
	@IBOutlet var playlistController: PlaylistController?

	func configureMiniWindow() {
		isExcludedFromWindowsMenu = true
		standardWindowButton(.zoomButton)?.isEnabled = false
		collectionBehavior = .fullScreenAuxiliary
	}

	override func awakeFromNib() {
		super.awakeFromNib()
		registerForDraggedTypes([.fileURL, iTunesDrop])
	}

	override func toggleToolbarShown(_ sender: Any?) {
		// The mini window IS the toolbar; the plus window's toolbar holds the
		// controls. Neither should hide it.
	}

	override func keyDown(with event: NSEvent) {
		let modifiers: NSEvent.ModifierFlags = [.shift, .control, .option, .command]
		if !event.modifierFlags.intersection(modifiers).isEmpty {
			super.keyDown(with: event)
			return
		}

		switch Int(event.keyCode) {
		case kVK_Space:
			playbackController?.playPauseResume(self)
		case kVK_Return:
			playbackController?.play(self)
		case kVK_LeftArrow:
			playbackController?.eventSeekBackward(self)
		case kVK_RightArrow:
			playbackController?.eventSeekForward(self)
		case kVK_UpArrow:
			playbackController?.volumeUp(self)
		case kVK_DownArrow:
			playbackController?.volumeDown(self)
		default:
			super.keyDown(with: event)
		}
	}

	private func dragOperation(for sender: NSDraggingInfo) -> NSDragOperation {
		let types = sender.draggingPasteboard.types ?? []
		let mask = sender.draggingSourceOperationMask
		if (types.contains(iTunesDrop) || types.contains(.fileURL)) && mask.contains(.generic) {
			return .generic
		}
		return []
	}

	func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
		return dragOperation(for: sender)
	}

	func draggingUpdated(_ sender: NSDraggingInfo) -> NSDragOperation {
		return dragOperation(for: sender)
	}

	func prepareForDragOperation(_ sender: NSDraggingInfo) -> Bool {
		return !dragOperation(for: sender).isEmpty
	}

	func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
		let types = sender.draggingPasteboard.types ?? []
		guard types.contains(iTunesDrop) || types.contains(.fileURL),
		      let playlistController, let tableView = playlistController.tableView
		else {
			return false
		}
		let row = (playlistController.arrangedObjects as? NSArray)?.count ?? 0
		return playlistController.tableView(tableView, acceptDrop: sender, row: row, dropOperation: .on)
	}
}

@objc(MiniWindow)
class MiniWindow: MiniWindowBase {
	@IBOutlet var miniToolbar: NSToolbar?

	override init(contentRect: NSRect, styleMask: NSWindow.StyleMask, backing: NSWindow.BackingStoreType, defer flag: Bool) {
		super.init(contentRect: contentRect, styleMask: styleMask, backing: backing, defer: flag)
		showsResizeIndicator = false
		configureMiniWindow()
		// Disallow height resize.
		contentMinSize = NSSize(width: 325, height: 1)
		contentMaxSize = NSSize(width: CGFloat.greatestFiniteMagnitude, height: 1)
		setFrameUsingName("Mini Window")
	}

	override func awakeFromNib() {
		super.awakeFromNib()
		if UserDefaults.standard.bool(forKey: "miniMode") {
			showSentryConsent(self)
		}
	}
}

@objc(MiniPlayerPlusWindow)
class MiniPlayerPlusWindow: MiniWindowBase {
	override init(contentRect: NSRect, styleMask: NSWindow.StyleMask, backing: NSWindow.BackingStoreType, defer flag: Bool) {
		super.init(contentRect: contentRect, styleMask: styleMask, backing: backing, defer: flag)
		showsResizeIndicator = true
		configureMiniWindow()
		contentMinSize = NSSize(width: 240, height: 200)
		setFrameUsingName("Mini Plus Window")
	}
}