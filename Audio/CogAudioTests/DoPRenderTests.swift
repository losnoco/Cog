//
//  DoPRenderTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

import CogAudio
import XCTest

/// DSD over PCM through the renderer: bit-exact, marker phase kept, and DoP
/// silence wherever there is no carrier to play.
final class DoPRenderTests: XCTestCase {
	private var ring: OpaquePointer!
	private var renderer: OpaquePointer!

	override func setUp() {
		super.setUp()
		ring = cog_ring_create(8192, 2)
		renderer = cog_renderer_create(ring)
		XCTAssertTrue(cog_renderer_set_crossfade_frames(renderer, 1000))
	}

	override func tearDown() {
		cog_renderer_destroy(renderer)
		cog_ring_destroy(ring)
		super.tearDown()
	}

	/// DoP frames starting on `marker`, with a varying payload.
	private func carrier(_ count: Int, startingWith marker: UInt8 = 0x05, seed: UInt32 = 1) -> [Float] {
		var samples: [Float] = []
		var next = marker
		for frame in 0..<count {
			for channel in 0..<2 {
				let payload = UInt32(truncatingIfNeeded: (UInt32(frame) &* 2654435761 &+ UInt32(channel) &* 40503 &+ seed)) & 0xFFFF
				let word = Int32(bitPattern: (UInt32(next) << 24) | (payload << 8))
				samples.append(Float(Double(word) / 2147483648.0))
			}
			next = next == 0x05 ? 0xFA : 0x05
		}
		return samples
	}

	private func fill(_ samples: [Float]) {
		XCTAssertEqual(samples.withUnsafeBufferPointer { cog_ring_write(ring, $0.baseAddress!, samples.count / 2) }, samples.count / 2)
	}

