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
	var playback: LyricsPlayback?
}

/// Owns the lyrics, in their window or docked. Instantiated by MainMenu.xib,
/// which connects the outlets below and targets toggleWindow:.
@objc(LyricsWindowController)
final class LyricsWindowController: PaneWindowController {
	@IBOutlet var playlistSelectionController: NSArrayController?
	@IBOutlet var currentEntryController: NSObjectController?
	@IBOutlet var appController: AppController?

	private let model = LyricsWindowModel()
	private var observations: [NSKeyValueObservation] = []

	override var paneIdentifier: String {
		"lyrics"
	}

	override var paneTitle: String {
		NSLocalizedString("Lyrics", comment: "Lyrics window title")
	}

	override var defaultPlacement: PanePlacement {
		PanePlacement(edge: .right, position: 2, size: 300)
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
			observations.append(UserDefaults.standard.observe(\.trackInfoSource) { update($0, $1) })
		}
	}

	private func updateEntry() {
		guard isPaneOpen else {
			model.entry = nil
			return
		}
		// Avoid "selection" because it creates a proxy that's hard to reason
		// with when we don't need to write.
		let entry = trackToShow(selected: playlistSelectionController?.selectedObjects.first as? PlaylistEntry,
		                        playing: currentEntryController?.content as? PlaylistEntry)
		if model.entry !== entry {
			model.entry = entry
		}
	}

	/// Follows the track playing, which the selection may not be.
	private var playback: LyricsPlayback {
		LyricsPlayback(position: { [weak self] entry in
			guard let self, self.currentEntryController?.content as? PlaylistEntry === entry else { return nil }
			return self.appController?.playbackController.audioPlayer().amountPlayed()
		}, seek: { [weak self] seconds in
			self?.appController?.playbackController.seek(self, toTime: seconds)
		})
	}

	override func makePaneView() -> NSView {
		model.playback = playback
		return hostingView(LyricsWindowView(model: model))
	}

	override func makeWindow() -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(x: 196, y: 240, width: 480, height: 270),
			styleMask: [.titled, .closable, .miniaturizable, .resizable],
			backing: .buffered,
			defer: true
		)
		window.title = paneTitle
		window.isReleasedWhenClosed = false
		window.contentMinSize = NSSize(width: 240, height: 135)
		window.setFrameAutosaveName("LyricsWindow")
		return window
	}

	override func paneVisibilityDidChange() {
		updateEntry()
	}
}

private struct LyricsWindowView: View {
	@ObservedObject var model: LyricsWindowModel

	var body: some View {
		if let entry = model.entry {
			LyricsText(entry: entry, font: .body, playback: model.playback)
		} else {
			Color.clear
		}
	}
}
