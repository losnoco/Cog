//
//  EqualizerView.swift
//  Cog (iOS)
//

import SwiftUI

struct EqualizerView: View {
	@EnvironmentObject private var equalizer: Equalizer
	@Environment(\.dismiss) private var dismiss

	var body: some View {
		NavigationStack {
			VStack(spacing: 20) {
				Toggle("Equalizer", isOn: $equalizer.isEnabled)
					.font(.headline)

				// What the switch governs: dimmed and still while it is off.
				VStack(spacing: 20) {
					HStack {
						Text("Preset")
						Spacer()
						Menu(presetName) {
							ForEach(Array(Equalizer.presets.enumerated()), id: \.offset) { index, preset in
								Button(preset.name) { equalizer.apply(index) }
							}
						}
					}

					VStack(alignment: .leading, spacing: 4) {
						HStack {
							Text("Preamp")
							Spacer()
							Text(decibels(equalizer.preamp))
								.monospacedDigit()
								.foregroundStyle(.secondary)
						}
						Slider(value: Binding(get: { equalizer.preamp }, set: { equalizer.setPreamp(($0 * 2).rounded() / 2) }),
						       in: Equalizer.range)
					}

					ScrollView(.horizontal) {
						HStack(alignment: .bottom, spacing: 2) {
							ForEach(Equalizer.frequencies.indices, id: \.self) { band in
								BandSlider(band: band)
							}
						}
						.padding(.vertical, 8)
					}
					.scrollIndicators(.visible)
				}
				.disabled(!equalizer.isEnabled)

				Spacer()
			}
			.padding()
			.navigationTitle("Equalizer")
			.navigationBarTitleDisplayMode(.inline)
			.toolbar {
				ToolbarItem(placement: .topBarLeading) {
					Button("Flat") { equalizer.flatten() }
						.disabled(!equalizer.isEnabled)
				}
				ToolbarItem(placement: .confirmationAction) {
					Button("Done") { dismiss() }
				}
			}
		}
	}

	private var presetName: String {
		Equalizer.presets.indices.contains(equalizer.presetIndex) ? Equalizer.presets[equalizer.presetIndex].name : "Custom"
	}
}

/// One band: its gain, a vertical slider, and its frequency.
private struct BandSlider: View {
	let band: Int
	@EnvironmentObject private var equalizer: Equalizer
	private let height: CGFloat = 220

	var body: some View {
		VStack(spacing: 6) {
			Text(String(format: "%+.0f", equalizer.gains[band]))
				.font(.caption2.monospacedDigit())
				.foregroundStyle(.secondary)
			Slider(value: Binding(get: { equalizer.gains[band] }, set: { equalizer.setGain(($0 * 2).rounded() / 2, band: band) }),
			       in: Equalizer.range)
				// Sideways, then turned up: SwiftUI has no vertical slider.
				.frame(width: height)
				.rotationEffect(.degrees(-90))
				.frame(width: 32, height: height)
			Text(frequencyLabel)
				.font(.caption2)
				.foregroundStyle(.secondary)
				.lineLimit(1)
				.fixedSize()
		}
		.frame(width: 36)
		.accessibilityElement(children: .combine)
		.accessibilityLabel("\(frequencyLabel)z")
	}

	/// "31.5", "1k", "2.5k"…
	private var frequencyLabel: String {
		let frequency = Equalizer.frequencies[band]
		if frequency >= 1000 {
			let thousands = frequency / 1000
			return thousands == thousands.rounded() ? "\(Int(thousands))k" : String(format: "%.1fk", thousands)
		}
		return frequency == frequency.rounded() ? "\(Int(frequency))" : String(format: "%.1f", frequency)
	}
}

private func decibels(_ value: Float) -> String {
	String(format: "%+.1f dB", value)
}
