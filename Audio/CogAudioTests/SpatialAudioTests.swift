//
//  SpatialAudioTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 10/3/26.
//

@testable import CogAudio
import XCTest

/// Surround to a stereo device goes through Apple's spatial mixer: when it is
/// planned, and that the mixer path renders on a real device.
final class SpatialAudioTests: XCTestCase {
	typealias Plan = PlaybackEngine.OutputPlan

	private let stereo: [AnyHashable: Any] = ["bitsPerSample": 16, "sampleRate": 44100.0, "channels": 2]
	private let mono: [AnyHashable: Any] = ["bitsPerSample": 16, "sampleRate": 44100.0, "channels": 1]
	private let surround51: [AnyHashable: Any] = ["bitsPerSample": 24, "sampleRate": 48000.0, "channels": 6]
	private let surround71: [AnyHashable: Any] = ["bitsPerSample": 24, "sampleRate": 48000.0, "channels": 8]
	private let dsd64: [AnyHashable: Any] = ["bitsPerSample": 1, "sampleRate": 2_822_400.0, "channels": 2]

	private var headphones: PlaybackEngine.DevicePlanning {
		PlaybackEngine.DevicePlanning(channels: 2, rates: [AudioValueRange(mMinimum: 44100, mMaximum: 192_000)], spatial: true)
	}

	private let spatial = Plan(spatial: true)

	// MARK: - Planning

	func testSurroundOnHeadphonesIsSpatialized() {
		XCTAssertEqual(PlaybackEngine.plan(for: surround51, device: headphones), spatial)
		XCTAssertEqual(PlaybackEngine.plan(for: surround71, device: headphones), spatial)
	}

	func testStereoIsSpatializedOnlyWhenFreeSurroundUpmixesIt() {
		XCTAssertEqual(PlaybackEngine.plan(for: stereo, device: headphones), .shared)
		XCTAssertEqual(PlaybackEngine.plan(for: mono, device: headphones), .shared)
		var upmixing = headphones
		upmixing.freeSurround = true
		XCTAssertEqual(PlaybackEngine.plan(for: stereo, device: upmixing), spatial)
		XCTAssertEqual(PlaybackEngine.plan(for: mono, device: upmixing), .shared, "FreeSurround takes stereo only")
	}

	func testADeviceWithSpatialAudioOffDownmixes() {
		var downmixing = headphones
		downmixing.spatial = false
		downmixing.freeSurround = true
		XCTAssertEqual(PlaybackEngine.plan(for: surround51, device: downmixing), .shared)
		XCTAssertEqual(PlaybackEngine.plan(for: stereo, device: downmixing), .shared)
	}

	func testHeldAndDoPOutputIsNeverSpatialized() {
		var held = headphones
		held.exclusive = true
		XCTAssertEqual(PlaybackEngine.plan(for: surround51, device: held), Plan(deviceRate: 48000, exclusive: true))

		var dop = headphones
		dop.dop = true
		dop.freeSurround = true
		XCTAssertEqual(PlaybackEngine.plan(for: dsd64, device: dop), Plan(deviceRate: 176_400, dop: true, exclusive: true))
	}

	func testSurroundLayoutsShareTheBedButNotStereo() {
		XCTAssertTrue(PlaybackEngine.admits(surround71, into: spatial, device: headphones), "5.1 to 7.1 is gapless")
		XCTAssertTrue(PlaybackEngine.admits(surround51, into: spatial, device: headphones))
		XCTAssertFalse(PlaybackEngine.admits(stereo, into: spatial, device: headphones), "stereo is not spatialized")
		XCTAssertFalse(PlaybackEngine.admits(surround51, into: .shared, device: headphones), "surround is")
		XCTAssertTrue(PlaybackEngine.admits(stereo, into: .shared, device: headphones))
	}

	// MARK: - Devices

