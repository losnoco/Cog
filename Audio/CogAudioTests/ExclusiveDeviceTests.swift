//
//  ExclusiveDeviceTests.swift
//  CogAudioTests
//
//  Created by Marko Jurkovic on 9/30/26.
//

@testable import CogAudio
import XCTest

/// Holds a real output device exclusively. Taking a device from under
/// whatever else plays through it is not something to do to a machine
/// unasked, so these run only against the device named (in part) by the
/// environment variable `COG_EXCLUSIVE_TEST_DEVICE`, which must not be the
/// system default; xcodebuild passes it on as `TEST_RUNNER_COG_EXCLUSIVE_TEST_DEVICE`.
/// The audio is digital silence, and the device is left as it was found.
final class ExclusiveDeviceTests: XCTestCase {
	private var device: DeviceOutput.Device!
	private var stream: AudioStreamID = 0
	private var originalPhysical = AudioStreamBasicDescription()
	private var originalVirtual = AudioStreamBasicDescription()

	override func setUpWithError() throws {
		guard let name = ProcessInfo.processInfo.environment["COG_EXCLUSIVE_TEST_DEVICE"], !name.isEmpty else {
			throw XCTSkip("Set COG_EXCLUSIVE_TEST_DEVICE to the name of an output device these tests may hold")
		}
		guard let device = DeviceOutput.outputDevices().first(where: { $0.name.contains(name) }) else {
			throw XCTSkip("No output device named like \(name) is connected")
		}
		guard device.id != DeviceOutput.systemDefaultOutput() else {
			throw XCTSkip("\(device.name) is the system default output; these tests leave that alone")
		}
		self.device = device
		stream = try XCTUnwrap(DeviceOutput.outputStreams(of: device.id).first)
		originalPhysical = try XCTUnwrap(DeviceOutput.streamFormat(stream, physical: true))
		originalVirtual = try XCTUnwrap(DeviceOutput.streamFormat(stream, physical: false))
		UserDefaults.standard.removeObject(forKey: DeviceOutput.sessionKey)
	}

	override func tearDownWithError() throws {
		guard device != nil else { return }
		// Whatever happened, the device is back as it was.
		XCTAssertEqual(hogOwner(), -1, "released")
		XCTAssertTrue(same(DeviceOutput.streamFormat(stream, physical: true), originalPhysical), "physical format put back")
		XCTAssertTrue(same(DeviceOutput.streamFormat(stream, physical: false), originalVirtual), "virtual format put back")
		XCTAssertNil(UserDefaults.standard.data(forKey: DeviceOutput.sessionKey), "nothing left to recover")
	}

	private var setting: [String: Any] { ["deviceID": NSNumber(value: device.id), "name": device.name] }

	private func hogOwner() -> pid_t {
		var owner: pid_t = 0
		DeviceOutput.getProperty(device.id, kAudioDevicePropertyHogMode, &owner)
		return owner
	}

	private func same(_ a: AudioStreamBasicDescription?, _ b: AudioStreamBasicDescription) -> Bool {
		guard let a else { return false }
		return DeviceOutput.sameRepresentation(a, b) && abs(a.mSampleRate - b.mSampleRate) < 1
	}

	private func runMainLoop(until condition: () -> Bool, timeout: TimeInterval) {
		let deadline = Date().addingTimeInterval(timeout)
		while !condition() && Date() < deadline {
			RunLoop.main.run(mode: .default, before: Date(timeIntervalSinceNow: 0.01))
		}
	}

	/// Held at each rate the device offers, its stream runs integer words the
	/// system cannot mix, which are what the renderer writes; released, it
	/// is as it was.
	func testHoldingTheDeviceAtItsRatesAndGivingItBack() throws {
		let output = try DeviceOutput()
		XCTAssertTrue(try output.selectDevice(setting))
		XCTAssertNotNil(output.exclusiveStream, "one stream, a device chosen by name")

		for rate in [44100.0, 96000, 192_000] where output.supportsSampleRate(rate) {
			XCTAssertEqual(output.takeExclusive(rate: rate), .taken)
			try output.refreshFormat()
			XCTAssertTrue(output.isExclusive)
			XCTAssertEqual(hogOwner(), getpid())
			XCTAssertEqual(output.format.sampleRate, rate)
			XCTAssertEqual(output.nominalSampleRate, rate)

			let physical = try XCTUnwrap(DeviceOutput.streamFormat(stream, physical: true))
			let virtual = try XCTUnwrap(DeviceOutput.streamFormat(stream, physical: false))
			if physical.mFormatFlags & kAudioFormatFlagIsNonMixable != 0 {
				XCTAssertTrue(same(virtual, physical), "a non-mixable stream's virtual format is its physical one")
				XCTAssertTrue(output.integerRender)
			}
			var render = virtual
			render.mFormatFlags &= ~kAudioFormatFlagIsNonMixable
			XCTAssertTrue(same(output.renderFormat, render), "the renderer writes the stream's own words")
			XCTAssertEqual(output.sampleFormat, DeviceOutput.sampleFormat(of: virtual))
			XCTAssertNotNil(UserDefaults.standard.data(forKey: DeviceOutput.sessionKey), "recorded while held")
		}
		output.releaseExclusive()
		XCTAssertFalse(output.isExclusive)
		try output.refreshFormat()
		XCTAssertEqual(output.sampleFormat, .float32, "shared again, through the unit")
	}

