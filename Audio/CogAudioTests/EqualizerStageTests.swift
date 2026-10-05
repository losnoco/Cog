//
//  EqualizerStageTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

@testable import CogAudio
import XCTest

final class EqualizerStageTests: XCTestCase {
	private let rate = 48000.0
	private let format = StreamFormat(sampleRate: 48000, channels: 2, channelConfig: 3)

	override func setUp() {
		super.setUp()
		UserDefaults.standard.set(true, forKey: "GraphicEQenable")
		UserDefaults.standard.set(0.0, forKey: "eqPreamp")
	}

	override func tearDown() {
		UserDefaults.standard.removeObject(forKey: "GraphicEQenable")
		UserDefaults.standard.removeObject(forKey: "eqPreamp")
		super.tearDown()
	}

	/// Runs a stereo sine through `stage` and returns the steady-state peak.
	private func peak(through stage: EqualizerStage, frequency: Double) -> Float {
		_ = stage.configure(input: format)
		let buffer = DSPBuffer()
		var peak: Float = 0
		for block in 0..<24 {
			buffer.resize(frames: 4096, format: format)
			for frame in 0..<4096 {
				let t = Double(block * 4096 + frame) / rate
				let value = Float(0.1 * sin(2 * .pi * frequency * t))
				buffer.samples[frame * 2] = value
				buffer.samples[frame * 2 + 1] = value
			}
			stage.process(buffer)
			if block >= 12 {
				peak = max(peak, buffer.samples[0..<(4096 * 2)].map(abs).max()!)
			}
		}
		return peak
	}

	private func db(_ ratio: Float) -> Float { 20 * log10(ratio) }

	func testFlatPassesThrough() {
		let stage = EqualizerStage()
		XCTAssertTrue(stage.isActive)
		var flat = [Float](repeating: 0, count: 31)
		flat.withUnsafeMutableBufferPointer { stage.setAllBands($0.baseAddress!) }
		XCTAssertEqual(peak(through: stage, frequency: 1000), 0.1, accuracy: 1e-3)
	}

	func testABoostedBandRaisesItsFrequencyOnly() {
		let stage = EqualizerStage()
		var gains = [Float](repeating: 0, count: 31)
		gains[EqualizerStage.bands.firstIndex(of: 1000)!] = 12
		gains.withUnsafeMutableBufferPointer { stage.setAllBands($0.baseAddress!) }

		let boosted = db(peak(through: stage, frequency: 1000) / 0.1)
		let untouched = db(peak(through: EqualizerStage(), frequency: 100) / 0.1)
		XCTAssertEqual(boosted, 12, accuracy: 1.5, "about 12 dB at the band centre")
		XCTAssertEqual(untouched, 0, accuracy: 0.1, "a flat stage leaves 100 Hz alone")

		let far = EqualizerStage()
		gains.withUnsafeMutableBufferPointer { far.setAllBands($0.baseAddress!) }
		XCTAssertLessThan(abs(db(peak(through: far, frequency: 100) / 0.1)), 0.5, "the 1 kHz boost barely reaches 100 Hz")
	}

	func testPreampScalesEverything() {
		let stage = EqualizerStage()
		var flat = [Float](repeating: 0, count: 31)
		flat.withUnsafeMutableBufferPointer { stage.setAllBands($0.baseAddress!) }
		stage.setPreamp(-6)
		XCTAssertEqual(db(peak(through: stage, frequency: 1000) / 0.1), -6, accuracy: 0.05)
	}

	func testDisablingMakesTheStageInactive() {
		let stage = EqualizerStage()
		XCTAssertTrue(stage.isActive)
		UserDefaults.standard.set(false, forKey: "GraphicEQenable")
		XCTAssertFalse(stage.isActive)
	}

	/// The output status learns the gains as the DSP thread applies them.
	func testInspectionReportsTheAppliedGains() {
		let stage = EqualizerStage()
		var gains = [Float](repeating: 0, count: 31)
		gains[4] = 3
		gains.withUnsafeMutableBufferPointer { stage.setAllBands($0.baseAddress!) }
		stage.setPreamp(-6)
		guard case .equalizer(_, let before)? = stage.inspection else { return XCTFail("an equalizer inspection") }
		XCTAssertEqual(before, [Float](repeating: 0, count: 31), "not applied until a block runs")

		_ = peak(through: stage, frequency: 1000)
		guard case let .equalizer(preamp, after)? = stage.inspection else { return XCTFail("an equalizer inspection") }
		XCTAssertEqual(preamp, -6, accuracy: 1e-4)
		XCTAssertEqual(after, gains)
	}
}
