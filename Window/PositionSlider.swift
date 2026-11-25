//
//  PositionSlider.swift
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

import AppKit

@objc(PositionSlider)
class PositionSlider: TrackingSlider {
	@IBOutlet var positionTextField: TimeField?

	private static let waveformContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	@objc class func waveformKeys() -> [String] {
		return [WaveformController.enabledKey, WaveformController.logScaleKey, WaveformController.rectifiedKey]
	}

	override func awakeFromNib() {
		super.awakeFromNib()

		// The cell draws the waveform; the slider redraws it when it changes.
		NotificationCenter.default.addObserver(self, selector: #selector(waveformDidChange(_:)), name: WaveformController.didChangeNotification, object: WaveformController.shared)
		let defaults = NSUserDefaultsController.shared
		for key in Self.waveformKeys() {
			defaults.addObserver(self, forKeyPath: "values." + key, options: [], context: Self.waveformContext)
		}
	}

	deinit {
		NotificationCenter.default.removeObserver(self)
		let defaults = NSUserDefaultsController.shared
		for key in Self.waveformKeys() {
			defaults.removeObserver(self, forKeyPath: "values." + key, context: Self.waveformContext)
		}
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.waveformContext {
			needsDisplay = true
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	@objc private func waveformDidChange(_ notification: Notification) {
		needsDisplay = true
	}

	/// The waveform settings, on the bar itself.
	override func menu(for event: NSEvent) -> NSMenu? {
		let menu = NSMenu(title: "")
		menu.autoenablesItems = false
		let shown = UserDefaults.standard.bool(forKey: WaveformController.enabledKey)
		let items: [[String]] = [
			[NSLocalizedString("Show Waveform", comment: "Position slider menu item"), WaveformController.enabledKey],
			[NSLocalizedString("Logarithmic Scale", comment: "Position slider menu item: waveform in decibels"), WaveformController.logScaleKey],
			[NSLocalizedString("Rectified", comment: "Position slider menu item: waveform folded above its baseline"), WaveformController.rectifiedKey],
		]
		for entry in items {
			let item = NSMenuItem(title: entry[0], action: #selector(toggleWaveformSetting(_:)), keyEquivalent: "")
			item.target = self
			item.representedObject = entry[1]
			item.state = UserDefaults.standard.bool(forKey: entry[1]) ? .on : .off
			// Scale and rectification only matter while the waveform shows.
			item.isEnabled = shown || entry[1] == WaveformController.enabledKey
			menu.addItem(item)
		}
		return menu
	}

	@IBAction func toggleWaveformSetting(_ sender: NSMenuItem) {
		let defaults = UserDefaults.standard
		guard let key = sender.representedObject as? String else { return }
		defaults.set(!defaults.bool(forKey: key), forKey: key)
	}

	override var doubleValue: Double {
		get { super.doubleValue }
		set {
			positionTextField?.currentTime = Int(newValue)
			super.doubleValue = newValue
		}
	}

	override var maxValue: Double {
		get { super.maxValue }
		set {
			var value = newValue
			if value.isNaN || value.isInfinite { value = 0.0 } // Clip invalid values from bad file playlist entries
			positionTextField?.duration = Int(value)
			super.maxValue = value
		}
	}

	override func mouseDragged(with event: NSEvent) {
		positionTextField?.currentTime = Int(doubleValue)
		super.mouseDragged(with: event)
	}
}