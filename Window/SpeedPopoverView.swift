//
//  SpeedPopoverView.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-19.
//

import Cocoa
import SwiftUI

/// Tempo and pitch, which the engine's TimeStretchStage reads from the
/// defaults as they change. The sliders move on PlaybackController's curve:
/// finer near the slow end, with 1× about two fifths along.
@MainActor
final class SpeedControl {
	enum Slider: Hashable {
		case tempo, pitch
	}

	static let range = 0.2...5.0
	/// The popover's coordinate space, which the scroll wheel is placed in.
	static let space = "SpeedPopover"

	static func speed(fromPosition position: Double) -> Double {
		let position = min(max(position, 0), 1)
		return position * position * (range.upperBound - range.lowerBound) + range.lowerBound
	}

	static func position(fromSpeed speed: Double) -> Double {
		((min(max(speed, range.lowerBound), range.upperBound) - range.lowerBound) / (range.upperBound - range.lowerBound)).squareRoot()
	}

	/// Where each slider is, for the scroll wheel.
	var sliderFrames: [Slider: CGRect] = [:]
	/// The sliders caught at 1×, which tick once as they arrive.
	private var snapped: Set<Slider> = []

	private var defaults: UserDefaults { .standard }
	private var isVarispeed: Bool { defaults.string(forKey: "rubberbandEngine") == "varispeed" }
	private var isDisabled: Bool { defaults.string(forKey: "rubberbandEngine") == "disabled" }
	private var isLocked: Bool { defaults.bool(forKey: "speedLock") }

	func position(of slider: Slider) -> Double {
		Self.position(fromSpeed: defaults.double(forKey: slider == .tempo ? "tempo" : "pitch"))
	}

	/// Moves a slider, catching it at 1×. Locked, pitch takes tempo along and
	/// tempo takes pitch (Varispeed's pitch is its tempo already).
	func set(_ slider: Slider, position: Double) {
		var speed = Self.speed(fromPosition: position)
		if abs(speed - 1) < 0.01 {
			speed = 1
			if snapped.insert(slider).inserted {
				NSHapticFeedbackManager.defaultPerformer.perform(.generic, performanceTime: .default)
			}
		} else {
			snapped.remove(slider)
		}
		switch slider {
		case .pitch:
			defaults.set(speed, forKey: "pitch")
			if isLocked { defaults.set(speed, forKey: "tempo") }
		case .tempo:
			defaults.set(speed, forKey: "tempo")
			if isLocked && !isVarispeed { defaults.set(speed, forKey: "pitch") }
		}
	}

	/// Locking meets the two sliders halfway.
	func toggleLock() {
		let locked = !isLocked
		defaults.set(locked, forKey: "speedLock")
		if locked {
			let halfway = (position(of: .tempo) + position(of: .pitch)) / 2
			set(.pitch, position: halfway)
		}
	}

	func reset() {
		defaults.set(1.0, forKey: "tempo")
		defaults.set(1.0, forKey: "pitch")
	}

	/// The scroll wheel over a slider moves it a hundredth of the way per
	/// line. Returns whether it was over one.
	func scroll(at point: CGPoint, by delta: CGFloat) -> Bool {
		guard !isDisabled, let slider = sliderFrames.first(where: { $0.value.contains(point) })?.key else { return false }
		set(slider, position: position(of: slider) + delta / 100)
		return true
	}
}

/// The speed button's popover: a slider for each of tempo and pitch, side by
/// side with the lock that ties them, or Varispeed's one speed.
struct SpeedPopoverView: View {
	let control: SpeedControl
	/// Opens the time-stretching settings.
	let showSettings: () -> Void

	@AppStorage("rubberbandEngine") private var engine = "varispeed"
	@AppStorage("tempo") private var tempo = 1.0
	@AppStorage("pitch") private var pitch = 1.0
	@AppStorage("speedLock") private var locked = true

	private var isVarispeed: Bool { engine == "varispeed" }
	private var isDisabled: Bool { engine == "disabled" }

	var body: some View {
		VStack(alignment: .leading, spacing: 10) {
			HStack(spacing: 6) {
				VStack(spacing: 6) {
					row(isVarispeed ? "Speed" : "Tempo", .tempo, tempo)
					if !isVarispeed {
						row("Pitch", .pitch, pitch)
					}
				}
				if !isVarispeed {
					Button {
						control.toggleLock()
					} label: {
						Image(systemName: locked ? "lock.fill" : "lock.open")
							.frame(width: 16)
					}
					.buttonStyle(.borderless)
					.help(Text("Lock Pitch to Tempo"))
					.accessibilityLabel(Text("Lock Pitch to Tempo"))
					.accessibilityValue(Text(locked ? "On" : "Off"))
				}
			}
			.disabled(isDisabled)

			HStack {
				if isDisabled {
					Image(systemName: "exclamationmark.triangle.fill")
						.foregroundColor(.yellow)
					Text("Time stretching is off.")
						.foregroundColor(.secondary)
					Button("Settings…", action: showSettings)
				}
				Spacer()
				Button("Reset to 1×") {
					control.reset()
				}
				.disabled(isDisabled || (tempo == 1 && pitch == 1))
			}
			.controlSize(.small)
		}
		.padding(12)
		.frame(width: 340)
		.onPreferenceChange(SliderFramesKey.self) { frames in
			MainActor.assumeIsolated { control.sliderFrames = frames }
		}
		.coordinateSpace(name: SpeedControl.space)
	}

	private func row(_ title: LocalizedStringKey, _ slider: SpeedControl.Slider, _ value: Double) -> some View {
		HStack(spacing: 6) {
			Text(title)
				.frame(width: 44, alignment: .leading)
			Text(verbatim: "0.2×")
				.font(.caption)
				.foregroundColor(.secondary)
			Slider(value: Binding(get: { SpeedControl.position(fromSpeed: value) },
			                      set: { control.set(slider, position: $0) }))
				.controlSize(.small)
				.accessibilityLabel(Text(title))
				.accessibilityValue(Text(Self.ratio(value)))
				.background(GeometryReader { geometry in
					Color.clear.preference(key: SliderFramesKey.self,
					                       value: [slider: geometry.frame(in: .named(SpeedControl.space))])
				})
			Text(verbatim: "5×")
				.font(.caption)
				.foregroundColor(.secondary)
			Text(Self.ratio(value))
				.monospacedDigit()
				.frame(width: 44, alignment: .trailing)
		}
	}

	/// Two decimals below 1×, one from there up, as the old sliders' tips.
	private static func ratio(_ value: Double) -> String {
		value.formatted(.number.precision(.fractionLength(value < 1 ? 2 : 1))) + "×"
	}
}

private struct SliderFramesKey: PreferenceKey {
	static let defaultValue: [SpeedControl.Slider: CGRect] = [:]

	static func reduce(value: inout [SpeedControl.Slider: CGRect], nextValue: () -> [SpeedControl.Slider: CGRect]) {
		value.merge(nextValue()) { $1 }
	}
}
