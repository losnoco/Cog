//
//  PaneWindowController.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-22.
//

import Cocoa
import SwiftUI

/// A window controller whose content is a pane: it shows in the controller's
/// own window, floating, or docked in the main window by `PaneManager`.
///
/// Subclasses make the content and the window, and do their work (metering,
/// drawing, asking LRCLIB) only while `isPaneVisible`, checking it again in
/// `paneVisibilityDidChange()`.
@MainActor
class PaneWindowController: NSWindowController, NSWindowDelegate {
	/// Names the pane in the saved layout.
	var paneIdentifier: String {
		preconditionFailure("Subclasses name their pane")
	}

	/// The caption over the docked pane.
	var paneTitle: String {
		preconditionFailure("Subclasses title their pane")
	}

	/// Where the pane docks the first time.
	var defaultPlacement: PanePlacement {
		PanePlacement(edge: .right, size: 300)
	}

	/// The docked pane's appearance, for content drawn for a dark window.
	var paneAppearance: NSAppearance? {
		nil
	}

	/// Shown beside the main window, as the inspectors open.
	var opensBesideMainWindow: Bool {
		false
	}

	/// The content, made when first shown. It moves between the window and
	/// the dock.
	var paneView: NSView {
		if let madePaneView {
			return madePaneView
		}
		let view = makePaneView()
		madePaneView = view
		return view
	}

	private var madePaneView: NSView?

	private var occlusionObserver: NSObjectProtocol?
	private var dockButton: NSButton?

	@objc init() {
		super.init(window: nil)
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}

	override func awakeFromNib() {
		super.awakeFromNib()
		// Nib loading happens on the main thread.
		MainActor.assumeIsolated {
			PaneManager.shared.register(self)
		}
	}

	/// The pane's content.
	func makePaneView() -> NSView {
		preconditionFailure("Subclasses make their pane's content")
	}

	/// The floating window, without content: it is given `paneView`.
	func makeWindow() -> NSWindow {
		preconditionFailure("Subclasses make their pane's window")
	}

	/// The pane may have been shown, hidden, covered, docked or floated.
	func paneVisibilityDidChange() {}

	/// Whether the pane is showing: in a window on screen, though perhaps
	/// covered.
	var isPaneOpen: Bool {
		guard let view = madePaneView, let host = view.window else { return false }
		return host.isVisible && !view.isHiddenOrHasHiddenAncestor
	}

	/// Whether the pane can be seen: showing and not covered.
	var isPaneVisible: Bool {
		isPaneOpen && madePaneView?.window?.occlusionState.contains(.visible) == true
	}

	/// A hosting view that leaves its size to the window or the dock.
	func hostingView(_ rootView: some View) -> NSView {
		let view = NSHostingView(rootView: rootView)
		if #available(macOS 13.0, *) {
			view.sizingOptions = []
		}
		return view
	}

	// MARK: - The floating window

	override func loadWindow() {
		let window = makeWindow()
		window.delegate = self
		self.window = window
		let accessory = NSTitlebarAccessoryViewController()
		accessory.view = makeDockButton()
		accessory.layoutAttribute = .trailing
		window.addTitlebarAccessoryViewController(accessory)
		occlusionObserver = NotificationCenter.default.addObserver(forName: NSWindow.didChangeOcclusionStateNotification, object: window, queue: .main) { [weak self] _ in
			MainActor.assumeIsolated { self?.paneVisibilityDidChange() }
		}
	}

	private func makeDockButton() -> NSView {
		let button = NSButton(image: NSImage(), target: self, action: #selector(dockInMainWindow(_:)))
		button.bezelStyle = .texturedRounded
		button.isBordered = false
		button.toolTip = NSLocalizedString("Dock in Main Window", comment: "Pane window: the titlebar button that docks it")
		button.frame = NSRect(x: 0, y: 0, width: 28, height: 22)
		dockButton = button
		updateDockButton()
		return button
	}

	/// Shows the edge the pane would dock to.
	private func updateDockButton() {
		dockButton?.image = NSImage(systemSymbolName: PanePlacement.symbolName(PaneManager.shared.placement(of: self).edge),
		                            accessibilityDescription: nil)
	}

	func windowWillMove(_ notification: Notification) {
		PaneManager.shared.paneWindowWillMove(self)
	}

	@objc private func dockInMainWindow(_ sender: Any?) {
		PaneManager.shared.dock(self)
	}

	/// Puts the content back in the window and shows it, beside the main
	/// window for an inspector that was hidden.
	func showFloating(besideMainWindow: Bool = true) {
		if window == nil {
			loadWindow()
		}
		if window?.contentView !== paneView {
			window?.contentView = paneView
		}
		updateDockButton()
		if besideMainWindow, window?.isVisible != true, opensBesideMainWindow, let mainFrame = NSApp.mainWindow?.frame {
			window?.setFrameTopLeftPoint(NSPoint(x: mainFrame.maxX, y: mainFrame.maxY))
		}
		super.showWindow(nil)
		paneVisibilityDidChange()
	}

	/// Shows the window with the middle of its titlebar under `point`, in
	/// screen coordinates and clear of its buttons, as a pane is pulled out of
	/// the dock.
	func showFloating(under point: NSPoint) {
		showFloating(besideMainWindow: false)
		if let window {
			let frame = window.frame
			window.setFrameOrigin(NSPoint(x: point.x - frame.width / 2, y: point.y + 14 - frame.height))
		}
	}

	/// Hides the window, keeping its content where it is.
	func hideFloating() {
		window?.orderOut(nil)
		paneVisibilityDidChange()
	}

	/// The content left for the dock: the window keeps an empty view.
	func releaseFromWindow() {
		window?.orderOut(nil)
		if let madePaneView, window?.contentView === madePaneView {
			window?.contentView = NSView()
		}
	}

	override func showWindow(_ sender: Any?) {
		if PaneManager.shared.show(self) {
			return
		}
		showFloating()
	}

	@IBAction func toggleWindow(_ sender: Any?) {
		if PaneManager.shared.toggle(self) {
			return
		}
		if window?.isVisible == true {
			hideFloating()
		} else {
			showFloating()
		}
	}

	func windowWillClose(_ notification: Notification) {
		// The window is still visible until it has closed.
		DispatchQueue.main.async { [weak self] in
			self?.paneVisibilityDidChange()
		}
	}
}
