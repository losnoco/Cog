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
//  Given a waveform, it draws that as the track, the played part in the
//  tint, with a slim playhead for the thumb.
//

import CogAudio
import SwiftUI
import UIKit

struct ScrubbingSlider: View {
	@Binding var value: Double
	var range: ClosedRange<Double> = 0...1
	/// The filled part of the track; nil for the app's tint.
	var tint: Color?
	/// Drawn as the track in place of the stock one; nil for the stock one.
	var waveform: Waveform?
	var logScale = true
	var rectified = false
	var onEditingChanged: (Bool) -> Void = { _ in }

	@State private var speed: ScrubSpeed?

	var body: some View {
		SliderRepresentable(value: $value, range: range, tint: tint, waveform: waveform.map { WaveformStyle(waveform: $0, logScale: logScale, rectified: rectified) },
		                    onEditingChanged: onEditingChanged) { speed = $0 }
			.frame(height: waveform == nil ? 32 : 44)
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
	let waveform: WaveformStyle?
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
		view.waveform = waveform
		view.updateWaveform()
		view.onChange = { value = Double($0) }
		view.onScrubbing = onEditingChanged
		view.onSpeedChange = onSpeedChange
	}
}

/// A waveform and how to draw it.
struct WaveformStyle: Equatable {
	var waveform: Waveform
	var logScale: Bool
	var rectified: Bool
}

final class ScrubbingSliderView: UIView {
	let slider = UISlider()
	var waveform: WaveformStyle? {
		didSet {
			guard (waveform == nil) != (oldValue == nil) else { return }
			// The stock track and thumb give way to the waveform's; the
			// clear thumb keeps its size, so a touch on it still takes it.
			let clear = waveform == nil ? nil : UIImage()
			slider.setMinimumTrackImage(clear, for: .normal)
			slider.setMaximumTrackImage(clear, for: .normal)
			slider.setThumbImage(waveform == nil ? nil : UIGraphicsImageRenderer(size: CGSize(width: 12, height: 32)).image { _ in }, for: .normal)
			waveformView.isHidden = waveform == nil
		}
	}
	private let waveformView = WaveformTrackView()
	/// About UISlider's own thumb.
	private static let stockThumbWidth: CGFloat = 32
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
		waveformView.isHidden = true
		waveformView.isUserInteractionEnabled = false
		waveformView.backgroundColor = .clear
		waveformView.contentMode = .redraw
		addSubview(waveformView)
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

	override func layoutSubviews() {
		super.layoutSubviews()
		waveformView.frame = bounds.insetBy(dx: 0, dy: 4)
		updateWaveform()
	}

	/// Redraws the waveform for the slider's value, colours and size.
	func updateWaveform() {
		guard let waveform else { return }
		slider.layoutIfNeeded()
		let track = slider.trackRect(forBounds: slider.bounds)
		func centre(_ value: Float) -> CGFloat {
			slider.thumbRect(forBounds: slider.bounds, trackRect: track, value: value).midX
		}
		// From the thumb's centre at one end to its centre at the other, so
		// the playhead meets the waveform where the value has it.
		let start = convert(CGPoint(x: centre(slider.minimumValue), y: 0), from: slider).x - waveformView.frame.minX
		let end = convert(CGPoint(x: centre(slider.maximumValue), y: 0), from: slider).x - waveformView.frame.minX
		let playhead = convert(CGPoint(x: centre(slider.value), y: 0), from: slider).x - waveformView.frame.minX
		waveformView.update(style: waveform, span: start...max(start, end), playhead: playhead,
		                    tint: slider.isEnabled ? slider.minimumTrackTintColor ?? tintColor : .secondaryLabel)
	}

	/// Only a touch on the thumb, or near it, takes it; and one there is
	/// not a scroll or a sheet's pull, as UISlider has it. Over a waveform a
	/// sideways drag anywhere takes it too, and moves the playhead from
	/// where it is; an upward or downward one is still the scroll's or the
	/// sheet's.
	override func gestureRecognizerShouldBegin(_ recognizer: UIGestureRecognizer) -> Bool {
		let takes = slider.isEnabled && (isOnThumb(recognizer.location(in: slider)) || isSidewaysOnWaveform(recognizer))
		if recognizer.view == self { return takes }
		return !takes && super.gestureRecognizerShouldBegin(recognizer)
	}

	private func isSidewaysOnWaveform(_ recognizer: UIGestureRecognizer) -> Bool {
		guard waveform != nil, let pan = recognizer as? UIPanGestureRecognizer else { return false }
		let velocity = pan.velocity(in: self)
		return abs(velocity.x) > abs(velocity.y)
	}

	private func isOnThumb(_ point: CGPoint) -> Bool {
		let track = slider.trackRect(forBounds: slider.bounds)
		var thumb = slider.thumbRect(forBounds: slider.bounds, trackRect: track, value: slider.value)
		// The waveform's playhead is slim, but takes a touch as wide as the
		// stock thumb does.
		if thumb.width < Self.stockThumbWidth {
			thumb = thumb.insetBy(dx: (thumb.width - Self.stockThumbWidth) / 2, dy: 0)
		}
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
			updateWaveform()
			onChange(scrubbed)
		default:
			guard isScrubbing else { return }
			isScrubbing = false
			speed = nil
			onScrubbing(false)
		}
	}
}

/// The waveform under a ScrubbingSlider's clear track.
private final class WaveformTrackView: UIView {
	private var style: WaveformStyle?
	private var span: ClosedRange<CGFloat> = 0...0
	private var playhead: CGFloat = 0
	private var tint: UIColor = .tintColor

	func update(style: WaveformStyle, span: ClosedRange<CGFloat>, playhead: CGFloat, tint: UIColor) {
		guard style != self.style || span != self.span || playhead != self.playhead || tint != self.tint else { return }
		self.style = style
		self.span = span
		self.playhead = playhead
		self.tint = tint
		setNeedsDisplay()
	}

	override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
		super.traitCollectionDidChange(previousTraitCollection)
		setNeedsDisplay()
	}

	override func draw(_ rect: CGRect) {
		guard let style, let context = UIGraphicsGetCurrentContext() else { return }
		let area = CGRect(x: span.lowerBound, y: bounds.minY, width: span.upperBound - span.lowerBound, height: bounds.height)
		guard area.width > 0 else { return }
		let (peak, rms) = style.waveform.paths(in: area, columns: Int(area.width), logScale: style.logScale, rectified: style.rectified, flipped: true)
		let x = min(max(playhead, area.minX), area.maxX)
		let parts: [(clip: CGRect, peak: UIColor, rms: UIColor)] = [
			(CGRect(x: area.minX, y: area.minY, width: x - area.minX, height: area.height), tint.withAlphaComponent(0.5), tint),
			(CGRect(x: x, y: area.minY, width: area.maxX - x, height: area.height), .tertiaryLabel, .secondaryLabel),
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
		UIColor.label.setFill()
		UIRectFill(CGRect(x: x - 1, y: area.minY, width: 2, height: area.height))
	}
}
