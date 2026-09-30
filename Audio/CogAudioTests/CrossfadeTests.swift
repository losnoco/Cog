//
//  CrossfadeTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

import CogAudio
import XCTest

/// The renderer's seek crossfade. Each stretch of audio is a constant 1 in
/// its own channel, so a channel's output is exactly that audio's gain.
final class CrossfadeTests: XCTestCase {
	private let fade = 1000
	private var ring: OpaquePointer!
	private var renderer: OpaquePointer!

	override func setUp() {
		super.setUp()
		ring = cog_ring_create(8192, 3)
		renderer = cog_renderer_create(ring)
		XCTAssertTrue(cog_renderer_set_crossfade_frames(renderer, fade))
	}

	override func tearDown() {
		cog_renderer_destroy(renderer)
		cog_ring_destroy(ring)
		super.tearDown()
	}

	private func fill(_ count: Int, channel: Int) {
		var samples = [Float](repeating: 0, count: count * 3)
		for frame in 0..<count {
			samples[frame * 3 + channel] = 1
		}
		XCTAssertEqual(samples.withUnsafeBufferPointer { cog_ring_write(ring, $0.baseAddress!, count) }, count)
	}

	/// Each channel's gain per frame.
	private func render(_ count: Int) -> [[Float]] {
		var samples = [Float](repeating: .nan, count: count * 3)
		_ = samples.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, count) }
		return (0..<3).map { channel in stride(from: channel, to: samples.count, by: 3).map { samples[$0] } }
	}

	/// The largest step between neighbouring frames.
	private func largestStep(_ gains: [Float]) -> Float {
		zip(gains, gains.dropFirst()).map { abs($1 - $0) }.max() ?? 0
	}

	func testASeekCrossfadesAtEqualPower() {
		fill(4000, channel: 0)
		let before = render(100)
		_ = cog_ring_request_flush(ring)
		fill(3000, channel: 1)
		let after = render(1500)

		let old = before[0] + after[0]
		let new = before[1] + after[1]
		XCTAssertEqual(old[100], 1, accuracy: 0.001, "the old audio carries on from where it was")
		XCTAssertEqual(new[100], 0, accuracy: 0.001, "the new audio starts from silence")
		XCTAssertLessThan(largestStep(old), 0.002)
		XCTAssertLessThan(largestStep(new), 0.002)
		for frame in 100..<(100 + fade) {
			XCTAssertEqual(old[frame] * old[frame] + new[frame] * new[frame], 1, accuracy: 0.001, "equal power at \(frame)")
		}
		XCTAssertEqual(old[100 + fade], 0)
		XCTAssertEqual(new[100 + fade], 1)
		XCTAssertTrue(after[2].allSatisfy { $0 == 0 })
	}

	func testTheNewAudioFadesInFromItsFirstFrameHoweverLateItArrives() {
		fill(4000, channel: 0)
		_ = render(100)
		_ = cog_ring_request_flush(ring)
		let waiting = render(300)
		XCTAssertLessThan(largestStep(waiting[0]), 0.002, "the old audio fades while nothing new has come")
		XCTAssertTrue(waiting[1].allSatisfy { $0 == 0 })
		XCTAssertEqual(cog_renderer_underrun_events(renderer), 0, "a seek is not an underrun")

		fill(3000, channel: 1)
		let after = render(1500)
		XCTAssertEqual(after[1][0], 0, accuracy: 0.001)
		XCTAssertLessThan(largestStep(after[1]), 0.002)
		XCTAssertEqual(after[1][fade], 1)
		XCTAssertEqual(after[0][fade - 300], 0, "the old audio finished on its own clock")
	}

	func testASeekDuringACrossfadeDoesNotStep() {
		fill(4000, channel: 0)
		_ = render(100)
		_ = cog_ring_request_flush(ring)
		fill(3000, channel: 1)
		let first = render(400)
		_ = cog_ring_request_flush(ring)
		fill(3000, channel: 2)
		let second = render(1500)

		for channel in 0..<3 {
			let gains = first[channel] + second[channel]
			XCTAssertLessThan(largestStep(gains), 0.003, "channel \(channel)")
		}
		XCTAssertEqual(second[0][fade], 0)
		XCTAssertEqual(second[1][fade], 0)
		XCTAssertEqual(second[2][fade], 1)
	}

	func testDisabledMeansACut() {
		fill(4000, channel: 0)
		_ = render(100)
		cog_renderer_set_crossfade_enabled(renderer, false)
		_ = cog_ring_request_flush(ring)
		fill(3000, channel: 1)
		let after = render(100)
		XCTAssertTrue(after[0].allSatisfy { $0 == 0 })
		XCTAssertTrue(after[1].allSatisfy { $0 == 1 })
	}

	func testASeekWhilePausedCuts() {
		fill(4000, channel: 0)
		cog_gain_ramp_to(cog_renderer_transport(renderer), 0, 0)
		_ = render(100)
		_ = cog_ring_request_flush(ring)
		fill(3000, channel: 1)
		cog_gain_ramp_to(cog_renderer_transport(renderer), 1, 0)
		let after = render(100)
		XCTAssertTrue(after[0].allSatisfy { $0 == 0 }, "nothing from before the seek on resuming")
		XCTAssertTrue(after[1].allSatisfy { $0 == 1 })
	}
}
