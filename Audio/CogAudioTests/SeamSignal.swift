//
//  SeamSignal.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import CogAudio
import XCTest

/// Test material for track-boundary tests: a loopable stereo signal and the
/// continuous resample a seamless join must reproduce.
enum SeamSignal {
	static let channels = 2

	/// A deterministic, noise-like, loopable signal: a sum of sines that each
	/// complete a whole number of cycles over the track, so the track joins
	/// onto itself without a discontinuity, while the random phases keep the
	/// waveform unpredictable enough to exercise the LPC extrapolator.
	static func loopable(frames: Int, sampleRate: Double) -> [Float] {
		var rng = SplitMix64(seed: 0x436F_6741_7564_696F)
		let partials = 48
		let seconds = Double(frames) / sampleRate
		let maxCycles = Int(16000.0 * seconds)
		let minCycles = Int(40.0 * seconds)
		var samples = [Float](repeating: 0, count: frames * channels)
		for channel in 0..<channels {
			var phases: [(step: Double, phase: Double)] = []
			for _ in 0..<partials {
				let cycles = Double(minCycles + Int(rng.next() % UInt64(maxCycles - minCycles)))
				let phase = Double(rng.next() % 1_000_000) / 1_000_000.0 * 2.0 * .pi
				phases.append((2.0 * .pi * cycles / Double(frames), phase))
			}
			let gain = 0.5 / Double(partials).squareRoot()
			for frame in 0..<frames {
				var sum = 0.0
				for p in phases {
					sum += sin(p.step * Double(frame) + p.phase)
				}
				samples[frame * channels + channel] = Float(sum * gain)
			}
		}
		return samples
	}

	/// One soxr pass over the whole input, at the quality `ConverterNode` uses.
	static func reference(_ samples: [Float], inputRate: Double, outputRate: Double) -> [Float] {
		let inputFrames = samples.count / channels
		let capacity = Int((Double(inputFrames) * outputRate / inputRate).rounded(.up)) + 1024
		var output = [Float](repeating: 0, count: capacity * channels)
		var inputDone = 0
		var outputDone = 0
		var ioSpec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I)
		var qualitySpec = soxr_quality_spec(UInt(SOXR_HQ), 0)
		let error = samples.withUnsafeBufferPointer { input in
			output.withUnsafeMutableBufferPointer { out in
				soxr_oneshot(inputRate, outputRate, UInt32(channels),
				             input.baseAddress, inputFrames, &inputDone,
				             out.baseAddress, capacity, &outputDone,
				             &ioSpec, &qualitySpec, nil)
			}
		}
		XCTAssertNil(error)
		return Array(output[0..<(outputDone * channels)])
	}
}

/// Small deterministic PRNG so the test signal is identical on every run.
struct SplitMix64 {
	private var state: UInt64

	init(seed: UInt64) {
		state = seed
	}

	mutating func next() -> UInt64 {
		state &+= 0x9E37_79B9_7F4A_7C15
		var z = state
		z = (z ^ (z >> 30)) &* 0xBF58_476D_1CE4_E5B9
		z = (z ^ (z >> 27)) &* 0x94D0_49BB_1331_11EB
		return z ^ (z >> 31)
	}
}
