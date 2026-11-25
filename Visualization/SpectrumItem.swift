//
//  SpectrumItem.swift
//  Cog
//
//  Created by Christopher Snowhill on 2/13/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

import AppKit
import CogAudio

// Space either side of the spectrum, so it doesn't touch its neighbours.
private let spectrumItemPadding: CGFloat = 4.0

/// Holds the spectrum, padded, and takes its clicks: the item opens the
/// spectrum window.
private class SpectrumItemView: NSView {
	var onClick: (() -> Void)?

	override func hitTest(_ point: NSPoint) -> NSView? {
		return NSPointInRect(point, frame) ? self : nil
	}

	override func mouseDown(with event: NSEvent) {
		onClick?()
	}
}

@objc(SpectrumItem)
class SpectrumItem: NSToolbarItem {
	private static let observationContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	@IBOutlet var playbackController: PlaybackController?
	@IBOutlet var windowController: NSWindowController?

	private var spectrumViewSK: SpectrumViewSK?
	private var spectrumViewCG: SpectrumViewCG?

	override func awakeFromNib() {
		startRunning()

		NSUserDefaultsController.shared.addObserver(self, forKeyPath: "values.spectrumSceneKit", options: [], context: Self.observationContext)
	}

	private func startRunning() {
		if spectrumViewSK == nil && spectrumViewCG == nil {
			let frame = NSRect(x: 0, y: 0, width: 64, height: 26)
			if let skView = SpectrumViewSK.createGuard(withFrame: frame) {
				spectrumViewSK = skView
				setPaddedView(skView)
			} else {
				let cgView = SpectrumViewCG(frame: frame)
				spectrumViewCG = cgView
				setPaddedView(cgView)
			}
		}

		if playbackController?.playbackStatus == .playing {
			if let spectrumViewSK {
				spectrumViewSK.startPlayback()
			} else if let spectrumViewCG {
				spectrumViewCG.startPlayback()
			}
		}
	}

	private func setPaddedView(_ view: NSView) {
		let frame = view.frame
		let container = SpectrumItemView(frame: NSRect(x: 0, y: 0, width: frame.width + 2 * spectrumItemPadding, height: frame.height))
		view.frame = frame.offsetBy(dx: spectrumItemPadding, dy: 0)
		container.addSubview(view)
		container.toolTip = NSLocalizedString("Spectrum. Click to open the spectrum.", comment: "Spectrum toolbar item tooltip")
		container.onClick = { [weak self] in
			self?.windowController?.showWindow(nil)
		}
		self.view = container
	}

	deinit {
		NSUserDefaultsController.shared.removeObserver(self, forKeyPath: "values.spectrumSceneKit", context: Self.observationContext)
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.observationContext {
			if keyPath == "values.spectrumSceneKit" {
				spectrumViewSK = nil
				spectrumViewCG = nil
				startRunning()
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}
}