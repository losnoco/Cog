//
//  CrossfeedStageTests.swift
//  CogAudioTests
//

@testable import CogAudio
import XCTest

final class CrossfeedStageTests: XCTestCase {
	private let stereo = StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))

	override func setUp() {
		super.setUp()
		UserDefaults.standard.set(true, forKey: CrossfeedStage.enabledKey)
		UserDefaults.standard.removeObject(forKey: CrossfeedStage.presetKey)
	}

	override func tearDown() {
		UserDefaults.standard.removeObject(forKey: CrossfeedStage.enabledKey)
		UserDefaults.standard.removeObject(forKey: CrossfeedStage.presetKey)
		super.tearDown()
	}

	/// One second of `left` and `right` (constant, or a tone at `frequency`).
	private func signal(left: Float, right: Float, frequency: Double? = nil) -> [Float] {
		var samples = [Float](repeating: 0, count: 48000 * 2)
		for frame in 0..<48000 {
			let wave = frequency.map { Float(sin(2 * .pi * $0 * Double(frame) / 48000)) } ?? 1
			samples[frame * 2] = left * wave
			samples[frame * 2 + 1] = right * wave
		}
		return samples
	}

	/// Feeds `input` in awkward block sizes, as the pump may.
	private func run(_ stage: CrossfeedStage, _ input: [Float], format: StreamFormat) -> [Float] {
		_ = stage.configure(input: format)
		let sizes = [1000, 4096, 17, 3000, 777]
		let buffer = DSPBuffer()
		var result: [Float] = []
		var position = 0
		var index = 0
		let frames = input.count / format.channels
		while position < frames {
			let count = min(sizes[index % sizes.count], frames - position)
			buffer.resize(frames: count, format: format)
			for i in 0..<(count * format.channels) {
				buffer.samples[i] = input[position * format.channels + i]
			}
			stage.process(buffer)
			XCTAssertEqual(buffer.frames, count, "crossfeed holds nothing back")
			result.append(contentsOf: buffer.samples[0..<(count * format.channels)])
			position += count
			index += 1
		}
		return result
	}

	/// The peak of one channel over the last tenth of a second, once the
	/// filters have settled.
	private func settledPeak(_ samples: [Float], channel: Int) -> Double {
		var peak = 0.0
		for frame in (48000 - 4800)..<48000 {
			peak = max(peak, Double(abs(samples[frame * 2 + channel])))
		}
		return peak
	}

	private func decibels(_ ratio: Double) -> Double {
		20 * log10(ratio)
	}

	func testWhatBothChannelsShareStaysAtUnityInTheBass() {
		let output = run(CrossfeedStage(), signal(left: 0.5, right: 0.5), format: stereo)
		XCTAssertEqual(settledPeak(output, channel: 0), 0.5, accuracy: 1e-4)
		XCTAssertEqual(settledPeak(output, channel: 1), 0.5, accuracy: 1e-4)
	}

	func testOneChannelFeedsTheOtherByThePresetsLevel() {
		for (name, feed) in [(nil, 4.5), ("chuMoy", 6.0), ("janMeier", 9.5), ("nonsense", 4.5)] as [(String?, Double)] {
			UserDefaults.standard.set(name, forKey: CrossfeedStage.presetKey)
			let output = run(CrossfeedStage(), signal(left: 0.5, right: 0), format: stereo)
			let direct = settledPeak(output, channel: 0)
			let fed = settledPeak(output, channel: 1)
			XCTAssertEqual(decibels(fed / direct), -feed, accuracy: 0.01, "\(name ?? "default")")
			XCTAssertLessThan(direct, 0.5, "the direct channel gives up what the other gains")
		}
	}

	func testHighsStayInTheirChannel() {
		let output = run(CrossfeedStage(), signal(left: 0.5, right: 0, frequency: 10000), format: stereo)
		let direct = settledPeak(output, channel: 0)
		let fed = settledPeak(output, channel: 1)
		XCTAssertLessThan(decibels(fed / direct), -25)
		XCTAssertLessThan(direct, 0.5, "never louder than the input")
	}

	func testNeverLouderThanTheInputAtAnyFrequency() {
		for frequency in [30.0, 200, 700, 1000, 3000, 15000] {
			let output = run(CrossfeedStage(), signal(left: 0.5, right: 0.5, frequency: frequency), format: stereo)
			XCTAssertLessThanOrEqual(settledPeak(output, channel: 0), 0.5 + 1e-4, "\(frequency) Hz")
		}
	}

	func testAPresetChangeIsTakenAtTheNextBlock() {
		let stage = CrossfeedStage()
		_ = run(stage, signal(left: 0.5, right: 0), format: stereo)
		XCTAssertEqual(stage.inspection, .crossfeed(applies: true, cutoffHz: 700, feedDB: 4.5))
		UserDefaults.standard.set("janMeier", forKey: CrossfeedStage.presetKey)
		let buffer = DSPBuffer()
		let input = signal(left: 0.5, right: 0)
		buffer.resize(frames: 48000, format: stereo)
		for i in 0..<input.count {
			buffer.samples[i] = input[i]
		}
		stage.process(buffer)
		XCTAssertEqual(stage.inspection, .crossfeed(applies: true, cutoffHz: 650, feedDB: 9.5))
		XCTAssertEqual(decibels(settledPeak(buffer.samples, channel: 1) / settledPeak(buffer.samples, channel: 0)), -9.5, accuracy: 0.01)
	}

	func testAnythingButStereoPassesThrough() {
		let surround = StreamFormat(sampleRate: 48000, channels: 6, channelConfig: 0x3F)
		let stage = CrossfeedStage()
		XCTAssertEqual(stage.configure(input: surround), surround)
		let buffer = DSPBuffer()
		buffer.resize(frames: 100, format: surround)
		for i in 0..<600 {
			buffer.samples[i] = Float(i) / 600
		}
		let input = Array(buffer.samples[0..<600])
		stage.process(buffer)
		XCTAssertEqual(Array(buffer.samples[0..<600]), input)
		XCTAssertEqual(stage.inspection, .crossfeed(applies: false, cutoffHz: 700, feedDB: 4.5))
	}

	func testResetForgetsTheFilters() {
		let stage = CrossfeedStage()
		let first = run(stage, signal(left: 0.5, right: -0.25, frequency: 300), format: stereo)
		stage.reset()
		let buffer = DSPBuffer()
		let input = signal(left: 0.5, right: -0.25, frequency: 300)
		buffer.resize(frames: 48000, format: stereo)
		for i in 0..<input.count {
			buffer.samples[i] = input[i]
		}
		stage.process(buffer)
		XCTAssertEqual(Array(buffer.samples[0..<input.count]), first)
	}

	func testFollowsItsSwitch() {
		let stage = CrossfeedStage()
		XCTAssertTrue(stage.isActive)
		UserDefaults.standard.set(false, forKey: CrossfeedStage.enabledKey)
		XCTAssertFalse(stage.isActive)
	}
}
