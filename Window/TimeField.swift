//
//  TimeField.swift
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

import AppKit

private let timerModeKey = "timerShowTimeRemaining"

private func formatTimer(minutes: Int, seconds: Int, prefix: UInt16, padding: Int) -> String {
	let paddingChar = String(UnicodeScalar(0x2007)!) // Digit-width space
	let paddingString = "".padding(toLength: padding, withPad: paddingChar, startingAt: 0)
	return String.localizedStringWithFormat("%@%C%lu:%02lu", paddingString, prefix, minutes, seconds)
}

private func log10digits(_ minutes: Int) -> Int {
	var minutes = minutes
	var ret = 1
	while minutes >= 10 {
		minutes /= 10
		ret += 1
	}
	return ret
}

/// Centres the digits in the field's bezel. The field is a small control
/// with a larger font, which makes it taller than its bezel, and its own
/// layout left the digits low.
@objc(TimeFieldCell)
class TimeFieldCell: NSTextFieldCell {
	override func drawingRect(forBounds rect: NSRect) -> NSRect {
		var drawingRect = super.drawingRect(forBounds: rect)
		guard let font = font else { return drawingRect }
		// The bezel is as tall as the field would be with its control size's
		// own font, at the top of the field.
		let plain = NSTextFieldCell(textCell: "0")
		plain.isBezeled = isBezeled
		plain.bezelStyle = bezelStyle
		plain.controlSize = controlSize
		plain.font = NSFont.systemFont(ofSize: NSFont.systemFontSize(for: controlSize))
		let bezelHeight = min(plain.cellSize.height, rect.height)
		// The text is laid out from the top of the drawing rect; move it so
		// that the digits, from the baseline up, sit in the bezel's middle.
		let layoutManager = NSLayoutManager()
		let baseline = rect.minY + bezelHeight / 2.0 + font.capHeight / 2.0
		drawingRect.origin.y = baseline - layoutManager.defaultBaselineOffset(for: font)
		drawingRect.size.height = layoutManager.defaultLineHeight(for: font)
		return drawingRect
	}
}

@objc(TimeField)
class TimeField: NSTextField {
	private var showTimeRemaining = false
	private var allowZeroTime = false
	private var storedCurrentTime = 0
	private var storedDuration = 0

	@objc var currentTime: Int {
		get { storedCurrentTime }
		set {
			let zeroTimeAllowed = allowZeroTime
			allowZeroTime = false
			if newValue == 0 && storedCurrentTime > 0 && storedDuration == 0 && !zeroTimeAllowed {
				return
			}
			storedCurrentTime = newValue
			update()
		}
	}

	@objc var duration: Int {
		get { storedDuration }
		set { storedDuration = newValue }
	}

	override func awakeFromNib() {
		let fontSize: CGFloat = 13.0

		showTimeRemaining = UserDefaults.standard.bool(forKey: timerModeKey)
		NotificationCenter.default.addObserver(self, selector: #selector(playbackWillResetPosition(_:)), name: .CogPlaybackWillResetPosition, object: nil)

		font = NSFont.monospacedDigitSystemFont(ofSize: fontSize, weight: .regular)

		update()
	}

	deinit {
		NotificationCenter.default.removeObserver(self)
	}

	@objc private func playbackWillResetPosition(_ notification: Notification) {
		allowZeroTime = true
	}

	private func update() {
		let text: String
		if !showTimeRemaining {
			let sec = storedCurrentTime
			let sectotal = storedDuration
			let minutedigits = log10digits(sec / 60)
			let otherminutedigits = log10digits(sectotal / 60) + 1 // Plus hyphen
			let padding = max(0, otherminutedigits - minutedigits)
			text = formatTimer(minutes: sec / 60, seconds: sec % 60, prefix: 0x200B, padding: padding) // Zero-width space
		} else {
			let sec = max(0, storedDuration - storedCurrentTime)
			let sectotal = storedDuration
			let minutedigits = log10digits(sec / 60) + 1 // Plus hyphen
			let otherminutedigits = log10digits(sectotal / 60) + 1 // Also plus hyphen
			let padding = max(0, otherminutedigits - minutedigits)
			text = formatTimer(minutes: sec / 60, seconds: sec % 60, prefix: 0x2212, padding: padding) // Minus
		}
		stringValue = text
	}

	override func mouseDown(with event: NSEvent) {
		showTimeRemaining = !showTimeRemaining
		UserDefaults.standard.set(showTimeRemaining, forKey: timerModeKey)
		update()
	}
}