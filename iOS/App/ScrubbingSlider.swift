//
//  ScrubbingSlider.swift
//  Cog (iOS)
//
//  A slider whose thumb slows down as the finger moves away from the track,
//  as the Music app's position slider once did: full speed on it, then
//  half, quarter and fine further off. Neither Slider nor UISlider offers
//  that; a UISlider that tracks its touches its own way does, and as a
//  control it keeps the touch from scrolling a form or pulling down a sheet,
//  as the stock one does.
//

import SwiftUI
import UIKit

struct ScrubbingSlider: View {
	@Binding var value: Double
	var range: ClosedRange<Double> = 0...1
	/// The filled part of the track; nil for the app's tint.
	var tint: Color?
	var onEditingChanged: (Bool) -> Void = { _ in }

	@State private var speed: ScrubSpeed?

	var body: some View {
		SliderRepresentable(value: $value, range: range, tint: tint, onEditingChanged: onEditingChanged) { speed = $0 }
			.frame(height: 32)
			.overlay(alignment: .top) {
				if let speed, speed != .full {
					Text(speed.name)
						.font(.caption.weight(.semibold))
						.foregroundStyle(.secondary)
						.fixedSize()
						.offset(y: -18)
						.transition(.opacity)
				}
			}
			.animation(.easeOut(duration: 0.15), value: speed)
			.sensoryFeedback(.selection, trigger: speed) { old, new in old != nil && new != nil }
	}
}

/// How much of the finger's movement the thumb follows, by how far the
/// finger is from the track.
enum ScrubSpeed: Equatable {
	case full, half, quarter, fine

	init(distance: CGFloat) {
		switch distance {
		case ..<50: self = .full
		case ..<100: self = .half
		case ..<150: self = .quarter
		default: self = .fine
		}
	}

	var factor: Float {
		switch self {
		case .full: 1
		case .half: 0.5
		case .quarter: 0.25
		case .fine: 0.125
		}
	}

	var name: LocalizedStringKey {
		switch self {
		case .full: "Hi-Speed Scrubbing"
		case .half: "Half-Speed Scrubbing"
		case .quarter: "Quarter-Speed Scrubbing"
		case .fine: "Fine Scrubbing"
		}
	}
}

private struct SliderRepresentable: UIViewRepresentable {
	@Binding var value: Double
	let range: ClosedRange<Double>
	let tint: Color?
	let onEditingChanged: (Bool) -> Void
	let onSpeedChange: (ScrubSpeed?) -> Void

	func makeUIView(context: Context) -> ScrubbingUISlider {
		let slider = ScrubbingUISlider()
		slider.addTarget(context.coordinator, action: #selector(Coordinator.changed(_:)), for: .valueChanged)
		slider.setContentHuggingPriority(.defaultLow, for: .horizontal)
		slider.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
		return slider
	}

	func updateUIView(_ slider: ScrubbingUISlider, context: Context) {
		context.coordinator.parent = self
		slider.minimumValue = Float(range.lowerBound)
		slider.maximumValue = Float(range.upperBound)
		// Not while the finger has it: the binding may round or snap what it
		// is given, and the thumb would stick there.
		if !slider.isTracking {
			slider.value = Float(value)
		}
		slider.minimumTrackTintColor = tint.map(UIColor.init)
		slider.isEnabled = context.environment.isEnabled
		slider.onTracking = { context.coordinator.parent.onEditingChanged($0) }
		slider.onSpeedChange = { context.coordinator.parent.onSpeedChange($0) }
	}

	func makeCoordinator() -> Coordinator {
		Coordinator(parent: self)
	}

	final class Coordinator: NSObject {
		var parent: SliderRepresentable

		init(parent: SliderRepresentable) {
			self.parent = parent
		}

		@objc func changed(_ slider: UISlider) {
			parent.value = Double(slider.value)
		}
	}
}

final class ScrubbingUISlider: UISlider {
	var onTracking: (Bool) -> Void = { _ in }
	var onSpeedChange: (ScrubSpeed?) -> Void = { _ in }
	/// Where the finger was at the last move, and how fast it scrubs.
	private var lastX: CGFloat = 0
	private var speed: ScrubSpeed? {
		didSet { if speed != oldValue { onSpeedChange(speed) } }
	}

	override func beginTracking(_ touch: UITouch, with event: UIEvent?) -> Bool {
		// The stock slider decides whether the touch takes the thumb.
		guard super.beginTracking(touch, with: event) else { return false }
		lastX = touch.location(in: self).x
		speed = .full
		onTracking(true)
		return true
	}

	override func continueTracking(_ touch: UITouch, with event: UIEvent?) -> Bool {
		let location = touch.location(in: self)
		let track = trackRect(forBounds: bounds)
		speed = ScrubSpeed(distance: abs(location.y - bounds.midY))
		let moved = Float((location.x - lastX) / max(track.width, 1)) * (maximumValue - minimumValue) * (speed?.factor ?? 1)
		lastX = location.x
		setValue(value + moved, animated: false)
		sendActions(for: .valueChanged)
		return true
	}

	override func endTracking(_ touch: UITouch?, with event: UIEvent?) {
		super.endTracking(touch, with: event)
		speed = nil
		onTracking(false)
	}

	override func cancelTracking(with event: UIEvent?) {
		super.cancelTracking(with: event)
		speed = nil
		onTracking(false)
	}
}