	func testAutomaticSpatialOutputGuessesFromTheTransport() {
		typealias Output = DeviceOutput.SpatialOutput
		func guess(_ transport: UInt32, _ channels: Int, _ source: UInt32? = nil) -> Output {
			DeviceOutput.automaticSpatialOutput(transport: transport, outputChannels: channels, dataSource: source)
		}
		XCTAssertEqual(guess(kAudioDeviceTransportTypeBluetooth, 2), .headphones)
		XCTAssertEqual(guess(kAudioDeviceTransportTypeBluetoothLE, 2), .headphones)
		XCTAssertEqual(guess(kAudioDeviceTransportTypeBluetooth, 1), .off, "a headset in call mode")
		XCTAssertEqual(guess(kAudioDeviceTransportTypeUSB, 2), .headphones, "a USB DAC")
		XCTAssertEqual(guess(kAudioDeviceTransportTypeUSB, 8), .off, "a surround interface")
		XCTAssertEqual(guess(kAudioDeviceTransportTypeBuiltIn, 2, DeviceOutput.headphonesDataSource), .headphones)
		XCTAssertEqual(guess(kAudioDeviceTransportTypeBuiltIn, 2), .headphones, "Apple silicon's headphone device")
		XCTAssertEqual(guess(kAudioDeviceTransportTypeBuiltIn, 2, DeviceOutput.speakersDataSource), .speakers)
		XCTAssertEqual(guess(kAudioDeviceTransportTypeHDMI, 2), .off)
		XCTAssertEqual(guess(kAudioDeviceTransportTypeDisplayPort, 2), .off)
	}

	/// Whatever the default device is, the mixer path must render: the
	/// engine renders the 7.1 bed, the unit gets the mixer's stereo.
	func testTheMixerRendersTheBedOnTheDefaultDevice() throws {
		let output = try DeviceOutput()
		try output.selectDevice(nil)
		let deviceFormat = output.format
		try output.refreshFormat(spatial: true)
		XCTAssertTrue(output.isSpatial)
		XCTAssertEqual(output.format, DeviceOutput.spatialFormat(sampleRate: deviceFormat.sampleRate))
		XCTAssertEqual(output.renderFormat.mChannelsPerFrame, 8)
		XCTAssertFalse(output.hardwareFormatDiffers(), "the bed is not a device format change")

		let format = output.format
		let ring = try XCTUnwrap(cog_ring_create(Int(format.sampleRate / 10), UInt32(format.channels)))
		let renderer = try XCTUnwrap(cog_renderer_create(ring))
		defer {
			output.stop()
			cog_renderer_destroy(renderer)
			cog_ring_destroy(ring)
		}
		output.attach(renderer)
		try output.start()
		let deadline = Date().addingTimeInterval(2)
		while cog_renderer_frames_rendered(renderer) < UInt64(format.sampleRate / 10), Date() < deadline {
			Thread.sleep(forTimeInterval: 0.01)
		}
		XCTAssertGreaterThanOrEqual(cog_renderer_frames_rendered(renderer), UInt64(format.sampleRate / 10), "the mixer pulls the renderer")

		output.setHeadTracking(true)
		output.setHeadTracking(false)

		output.stop()
		try output.refreshFormat(spatial: false)
		XCTAssertFalse(output.isSpatial)
		XCTAssertEqual(output.format, deviceFormat, "back to the device's own channels")
	}

	// MARK: - Pausing

	/// Whether any process has the device's I/O running.
	private func deviceIsRunningSomewhere(_ id: AudioDeviceID) -> Bool {
		var running: UInt32 = 0
		DeviceOutput.getProperty(id, kAudioDevicePropertyDeviceIsRunningSomewhere, &running)
		return running != 0
	}

	private func runMainLoop(until condition: () -> Bool, timeout: TimeInterval) {
		let deadline = Date().addingTimeInterval(timeout)
		while !condition() && Date() < deadline {
			RunLoop.main.run(mode: .default, before: Date(timeIntervalSinceNow: 0.01))
		}
	}

