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
