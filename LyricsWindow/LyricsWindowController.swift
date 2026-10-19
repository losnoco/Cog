//
//  LyricsWindowController.swift
//  Cog
//
//  Created by Christopher Snowhill on 2/23/23.
//  Rewritten in SwiftUI by Kevin López Brante on 2026-10-19.
//

import Cocoa
import CogPlaylist
import SwiftUI

/// The entry whose lyrics show: nil while the window is hidden, so LRCLIB
/// is only asked while they can be read.
@MainActor
final class LyricsWindowModel: ObservableObject {
	@Published var entry: PlaylistEntry?
}

/// Owns the lyrics window. Instantiated by MainMenu.xib, which connects the
/// outlets below and targets toggleWindow:.
@objc(LyricsWindowController)
final class LyricsWindowController: NSWindowController, NSWindowDelegate {
	@IBOutlet var playlistSelectionController: NSArrayController?
	@IBOutlet var currentEntryController: NSObjectController?
	@IBOutlet var appController: AppController?

	private let model = LyricsWindowModel()
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
		}
	}

	private func updateEntry() {
		guard window?.isVisible == true else {
			model.entry = nil
			return
		}
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
		let window = NSWindow(
			contentRect: NSRect(x: 196, y: 240, width: 480, height: 270),
			styleMask: [.titled, .closable, .miniaturizable, .resizable],
			backing: .buffered,
			defer: true
		)
		window.title = NSLocalizedString("Lyrics", comment: "Lyrics window title")
		window.isReleasedWhenClosed = false
		window.contentMinSize = NSSize(width: 240, height: 135)
		window.contentView = NSHostingView(rootView: LyricsWindowView(model: model))
		window.setFrameAutosaveName("LyricsWindow")
		window.delegate = self
		self.window = window
	}

	func windowWillClose(_ notification: Notification) {
		model.entry = nil
	}

	override func showWindow(_ sender: Any?) {
		// Without a nib, NSWindowController never calls loadWindow() itself.
		if window == nil {
			loadWindow()
		}
		super.showWindow(sender)
		updateEntry()
	}

	@IBAction func toggleWindow(_ sender: Any?) {
		if window == nil {
			loadWindow()
		}
		if window?.isVisible == true {
			window?.orderOut(self)
			updateEntry()
		} else {
			if let mainFrame = NSApp.mainWindow?.frame {
				// Align the lyrics window to the right of the main window.
				window?.setFrameTopLeftPoint(NSPoint(x: mainFrame.maxX, y: mainFrame.maxY))
			}
			showWindow(self)
		}
	}
}

private struct LyricsWindowView: View {
	@ObservedObject var model: LyricsWindowModel

	var body: some View {
		if let entry = model.entry {
			LyricsText(entry: entry, font: .body)
		} else {
			Color.clear
		}
	}
}
