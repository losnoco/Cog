//
//  CogRenderTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import CogAudio
import XCTest

final class CogRenderTests: XCTestCase {
	private var ring: OpaquePointer!
	private var renderer: OpaquePointer!

	override func setUp() {
		super.setUp()
		ring = cog_ring_create(1024, 2)
		renderer = cog_renderer_create(ring)
	}

	override func tearDown() {
		cog_renderer_destroy(renderer)
		cog_ring_destroy(ring)
		super.tearDown()
	}

	private func fill(_ count: Int, value: Float = 1.0) {
		let samples = [Float](repeating: value, count: count * 2)
		XCTAssertEqual(samples.withUnsafeBufferPointer { cog_ring_write(ring, $0.baseAddress!, count) }, count)
	}

	private func render(_ count: Int) -> (got: Int, samples: [Float]) {
		var samples = [Float](repeating: .nan, count: count * 2)
		let got = samples.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, count) }
		return (got, samples)
	}

	func testAnEmptyRingAtStartUpIsSilenceButNotAnUnderrun() {
		let (got, samples) = render(256)
		XCTAssertEqual(got, 0)
		XCTAssertTrue(samples.allSatisfy { $0 == 0 })
		XCTAssertEqual(cog_renderer_silent_frames(renderer), 256)
		XCTAssertEqual(cog_renderer_underrun_events(renderer), 0)
	}

	func testRunningDryFillsSilenceAndCountsOneUnderrun() {
		fill(100)
		let first = render(256)
		XCTAssertEqual(first.got, 100)
		XCTAssertTrue(first.samples[0..<200].allSatisfy { $0 == 1 })
		XCTAssertTrue(first.samples[200...].allSatisfy { $0 == 0 })
		XCTAssertEqual(cog_renderer_underrun_events(renderer), 1)

		// Staying dry is the same underrun, not another one.
		_ = render(256)
		XCTAssertEqual(cog_renderer_underrun_events(renderer), 1)
		XCTAssertEqual(cog_renderer_silent_frames(renderer), 156 + 256)

		fill(512)
		_ = render(256)
		_ = render(512)
		XCTAssertEqual(cog_renderer_underrun_events(renderer), 2)
		XCTAssertEqual(cog_renderer_frames_rendered(renderer), 256 * 3 + 512)
	}

	func testRenderHonoursAPendingFlush() {
		fill(300, value: 0.25)
		cog_ring_request_flush(ring)
		fill(10, value: 0.75)
		let (got, samples) = render(64)
		XCTAssertEqual(got, 10)
		XCTAssertTrue(samples[0..<20].allSatisfy { $0 == 0.75 })
	}

	func testVolumeRampsLinearlyAndLandsExactlyOnTarget() {
		let volume = cog_renderer_volume(renderer)
		cog_gain_ramp_to(volume, 0.0, 100)
		XCTAssertFalse(cog_gain_settled(volume))
		XCTAssertEqual(cog_gain_target(volume), 0.0)

		fill(200)
		let (_, samples) = render(200)
		for frame in 0..<100 {
			let expected = 1.0 - Float(frame) / 100.0
			XCTAssertEqual(samples[frame * 2], expected, accuracy: 1e-5, "frame \(frame)")
			XCTAssertEqual(samples[frame * 2 + 1], samples[frame * 2], "channels share one gain")
		}
		XCTAssertTrue(samples[200...].allSatisfy { $0 == 0 }, "exactly zero after the ramp")
		XCTAssertTrue(cog_gain_settled(volume))
		XCTAssertEqual(cog_gain_current(volume), 0.0)
	}

	func testARampSpanningCallsContinuesWhereItLeftOff() {
		let transport = cog_renderer_transport(renderer)
		cog_gain_ramp_to(transport, 0.0, 64)
		fill(64)
		let first = render(32)
		XCTAssertFalse(cog_gain_settled(transport))
		XCTAssertEqual(cog_gain_current(transport), 0.5, accuracy: 1e-5)
		let second = render(32)
		XCTAssertEqual(first.samples[62], 1.0 - 31.0 / 64.0, accuracy: 1e-5)
		XCTAssertEqual(second.samples[0], 0.5, accuracy: 1e-5)
		XCTAssertTrue(cog_gain_settled(transport))
	}

	func testZeroLengthRampJumps() {
		let volume = cog_renderer_volume(renderer)
		cog_gain_ramp_to(volume, 0.5, 0)
		fill(16)
		let (_, samples) = render(16)
		XCTAssertTrue(samples.allSatisfy { $0 == 0.5 })
		XCTAssertTrue(cog_gain_settled(volume))
	}

	func testVolumeAndTransportMultiply() {
		cog_gain_ramp_to(cog_renderer_volume(renderer), 0.5, 0)
		cog_gain_ramp_to(cog_renderer_transport(renderer), 0.5, 0)
		fill(8)
		let (_, samples) = render(8)
		XCTAssertTrue(samples.allSatisfy { $0 == 0.25 })
	}

	func testANewRequestMidRampStartsFromTheCurrentLevel() {
		let volume = cog_renderer_volume(renderer)
		cog_gain_ramp_to(volume, 0.0, 100)
		fill(100)
		_ = render(50)
		XCTAssertEqual(cog_gain_current(volume), 0.5, accuracy: 1e-5)
		cog_gain_ramp_to(volume, 1.0, 50)
		let (_, samples) = render(50)
		XCTAssertEqual(samples[0], 0.5, accuracy: 1e-5, "no jump when the direction changes")
		XCTAssertEqual(samples[98], 0.5 + 49.0 / 100.0, accuracy: 1e-5)
		XCTAssertEqual(cog_gain_current(volume), 1.0)
	}

	/// The start of playback: the renderer has not run yet when the engine
	/// asks for a fade-in, so the fade must carry its own starting level.
	/// Before, "jump to 0" then "ramp to 1" coalesced into a ramp from
	/// wherever the gain had been (1), and the track started at full level.
	func testAFadeInRequestedBeforeTheFirstRenderStartsFromSilence() {
		let transport = cog_renderer_transport(renderer)
		cog_gain_ramp_to(transport, 0, 0)
		cog_gain_ramp(transport, 0, 1, 100)
		fill(200)
		let (_, samples) = render(200)
		XCTAssertEqual(samples[0], 0, accuracy: 1e-6, "starts silent")
		XCTAssertEqual(samples[50 * 2], 0.5, accuracy: 1e-5, "rises linearly")
		XCTAssertEqual(samples[150 * 2], 1)
	}

	func testARampWithoutAStartLevelContinuesFromTheCurrentOne() {
		let volume = cog_renderer_volume(renderer)
		cog_gain_ramp_to(volume, 0.5, 0)
		fill(10)
		_ = render(10)
		cog_gain_ramp(volume, .nan, 1, 10)
		fill(10)
		let (_, samples) = render(10)
		XCTAssertEqual(samples[0], 0.5, accuracy: 1e-5)
	}

	// MARK: - Metering

	private func peaks(_ snapshot: CogMeterSnapshot) -> [Float] {
		withUnsafeBytes(of: snapshot.peak) { Array($0.bindMemory(to: Float.self)) }
	}

	private func sums(_ snapshot: CogMeterSnapshot) -> [Double] {
		withUnsafeBytes(of: snapshot.sumOfSquares) { Array($0.bindMemory(to: Double.self)) }
	}

	private func takeMeter() -> CogMeterSnapshot {
		var snapshot = CogMeterSnapshot()
		cog_renderer_take_meter(renderer, &snapshot)
		return snapshot
	}

	func testNothingIsMeteredUntilMeteringIsOn() {
		fill(256, value: 0.5)
		_ = render(256)
		let snapshot = takeMeter()
		XCTAssertEqual(snapshot.frames, 0)
		XCTAssertEqual(peaks(snapshot)[0], 0)
	}

	func testEachChannelIsMeteredAfterTheGainsAndTakingResets() {
		cog_renderer_set_metering(renderer, true)
		// Left a full-scale square wave, right silent.
		var samples = [Float](repeating: 0, count: 512)
		for frame in 0..<256 {
			samples[frame * 2] = frame % 2 == 0 ? 0.8 : -0.8
		}
		XCTAssertEqual(samples.withUnsafeBufferPointer { cog_ring_write(ring, $0.baseAddress!, 256) }, 256)
		cog_gain_ramp_to(cog_renderer_volume(renderer), 0.5, 0)
		_ = render(256)

		let snapshot = takeMeter()
		XCTAssertEqual(snapshot.frames, 256)
		XCTAssertEqual(peaks(snapshot)[0], 0.4, accuracy: 1e-6)
		XCTAssertEqual(peaks(snapshot)[1], 0)
		XCTAssertEqual((sums(snapshot)[0] / 256).squareRoot(), 0.4, accuracy: 1e-6, "a square wave's RMS is its peak")
		XCTAssertEqual(snapshot.clippedSamples, 0)

		let again = takeMeter()
		XCTAssertEqual(again.frames, 0, "taken, so reset")
		XCTAssertEqual(peaks(again)[0], 0)
	}

	func testSilenceFromARunningDryRingIsNotMetered() {
		cog_renderer_set_metering(renderer, true)
		fill(100, value: 0.25)
		_ = render(256)
		XCTAssertEqual(takeMeter().frames, 100)
	}

	func testSamplesBeyondFullScaleAreCountedAsClipped() {
		cog_renderer_set_metering(renderer, true)
		fill(64, value: 0.9)
		cog_gain_ramp_to(cog_renderer_volume(renderer), 2, 0)
		_ = render(64)
		let snapshot = takeMeter()
		XCTAssertEqual(snapshot.clippedSamples, 128)
		XCTAssertEqual(peaks(snapshot)[0], 1.8, accuracy: 1e-6)
	}

	func testChannelsBeyondTheMeterShareItsLastSlot() throws {
		let wide = try XCTUnwrap(cog_ring_create(256, 10))
		defer { cog_ring_destroy(wide) }
		let wideRenderer = try XCTUnwrap(cog_renderer_create(wide))
		defer { cog_renderer_destroy(wideRenderer) }
		cog_renderer_set_metering(wideRenderer, true)
		var frame = [Float](repeating: 0.1, count: 10)
		frame[9] = 0.7
		XCTAssertEqual(frame.withUnsafeBufferPointer { cog_ring_write(wide, $0.baseAddress!, 1) }, 1)
		var out = [Float](repeating: 0, count: 10)
		_ = out.withUnsafeMutableBufferPointer { cog_renderer_render(wideRenderer, $0.baseAddress!, 1) }

		var snapshot = CogMeterSnapshot()
		cog_renderer_take_meter(wideRenderer, &snapshot)
		XCTAssertEqual(peaks(snapshot)[7], 0.7, accuracy: 1e-6)
		XCTAssertEqual(sums(snapshot)[7], 0.01 + 0.01 + 0.49, accuracy: 1e-6, "channels 8 to 10 summed together")
	}
}