	/// The IOProc runs, renders in the stream's channel count and drains the
	/// ring at the device's rate.
	func testTheHeldDeviceRendersFromTheRing() throws {
		let output = try DeviceOutput()
		try output.selectDevice(setting)
		XCTAssertEqual(output.takeExclusive(rate: 44100), .taken)
		try output.refreshFormat()
		defer { output.releaseExclusive() }

		let channels = output.format.channels
		let ring = try XCTUnwrap(cog_ring_create(44100, UInt32(channels)))
		let renderer = try XCTUnwrap(cog_renderer_create(ring))
		defer {
			output.stop()
			output.releaseExclusive()
			cog_renderer_destroy(renderer)
			cog_ring_destroy(ring)
		}
		XCTAssertTrue(cog_renderer_set_output_format(renderer, output.sampleFormat, 4096))
		let silence = [Float](repeating: 0, count: 22050 * channels)
		silence.withUnsafeBufferPointer { _ = cog_ring_write(ring, $0.baseAddress!, 22050) }
		output.attach(renderer)
		try output.start()
		runMainLoop(until: { cog_ring_readable(ring) == 0 }, timeout: 2)
		output.stop()

		XCTAssertEqual(cog_ring_readable(ring), 0, "half a second of audio played out")
		XCTAssertGreaterThanOrEqual(cog_renderer_frames_rendered(renderer), 22050)
	}

	/// Holds the device at 48 kHz and lets go of it as a crash would: no
	/// holder, the stream as the hold set it, and the record left behind.
	private func crash() throws -> DeviceOutput {
		let output = try DeviceOutput()
		try output.selectDevice(setting)
		XCTAssertEqual(output.takeExclusive(rate: 48000), .taken)
		var none: pid_t = -1
		DeviceOutput.setProperty(device.id, kAudioDevicePropertyHogMode, &none)
		XCTAssertEqual(hogOwner(), -1)
		XCTAssertNotNil(UserDefaults.standard.data(forKey: DeviceOutput.sessionKey))
		XCTAssertFalse(same(DeviceOutput.streamFormat(stream, physical: true), originalPhysical), "left in the held format")
		return output
	}

	/// A crash frees hog mode but leaves the stream non-mixable, where no
	/// other app can play; the next launch puts it back.
	func testTheNextLaunchPutsBackADeviceACrashLeftHeld() throws {
		let output = try crash()
		defer { output.releaseExclusive() }

		PlaybackEngine.recoverAbandonedExclusiveOutput()
		XCTAssertTrue(same(DeviceOutput.streamFormat(stream, physical: true), originalPhysical), "put back")
		XCTAssertNil(UserDefaults.standard.data(forKey: DeviceOutput.sessionKey))
	}

	/// A format chosen after the crash (in Audio MIDI Setup, say) is left
	/// alone; only what the hold left is undone.
	func testRecoveryLeavesAFormatChangedSince() throws {
		let output = try crash()
		defer { output.releaseExclusive() }

		let formats = DeviceOutput.availablePhysicalFormats(of: stream).map(\.mFormat)
		var chosen = try XCTUnwrap(formats.first { $0.mFormatFlags & kAudioFormatFlagIsNonMixable == 0 && $0.mSampleRate == 96000 && !same($0, originalPhysical) })
		XCTAssertTrue(DeviceOutput.setProperty(stream, kAudioStreamPropertyPhysicalFormat, &chosen))
		runMainLoop(until: { self.same(DeviceOutput.streamFormat(self.stream, physical: true), chosen) }, timeout: 1)

		PlaybackEngine.recoverAbandonedExclusiveOutput()
		XCTAssertTrue(same(DeviceOutput.streamFormat(stream, physical: true), chosen), "left as chosen")
		XCTAssertNil(UserDefaults.standard.data(forKey: DeviceOutput.sessionKey), "and forgotten")
	}

