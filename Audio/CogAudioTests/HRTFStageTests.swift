//
//  HRTFStageTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

@testable import CogAudio
import XCTest

final class HRTFStageTests: XCTestCase {
	/// The impulse set, from the repository (a test bundle has no app bundle).
	private let impulses = URL(fileURLWithPath: #filePath)
		.deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
		.appendingPathComponent("SADIE_D02-96000.mhr")

	override func setUp() {
		super.setUp()
		UserDefaults.standard.set(true, forKey: "enableHrtf")
		UserDefaults.standard.set(false, forKey: "enableHeadTracking")
	}

	override func tearDown() {
		UserDefaults.standard.removeObject(forKey: "enableHrtf")
		UserDefaults.standard.removeObject(forKey: "enableHeadTracking")
		super.tearDown()
	}

	private func stage() -> HRTFStage {
		let stage = HRTFStage()
		stage.impulseFile = impulses
		return stage
	}

	private func tone(frames: Int, channels: Int) -> [Float] {
		var samples = [Float](repeating: 0, count: frames * channels)
		for frame in 0..<frames {
			let value = Float(0.25 * sin(2 * .pi * 1000 * Double(frame) / 48000))
			for channel in 0..<channels {
				samples[frame * channels + channel] = value
			}
		}
		return samples
	}

	private func run(_ stage: HRTFStage, channels: Int, config: UInt32, frames: Int = 4800) -> DSPBuffer {
		let format = StreamFormat(sampleRate: 48000, channels: channels, channelConfig: config)
		let output = stage.configure(input: format)
		XCTAssertEqual(output.channels, 2)
		XCTAssertEqual(output.channelConfig, HRTFStage.outputConfig)
		let buffer = DSPBuffer()
		buffer.resize(frames: frames, format: format)
		let input = tone(frames: frames, channels: channels)
		for i in 0..<input.count {
			buffer.samples[i] = input[i]
		}
		stage.process(buffer)
		return buffer
	}

	func testTheImpulseSetIsInTheRepository() {
		XCTAssertTrue(FileManager.default.fileExists(atPath: impulses.path))
	}

	func testEveryLayoutBecomesBinauralStereo() {
		let layouts: [(Int, UInt32)] = [(1, UInt32(AudioConfigMono)), (2, UInt32(AudioConfigStereo)), (6, UInt32(AudioConfig5Point1))]
		for (channels, config) in layouts {
			let buffer = run(stage(), channels: channels, config: config)
			XCTAssertEqual(buffer.frames, 4800, "\(channels) channels: same length")
			XCTAssertEqual(buffer.format.channels, 2)
			XCTAssertGreaterThan(buffer.samples[0..<(4800 * 2)].map(abs).max()!, 0.01, "\(channels) channels: audible")
		}
	}

	/// Primed with extrapolated history, the filter's first output is already
	/// at the steady level, not a ramp up out of silence.
	func testPrimingAvoidsARampOutOfSilence() {
		let buffer = run(stage(), channels: 2, config: UInt32(AudioConfigStereo))
		let left = stride(from: 0, to: 4800 * 2, by: 2).map { abs(buffer.samples[$0]) }
		let steady = left[2400..<4800].max()!
		let first = left[0..<48].max()!
		XCTAssertGreaterThan(first, steady * 0.8, "the first millisecond is at full level")
	}

	func testDisabledMeansInactive() {
		let stage = stage()
		XCTAssertTrue(stage.isActive)
		UserDefaults.standard.set(false, forKey: "enableHrtf")
		XCTAssertFalse(stage.isActive)
	}
}