	private func render(_ count: Int) -> [Float] {
		var samples = [Float](repeating: .nan, count: count * 2)
		_ = samples.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, count) }
		return samples
	}

	private func isDoP(_ samples: [Float]) -> Bool {
		samples.withUnsafeBufferPointer { cog_dop_validate($0.baseAddress!, 2, samples.count / 2, nil) }
	}

	func testSilenceIsValidDoPAndContinuesThePhase() {
		var marker: UInt8 = 0xFA
		var samples = [Float](repeating: 0, count: 10 * 2)
		samples.withUnsafeMutableBufferPointer { cog_dop_fill_silence($0.baseAddress!, 2, 5, &marker) }
		XCTAssertEqual(marker, 0x05, "five frames from 0xFA leave 0x05 next")
		samples.withUnsafeMutableBufferPointer { cog_dop_fill_silence($0.baseAddress! + 10, 2, 5, &marker) }
		XCTAssertTrue(isDoP(samples))
		XCTAssertFalse(isDoP([Float](repeating: 0.1, count: 8)))
	}

	func testDoPPassesBitExactWhateverTheGains() {
		cog_gain_ramp_to(cog_renderer_volume(renderer), 0.3, 0)
		cog_gain_ramp(cog_renderer_transport(renderer), 0.5, 1, 100)
		let input = carrier(1000)
		fill(input)
		XCTAssertEqual(render(1000), input)
	}

	func testAShortfallIsDoPSilenceInPhase() {
		fill(carrier(101))
		let out = render(300)
		XCTAssertTrue(isDoP(out), "carrier then silence, one unbroken phase")
		XCTAssertEqual(Array(out.prefix(202)), carrier(101))
		XCTAssertTrue(isDoP(render(64)), "still locked while the ring is dry")
	}

	func testAPhaseSlipDropsAFrameRatherThanRepeatAMarker() {
		fill(carrier(101))
		_ = render(101) // next expected marker: 0xFA
		fill(carrier(100, startingWith: 0x05, seed: 7))
		let out = render(100)
		XCTAssertTrue(isDoP(render(1) + []), "the stream stays valid")
		XCTAssertEqual(Array(out.prefix(198)), Array(carrier(100, startingWith: 0x05, seed: 7)[2...]))
		var joined = carrier(101)
		joined += out
		XCTAssertTrue(isDoP(joined), "no repeated marker across the join")
	}

	func testASeekDuringDoPCutsAndHoldsLockUntilTheNewCarrier() {
		fill(carrier(2000))
		_ = render(100)
		_ = cog_ring_request_flush(ring)
		let waiting = render(200)
		XCTAssertTrue(isDoP(waiting), "DoP silence, not a crossfade and not zeroes")
		fill(carrier(500, startingWith: 0x05, seed: 3))
		let out = render(500)
		XCTAssertTrue(isDoP(render(100) + []))
		XCTAssertTrue(isDoP(Array(waiting.suffix(2)) + out), "the new carrier joins in phase")
	}

	func testAPausedTransportSilencesDoP() {
		fill(carrier(1000))
		_ = render(10)
		cog_gain_ramp_to(cog_renderer_transport(renderer), 0, 0)
		let out = render(100)
		XCTAssertTrue(isDoP(out))
		var silence = [Float](repeating: 0, count: 200)
		var marker: UInt8 = 0x05
		silence.withUnsafeMutableBufferPointer { cog_dop_fill_silence($0.baseAddress!, 2, 100, &marker) }
		XCTAssertEqual(out, silence, "silence, not carrier")
	}

	func testPCMAfterDoPIsPCMAgain() {
		fill(carrier(10))
		_ = render(10)
		cog_gain_ramp_to(cog_renderer_volume(renderer), 0.5, 0)
		fill([Float](repeating: 0.5, count: 200))
		let out = render(100)
		XCTAssertTrue(out.allSatisfy { $0 == 0.25 }, "volume applies to PCM")
		XCTAssertEqual(render(10), [Float](repeating: 0, count: 20), "and a dry ring is zeroes again")
	}

	func testAHeldRendererKeepsTheRingAndTheDACLocked() {
		fill(carrier(1000))
		_ = render(100)
		cog_renderer_set_held(renderer, true)
		XCTAssertEqual(cog_ring_readable(ring), 900)
		XCTAssertTrue(isDoP(render(300)), "DoP silence while held")
		XCTAssertEqual(cog_ring_readable(ring), 900, "nothing read while held")
		XCTAssertEqual(cog_renderer_underrun_events(renderer), 0, "a pause is not an underrun")
		cog_renderer_set_held(renderer, false)
		XCTAssertEqual(render(900), Array(carrier(1000)[200...]), "resumes exactly where it held")
	}

	func testAHeldRendererIsSilentForPCM() {
		fill([Float](repeating: 0.5, count: 200))
		cog_renderer_set_held(renderer, true)
		XCTAssertEqual(render(50), [Float](repeating: 0, count: 100))
		XCTAssertEqual(cog_ring_readable(ring), 100)
	}

	/// The carrier word, low byte clear, in every layout that can hold it.
	func testIntegerConversionKeepsTheCarrierWordExactly() {
		let input = carrier(64)
		let words = input.map { Int32(Double($0) * 2147483648.0) }
		XCTAssertTrue(words.allSatisfy { $0 & 0xFF == 0 })

		var int32 = [Int32](repeating: 0, count: input.count)
		var high = [Int32](repeating: 0, count: input.count)
		var low = [Int32](repeating: 0, count: input.count)
		input.withUnsafeBufferPointer { samples in
			cog_convert_samples(&int32, .int32, samples.baseAddress!, input.count)
			cog_convert_samples(&high, .int24High, samples.baseAddress!, input.count)
			cog_convert_samples(&low, .int24Low, samples.baseAddress!, input.count)
		}
		XCTAssertEqual(int32, words)
		XCTAssertEqual(high, words)
		XCTAssertEqual(low, words.map { $0 >> 8 })
	}
}
