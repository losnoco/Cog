//
//  TimeStretchStageTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

@testable import CogAudio
import XCTest

final class TimeStretchStageTests: XCTestCase {
	private let stereo = StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))
	private let keys = ["rubberbandEngine", "tempo", "pitch"]

	override func tearDown() {
		for key in keys {
			UserDefaults.standard.removeObject(forKey: key)
		}
		super.tearDown()
	}

	private func set(engine: String, tempo: Double, pitch: Double) {
		UserDefaults.standard.set(engine, forKey: "rubberbandEngine")
		UserDefaults.standard.set(tempo, forKey: "tempo")
		UserDefaults.standard.set(pitch, forKey: "pitch")
	}

	private func tone(_ frequency: Double, frames: Int) -> [Float] {
		var samples = [Float](repeating: 0, count: frames * 2)
		for frame in 0..<frames {
			let value = Float(0.3 * sin(2 * .pi * frequency * Double(frame) / 48000))
			samples[frame * 2] = value
			samples[frame * 2 + 1] = value
		}
		return samples
	}

	/// Feeds `input` in awkward blocks, then drains, as the pump does.
	private func run(_ stage: TimeStretchStage, _ input: [Float]) -> [Float] {
		_ = stage.configure(input: stereo)
		let buffer = DSPBuffer()
		var result: [Float] = []
		let frames = input.count / 2
		var position = 0
		var index = 0
		let sizes = [4096, 1000, 17, 3000]
		while position < frames {
			let count = min(sizes[index % sizes.count], frames - position)
			buffer.resize(frames: count, format: stereo)
			for i in 0..<(count * 2) {
				buffer.samples[i] = input[position * 2 + i]
			}
			stage.process(buffer)
			result.append(contentsOf: buffer.samples[0..<(buffer.frames * 2)])
			position += count
			index += 1
		}
		XCTAssertGreaterThan(stage.pendingFrames, 0, "the stretcher holds some back until drained")
		buffer.resize(frames: 0, format: stereo)
		stage.drain(buffer)
		result.append(contentsOf: buffer.samples[0..<(buffer.frames * 2)])
		XCTAssertEqual(stage.pendingFrames, 0, "nothing owed after draining")
		return result
	}

	/// Estimated frequency of the left channel from zero crossings over the
	/// middle half.
	private func frequency(of samples: [Float]) -> Double {
		let left = stride(from: 0, to: samples.count, by: 2).map { samples[$0] }
		let start = left.count / 4
		let end = left.count * 3 / 4
		var crossings = 0
		for i in (start + 1)..<end where (left[i - 1] < 0) != (left[i] < 0) {
			crossings += 1
		}
		return Double(crossings) / 2 / (Double(end - start) / 48000)
	}

	func testUnityIsInactive() {
		set(engine: "faster", tempo: 1, pitch: 1)
		XCTAssertFalse(TimeStretchStage().isActive)
		set(engine: "disabled", tempo: 2, pitch: 1)
		XCTAssertFalse(TimeStretchStage().isActive)
	}

	func testRubberBandDoublesTheTempoToExactlyHalfTheLength() {
		set(engine: "faster", tempo: 2, pitch: 1)
		let stage = TimeStretchStage()
		XCTAssertTrue(stage.isActive)
		let output = run(stage, tone(440, frames: 48000))
		XCTAssertEqual(stage.timeRatio, 2)
		XCTAssertEqual(output.count / 2, 24000)
		XCTAssertEqual(frequency(of: output), 440, accuracy: 15, "tempo alone keeps the pitch")
	}

	func testSignalsmithHalvesTheTempoToExactlyTwiceTheLength() {
		set(engine: "signalsmith", tempo: 0.5, pitch: 1)
		let stage = TimeStretchStage()
		let output = run(stage, tone(440, frames: 48000))
		XCTAssertEqual(output.count / 2, 96000)
		XCTAssertEqual(frequency(of: output), 440, accuracy: 15)
	}

	func testPitchAloneKeepsTheLength() {
		for engine in ["faster", "signalsmith"] {
			set(engine: engine, tempo: 1, pitch: 1.5)
			let stage = TimeStretchStage()
			XCTAssertTrue(stage.isActive)
			let output = run(stage, tone(440, frames: 48000))
			XCTAssertEqual(output.count / 2, 48000, "\(engine): same length")
			XCTAssertEqual(frequency(of: output), 660, accuracy: 25, "\(engine): a fifth up")
		}
	}

	/// Both Rubber Band engines, both directions: exact lengths show the
	/// start delay is dropped in output frames.
	func testRubberBandLengthsAreExactForBothEngines() {
		for engine in ["faster", "finer"] {
			for tempo in [2.0, 0.5] {
				set(engine: engine, tempo: tempo, pitch: 1)
				let output = run(TimeStretchStage(), tone(440, frames: 48000))
				XCTAssertEqual(output.count / 2, Int(48000 / tempo), "\(engine) at \(tempo)")
			}
		}
	}

	// MARK: - Varispeed

	func testVarispeedMovesPitchAndTempoTogether() {
		for (tempo, expected) in [(2.0, 880.0), (0.5, 220.0)] {
			set(engine: "varispeed", tempo: tempo, pitch: 1)
			let stage = TimeStretchStage()
			XCTAssertTrue(stage.isActive)
			let output = run(stage, tone(440, frames: 48000))
			XCTAssertEqual(stage.timeRatio, tempo)
			XCTAssertEqual(output.count / 2, Int(48000 / tempo), "exact length at \(tempo)")
			XCTAssertEqual(frequency(of: output), expected, accuracy: 25, "the pitch follows the tempo at \(tempo)")
		}
	}

	func testVarispeedIgnoresALeftoverPitch() {
		set(engine: "varispeed", tempo: 1, pitch: 1.5)
		XCTAssertFalse(TimeStretchStage().isActive, "pitch belongs to the stretchers; at unity varispeed stays out of the way")
	}

	/// Once it has run, varispeed glides through unity instead of dropping
	/// out (and restarting with a click), until the next reset.
	func testVarispeedStaysActiveThroughUnityUntilReset() {
		set(engine: "varispeed", tempo: 1.5, pitch: 1)
		let stage = TimeStretchStage()
		_ = stage.configure(input: stereo)
		let buffer = DSPBuffer()
		let input = tone(440, frames: 4096)
		buffer.resize(frames: 4096, format: stereo)
		for i in 0..<input.count {
			buffer.samples[i] = input[i]
		}
		stage.process(buffer)

		UserDefaults.standard.set(1.0, forKey: "tempo")
		XCTAssertTrue(stage.isActive, "still holding audio, so it keeps running at 1×")
		stage.reset()
		XCTAssertFalse(stage.isActive, "bit-exact again after a reset")
	}

	/// The output status learns the settings the stretcher applied, as they
	/// change while it runs.
	func testInspectionFollowsTheAppliedSettings() {
		set(engine: "finer", tempo: 1.25, pitch: 1)
		let stage = TimeStretchStage()
		XCTAssertNil(stage.inspection, "nothing applied before the first block")
		_ = stage.configure(input: stereo)
		XCTAssertEqual(stage.inspection, .timeStretch(engine: "finer", tempo: 1.25, pitch: 1))

		set(engine: "finer", tempo: 1.5, pitch: 2)
		let buffer = DSPBuffer()
		buffer.resize(frames: 1024, format: stereo)
		stage.process(buffer)
		XCTAssertEqual(stage.inspection, .timeStretch(engine: "finer", tempo: 1.5, pitch: 2))
	}
}
