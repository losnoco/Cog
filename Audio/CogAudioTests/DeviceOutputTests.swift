//
//  DeviceOutputTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import CogAudio
import XCTest

/// Runs against the machine's real default output device. Only silence is
/// rendered: the ring stays empty.
final class DeviceOutputTests: XCTestCase {
	func testTheDefaultDeviceIsListed() throws {
		let defaultID = try XCTUnwrap(DeviceOutput.systemDefaultOutput())
		XCTAssertTrue(DeviceOutput.outputDevices().contains { $0.id == defaultID })
	}

	func testRendersFromTheRendererOnTheDefaultDevice() throws {
		let output = try DeviceOutput()
		try output.selectDevice(nil)
		XCTAssertTrue(output.followsSystemDefault)
		XCTAssertEqual(output.deviceID, DeviceOutput.systemDefaultOutput())

		let format = output.format
		XCTAssertGreaterThan(format.sampleRate, 0)
		XCTAssertTrue((1...8).contains(format.channels))
		XCTAssertGreaterThan(output.latencyFrames, 0)
		print("[DeviceOutput] \(format.sampleRate) Hz, \(format.channels) ch, latency \(output.latencyFrames) frames")

		let ring = try XCTUnwrap(cog_ring_create(Int(format.sampleRate / 10), UInt32(format.channels)))
		let renderer = try XCTUnwrap(cog_renderer_create(ring))
		defer {
			output.stop()
			cog_renderer_destroy(renderer)
			cog_ring_destroy(ring)
		}
		output.attach(renderer)
		try output.start()
		XCTAssertTrue(output.isRunning)

		let deadline = Date().addingTimeInterval(2)
		while cog_renderer_frames_rendered(renderer) < UInt64(format.sampleRate / 10), Date() < deadline {
			Thread.sleep(forTimeInterval: 0.01)
		}
		XCTAssertGreaterThanOrEqual(cog_renderer_frames_rendered(renderer), UInt64(format.sampleRate / 10))
		XCTAssertEqual(cog_renderer_underrun_events(renderer), 0, "an empty ring from the start is not an underrun")

		output.stop()
		XCTAssertFalse(output.isRunning)
		let stopped = cog_renderer_frames_rendered(renderer)
		Thread.sleep(forTimeInterval: 0.1)
		XCTAssertEqual(cog_renderer_frames_rendered(renderer), stopped, "nothing renders after stop")

		try output.start()
		let resumed = Date().addingTimeInterval(2)
		while cog_renderer_frames_rendered(renderer) == stopped, Date() < resumed {
			Thread.sleep(forTimeInterval: 0.01)
		}
		XCTAssertGreaterThan(cog_renderer_frames_rendered(renderer), stopped, "start resumes")
	}

	func testAnUnknownSavedDeviceFallsBackToTheDefault() throws {
		let output = try DeviceOutput()
		let found = try output.selectDevice(["deviceID": NSNumber(value: UInt32.max - 1), "name": "No Such Device"])
		XCTAssertFalse(found)
		XCTAssertTrue(output.followsSystemDefault)
		XCTAssertEqual(output.deviceID, DeviceOutput.systemDefaultOutput())
	}
}