	/// Held by this process, the record is not a crash's.
	func testRecoveryLeavesADeviceThisProcessHolds() throws {
		let output = try DeviceOutput()
		try output.selectDevice(setting)
		XCTAssertEqual(output.takeExclusive(rate: 48000), .taken)
		defer { output.releaseExclusive() }
		let held = DeviceOutput.streamFormat(stream, physical: true)

		PlaybackEngine.recoverAbandonedExclusiveOutput()
		XCTAssertEqual(hogOwner(), getpid())
		XCTAssertTrue(same(DeviceOutput.streamFormat(stream, physical: true), try XCTUnwrap(held)))
		XCTAssertNotNil(UserDefaults.standard.data(forKey: DeviceOutput.sessionKey))
	}

	/// With exclusive output on, the device runs at each track's rate: tracks
	/// at one rate follow each other gaplessly whatever their bit depth, a
	/// track at another rebuilds with the device still held, and stopping
	/// gives it back. What is heard is reported bit perfect and exclusive.
	func testTheEngineHoldsTheDeviceAtEachTracksRate() throws {
		// In the argument domain, which is never saved: a crash here must not
		// leave other tests playing through this device.
		let arguments = UserDefaults.standard.volatileDomain(forName: UserDefaults.argumentDomain)
		var settings = arguments
		settings["outputDevice"] = setting
		settings[PlaybackEngine.exclusiveKey] = true
		UserDefaults.standard.setVolatileDomain(settings, forName: UserDefaults.argumentDomain)
		defer { UserDefaults.standard.setVolatileDomain(arguments, forName: UserDefaults.argumentDomain) }
		let decoders: [String: () -> CogDecoder] = [
			"cd": { IntegerMemoryDecoder(samples: [Int32](repeating: 0, count: 44100), bits: 16, sampleRate: 44100) },
			"cd24": { IntegerMemoryDecoder(samples: [Int32](repeating: 0, count: 44100), bits: 24, sampleRate: 44100) },
			"hires": { IntegerMemoryDecoder(samples: [Int32](repeating: 0, count: 96000), bits: 24, sampleRate: 96000) },
		]
		let host = RecordingHost()
		let queued: [String] = ["cd24", "hires"]
		host.queue = queued.map { EngineTrack(url: URL(string: "memory://\($0)")!, userInfo: $0, gain: 1) }
		let engine = PlaybackEngine()
		engine.host = host
		engine.opener = { track in decoders[track.url.host ?? ""]?() }

		var rates: [String: Double] = [:]
		var holders: [String: pid_t] = [:]
		XCTAssertTrue(engine.play(URL(string: "memory://cd")!, userInfo: "cd", rgInfo: nil, startPaused: false, seekTo: 0))
		rates["cd"] = DeviceOutput.streamFormat(stream, physical: true)?.mSampleRate
		holders["cd"] = hogOwner()
		runMainLoop(until: { host.log.contains("begin cd24") }, timeout: 5)
		XCTAssertEqual(engine.pipelineBuilds, 1, "24 bits at the same rate joined the stream")
		runMainLoop(until: { host.log.contains("begin hires") }, timeout: 5)
		rates["hires"] = DeviceOutput.streamFormat(stream, physical: true)?.mSampleRate
		holders["hires"] = hogOwner()
		XCTAssertEqual(engine.pipelineBuilds, 2, "another rate, another pipeline")
		runMainLoop(until: { host.stopped }, timeout: 5)

		XCTAssertEqual(rates, ["cd": 44100, "hires": 96000])
		XCTAssertEqual(holders, ["cd": getpid(), "hires": getpid()])
		XCTAssertFalse(host.log.contains("restart"), "the held device started")
		XCTAssertEqual(host.log.filter { !$0.hasPrefix("next") }, ["played cd", "begin cd24", "played cd24", "begin hires", "played hires", "stopped after hires"])

		let heard = host.outputStatuses.compactMap { $0 }
		XCTAssertFalse(heard.isEmpty)
		for status in heard {
			XCTAssertEqual(status[CogAudioOutputExclusiveKey] as? Bool, true)
			XCTAssertEqual(status[CogAudioOutputModificationsKey] as? [String], [], "bit perfect")
			var render = AudioStreamBasicDescription()
			(status[CogAudioOutputRenderFormatKey] as? NSValue)?.getValue(&render, size: MemoryLayout<AudioStreamBasicDescription>.size)
			XCTAssertEqual(render.mFormatFlags & kAudioFormatFlagIsFloat, 0, "integer words")
		}
		XCTAssertTrue(host.outputStatuses.last.map { $0 == nil } ?? false, "cleared on stopping")
	}
}
