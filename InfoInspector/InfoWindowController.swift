//
//  InfoWindowController.swift
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Rewritten in SwiftUI by Kevin López Brante on 2026-10-02.
//

import Cocoa
import CogPlaylist
import SwiftUI

/// Owns the Info Inspector HUD panel. Instantiated by MainMenu.xib, which
/// connects the outlets below and targets toggleWindow:/showWindow:.
@objc(InfoWindowController)
final class InfoWindowController: NSWindowController {
	@IBOutlet var playlistSelectionController: NSArrayController?
	@IBOutlet var currentEntryController: NSObjectController?
	@IBOutlet var appController: AppController?

	private let model = InfoInspectorModel()
	private var observations: [NSKeyValueObservation] = []

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
			let update: @Sendable (Any, Any) -> Void = { [weak self] _, _ in
				MainActor.assumeIsolated { self?.updateEntry() }
			}
			if let playlistSelectionController {
				observations.append(playlistSelectionController.observe(\.selectionIndexes) { update($0, $1) })
			}
			if let currentEntryController {
				observations.append(currentEntryController.observe(\.content) { update($0, $1) })
			}
			if let appController {
				observations.append(appController.observe(\.miniMode) { update($0, $1) })
			}
			updateEntry()
		}
	}

	private func updateEntry() {
		// Prefer the playlist selection, falling back to the playing track.
		// Avoid "selection" because it creates a proxy that's hard to reason
		// with when we don't need to write.
		let selected = playlistSelectionController?.selectedObjects.first as? PlaylistEntry
		let entry = selected ?? currentEntryController?.content as? PlaylistEntry
		if model.entry !== entry {
			model.entry = entry
		}
	}

	override func loadWindow() {
		let panel = NSPanel(
			contentRect: NSRect(x: 0, y: 0, width: 300, height: 604),
			styleMask: [.titled, .closable, .resizable, .utilityWindow, .nonactivatingPanel, .hudWindow],
			backing: .buffered,
			defer: true
		)
		panel.title = NSLocalizedString("Info Inspector", comment: "Info Inspector window title")
		panel.titlebarAppearsTransparent = true
		panel.hidesOnDeactivate = true
		panel.isReleasedWhenClosed = false
		panel.animationBehavior = .utilityWindow
		panel.contentMinSize = NSSize(width: 240, height: 594)
		panel.contentMaxSize = NSSize(width: 400, height: 622)
		panel.contentView = NSHostingView(rootView: InfoInspectorView(model: model))
		panel.setFrameAutosaveName("InfoInspector")
		window = panel
	}

	override func showWindow(_ sender: Any?) {
		// Without a nib, NSWindowController never calls loadWindow() itself.
		if window == nil {
			loadWindow()
		}
		super.showWindow(sender)
	}

	@IBAction func toggleWindow(_ sender: Any?) {
		if window == nil {
			loadWindow()
		}
		if window?.isVisible == true {
			window?.orderOut(self)
		} else {
			if let mainFrame = NSApp.mainWindow?.frame {
				// Align the Info Inspector to the right of the main window.
				window?.setFrameTopLeftPoint(NSPoint(x: mainFrame.maxX, y: mainFrame.maxY))
			}
			showWindow(self)
		}
	}
}
