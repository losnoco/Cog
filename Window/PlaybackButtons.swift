//
//  PlaybackButtons.swift
//  Cog
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 Vincent Spader. All rights reserved.
//

import AppKit
import CogAudio

@objc(PlaybackButtons)
class PlaybackButtons: NSSegmentedControl {
	private static let observationContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	@IBOutlet var playbackController: PlaybackController?

	// The controller observed, for deinit to let go of; only touched on the
	// main thread.
	nonisolated(unsafe) private var observedPlaybackController: PlaybackController?

	private var playImage: NSImage?
	private var pauseImage: NSImage?
	private var stopImage: NSImage?
	private var prevImage: NSImage?
	private var nextImage: NSImage?

	deinit {
		observedPlaybackController?.removeObserver(self, forKeyPath: "playbackStatus")
		NSUserDefaultsController.shared.removeObserver(self, forKeyPath: "values.enableStopButton")
	}

	override func awakeFromNib() {
		startObserving()

		playImage = NSImage(systemSymbolName: "play.fill", accessibilityDescription: NSLocalizedString("PlayButtonTooltip", comment: ""))
		pauseImage = NSImage(systemSymbolName: "pause.fill", accessibilityDescription: NSLocalizedString("PauseButtonTooltip", comment: ""))
		stopImage = NSImage(systemSymbolName: "stop.fill", accessibilityDescription: NSLocalizedString("StopButtonTooltip", comment: ""))
		prevImage = NSImage(systemSymbolName: "backward.fill", accessibilityDescription: NSLocalizedString("PrevButtonTooltip", comment: ""))
		nextImage = NSImage(systemSymbolName: "forward.fill", accessibilityDescription: NSLocalizedString("NextButtonTooltip", comment: ""))

		setImage(prevImage, forSegment: 0)
		setImage(playImage, forSegment: 1)

		updateStopSegment()
	}

	private func startObserving() {
		observedPlaybackController = playbackController
		playbackController?.addObserver(self, forKeyPath: "playbackStatus", options: [.new, .initial], context: Self.observationContext)
		NSUserDefaultsController.shared.addObserver(self, forKeyPath: "values.enableStopButton", options: [.new, .initial], context: Self.observationContext)
	}

	private func updateStopSegment() {
		let segmentEnabled = UserDefaults.standard.bool(forKey: "enableStopButton")
		if segmentEnabled {
			segmentCount = 4
			setImage(stopImage, forSegment: 2)
			setImage(nextImage, forSegment: 3)
			setToolTip(NSLocalizedString("StopButtonTooltip", comment: ""), forSegment: 2)
			setToolTip(NSLocalizedString("NextButtonTooltip", comment: ""), forSegment: 3)
		} else {
			segmentCount = 3
			setImage(nextImage, forSegment: 2)
			setToolTip(NSLocalizedString("NextButtonTooltip", comment: ""), forSegment: 2)
		}
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.observationContext {
			if keyPath == "playbackStatus" {
				let playbackStatus = (change?[.newKey] as? NSNumber)?.intValue ?? 0

				let image = playbackStatus == CogStatus.playing.rawValue ? pauseImage : playImage

				setImage(image, forSegment: 1)
			} else if keyPath == "values.enableStopButton" {
				updateStopSegment()
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	override func sendAction(_ action: Selector?, to target: Any?) -> Bool {
		#if DEBUG
			NSLog("Mouse down!")
		#endif

		var clickedSegment = selectedSegment
		if segmentCount == 3 && clickedSegment == 2 {
			clickedSegment = 3
		}

		if clickedSegment == 0 { // Previous
			playbackController?.prev(self)
		} else if clickedSegment == 1 { // Play
			playbackController?.playPauseResume(self)
		} else if clickedSegment == 2 { // Stop
			playbackController?.stop(self)
		} else if clickedSegment == 3 { // Next
			playbackController?.next(self)
		} else {
			return false
		}

		return true
	}
}