	/// Whether this process has output running on any device, as Core
	/// Audio sees it; nil before macOS 14.
	private func thisProcessIsRunningOutput() -> Bool? {
		guard #available(macOS 14, *) else { return nil }
		var pid = getpid()
		var process = AudioObjectID(kAudioObjectUnknown)
		var address = AudioObjectPropertyAddress(mSelector: kAudioHardwarePropertyTranslatePIDToProcessObject, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		var size = UInt32(MemoryLayout<AudioObjectID>.size)
		guard AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject), &address, UInt32(MemoryLayout<pid_t>.size), &pid, &size, &process) == noErr,
		      process != kAudioObjectUnknown else { return nil }
		var running: UInt32 = 0
		guard DeviceOutput.getProperty(process, kAudioProcessPropertyIsRunningOutput, &running) else { return nil }
		return running != 0
	}

	/// Paused surround through the spatial mixer is suspended like any
	/// other output: the device itself stops, so it no longer keeps the
	/// machine awake. Another process can keep the device running (an app
	/// recording from it, as FineTune does while anything plays); then the
	/// test only checks, through Core Audio's process objects, that this
	/// process let go.
	func testPausedSpatialOutputSuspendsTheDevice() throws {
		let device = try XCTUnwrap(DeviceOutput.systemDefaultOutput())
		try XCTSkipUnless(DeviceOutput.spatialOutput(of: device) != .off, "the default device is not spatialized")
		UserDefaults.standard.set(true, forKey: PlaybackEngine.spatialKey)
		defer { UserDefaults.standard.removeObject(forKey: PlaybackEngine.spatialKey) }

		let frames = 480_000 // 10 s
		let samples = (0..<(frames * 6)).map { Float(sin(Double($0 / 6) * 0.05)) * 0.1 }
		let host = RecordingHost()
		let engine = PlaybackEngine()
		engine.host = host
		engine.opener = { _ in MemoryDecoder(samples: samples, sampleRate: 48000, channels: 6) }
		engine.volume = 0
		engine.suspendDelay = 0.5
		defer { engine.stop() }

		XCTAssertTrue(engine.play(URL(string: "memory://surround")!, userInfo: "surround", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { engine.amountPlayed > 0.3 }, timeout: 5)
		XCTAssertTrue(host.outputStatuses.compactMap { $0 }.contains { ($0[CogAudioOutputModificationsKey] as? [String])?.contains(CogAudioOutputModificationSpatialAudio) == true },
		              "played through the spatial mixer")
		XCTAssertEqual(thisProcessIsRunningOutput(), true, "this process plays")

		engine.pause()
		runMainLoop(until: { !engine.isDeviceRunning }, timeout: 3)
		XCTAssertFalse(engine.isDeviceRunning, "suspended after the delay")
		runMainLoop(until: { !deviceIsRunningSomewhere(device) }, timeout: 2)
		XCTAssertEqual(thisProcessIsRunningOutput(), false, "this process let go")
		guard deviceIsRunningSomewhere(device) else { return }
		throw XCTSkip("this process let go, but another holds the device running")
	}

	/// After surround through the spatial mixer, the engine rebuilds for a
	/// stereo track (not spatialized, and resampled to the device); pausing
	/// that must suspend the device just the same.
	func testPausedStereoAfterSpatialSurroundSuspendsTheDevice() throws {
		let device = try XCTUnwrap(DeviceOutput.systemDefaultOutput())
		try XCTSkipUnless(DeviceOutput.spatialOutput(of: device) != .off, "the default device is not spatialized")
		UserDefaults.standard.set(true, forKey: PlaybackEngine.spatialKey)
		defer { UserDefaults.standard.removeObject(forKey: PlaybackEngine.spatialKey) }

		let surround = (0..<(48000 * 6)).map { Float(sin(Double($0 / 6) * 0.05)) * 0.1 } // 1 s
		let stereo = (0..<(96000 * 10 * 2)).map { Float(sin(Double($0 / 2) * 0.03)) * 0.1 } // 10 s at 96 kHz
		let host = RecordingHost()
		host.queue = [EngineTrack(url: URL(string: "memory://stereo")!, userInfo: "stereo", gain: 1)]
		let engine = PlaybackEngine()
		engine.host = host
		engine.opener = { track in
			track.url.host == "stereo"
				? MemoryDecoder(samples: stereo, sampleRate: 96000, channels: 2)
				: MemoryDecoder(samples: surround, sampleRate: 48000, channels: 6)
		}
		engine.volume = 0
		engine.suspendDelay = 0.5
		defer { engine.stop() }

		XCTAssertTrue(engine.play(URL(string: "memory://surround")!, userInfo: "surround", rgInfo: nil, startPaused: false, seekTo: 0))
		runMainLoop(until: { host.log.contains("begin stereo") && engine.amountPlayed > 0.5 }, timeout: 8)
		XCTAssertTrue(host.log.contains("begin stereo"), "moved on to the stereo track")
		let spatial = host.outputStatuses.compactMap { $0 }.map { ($0[CogAudioOutputModificationsKey] as? [String])?.contains(CogAudioOutputModificationSpatialAudio) == true }
		XCTAssertEqual(spatial.first, true, "surround was spatialized")
		XCTAssertEqual(spatial.last, false, "stereo is not")
		XCTAssertEqual(thisProcessIsRunningOutput(), true, "this process plays")

		engine.pause()
		runMainLoop(until: { !engine.isDeviceRunning }, timeout: 3)
		XCTAssertFalse(engine.isDeviceRunning, "suspended after the delay")
		runMainLoop(until: { !deviceIsRunningSomewhere(device) }, timeout: 2)
		XCTAssertEqual(thisProcessIsRunningOutput(), false, "this process let go")
		guard deviceIsRunningSomewhere(device) else { return }
		throw XCTSkip("this process let go, but another holds the device running")
	}
}
