//
//  Equalizer.swift
//  Cog (iOS)
//
//  The 31-band graphic equalizer's settings, under the macOS app's defaults
//  keys (GraphicEQenable, eqPreamp, eq20Hz…eq20kHz, GraphicEQpreset), and its
//  presets, from Cog.q1.json as Equalizer/EqualizerWindowController.m reads
//  them: ten bands (Winamp's), stretched to the engine's 31.
//

import CogAudio
import Foundation

@MainActor
final class Equalizer: ObservableObject {
	/// The engine's band centres (EqualizerStage.bands).
	static let frequencies: [Float] = [20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800,
	                                   1000, 1200, 1600, 2000, 2500, 3100, 4000, 5000, 6300, 8000, 10000, 12000, 16000, 20000]
	static let bandKeys = ["eq20Hz", "eq25Hz", "eq31p5Hz", "eq40Hz", "eq50Hz", "eq63Hz", "eq80Hz", "eq100Hz", "eq125Hz", "eq160Hz",
	                       "eq200Hz", "eq250Hz", "eq315Hz", "eq400Hz", "eq500Hz", "eq630Hz", "eq800Hz", "eq1kHz", "eq1p2kHz",
	                       "eq1p6kHz", "eq2kHz", "eq2p5kHz", "eq3p1kHz", "eq4kHz", "eq5kHz", "eq6p3kHz", "eq8kHz", "eq10kHz",
	                       "eq12kHz", "eq16kHz", "eq20kHz"]
	/// Gains run from -20 to +20 dB, as presets can.
	static let range: ClosedRange<Float> = -20...20

	struct Preset: Identifiable {
		let name: String
		let gains: [Float]
		let preamp: Float
		var id: String { name }
	}

	static let presets: [Preset] = loadPresets()

	private let defaults: UserDefaults

	@Published var isEnabled: Bool {
		didSet { defaults.set(isEnabled, forKey: "GraphicEQenable") }
	}

	/// dB, one per band.
	@Published private(set) var gains: [Float]

	@Published private(set) var preamp: Float

	/// The preset last applied, as its index in `presets`; -1 once a band or
	/// the preamp is moved by hand.
	@Published private(set) var presetIndex: Int

	/// The engine's equalizer while it runs, which takes the gains.
	private var stage: CogEqualizer?

	init(defaults: UserDefaults = .standard) {
		self.defaults = defaults
		isEnabled = defaults.bool(forKey: "GraphicEQenable")
		gains = Self.bandKeys.map { defaults.float(forKey: $0) }
		preamp = defaults.float(forKey: "eqPreamp")
		presetIndex = (defaults.object(forKey: "GraphicEQpreset") as? Int) ?? -1
	}

	/// The engine started (or stopped, nil) using its equalizer: it gets the
	/// gains as they are.
	func attach(_ equalizer: CogEqualizer?) {
		stage = equalizer
		push()
	}

	func setGain(_ gain: Float, band: Int) {
		gains[band] = gain
		defaults.set(gain, forKey: Self.bandKeys[band])
		stage?.setBandGain(gain, for: Int32(band))
		choose(-1)
	}

	func setPreamp(_ value: Float) {
		preamp = value
		// The engine follows eqPreamp itself.
		defaults.set(value, forKey: "eqPreamp")
		choose(-1)
	}

	func apply(_ index: Int) {
		let preset = Self.presets[index]
		gains = preset.gains
		preamp = preset.preamp
		for (band, gain) in gains.enumerated() {
			defaults.set(gain, forKey: Self.bandKeys[band])
		}
		defaults.set(preamp, forKey: "eqPreamp")
		push()
		choose(index)
	}

	/// Every band and the preamp at 0 dB.
	func flatten() {
		gains = Array(repeating: 0, count: gains.count)
		preamp = 0
		for key in Self.bandKeys { defaults.set(Float(0), forKey: key) }
		defaults.set(Float(0), forKey: "eqPreamp")
		push()
		choose(Self.presets.firstIndex { $0.name == "Flat" } ?? -1)
	}

	private func choose(_ index: Int) {
		presetIndex = index
		defaults.set(index, forKey: "GraphicEQpreset")
	}

	private func push() {
		guard let stage else { return }
		var values = gains
		values.withUnsafeMutableBufferPointer { stage.setAllBands($0.baseAddress!) }
	}

	// MARK: - Presets

	/// Cog.q1.json's presets: values 1…401 meaning -20…+20 dB in tenths, at
	/// Winamp's ten bands.
	private static func loadPresets() -> [Preset] {
		guard let url = Bundle.main.url(forResource: "Cog.q1", withExtension: "json"),
		      let data = try? Data(contentsOf: url),
		      let root = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
		      root["type"] as? String == "Cog EQ library file v1.0",
		      let list = root["presets"] as? [[String: Any]] else { return [] }
		let keys = ["hz32", "hz64", "hz128", "hz256", "hz512", "hz1000", "hz2000", "hz4000", "hz8000", "hz16000"]
		func decibels(_ value: Any?) -> Float {
			guard let value = (value as? NSNumber)?.intValue, (1...401).contains(value) else { return 0 }
			return Float(value - 201) / 10
		}
		return list.compactMap { item in
			guard let name = item["name"] as? String else { return nil }
			let tenBands = keys.map { decibels(item[$0]) }
			return Preset(name: name, gains: frequencies.map { interpolate(tenBands, at: $0) }, preamp: decibels(item["preamp"]))
		}
	}

	private static let presetFrequencies: [Float] = [32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 16000]

	/// A ten-band preset's gain at `frequency`: linear between its bands,
	/// and past either end, extended in steps that grow by 5% each, as macOS
	/// does.
	static func interpolate(_ values: [Float], at frequency: Float) -> Float {
		let bands = presetFrequencies
		if frequency < bands[0] {
			return extend(Array(values.reversed()), Array(bands.reversed()), to: frequency, falling: true)
		}
		if frequency > bands[9] {
			return extend(values, bands, to: frequency, falling: false)
		}
		if frequency == bands[0] { return values[0] }
		if frequency == bands[9] { return values[9] }
		for i in 0..<9 where frequency >= bands[i] && frequency < bands[i + 1] {
			let delta = (frequency - bands[i]) / (bands[i + 1] - bands[i])
			return values[i] + (values[i + 1] - values[i]) * delta
		}
		return 0
	}

	/// Extrapolation beyond the last band of `values` (ordered towards the
	/// side extended), four more steps.
	private static func extend(_ values: [Float], _ frequencies: [Float], to target: Float, falling: Bool) -> Float {
		var work = values
		var workFrequencies = frequencies
		for i in 10..<14 {
			work.append(work[i - 1] + (work[i - 1] - work[i - 2]) * 1.05)
			workFrequencies.append(workFrequencies[i - 1] + (workFrequencies[i - 1] - workFrequencies[i - 2]) * 1.05)
		}
		for i in 0..<13 {
			let (near, far) = (workFrequencies[i], workFrequencies[i + 1])
			let inside = falling ? (target <= near && target > far) : (target >= near && target < far)
			if inside {
				let delta = (target - near) / (far - near)
				return work[i] + (work[i + 1] - work[i]) * delta
			}
		}
		return work[13]
	}
}
