//
//  DeviceOutput.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import AudioToolbox
import AVFoundation
import CoreAudio
import Foundation

/// The device side of the engine: an AUHAL output unit whose render block
/// pulls from a `CogRenderer`.
///
/// The render block captures only the renderer's C pointer and plain
/// integers, so the real-time thread sees no ARC, locks or Objective-C. It
/// renders interleaved float at the device's rate and channel count (at most
/// eight, as Cog always has), and AUHAL converts to whatever the hardware
/// takes.
///
/// Device selection follows Cog's `outputDevice` setting: a saved device is
/// found by ID, then by name, and otherwise the system default is used and
/// followed when it changes. Changes to the device's rate or stream format,
/// or its disappearance, are reported through `onDeviceChange`; the engine
/// decides what to do (usually rebuild at the current position).
///
/// Control methods are for one thread at a time (the engine's control queue).
public final class DeviceOutput {
	public enum Change {
		/// The device's nominal rate or stream format changed.
		case format
		/// The system default output changed while following it, or the
		/// selected device went away.
		case device
	}

	/// Called on a private queue when the device changes under us.
	public var onDeviceChange: ((Change) -> Void)?

	public private(set) var deviceID = AudioDeviceID(kAudioObjectUnknown)
	public private(set) var followsSystemDefault = true

	/// The format the engine must render in: interleaved float at the
	/// device's rate, at most eight channels.
	public private(set) var format = StreamFormat(sampleRate: 0, channels: 0)

	/// Frames between the renderer handing audio over and it being heard.
	public private(set) var latencyFrames = 0

	public private(set) var isRunning = false

	private let unit: AUAudioUnit
	private var renderer: OpaquePointer?
	private let listenerQueue = DispatchQueue(label: "Cog DeviceOutput listeners")
	private var listeners: [(AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyListenerBlock)] = []

	public init() throws {
		let description = AudioComponentDescription(componentType: kAudioUnitType_Output,
		                                            componentSubType: kAudioUnitSubType_HALOutput,
		                                            componentManufacturer: kAudioUnitManufacturer_Apple,
		                                            componentFlags: 0,
		                                            componentFlagsMask: 0)
		unit = try AUAudioUnit(componentDescription: description)
	}

	deinit {
		stop()
		removeListeners()
		unit.deallocateRenderResources()
	}

	// MARK: - Device selection

