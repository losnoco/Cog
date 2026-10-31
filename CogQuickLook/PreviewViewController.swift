//
//  PreviewViewController.swift
//  CogQuickLook
//
//  Quick Look's entry point: previews a file Cog plays, which macOS does
//  not preview itself, as a small player.
//

import AppKit
import Quartz
import SwiftUI

final class PreviewViewController: NSViewController, QLPreviewingController {
	private let player = PreviewPlayer()

	override func loadView() {
		view = NSHostingView(rootView: PreviewView(player: player))
		preferredContentSize = NSSize(width: 480, height: 168)
	}

	func preparePreviewOfFile(at url: URL) async throws {
		await player.load(url)
	}

	override func viewWillDisappear() {
		super.viewWillDisappear()
		// The extension outlives the preview; the music should not.
		player.stop()
	}
}
