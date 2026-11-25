//
//  VolumeSlider.swift
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

import AppKit
import CogAudio

@objc(VolumeSlider)
class VolumeSlider: NSSlider {
	private static let context = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	private var popover: NSPopover?
	private var textView: NSTextField?
	private var maxVolume: Double = 800
	private var currentTimer: Timer?
	private var wasInsideSnapRange = false
	private var observersAdded = false
	private var scrollDeltaRemainder: Double = 0
	nonisolated(unsafe) private var defaultsController: NSUserDefaultsController?

	override func awakeFromNib() {
		let volumeLimit = UserDefaults.standard.bool(forKey: "volumeLimit")
		maxVolume = volumeLimit ? 100.0 : 800.0

		wasInsideSnapRange = false
		// A label centred in a view of a set size. An NSText sized itself to its
		// line of text, and the popover then held it low in the bubble.
		let label = NSTextField(labelWithString: "")
		label.alignment = .center
		// Even digits keep the text still as the value changes.
		label.font = NSFont.monospacedDigitSystemFont(ofSize: 12, weight: .regular)
		label.translatesAutoresizingMaskIntoConstraints = false
		textView = label

		let contentView = NSView(frame: NSRect(x: 0, y: 0, width: 50, height: 20))
		contentView.addSubview(label)
		NSLayoutConstraint.activate([
			label.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
			label.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
			label.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
		])

		let viewController = NSViewController()
		viewController.view = contentView

		let pop = NSPopover()
		pop.contentViewController = viewController
		// Hidden on a timer. Transient, before macOS 26, the click that closed it
		// was sometimes dropped, so a click on the slider while it showed did
		// nothing.
		pop.behavior = .applicationDefined
		pop.animates = false
		pop.contentSize = contentView.bounds.size
		popover = pop

		// The toolbar sizes the item by the slider's constraints, and a slider has
		// no width of its own: on macOS 15 it was squeezed to just its knob.
		// With a minimum, the toolbar moves it to its overflow menu instead.
		widthAnchor.constraint(greaterThanOrEqualToConstant: 64).isActive = true
		let preferredWidth = widthAnchor.constraint(equalToConstant: 96)
		preferredWidth.priority = .defaultLow
		preferredWidth.isActive = true

		let defaults = NSUserDefaultsController.shared
		defaultsController = defaults
		defaults.addObserver(self, forKeyPath: "values.volumeLimit", options: [], context: Self.context)
		observersAdded = true
	}

	deinit {
		if observersAdded {
			defaultsController?.removeObserver(self, forKeyPath: "values.volumeLimit", context: Self.context)
		}
	}

	private func updateToolTip() {
		textView?.stringValue = String(format: "%.0lf%%", round(displayedVolume))
	}

	private var displayedVolume: Double {
		let value = doubleValue
		return maxVolume == 100 ? value : linearToLogarithmic(value, maxVolume)
	}

	private func setDisplayedVolume(_ volume: Double) {
		let volume = max(0.0, min(volume, maxVolume))
		let value = maxVolume == 100 ? volume : logarithmicToLinear(volume, maxVolume)
		doubleValue = value
	}

	private func snapToVolumeStep() {
		setDisplayedVolume(round(displayedVolume))
	}

	@objc func showToolTip() {
		updateToolTip()

		let range = maxValue - minValue
		let progress = range == 0 ? 0 : (doubleValue - minValue) / range
		let knobCenter = knobThickness / 2 + (bounds.size.width - knobThickness) * progress
		// From the slider's edge, not the knob's middle, so it covers less of
		// the knob, and it lets clicks through to what it does cover.
		let anchor = NSRect(x: knobCenter - 1, y: bounds.minY, width: 2, height: bounds.height)

		popover?.show(relativeTo: anchor, of: self, preferredEdge: .maxY)
		popover?.contentViewController?.view.window?.ignoresMouseEvents = true
		window?.parent?.makeKey()
	}

	@objc func showToolTip(forDuration duration: TimeInterval) {
		showToolTip()
		scheduleHide(afterDelay: duration)
	}

	@objc func showToolTip(forView view: NSView, closeAfter duration: TimeInterval) {
		updateToolTip()
		popover?.show(relativeTo: view.bounds, of: view, preferredEdge: .maxY)
		popover?.contentViewController?.view.window?.ignoresMouseEvents = true
		scheduleHide(afterDelay: duration)
	}

	@objc func hideToolTip() {
		popover?.close()
	}

	private func scheduleHide(afterDelay duration: TimeInterval) {
		currentTimer?.invalidate()
		currentTimer = nil

		if duration > 0.0 {
			let t = Timer.scheduledTimer(timeInterval: duration, target: self, selector: #selector(hideToolTip), userInfo: nil, repeats: false)
			RunLoop.main.add(t, forMode: .common)
			currentTimer = t
		}
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context != Self.context {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}

		if keyPath == "values.volumeLimit" {
			let volumeLimit = UserDefaults.standard.bool(forKey: "volumeLimit")
			let newMax = volumeLimit ? 100.0 : 800.0
			if maxVolume != newMax {
				let currentLevel = linearToLogarithmic(doubleValue, maxVolume)
				doubleValue = logarithmicToLinear(currentLevel, newMax)
			}
			maxVolume = newMax
		}
	}

	override func sendAction(_ action: Selector?, to target: Any?) -> Bool {
		// Snap to 100% if value is close
		let snapTarget = logarithmicToLinear(100.0, maxVolume)
		let snapProgress = (doubleValue - snapTarget) / (maxValue - minValue)

		if abs(snapProgress) < 0.005 {
			setDisplayedVolume(100.0)
			if !wasInsideSnapRange {
				NSHapticFeedbackManager.defaultPerformer.perform(.generic, performanceTime: .default)
			}
			wasInsideSnapRange = true
		} else {
			snapToVolumeStep()
			wasInsideSnapRange = false
		}

		showToolTip()

		return super.sendAction(action, to: target)
	}

	override func mouseDown(with event: NSEvent) {
		// Keep the tooltip up while the knob moves, and for a moment after.
		scheduleHide(afterDelay: 0)
		if trackPointerFromEvent(event) {
			scheduleHide(afterDelay: 1.0)
		} else {
			super.mouseDown(with: event)
		}
	}

	override func scrollWheel(with event: NSEvent) {
		scrollDeltaRemainder += event.deltaY
		let steps = trunc(scrollDeltaRemainder)

		if steps != 0 {
			scrollDeltaRemainder -= steps
			setDisplayedVolume(round(displayedVolume) + steps)
			(target as? PlaybackController)?.changeVolume(self)
			showToolTip(forDuration: 1.0)
		}

		if event.phase == .ended || event.phase == .cancelled {
			scrollDeltaRemainder = 0
		}
	}
}