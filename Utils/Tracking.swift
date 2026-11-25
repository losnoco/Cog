//
//  Tracking.swift
//  Cog
//
//  TrackingCell / TrackingSlider / NSSlider(PointerTracking), converted from
//  Utils/TrackingCell.m and Utils/TrackingSlider.m.
//

import AppKit

@objc(TrackingCell)
class TrackingCell: NSSliderCell {
	private var tracking = false

	override func startTracking(at startPoint: NSPoint, in controlView: NSView) -> Bool {
		tracking = true
		return super.startTracking(at: startPoint, in: controlView)
	}

	override func continueTracking(last lastPoint: NSPoint, current currentPoint: NSPoint, in controlView: NSView) -> Bool {
		if let event = NSEvent.mouseEvent(with: .leftMouseDragged, location: currentPoint, modifierFlags: [], timestamp: 0, windowNumber: controlView.window?.windowNumber ?? 0, context: nil, eventNumber: 0, clickCount: 0, pressure: 0) {
			controlView.mouseDragged(with: event)
		}
		return super.continueTracking(last: lastPoint, current: currentPoint, in: controlView)
	}

	override func stopTracking(last lastPoint: NSPoint, current stopPoint: NSPoint, in controlView: NSView, mouseIsUp flag: Bool) {
		tracking = false
		super.stopTracking(last: lastPoint, current: stopPoint, in: controlView, mouseIsUp: flag)
	}

	@objc var isTracking: Bool { tracking }

	@objc func setTracking(_ flag: Bool) {
		tracking = flag
	}
}

@objc(TrackingSlider)
class TrackingSlider: NSSlider {
	private static let valueContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)
	private var bindingInfo: [String: [NSBindingInfoKey: Any]] = [:]

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.valueContext {
			if !isTracking, let value = change?[.newKey] as? NSNumber {
				doubleValue = value.doubleValue
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	override func bind(_ binding: NSBindingName, to observableController: Any, withKeyPath keyPath: String, options: [NSBindingOption: Any]? = nil) {
		if binding == .value {
			(observableController as AnyObject).addObserver(self, forKeyPath: keyPath, options: .new, context: Self.valueContext)
			bindingInfo[binding.rawValue] = [NSBindingInfoKey.observedObject: observableController,
			                                 NSBindingInfoKey.observedKeyPath: keyPath,
			                                 NSBindingInfoKey.options: options ?? [:]]
		} else {
			super.bind(binding, to: observableController, withKeyPath: keyPath, options: options)
		}
	}

	override func unbind(_ binding: NSBindingName) {
		if binding == .value, let info = bindingInfo[binding.rawValue] {
			let keyPath = info[NSBindingInfoKey.observedKeyPath] as? String ?? ""
			(info[NSBindingInfoKey.observedObject] as AnyObject).removeObserver(self, forKeyPath: keyPath)
		} else {
			super.unbind(binding)
		}
	}

	@objc var isTracking: Bool {
		return (cell as? TrackingCell)?.isTracking ?? false
	}

	override func mouseDown(with event: NSEvent) {
		let trackingCell = cell as? TrackingCell
		trackingCell?.setTracking(true)
		let handled = trackPointerFromEvent(event)
		trackingCell?.setTracking(false)
		if !handled {
			super.mouseDown(with: event)
		}
	}
}

extension NSSlider {
	// The value whose knob centre is at the pointer: the knob's centre travels
	// the bar less half the knob at either end.
	@objc func valueAtPointerOfEvent(_ event: NSEvent) -> Double {
		guard let sliderCell = cell as? NSSliderCell else { return minValue }
		let bar = sliderCell.barRect(flipped: isFlipped)
		let inset = sliderCell.knobThickness / 2
		let span = bar.width - 2 * inset
		let point = convert(event.locationInWindow, from: nil)
		let fraction = span > 0 ? min(max((point.x - bar.minX - inset) / span, 0.0), 1.0) : 0.0
		return minValue + fraction * (maxValue - minValue)
	}

	// On macOS 15, NSSlider takes a click only on its knob or its thin bar,
	// and a click on the knob drags it from where it was grabbed. So a click
	// beside the knob or below the bar, as on the waveform, did nothing, and
	// the knob did not follow the pointer exactly.
	@objc func trackPointerFromEvent(_ event: NSEvent) -> Bool {
		if !isEnabled || isVertical { return false }

		doubleValue = valueAtPointerOfEvent(event)
		if isContinuous { sendAction(action, to: target) }

		while true {
			guard let next = NSApp.nextEvent(matching: [.leftMouseDragged, .leftMouseUp], until: .distantFuture, inMode: .eventTracking, dequeue: true) else { break }
			if next.type == .leftMouseUp { break }
			let value = valueAtPointerOfEvent(next)
			if value != doubleValue {
				doubleValue = value
				if isContinuous { sendAction(action, to: target) }
			}
		}

		if !isContinuous { sendAction(action, to: target) }
		return true
	}
}