	/// Selects the device described by Cog's `outputDevice` setting (a
	/// dictionary with `deviceID` and `name`), or the system default when
	/// nil. Returns false if a described device cannot be found, in which case
	/// the system default is selected instead.
	@discardableResult
	public func selectDevice(_ description: [String: Any]?) throws -> Bool {
		var found = true
		if let description {
			let id = (description["deviceID"] as? NSNumber).map { AudioDeviceID($0.uint32Value) }
			let name = description["name"] as? String
			if let id, Self.isAliveOutput(id) {
				try use(id, followingDefault: false)
			} else if let name, let match = Self.outputDevices().first(where: { $0.name == name }) {
				try use(match.id, followingDefault: false)
			} else {
				found = false
			}
		}
		if description == nil || !found {
			guard let id = Self.systemDefaultOutput() else {
				throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioHardwareBadDeviceError))
			}
			try use(id, followingDefault: true)
		}
		return found
	}

	private func use(_ id: AudioDeviceID, followingDefault: Bool) throws {
		let wasRunning = isRunning
		if wasRunning { stop() }
		try unit.setDeviceID(id)
		deviceID = id
		followsSystemDefault = followingDefault
		try refreshFormat()
		installListeners()
		if wasRunning { try start() }
	}

	/// Re-reads the device format and sets the render format to match.
	/// Call after `onDeviceChange(.format)` before rebuilding the renderer.
	public func refreshFormat() throws {
		let hardware = unit.outputBusses[0].format
		let channels = min(Int(hardware.channelCount), 8)
		guard hardware.sampleRate > 0, channels > 0 else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
		}
		let layout = AVAudioChannelLayout(layoutTag: Self.layoutTag(channels: channels))!
		let render = AVAudioFormat(commonFormat: .pcmFormatFloat32, sampleRate: hardware.sampleRate, interleaved: true, channelLayout: layout)
		if unit.renderResourcesAllocated {
			unit.deallocateRenderResources()
		}
		try unit.inputBusses[0].setFormat(render)
		format = StreamFormat(sampleRate: hardware.sampleRate, channels: channels, channelConfig: Self.channelConfig(channels: channels))
		latencyFrames = Self.presentationLatency(of: deviceID)
	}

	// MARK: - Rendering

	/// Renders from `renderer`, whose ring must have `format.channels`
	/// channels. The renderer must outlive this output or be replaced first.
	public func attach(_ renderer: OpaquePointer) {
		precondition(Int(cog_ring_channels(cog_renderer_ring(renderer))) == format.channels)
		self.renderer = renderer
		let bytesPerFrame = UInt32(MemoryLayout<Float>.size * format.channels)
		unit.outputProvider = { _, _, frameCount, _, inputData in
			let buffers = UnsafeMutableAudioBufferListPointer(inputData)
			guard let data = buffers[0].mData else { return noErr }
			buffers[0].mDataByteSize = frameCount * bytesPerFrame
			cog_renderer_render(renderer, data.assumingMemoryBound(to: Float.self), Int(frameCount))
			return noErr
		}
	}

	public func start() throws {
		guard renderer != nil, !isRunning else { return }
		if !unit.renderResourcesAllocated {
			try unit.allocateRenderResources()
		}
		try unit.startHardware()
		isRunning = true
	}

	/// Stops the hardware. The renderer stays attached, so `start()` resumes.
	public func stop() {
		guard isRunning else { return }
		unit.stopHardware()
		isRunning = false
	}

	// MARK: - Listeners

	private func installListeners() {
		removeListeners()
		addListener(AudioObjectID(kAudioObjectSystemObject), kAudioHardwarePropertyDefaultOutputDevice) { [weak self] in
			guard let self, self.followsSystemDefault else { return }
			self.onDeviceChange?(.device)
		}
		addListener(deviceID, kAudioDevicePropertyDeviceIsAlive) { [weak self] in
			self?.onDeviceChange?(.device)
		}
		addListener(deviceID, kAudioDevicePropertyNominalSampleRate) { [weak self] in
			self?.onDeviceChange?(.format)
		}
		addListener(deviceID, kAudioDevicePropertyStreamFormat) { [weak self] in
			self?.onDeviceChange?(.format)
		}
	}

	private func addListener(_ object: AudioObjectID, _ selector: AudioObjectPropertySelector, _ action: @escaping () -> Void) {
		var address = AudioObjectPropertyAddress(mSelector: selector, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		let block: AudioObjectPropertyListenerBlock = { _, _ in action() }
		if AudioObjectAddPropertyListenerBlock(object, &address, listenerQueue, block) == noErr {
			listeners.append((object, address, block))
		}
	}

	private func removeListeners() {
		for (object, address, block) in listeners {
			var address = address
			AudioObjectRemovePropertyListenerBlock(object, &address, listenerQueue, block)
		}
		listeners.removeAll()
	}

	// MARK: - Device queries

	public struct Device {
		public let id: AudioDeviceID
		public let name: String
	}

	public static func systemDefaultOutput() -> AudioDeviceID? {
		var id = AudioDeviceID(kAudioObjectUnknown)
		guard getProperty(AudioObjectID(kAudioObjectSystemObject), kAudioHardwarePropertyDefaultOutputDevice, &id), id != kAudioObjectUnknown else {
			return nil
		}
		return id
	}

	/// Live devices that have output streams, as Cog's device menu lists them.
	public static func outputDevices() -> [Device] {
		var address = AudioObjectPropertyAddress(mSelector: kAudioHardwarePropertyDevices, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(AudioObjectID(kAudioObjectSystemObject), &address, 0, nil, &size) == noErr else { return [] }
		var ids = [AudioDeviceID](repeating: 0, count: Int(size) / MemoryLayout<AudioDeviceID>.size)
		guard AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject), &address, 0, nil, &size, &ids) == noErr else { return [] }
		return ids.compactMap { id in
			guard isAliveOutput(id) else { return nil }
			var name: Unmanaged<CFString>?
			let named = getProperty(id, kAudioDevicePropertyDeviceNameCFString, &name)
			return Device(id: id, name: named ? (name?.takeRetainedValue() as String? ?? "") : "Unknown device \(id)")
		}
	}

	static func isAliveOutput(_ id: AudioDeviceID) -> Bool {
		var alive: UInt32 = 0
		guard getProperty(id, kAudioDevicePropertyDeviceIsAlive, &alive, scope: kAudioDevicePropertyScopeOutput), alive != 0 else { return false }
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyStreamConfiguration, mScope: kAudioDevicePropertyScopeOutput, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(id, &address, 0, nil, &size) == noErr, size >= MemoryLayout<UInt32>.size else { return false }
		let list = UnsafeMutableRawPointer.allocate(byteCount: Int(size), alignment: MemoryLayout<AudioBufferList>.alignment)
		defer { list.deallocate() }
		guard AudioObjectGetPropertyData(id, &address, 0, nil, &size, list) == noErr else { return false }
		return list.assumingMemoryBound(to: AudioBufferList.self).pointee.mNumberBuffers > 0
	}

	/// Device latency, safety offset, stream latency and one I/O buffer: how
	/// long after rendering a frame is heard.
	static func presentationLatency(of id: AudioDeviceID) -> Int {
		var deviceLatency: UInt32 = 0
		var safetyOffset: UInt32 = 0
		var bufferSize: UInt32 = 0
		_ = getProperty(id, kAudioDevicePropertyLatency, &deviceLatency, scope: kAudioDevicePropertyScopeOutput)
		_ = getProperty(id, kAudioDevicePropertySafetyOffset, &safetyOffset, scope: kAudioDevicePropertyScopeOutput)
		_ = getProperty(id, kAudioDevicePropertyBufferFrameSize, &bufferSize, scope: kAudioDevicePropertyScopeOutput)

		var streamLatency: UInt32 = 0
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyStreams, mScope: kAudioDevicePropertyScopeOutput, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		if AudioObjectGetPropertyDataSize(id, &address, 0, nil, &size) == noErr, size >= MemoryLayout<AudioStreamID>.size {
			var streams = [AudioStreamID](repeating: 0, count: Int(size) / MemoryLayout<AudioStreamID>.size)
			if AudioObjectGetPropertyData(id, &address, 0, nil, &size, &streams) == noErr {
				_ = getProperty(streams[0], kAudioStreamPropertyLatency, &streamLatency)
			}
		}
		return Int(deviceLatency + safetyOffset + bufferSize + streamLatency)
	}

	@discardableResult
	static func getProperty<Value>(_ object: AudioObjectID, _ selector: AudioObjectPropertySelector, _ value: inout Value, scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal) -> Bool {
		var address = AudioObjectPropertyAddress(mSelector: selector, mScope: scope, mElement: kAudioObjectPropertyElementMain)
		var size = UInt32(MemoryLayout<Value>.size)
		return AudioObjectGetPropertyData(object, &address, 0, nil, &size, &value) == noErr
	}

	// MARK: - Channel layouts

	/// The layouts Cog has always used for 1–8 device channels.
	static func layoutTag(channels: Int) -> AudioChannelLayoutTag {
		switch channels {
		case 1: return kAudioChannelLayoutTag_Mono
		case 2: return kAudioChannelLayoutTag_Stereo
		case 3: return kAudioChannelLayoutTag_DVD_4
		case 4: return kAudioChannelLayoutTag_Quadraphonic
		case 5: return kAudioChannelLayoutTag_MPEG_5_0_A
		case 6: return kAudioChannelLayoutTag_MPEG_5_1_A
		case 7: return kAudioChannelLayoutTag_MPEG_6_1_A
		default: return kAudioChannelLayoutTag_MPEG_7_1_A
		}
	}

	/// Cog's channel-flag configuration for the same layouts.
	static func channelConfig(channels: Int) -> UInt32 {
		switch channels {
		case 1: return UInt32(AudioConfigMono)
		case 2: return UInt32(AudioConfigStereo)
		case 3: return UInt32(AudioConfig3Point0)
		case 4: return UInt32(AudioConfig4Point0)
		case 5: return UInt32(AudioConfig5Point0)
		case 6: return UInt32(AudioConfig5Point1)
		case 7: return UInt32(AudioConfig6Point1)
		default: return UInt32(AudioConfig7Point1)
		}
	}
}
