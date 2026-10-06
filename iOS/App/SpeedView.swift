//
//  SpeedView.swift
//  Cog (iOS)
//
//  Tempo and pitch, as the macOS window's sliders and Rubber Band pane set
//  them: the engine's TimeStretchStage reads these defaults as they change.
//

import SwiftUI

enum Speed {
	/// The macOS app's defaults (PlaybackController's initDefaults).
	static func registerDefaults() {
		UserDefaults.standard.register(defaults: [
			"tempo": 1.0,
			"pitch": 1.0,
			"speedLock": true,
			"rubberbandEngine": "varispeed",
			"rubberbandTransients": "crisp",
			"rubberbandDetector": "compound",
			"rubberbandPhase": "laminar",
			"rubberbandWindow": "standard",
			"rubberbandSmoothing": "off",
			"rubberbandFormant": "shifted",
			"rubberbandPitch": "highspeed",
			"rubberbandChannels": "apart",
		])
	}

	static let range = 0.2...5.0

	/// A slider's 0…1 to a speed, on the macOS sliders' curve: finer near
	/// the slow end, with 1× about two fifths along.
	static func speed(fromSlider position: Double) -> Double {
		position * position * (range.upperBound - range.lowerBound) + range.lowerBound
	}

	static func slider(fromSpeed speed: Double) -> Double {
		((min(max(speed, range.lowerBound), range.upperBound) - range.lowerBound) / (range.upperBound - range.lowerBound)).squareRoot()
	}

	/// Whether anything plays other than as recorded.
	static func isChanged(engine: String, tempo: Double, pitch: Double) -> Bool {
		engine != "disabled" && (abs(tempo - 1) > 1e-6 || (engine != "varispeed" && abs(pitch - 1) > 1e-6))
	}
}

/// The sheet that holds the speed controls, where there is no room for
/// them beside Now Playing.
struct SpeedView: View {
	@Environment(\.dismiss) private var dismiss

	var body: some View {
		NavigationStack {
			SpeedControls()
				.navigationTitle("Speed")
				.navigationBarTitleDisplayMode(.inline)
				.toolbar {
					ToolbarItem(placement: .confirmationAction) {
						Button("Done") { dismiss() }
					}
				}
		}
	}
}

/// The engine, tempo, pitch and Rubber Band options, in a form.
struct SpeedControls: View {
	@AppStorage("rubberbandEngine") private var engine = "varispeed"
	@AppStorage("tempo") private var tempo = 1.0
	@AppStorage("pitch") private var pitch = 1.0
	@AppStorage("speedLock") private var locked = true
	@AppStorage("rubberbandTransients") private var transients = "crisp"
	@AppStorage("rubberbandDetector") private var detector = "compound"
	@AppStorage("rubberbandPhase") private var phase = "laminar"
	@AppStorage("rubberbandWindow") private var window = "standard"
	@AppStorage("rubberbandSmoothing") private var smoothing = "off"
	@AppStorage("rubberbandFormant") private var formant = "shifted"
	@AppStorage("rubberbandPitch") private var pitchMode = "highspeed"
	@AppStorage("rubberbandChannels") private var channels = "apart"

	private var isVarispeed: Bool { engine == "varispeed" }
	private var isRubberBand: Bool { engine == "faster" || engine == "finer" }
	private var isR3: Bool { engine == "finer" }

	var body: some View {
		Form {
			Section {
				Picker("Engine", selection: $engine) {
					Text("Off").tag("disabled")
					Text("Varispeed").tag("varispeed")
					Text("Signalsmith Stretch").tag("signalsmith")
					Text("Rubber Band (Faster)").tag("faster")
					Text("Rubber Band (Finer)").tag("finer")
				}
			} footer: {
				Text(isVarispeed
					? "Varies the playback speed, like a record player: the pitch follows it."
					: "Changes the tempo and the pitch apart from each other.")
			}

			if engine != "disabled" {
				Section {
					SpeedSlider(title: isVarispeed ? "Speed" : "Tempo", value: tempo, detail: ratio(tempo)) { setTempo($0) }
					if !isVarispeed {
						SpeedSlider(title: "Pitch", value: pitch, detail: "\(ratio(pitch)), \(semitones(pitch))") { setPitch($0) }
						Toggle("Lock Pitch to Tempo", isOn: $locked)
					}
					Button("Reset") {
						tempo = 1
						pitch = 1
					}
					.disabled(tempo == 1 && pitch == 1)
				}
			}

			if isRubberBand {
				Section("Rubber Band") {
					if !isR3 {
						Picker("Transients", selection: $transients) {
							Text("Crisp").tag("crisp")
							Text("Mixed").tag("mixed")
							Text("Smooth").tag("smooth")
						}
						Picker("Detector", selection: $detector) {
							Text("Compound").tag("compound")
							Text("Percussive").tag("percussive")
							Text("Soft").tag("soft")
						}
						Picker("Phase", selection: $phase) {
							Text("Laminar").tag("laminar")
							Text("Independent").tag("independent")
						}
					}
					Picker("Window", selection: $window) {
						Text("Standard").tag("standard")
						Text("Short").tag("short")
						if !isR3 {
							Text("Long").tag("long")
						}
					}
					if !isR3 {
						Picker("Smoothing", selection: $smoothing) {
							Text("Off").tag("off")
							Text("On").tag("on")
						}
					}
					Picker("Formant", selection: $formant) {
						Text("Shifted").tag("shifted")
						Text("Preserved").tag("preserved")
					}
					Picker("Pitch Mode", selection: $pitchMode) {
						Text("High Speed").tag("highspeed")
						Text("High Quality").tag("highquality")
						Text("High Consistency").tag("highconsistency")
					}
					Picker("Channels", selection: $channels) {
						Text("Apart").tag("apart")
						Text("Together").tag("together")
					}
				}
			}
		}
	}

	/// As the macOS sliders do: locked, one moves the other (Varispeed's
	/// pitch is its tempo already).
	private func setTempo(_ value: Double) {
		tempo = value
		if locked && !isVarispeed { pitch = value }
	}

	private func setPitch(_ value: Double) {
		pitch = value
		if locked { tempo = value }
	}

	private func ratio(_ value: Double) -> String {
		String(format: "%.2f×", value)
	}

	private func semitones(_ value: Double) -> String {
		let steps = 12 * log2(value)
		return abs(steps) < 0.05 ? String(localized: "0 semitones") : String(format: "%+.1f semitones", steps)
	}
}

/// A speed from 0.2× to 5×, on the curve the macOS sliders use, catching at
/// 1× so it is easy to come back to.
private struct SpeedSlider: View {
	let title: LocalizedStringKey
	let value: Double
	let detail: String
	let set: (Double) -> Void

	var body: some View {
		VStack(alignment: .leading, spacing: 4) {
			HStack {
				Text(title)
				Spacer()
				Text(detail)
					.monospacedDigit()
					.foregroundStyle(.secondary)
			}
			HStack {
				Text("0.2×").font(.caption2)
				ScrubbingSlider(value: Binding(get: { Speed.slider(fromSpeed: value) }, set: { position in
					let speed = Speed.speed(fromSlider: position)
					set(abs(speed - 1) < 0.02 ? 1 : (speed * 100).rounded() / 100)
				}))
				.accessibilityLabel(Text(title))
				Text("5×").font(.caption2)
			}
			.padding(.top, 8)
			.sensoryFeedback(.selection, trigger: value == 1)
		}
	}
}
