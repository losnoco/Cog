//
//  WaveformSliderCell.swift
//  Cog
//
//  The position slider's cell: while the waveform seek bar is on and the
//  playing track has one, it draws the waveform as the bar, the played part
//  in the accent colour, with a slim playhead for the knob. Otherwise it
//  draws the stock slider. It keeps no state (cells get copied); the slider
//  redraws it when the waveform or a setting changes.
//

import AppKit
import CogAudio

@objc(WaveformSliderCell)
final class WaveformSliderCell: TrackingCell {
	private var shownWaveform: Waveform? {
		guard UserDefaults.standard.bool(forKey: WaveformController.enabledKey) else { return nil }
		return WaveformController.shared.waveform
	}

	/// Where the waveform goes: the bar's span between the knob's centres at
	/// either end, and the slider's height.
	private func waveformArea(bar: NSRect) -> NSRect {
		let bounds = controlView?.bounds ?? bar
		let inset = knobThickness / 2
		return NSRect(x: bar.minX + inset, y: bounds.minY + 2, width: max(bar.width - 2 * inset, 1), height: max(bounds.height - 4, 1))
	}

	private func playheadX(in area: NSRect) -> CGFloat {
		let span = maxValue - minValue
		let fraction = span > 0 ? min(max((doubleValue - minValue) / span, 0), 1) : 0
		return area.minX + CGFloat(fraction) * area.width
	}

	override func drawBar(inside rect: NSRect, flipped: Bool) {
		guard let waveform = shownWaveform, let context = NSGraphicsContext.current?.cgContext else {
			return super.drawBar(inside: rect, flipped: flipped)
		}
		let defaults = UserDefaults.standard
		let area = waveformArea(bar: rect)
		let (peak, rms) = waveform.paths(in: area, columns: Int(area.width), logScale: defaults.bool(forKey: WaveformController.logScaleKey),
		                                 rectified: defaults.bool(forKey: WaveformController.rectifiedKey), flipped: flipped)
		let playhead = playheadX(in: area)
		let accent = isEnabled ? NSColor.controlAccentColor : NSColor.secondaryLabelColor
		let parts: [(clip: NSRect, peak: NSColor, rms: NSColor)] = [
			(NSRect(x: area.minX, y: area.minY, width: playhead - area.minX, height: area.height), accent.withAlphaComponent(0.5), accent),
			(NSRect(x: playhead, y: area.minY, width: area.maxX - playhead, height: area.height), .tertiaryLabelColor, .secondaryLabelColor),
		]
		for part in parts where part.clip.width > 0 {
			context.saveGState()
			context.clip(to: part.clip)
			context.addPath(peak)
			context.setFillColor(part.peak.cgColor)
			context.fillPath()
			context.addPath(rms)
			context.setFillColor(part.rms.cgColor)
			context.fillPath()
			context.restoreGState()
		}
	}

	override func drawKnob(_ knobRect: NSRect) {
		guard shownWaveform != nil else { return super.drawKnob(knobRect) }
		let area = waveformArea(bar: barRect(flipped: controlView?.isFlipped ?? false))
		let x = playheadX(in: area)
		NSColor.labelColor.setFill()
		NSRect(x: x - 1, y: area.minY, width: 2, height: area.height).fill()
	}
}
