//
//  AboutWIndowController.swift
//  Cog
//
//  Created by Kevin López Brante on 20-06-22.
//

import Cocoa
import SwiftUI

/// One window with two faces. MainMenu.xib sends "About Cog" to
/// `showWindow(_:)` for the card, and that item's Option alternate to
/// `showDetails(_:)`.
class AboutWindowController: NSWindowController, NSWindowDelegate {
	private let model = AboutModel()

	static let cardSize = NSSize(width: 320, height: 420)
	static let detailsMinSize = NSSize(width: 640, height: 420)
	/// The details face keeps the size it was last given, for as long as
	/// the app runs.
	private var detailsSize = NSSize(width: 760, height: 560)

	/// Nil until the window has been laid out for a mode.
	private var configuredMode: AboutMode?

	override func loadWindow() {
		let hostingController = NSHostingController(rootView: AboutView(model: model))
		if #available(macOS 13.0, *) {
			// The window's size follows the mode, not the SwiftUI fitting size.
			hostingController.sizingOptions = []
		}
		let window = NSWindow(
			contentRect: NSRect(origin: .zero, size: Self.cardSize),
			styleMask: [.titled, .closable],
			backing: .buffered,
			defer: false
		)
		window.contentViewController = hostingController
		window.title = NSLocalizedString("About Cog", comment: "About window title")
		window.isReleasedWhenClosed = false
		// The details face is resizable, but it neither zooms nor goes full
		// screen.
		window.collectionBehavior = [.fullScreenNone]
		window.delegate = self
		self.window = window

		model.showDetails = { [weak self] tab in
			self?.show(.details, tab: tab)
		}
	}

	override func showWindow(_ sender: Any?) {
		show(.card, tab: nil)
	}

	@IBAction func showDetails(_ sender: Any?) {
		show(.details, tab: nil)
	}

	private func show(_ mode: AboutMode, tab: AboutTab?) {
		// Without a nib, NSWindowController never calls loadWindow() itself.
		if window == nil {
			loadWindow()
		}
		guard let window else { return }
		if let tab {
			model.tab = tab
		}
		if configuredMode != mode {
			configure(window, for: mode)
		}
		super.showWindow(nil)
	}

	/// Covers a double click on the title bar as well as the button.
	func windowShouldZoom(_ window: NSWindow, toFrame newFrame: NSRect) -> Bool {
		false
	}

	private func configure(_ window: NSWindow, for mode: AboutMode) {
		if configuredMode == .details {
			detailsSize = window.contentRect(forFrameRect: window.frame).size
		}
		let first = configuredMode == nil
		configuredMode = mode
		model.mode = mode

		let size: NSSize
		switch mode {
		case .card:
			window.styleMask = [.titled, .closable]
			window.titleVisibility = .hidden
			window.titlebarAppearsTransparent = true
			window.isMovableByWindowBackground = true
			window.contentMinSize = Self.cardSize
			window.contentMaxSize = Self.cardSize
			size = Self.cardSize
		case .details:
			window.styleMask = [.titled, .closable, .miniaturizable, .resizable]
			window.titleVisibility = .visible
			window.titlebarAppearsTransparent = false
			window.isMovableByWindowBackground = false
			window.contentMinSize = Self.detailsMinSize
			window.contentMaxSize = NSSize(width: CGFloat.greatestFiniteMagnitude, height: CGFloat.greatestFiniteMagnitude)
			size = detailsSize
		}

		// Changing the style mask can bring the button back enabled.
		window.standardWindowButton(.zoomButton)?.isEnabled = false

		var frame = window.frameRect(forContentRect: NSRect(origin: .zero, size: size))
		if first || !window.isVisible {
			window.setFrame(frame, display: false)
			window.center()
			return
		}
		// Keep the title bar's center where it is, as far as the screen allows.
		let old = window.frame
		frame.origin = NSPoint(x: old.midX - frame.width / 2, y: old.maxY - frame.height)
		if let visible = window.screen?.visibleFrame {
			frame.origin.x = max(visible.minX, min(frame.origin.x, visible.maxX - frame.width))
			frame.origin.y = max(visible.minY, min(frame.origin.y, visible.maxY - frame.height))
		}
		// The new face would otherwise be laid out in the old one's size and
		// stretch along with the window, so it stays hidden until the window
		// has its new size.
		let content = window.contentView
		content?.alphaValue = 0
		window.setFrame(frame, display: true, animate: true)
		NSAnimationContext.runAnimationGroup { context in
			context.duration = 0.15
			content?.animator().alphaValue = 1
		}
	}
}
