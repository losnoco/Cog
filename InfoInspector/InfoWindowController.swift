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

/// Owns the Info Inspector, in its HUD panel or docked. Instantiated by
/// MainMenu.xib, which connects the outlets below and targets
/// toggleWindow:/showWindow:.
@objc(InfoWindowController)
final class InfoWindowController: PaneWindowController {
	@IBOutlet var playlistSelectionController: NSArrayController?
	@IBOutlet var currentEntryController: NSObjectController?
	@IBOutlet var appController: AppController?

	private let model = InfoInspectorModel()
	private var observations: [NSKeyValueObservation] = []

	override var paneIdentifier: String {
		"infoInspector"
	}

	override var paneTitle: String {
		NSLocalizedString("Info Inspector", comment: "Info Inspector window title")
	}

	override var defaultPlacement: PanePlacement {
		PanePlacement(edge: .right, size: 300)
	}

	override var opensBesideMainWindow: Bool {
		true
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

	override func makePaneView() -> NSView {
		// Docked, the pane can be shorter than the inspector's layout.
		hostingView(ScrollView { InfoInspectorView(model: model) })
	}

	override func makeWindow() -> NSWindow {
		let panel = NSPanel(
			contentRect: NSRect(x: 0, y: 0, width: 300, height: 604),
			styleMask: [.titled, .closable, .resizable, .utilityWindow, .nonactivatingPanel, .hudWindow],
			backing: .buffered,
			defer: true
		)
		panel.title = paneTitle
		panel.titlebarAppearsTransparent = true
		panel.hidesOnDeactivate = true
		panel.isReleasedWhenClosed = false
		panel.animationBehavior = .utilityWindow
		panel.contentMinSize = NSSize(width: 240, height: 594)
		panel.contentMaxSize = NSSize(width: 400, height: 622)
		panel.setFrameAutosaveName("InfoInspector")
		return panel
	}
}
