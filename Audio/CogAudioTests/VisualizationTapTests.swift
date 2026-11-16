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

/// The stereo copy the projectM visualizer reads.
final class StereoVisualizationTests: XCTestCase {
	private let controller = VisualizationController.shared()

	override func setUp() {
		controller.reset()
	}

	/// One block of `frames` frames, the sample of channel `c` given by
	/// `value(c)`, through a fresh tap at 44.1 kHz; then the heard frames.
	private func tapped(channels: Int, config: UInt32, frames: Int = 4096, value: (Int) -> Float) -> [Float] {
		let tap = VisualizationTap()
		let format = StreamFormat(sampleRate: 44100, channels: channels, channelConfig: config)
		_ = tap.configure(input: format)
		let buffer = DSPBuffer()
		buffer.resize(frames: frames, format: format)
		for frame in 0..<frames {
			for channel in 0..<channels {
				buffer.samples[frame * channels + channel] = value(channel)
			}
		}
		tap.process(buffer)
		controller.postLatency(0)
		var out = [Float](repeating: .nan, count: 200)
		let copied = controller.copyHeardStereoPCM(&out, frames: 100, latencyOffset: 0)
		return Array(out[0..<(Int(copied) * 2)])
	}

	func testLeftAndRightStayApart() {
		controller.addStereoConsumer()
		defer { controller.removeStereoConsumer() }
		let out = tapped(channels: 2, config: UInt32(AudioConfigStereo)) { $0 == 0 ? 0.5 : -0.25 }
		XCTAssertEqual(out.count, 200)
		XCTAssertEqual(out, [Float](repeating: 0, count: 100).flatMap { _ in [0.5, -0.25] as [Float] })
	}

	func testMonoGoesToBothSides() {
		controller.addStereoConsumer()
		defer { controller.removeStereoConsumer() }
		let out = tapped(channels: 1, config: UInt32(AudioChannelFrontCenter)) { _ in 0.75 }
		XCTAssertEqual(out, [Float](repeating: 0.75, count: 200))
	}

	func testSurroundIsDownmixed() {
		controller.addStereoConsumer()
		defer { controller.removeStereoConsumer() }
		// Only the front left speaks.
		let out = tapped(channels: 6, config: UInt32(AudioConfig5Point1)) { $0 == 0 ? 0.5 : 0 }
		XCTAssertEqual(out.count, 200)
		XCTAssertTrue(stride(from: 0, to: 200, by: 2).allSatisfy { out[$0] > 0.1 }, "the left carries it")
		XCTAssertTrue(stride(from: 1, to: 200, by: 2).allSatisfy { abs(out[$0]) < 0.001 }, "the right stays silent")
	}

	func testNothingWithoutAConsumer() {
		XCTAssertFalse(controller.wantsStereo)
		XCTAssertEqual(tapped(channels: 2, config: UInt32(AudioConfigStereo)) { _ in 0.5 }, [])
	}

	func testResampledToTheDisplayRate() {
		controller.addStereoConsumer()
		defer { controller.removeStereoConsumer() }
		let tap = VisualizationTap()
		let format = StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))
		_ = tap.configure(input: format)
		let buffer = DSPBuffer()
		for _ in 0..<12 {
			buffer.resize(frames: 4000, format: format)
			for frame in 0..<4000 {
				buffer.samples[frame * 2] = 0.5
				buffer.samples[frame * 2 + 1] = -0.5
			}
			tap.process(buffer)
		}
		controller.postLatency(0)
		var out = [Float](repeating: .nan, count: 2000)
		XCTAssertEqual(controller.copyHeardStereoPCM(&out, frames: 1000, latencyOffset: 0), 1000)
		for frame in 0..<1000 {
			XCTAssertEqual(out[frame * 2], 0.5, accuracy: 0.01)
			XCTAssertEqual(out[frame * 2 + 1], -0.5, accuracy: 0.01)
		}
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

	/// The stereo ring at 1 kHz (10 000 frames) with `count` frames posted,
	/// frame i as (i, -i).
	private func stereoController(posting count: Int, latency: Double) -> VisualizationController {
		let controller = VisualizationController()
		controller.postSampleRate(1000)
		controller.addStereoConsumer()
		let frames = (0..<count).flatMap { [Float($0), -Float($0)] }
		var posted = 0
		while posted < count {
			let amount = min(4096, count - posted)
			frames.withUnsafeBufferPointer { controller.postStereoVisPCM($0.baseAddress! + posted * 2, frames: Int32(amount)) }
			posted += amount
		}
		controller.postLatency(latency)
		return controller
	}

	func testStereoEndsAtTheHeardFrame() {
		let controller = stereoController(posting: 5000, latency: 0.5)
		var out = [Float](repeating: .nan, count: 20)
		XCTAssertEqual(controller.copyHeardStereoPCM(&out, frames: 10, latencyOffset: 0), 10)
		XCTAssertEqual(out, (4490..<4500).flatMap { [Float($0), -Float($0)] })
	}

	func testStereoReadsAcrossTheRingsWrap() {
		// 12 000 posted into 10 000: the cursor sits at 2000.
		let controller = stereoController(posting: 12000, latency: 0)
		var out = [Float](repeating: .nan, count: 6000)
		XCTAssertEqual(controller.copyHeardStereoPCM(&out, frames: 3000, latencyOffset: 0), 3000)
		XCTAssertEqual(out, (9000..<12000).flatMap { [Float($0), -Float($0)] })
	}

	func testTheLastConsumerFreesTheRing() {
		let controller = stereoController(posting: 100, latency: 0)
		controller.removeStereoConsumer()
		var out = [Float](repeating: .nan, count: 20)
		XCTAssertEqual(controller.copyHeardStereoPCM(&out, frames: 10, latencyOffset: 0), 0)
	}
}
