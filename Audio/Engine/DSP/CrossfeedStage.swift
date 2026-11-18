//
//  CrossfeedStage.swift
//  CogAudio
//
//  Bauer stereophonic-to-binaural crossfeed, after Boris Mikhaylov's libbs2b.
//

import Foundation

/// Bauer crossfeed for headphones (bs2b), enabled by `enableCrossfeed` with
/// the preset in `crossfeedPreset`. Each ear gets its own channel through a
/// high-boost filter plus the other channel through a low-pass, as the far
/// ear hears a speaker: quieter, and without its highs. Anything other than
/// stereo passes through unchanged.
///
/// The filters are single poles, so the stage holds nothing back, and a
/// preset change is taken at the next block without resetting their history.
final class CrossfeedStage: NSObject, DSPStage {
	/// A cut frequency for the fed signal and how far below the direct one
	/// it sits, as libbs2b's levels are given.
	struct Preset: Equatable {
		var cutoffHz: Double
		var feedDB: Double

		/// Close to what speakers in a room sound like.
		static let standard = Preset(cutoffHz: 700, feedDB: 4.5)
		/// Chu Moy's crossfeeder.
		static let chuMoy = Preset(cutoffHz: 700, feedDB: 6)
		/// Jan Meier's, the mildest.
		static let janMeier = Preset(cutoffHz: 650, feedDB: 9.5)

		/// The preset a `crossfeedPreset` value names; the default for
		/// anything else.
		init(named name: String?) {
			switch name {
			case "chuMoy": self = .chuMoy
			case "janMeier": self = .janMeier
			default: self = .standard
			}
		}

		init(cutoffHz: Double, feedDB: Double) {
			self.cutoffHz = cutoffHz
			self.feedDB = feedDB
		}
	}

	static let enabledKey = "enableCrossfeed"
	static let presetKey = "crossfeedPreset"

	/// The two filters' coefficients and the gain that brings a signal common
	/// to both channels back to unity at DC.
	struct Coefficients: Equatable {
		var lowA0 = 0.0
		var lowB1 = 0.0
		var highA0 = 0.0
		var highA1 = 0.0
		var highB1 = 0.0
		var gain = 1.0

		init(_ preset: Preset, sampleRate: Double) {
			let lowDB = preset.feedDB * -5 / 6 - 3
			let highDB = preset.feedDB / 6 - 3
			let lowGain = pow(10, lowDB / 20)
			let highGain = 1 - pow(10, highDB / 20)
			let highCutoff = preset.cutoffHz * pow(2, (lowDB - 20 * log10(highGain)) / 12)

			var x = exp(-2 * .pi * preset.cutoffHz / sampleRate)
			lowB1 = x
			lowA0 = lowGain * (1 - x)

			x = exp(-2 * .pi * highCutoff / sampleRate)
			highB1 = x
			highA0 = 1 - highGain * (1 - x)
			highA1 = -x

			gain = 1 / (1 - highGain + lowGain)
		}
	}

	// Shared with the main thread, under `lock`.
	private let lock = UnfairLock()
	private var enabled: Bool
	private var preset: Preset

	// DSP thread only.
	private var sampleRate = 0.0
	private var isStereo = false
	private var applied: Preset?
	private var coefficients: Coefficients?
	private var low = (0.0, 0.0)
	private var high = (0.0, 0.0)
	private var last = (0.0, 0.0)

	override init() {
		let defaults = UserDefaults.standard
		enabled = defaults.bool(forKey: Self.enabledKey)
		preset = Preset(named: defaults.string(forKey: Self.presetKey))
		super.init()
		defaults.addObserver(self, forKeyPath: Self.enabledKey, options: [], context: &Self.context)
		defaults.addObserver(self, forKeyPath: Self.presetKey, options: [], context: &Self.context)
	}

	deinit {
		UserDefaults.standard.removeObserver(self, forKeyPath: Self.enabledKey, context: &Self.context)
		UserDefaults.standard.removeObserver(self, forKeyPath: Self.presetKey, context: &Self.context)
	}

	private static var context = 0

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard context == &Self.context else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}
		let defaults = UserDefaults.standard
		let enabled = defaults.bool(forKey: Self.enabledKey)
		let preset = Preset(named: defaults.string(forKey: Self.presetKey))
		lock.withLock {
			self.enabled = enabled
			self.preset = preset
		}
	}

	var isActive: Bool {
		lock.withLock { enabled }
	}

	var inspection: StageInspection? {
		guard sampleRate > 0 else { return nil }
		let preset = applied ?? lock.withLock { self.preset }
		return .crossfeed(applies: isStereo, cutoffHz: preset.cutoffHz, feedDB: preset.feedDB)
	}

	func configure(input: StreamFormat) -> StreamFormat {
		sampleRate = input.sampleRate
		isStereo = input.channels == 2
		applied = nil
		coefficients = nil
		reset()
		return input
	}

	func process(_ buffer: DSPBuffer) {
		guard isStereo, sampleRate > 0, buffer.format.channels == 2 else { return }
		let preset = lock.withLock { self.preset }
		if preset != applied {
			applied = preset
			coefficients = Coefficients(preset, sampleRate: sampleRate)
		}
		guard let c = coefficients else { return }

		var low = low, high = high, last = last
		buffer.samples.withUnsafeMutableBufferPointer { samples in
			for frame in 0..<buffer.frames {
				let left = Double(samples[frame * 2])
				let right = Double(samples[frame * 2 + 1])

				low.0 = c.lowA0 * left + c.lowB1 * low.0
				low.1 = c.lowA0 * right + c.lowB1 * low.1
				high.0 = c.highA0 * left + c.highA1 * last.0 + c.highB1 * high.0
				high.1 = c.highA0 * right + c.highA1 * last.1 + c.highB1 * high.1
				last = (left, right)

				samples[frame * 2] = Float((high.0 + low.1) * c.gain)
				samples[frame * 2 + 1] = Float((high.1 + low.0) * c.gain)
			}
		}
		self.low = low
		self.high = high
		self.last = last
	}

	func reset() {
		low = (0, 0)
		high = (0, 0)
		last = (0, 0)
	}
}
