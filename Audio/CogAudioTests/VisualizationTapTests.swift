//
//  VisualizationTapTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

@testable import CogAudio
import XCTest

final class VisualizationTapTests: XCTestCase {
	private func feed(_ tap: VisualizationTap, rate: Double, seconds: Int) -> (posted: UInt64, unchanged: Bool) {
		let format = StreamFormat(sampleRate: rate, channels: 2, channelConfig: UInt32(AudioConfigStereo))
		_ = tap.configure(input: format)
		let controller = VisualizationController.shared()
		let before = controller.samplesPosted()
		let buffer = DSPBuffer()
		var unchanged = true
		let block = 4096
		let total = Int(rate) * seconds
		var position = 0
		while position < total {
			let frames = min(block, total - position)
			buffer.resize(frames: frames, format: format)
			for i in 0..<(frames * 2) {
				buffer.samples[i] = Float(sin(Double(position * 2 + i) * 0.01))
			}
			let copy = Array(buffer.samples[0..<(frames * 2)])
			tap.process(buffer)
			unchanged = unchanged && buffer.frames == frames && Array(buffer.samples[0..<(frames * 2)]) == copy
			position += frames
		}
		return (controller.samplesPosted() - before, unchanged)
	}

	func testResampledToTheDisplayRateAndPassedThrough() {
		let (posted, unchanged) = feed(VisualizationTap(), rate: 48000, seconds: 2)
		XCTAssertTrue(unchanged, "the audio is not touched")
		XCTAssertEqual(Double(posted), 88200, accuracy: 600, "about 44.1 kHz worth, less the resampler's delay")
	}

	func testTheDisplayRateIsPostedDirectly() {
		let (posted, unchanged) = feed(VisualizationTap(), rate: 44100, seconds: 1)
		XCTAssertTrue(unchanged)
		XCTAssertEqual(posted, 44100)
	}
}

final class HeardPCMTests: XCTestCase {
	/// A 1 kHz controller (a 45 000-sample ring) with `count` samples
	/// posted, each its own index, and `latency` seconds still to be heard.
	private func controller(posting count: Int, latency: Double) -> VisualizationController {
		let controller = VisualizationController()
		controller.postSampleRate(1000)
		var ramp = (0..<count).map(Float.init)
		var posted = 0
		while posted < count {
			let amount = min(4096, count - posted)
			ramp.withUnsafeMutableBufferPointer { controller.postVisPCM($0.baseAddress! + posted, amount: Int32(amount)) }
			posted += amount
		}
		controller.postLatency(latency)
		return controller
	}

	func testEndsAtTheHeardSample() {
		let controller = controller(posting: 10000, latency: 0.5)
		var out = [Float](repeating: -1, count: 100)
		let copied = controller.copyHeardPCM(&out, count: 100, latencyOffset: 0)
		XCTAssertEqual(copied, 100)
		// 500 samples are still to be heard, so the last heard is 9499.
		XCTAssertEqual(out, (9400..<9500).map(Float.init))
	}

	func testTheOffsetMovesTheWindow() {
		let controller = controller(posting: 10000, latency: 0.5)
		var out = [Float](repeating: -1, count: 10)
		_ = controller.copyHeardPCM(&out, count: 10, latencyOffset: -0.1)
		XCTAssertEqual(out, (9590..<9600).map(Float.init))
	}

	func testReadsAcrossTheRingsWrap() {
		// 50 000 posted into 45 000: the cursor sits at 5000.
		let controller = controller(posting: 50000, latency: 0)
		var out = [Float](repeating: -1, count: 8000)
		XCTAssertEqual(controller.copyHeardPCM(&out, count: 8000, latencyOffset: 0), 8000)
		XCTAssertEqual(out, (42000..<50000).map(Float.init))
	}

	func testNothingWithoutAudio() {
		let controller = VisualizationController()
		var out = [Float](repeating: -1, count: 10)
		XCTAssertEqual(controller.copyHeardPCM(&out, count: 10, latencyOffset: 0), 0)
		XCTAssertEqual(out, [Float](repeating: -1, count: 10), "no stand-in sine")
	}
}
