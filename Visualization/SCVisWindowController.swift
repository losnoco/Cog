//
//  SCVisWindowController.swift
//  Cog
//
//  Created by Christopher Snowhill on 9/25/25.
//  Copyright 2025 Christopher Snowhill. All rights reserved.
//

import AppKit
import CogAudio

@objc(SCVisWindowController)
class SCVisWindowController: NSWindowController, NSWindowDelegate {
	@IBOutlet var playbackController: PlaybackController?

	private var sc55View: SCView?

	convenience init() {
		self.init(windowNibName: "SCVisWindow")
	}

	override func windowDidLoad() {
		super.windowDidLoad()

		startRunning()
	}

	private func startRunning() {
		if sc55View == nil {
			let view = SCView()
			sc55View = view
			if let window {
				var contentRect = view.bounds
				let frameRect = window.frame
				let originalContentRect = window.contentRect(forFrameRect: frameRect)
				contentRect.origin = originalContentRect.origin
				let windowFrame = window.frameRect(forContentRect: contentRect)
				window.setFrame(windowFrame, display: true, animate: true)
				window.minSize = windowFrame.size
				window.maxSize = windowFrame.size

				window.contentView = view
			}
		}

		if playbackController?.playbackStatus == .playing {
			sc55View?.startPlayback()
		}
	}

	private func stopRunning() {
		window?.contentView = nil
		sc55View = nil
	}

	func windowWillClose(_ notification: Notification) {
		if let currentWindow = notification.object as? NSWindow, currentWindow === window {
			stopRunning()
		}
	}

	@IBAction func toggleWindow(_ sender: Any?) {
		if let window, window.isVisible {
			window.orderOut(sender)
		} else {
			showWindow(sender)
		}
	}

	@IBAction override func showWindow(_ sender: Any?) {
		startRunning()
		super.showWindow(sender)
	}
}