//
//  EqualizerStage.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

import Accelerate
import Foundation

/// Cog's 31-band graphic equalizer (as `DSPEqualizerNode`): peaking biquads at
/// Apple's graphic EQ band centres with Q 1.4, run through `vDSP_biquadm`,
/// after a preamp. Enabled by `GraphicEQenable`, preamp from `eqPreamp`.
///
/// The EQ window drives it through `CogEqualizer` on the main thread. Band
/// changes are queued and applied by the DSP thread at the next block,
/// without resetting the filters' state, so moving a slider does not click.
final class EqualizerStage: NSObject, DSPStage, CogEqualizer {
	static let bands: [Double] = [20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800,
	                              1000, 1200, 1600, 2000, 2500, 3100, 4000, 5000, 6300, 8000, 10000, 12000, 16000, 20000]
	static let q = 1.4

	/// Called on the main thread when the equalizer starts or stops being
	/// used, so the EQ window can attach to it.
	var onActivation: ((Bool) -> Void)?

	// Shared with the main thread, under `lock`.
	private let lock = UnfairLock()
	private var enabled: Bool
	private var preamp: Float
	private var gains = [Float](repeating: 0, count: 31)
	private var gainsChanged = true

	// DSP thread only.
	private var setup: vDSP_biquadm_Setup?
	private var configured: StreamFormat?
	private var coefficients: [Double] = []
	private var announcedActive = false
	/// The preamp and band gains last applied, for `inspection`.
	private var appliedPreamp: Float = 1
	private var appliedGains = [Float](repeating: 0, count: 31)

	override init() {
		let defaults = UserDefaults.standard
		enabled = defaults.bool(forKey: "GraphicEQenable")
		preamp = powf(10, defaults.float(forKey: "eqPreamp") / 20)
		super.init()
		defaults.addObserver(self, forKeyPath: "GraphicEQenable", options: [], context: &Self.context)
		defaults.addObserver(self, forKeyPath: "eqPreamp", options: [], context: &Self.context)
	}

	deinit {
		UserDefaults.standard.removeObserver(self, forKeyPath: "GraphicEQenable", context: &Self.context)
		UserDefaults.standard.removeObserver(self, forKeyPath: "eqPreamp", context: &Self.context)
		if let setup {
			vDSP_biquadm_DestroySetup(setup)
		}
	}

	private static var context = 0

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard context == &Self.context else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}
		let defaults = UserDefaults.standard
		let enabled = defaults.bool(forKey: "GraphicEQenable")
		let preamp = powf(10, defaults.float(forKey: "eqPreamp") / 20)
		lock.withLock {
			self.enabled = enabled
			self.preamp = preamp
		}
	}

	// MARK: - CogEqualizer (main thread)

	func setBandGain(_ gainDB: Float, for index: Int32) {
		guard (0..<31).contains(Int(index)) else { return }
		lock.withLock {
			gains[Int(index)] = gainDB
			gainsChanged = true
		}
	}

	func setAllBands(_ gainsDB: UnsafeMutablePointer<Float>) {
		lock.withLock {
			for i in 0..<31 {
				gains[i] = gainsDB[i]
			}
			gainsChanged = true
		}
	}

	func setPreamp(_ preampDB: Float) {
		let linear = powf(10, preampDB / 20)
		lock.withLock { preamp = linear }
	}

	// MARK: - DSPStage (DSP thread)

	var isActive: Bool {
		let active = lock.withLock { enabled }
		announce(active)
		return active
	}

	/// Makes the next activation be announced again, for a new playback
	/// session (the app forgets the equalizer when playback stops).
	func rearm() {
		announcedActive = false
	}

	/// Tells the app once per change, so the EQ window attaches or detaches.
	private func announce(_ active: Bool) {
		guard active != announcedActive else { return }
		announcedActive = active
		if let onActivation {
			DispatchQueue.main.async { onActivation(active) }
		}
	}

	func configure(input: StreamFormat) -> StreamFormat {
		if input != configured {
			configured = input
			if let setup {
				vDSP_biquadm_DestroySetup(setup)
				self.setup = nil
			}
			lock.withLock { gainsChanged = true }
		}
		return input
	}

	func process(_ buffer: DSPBuffer) {
		let channels = buffer.format.channels
		let (preamp, newGains) = lock.withLock { () -> (Float, [Float]?) in
			defer { gainsChanged = false }
			return (self.preamp, gainsChanged ? gains : nil)
		}
		if let newGains {
			updateCoefficients(newGains, sampleRate: buffer.format.sampleRate, channels: channels)
			appliedGains = newGains
		}
		appliedPreamp = preamp
		guard let setup else { return }

		let count = buffer.frames * channels
		buffer.samples.withUnsafeMutableBufferPointer { samples in
			var level = preamp
			if level != 1 {
				vDSP_vsmul(samples.baseAddress!, 1, &level, samples.baseAddress!, 1, vDSP_Length(count))
			}
			var inputs = (0..<channels).map { UnsafePointer(samples.baseAddress! + $0) }
			var outputs = (0..<channels).map { samples.baseAddress! + $0 }
			inputs.withUnsafeMutableBufferPointer { x in
				outputs.withUnsafeMutableBufferPointer { y in
					vDSP_biquadm(setup, x.baseAddress!, vDSP_Stride(channels), y.baseAddress!, vDSP_Stride(channels), vDSP_Length(buffer.frames))
				}
			}
		}
	}

	func reset() {
		if let setup {
			vDSP_biquadm_ResetState(setup)
		}
	}

	var inspection: StageInspection? {
		.equalizer(preampDB: 20 * log10f(appliedPreamp), gainsDB: appliedGains)
	}

	/// Peaking EQ sections (RBJ cookbook), one per band, copied per channel.
	private func updateCoefficients(_ gains: [Float], sampleRate: Double, channels: Int) {
		coefficients = [Double](repeating: 0, count: 31 * channels * 5)
		for (band, frequency) in Self.bands.enumerated() {
			let section = Self.peaking(frequency: frequency, gainDB: Double(gains[band]), q: Self.q, sampleRate: sampleRate)
			for channel in 0..<channels {
				let base = (band * channels + channel) * 5
				for i in 0..<5 {
					coefficients[base + i] = section[i]
				}
			}
		}
		coefficients.withUnsafeBufferPointer { coefs in
			if let setup {
				vDSP_biquadm_SetCoefficientsDouble(setup, coefs.baseAddress!, 0, 0, 31, vDSP_Length(channels))
			} else {
				setup = vDSP_biquadm_CreateSetup(coefs.baseAddress!, 31, vDSP_Length(channels))
			}
		}
	}

	static func peaking(frequency: Double, gainDB: Double, q: Double, sampleRate: Double) -> [Double] {
		guard frequency > 0, frequency < sampleRate / 2 else { return [1, 0, 0, 0, 0] }
		let a = pow(10, gainDB / 40)
		let omega = 2 * Double.pi * frequency / sampleRate
		let alpha = sin(omega) / (2 * q)
		let cosW = cos(omega)
		let a0 = 1 + alpha / a
		return [(1 + alpha * a) / a0, (-2 * cosW) / a0, (1 - alpha * a) / a0, (-2 * cosW) / a0, (1 - alpha / a) / a0]
	}
}
