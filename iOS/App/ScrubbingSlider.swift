//
//  ScrubbingSlider.swift
//  Cog (iOS)
//
//  A slider whose thumb slows down as the finger moves away from the track,
//  as the Music app's position slider once did: full speed on it, then
//  half, quarter and fine further off. Neither Slider nor UISlider offers
//  that, and UISlider moves its thumb with gestures of its own that a
//  subclass cannot slow. So a stock UISlider only draws, and a pan on the
//  view around it, begun only on the thumb, moves it: a pan there also
//  keeps a form from scrolling and a sheet from being pulled down, as the
//  stock slider does. VoiceOver gets a stock Slider in its place.
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
			.accessibilityRepresentation {
				Slider(value: $value, in: range)
			}
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

	func makeUIView(context: Context) -> ScrubbingSliderView {
		let view = ScrubbingSliderView()
		view.setContentHuggingPriority(.defaultLow, for: .horizontal)
		view.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
		return view
	}

	func updateUIView(_ view: ScrubbingSliderView, context: Context) {
		view.slider.minimumValue = Float(range.lowerBound)
		view.slider.maximumValue = Float(range.upperBound)
		// Not while the finger has it: the binding may round or snap what it
		// is given, and the thumb would stick there.
		if !view.isScrubbing {
			view.slider.value = Float(value)
		}
		view.slider.minimumTrackTintColor = tint.map(UIColor.init)
		view.slider.isEnabled = context.environment.isEnabled
		view.onChange = { value = Double($0) }
		view.onScrubbing = onEditingChanged
		view.onSpeedChange = onSpeedChange
	}
}

final class ScrubbingSliderView: UIView {
	let slider = UISlider()
	var onChange: (Float) -> Void = { _ in }
	var onScrubbing: (Bool) -> Void = { _ in }
	var onSpeedChange: (ScrubSpeed?) -> Void = { _ in }
	private(set) var isScrubbing = false
	/// Where the finger was at the last move, and the value it has moved the
	/// thumb to (kept here, as what the binding does with it may differ).
	private var lastX: CGFloat = 0
	private var scrubbed: Float = 0
	private var speed: ScrubSpeed? {
		didSet { if speed != oldValue { onSpeedChange(speed) } }
	}

	override init(frame: CGRect) {
		super.init(frame: frame)
		slider.isUserInteractionEnabled = false
		slider.translatesAutoresizingMaskIntoConstraints = false
		addSubview(slider)
		NSLayoutConstraint.activate([
			slider.leadingAnchor.constraint(equalTo: leadingAnchor),
			slider.trailingAnchor.constraint(equalTo: trailingAnchor),
			slider.centerYAnchor.constraint(equalTo: centerYAnchor),
		])
		addGestureRecognizer(UIPanGestureRecognizer(target: self, action: #selector(panned(_:))))
	}

	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	/// Only a touch on the thumb, or near it, takes it; and one there is
	/// not a scroll or a sheet's pull, as UISlider has it.
	override func gestureRecognizerShouldBegin(_ recognizer: UIGestureRecognizer) -> Bool {
		let onThumb = slider.isEnabled && isOnThumb(recognizer.location(in: slider))
		if recognizer.view == self { return onThumb }
		return !onThumb && super.gestureRecognizerShouldBegin(recognizer)
	}

	private func isOnThumb(_ point: CGPoint) -> Bool {
		let track = slider.trackRect(forBounds: slider.bounds)
		let thumb = slider.thumbRect(forBounds: slider.bounds, trackRect: track, value: slider.value)
		return thumb.insetBy(dx: -16, dy: -16).contains(point)
	}

	@objc private func panned(_ pan: UIPanGestureRecognizer) {
		let location = pan.location(in: slider)
		switch pan.state {
		case .began:
			isScrubbing = true
			// Where the touch landed, which the pan only reports after the
			// finger has moved a little.
			lastX = location.x - pan.translation(in: slider).x
			scrubbed = slider.value
			speed = .full
			onScrubbing(true)
			fallthrough
		case .changed:
			speed = ScrubSpeed(distance: abs(location.y - slider.bounds.midY))
			let track = slider.trackRect(forBounds: slider.bounds)
			let span = slider.maximumValue - slider.minimumValue
			scrubbed += Float((location.x - lastX) / max(track.width, 1)) * span * (speed?.factor ?? 1)
			scrubbed = min(max(scrubbed, slider.minimumValue), slider.maximumValue)
			lastX = location.x
			slider.value = scrubbed
			onChange(scrubbed)
		default:
			guard isScrubbing else { return }
			isScrubbing = false
			speed = nil
			onScrubbing(false)
		}
	}
}
