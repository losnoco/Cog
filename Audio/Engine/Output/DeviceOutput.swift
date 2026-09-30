//
//  DeviceOutput.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import AudioToolbox
import CoreAudio
import Foundation

/// The device side of the engine: an AUHAL output unit whose render
/// callback pulls from a `CogRenderer`.
///
/// The unit is driven through the C AudioUnit API with a C render callback
/// (`cog_renderer_audio_unit_render`), not through `AUAudioUnit`: the latter
/// runs Objective-C on the device's I/O thread, and loading a bundle (opening
/// the preferences for the first time, say) holds the Objective-C runtime's
/// lock long enough to make the device miss its deadline. On this path the
/// I/O thread runs no Objective-C or Swift at all. It renders interleaved
/// float at the device's rate and channel count (at most eight, as Cog always
/// has), and AUHAL converts to whatever the hardware takes.
///
/// Device selection follows Cog's `outputDevice` setting: a saved device is
/// found by ID, then by name, and otherwise the system default is used and
/// followed when it changes. Changes to the device's rate or stream format,
/// or its disappearance, are reported through `onDeviceChange`; the engine
/// decides what to do (usually rebuild at the current position).
///
/// Control methods are for one thread at a time (the engine's main thread).
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

	private let unit: AudioComponentInstance
	private var initialized = false
	private var renderer: OpaquePointer?
	private let listenerQueue = DispatchQueue(label: "Cog DeviceOutput listeners")
	private var listeners: [(AudioObjectID, AudioObjectPropertyAddress, AudioObjectPropertyListenerBlock)] = []

	public init() throws {
		var description = AudioComponentDescription(componentType: kAudioUnitType_Output,
		                                            componentSubType: kAudioUnitSubType_HALOutput,
		                                            componentManufacturer: kAudioUnitManufacturer_Apple,
		                                            componentFlags: 0,
		                                            componentFlagsMask: 0)
		guard let component = AudioComponentFindNext(nil, &description) else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_NoConnection))
		}
		var instance: AudioComponentInstance?
		try Self.check(AudioComponentInstanceNew(component, &instance))
		unit = instance!
	}

	deinit {
		stop()
		removeListeners()
		if initialized {
			AudioUnitUninitialize(unit)
		}
		AudioComponentInstanceDispose(unit)
	}

	static func check(_ status: OSStatus) throws {
		if status != noErr {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(status))
		}
	}

	private func setProperty<Value>(_ property: AudioUnitPropertyID, scope: AudioUnitScope, _ value: inout Value) throws {
		try Self.check(AudioUnitSetProperty(unit, property, scope, 0, &value, UInt32(MemoryLayout<Value>.size)))
	}

	private func uninitialize() {
		if initialized {
			AudioUnitUninitialize(unit)
			initialized = false
		}
	}

	// MARK: - Device selection

	/// Selects the device described by Cog's `outputDevice` setting (a
	/// dictionary with `deviceID` and `name`), or the system default when
	/// nil. Returns false if a described device cannot be found, in which case
	/// the system default is selected instead.
	@discardableResult
	public func selectDevice(_ description: [String: Any]?) throws -> Bool {
		let choice = Self.resolve(description)
		guard let id = choice.id else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioHardwareBadDeviceError))
		}
		try use(id, followingDefault: choice.followsDefault)
		return choice.found
	}

	/// The device an `outputDevice` setting names, without selecting it:
	/// by ID, then by name, else the system default (which is then followed).
	/// `found` is false when a described device could not be found.
	public static func resolve(_ description: [String: Any]?) -> (id: AudioDeviceID?, followsDefault: Bool, found: Bool) {
		if let description {
			let id = (description["deviceID"] as? NSNumber).map { AudioDeviceID($0.uint32Value) }
			let name = description["name"] as? String
			if let id, isAliveOutput(id) {
				return (id, false, true)
			}
			if let name, let match = outputDevices().first(where: { $0.name == name }) {
				return (match.id, false, true)
			}
			return (systemDefaultOutput(), true, false)
		}
		return (systemDefaultOutput(), true, true)
	}

	/// Whether selecting `description` would change the device in use.
	public func wouldChange(for description: [String: Any]?) -> Bool {
		let choice = Self.resolve(description)
		return choice.id != deviceID || choice.followsDefault != followsSystemDefault
	}

	private func use(_ id: AudioDeviceID, followingDefault: Bool) throws {
		if id == deviceID && followingDefault == followsSystemDefault && format.channels > 0 {
			// Already selected: re-selecting would reset the unit for nothing.
			return
		}
		let wasRunning = isRunning
		if wasRunning { stop() }
		uninitialize()
		var device = id
		try setProperty(kAudioOutputUnitProperty_CurrentDevice, scope: kAudioUnitScope_Global, &device)
		deviceID = id
		followsSystemDefault = followingDefault
		try refreshFormat()
		installListeners()
		if wasRunning { try start() }
	}

	/// The device side of the unit: the hardware's current format.
	private func hardwareFormat() -> AudioStreamBasicDescription? {
		var asbd = AudioStreamBasicDescription()
		var size = UInt32(MemoryLayout<AudioStreamBasicDescription>.size)
		guard AudioUnitGetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd, &size) == noErr else {
			return nil
		}
		return asbd
	}

	/// Re-reads the device format and sets the render format to match.
	/// Call after `onDeviceChange(.format)` before rebuilding the renderer.
	public func refreshFormat() throws {
		let wasRunning = isRunning
		if wasRunning { stop() }
		uninitialize()

		guard let hardware = hardwareFormat(), hardware.mSampleRate > 0, hardware.mChannelsPerFrame > 0 else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
		}
		let channels = min(Int(hardware.mChannelsPerFrame), 8)
		let render = StreamFormat(sampleRate: hardware.mSampleRate, channels: channels, channelConfig: Self.channelConfig(channels: channels))
		var asbd = Pump.asbd(render)
		try setProperty(kAudioUnitProperty_StreamFormat, scope: kAudioUnitScope_Input, &asbd)
		var layout = AudioChannelLayout()
		layout.mChannelLayoutTag = Self.layoutTag(channels: channels)
		// Not every device takes a layout; the stream format is what matters.
		_ = AudioUnitSetProperty(unit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, 0, &layout, UInt32(MemoryLayout<AudioChannelLayout>.size))
		try Self.check(AudioUnitInitialize(unit))
		initialized = true

		format = render
		latencyFrames = Self.presentationLatency(of: deviceID)
		if wasRunning { try start() }
	}

	/// Whether the device's rate or channel count no longer matches the
	/// render format.
	public func hardwareFormatDiffers() -> Bool {
		guard let hardware = hardwareFormat() else { return true }
		return hardware.mSampleRate != format.sampleRate || min(Int(hardware.mChannelsPerFrame), 8) != format.channels
	}

	// MARK: - Rendering

	/// Renders from `renderer`, whose ring must have `format.channels`
	/// channels. The renderer must outlive this output or be replaced first.
	public func attach(_ renderer: OpaquePointer) {
		precondition(Int(cog_ring_channels(cog_renderer_ring(renderer))) == format.channels)
		let wasRunning = isRunning
		if wasRunning { stop() }
		self.renderer = renderer
		var callback = AURenderCallbackStruct(inputProc: cog_renderer_audio_unit_render,
		                                      inputProcRefCon: UnsafeMutableRawPointer(renderer))
		try? setProperty(kAudioUnitProperty_SetRenderCallback, scope: kAudioUnitScope_Input, &callback)
		if wasRunning { try? start() }
	}

	public func start() throws {
		guard renderer != nil, !isRunning else { return }
		if !initialized {
			try Self.check(AudioUnitInitialize(unit))
			initialized = true
		}
		try Self.check(AudioOutputUnitStart(unit))
		isRunning = true
	}

	/// Stops the hardware. The renderer stays attached, so `start()` resumes.
	public func stop() {
		guard isRunning else { return }
		AudioOutputUnitStop(unit)
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

	public static func isAliveOutput(_ id: AudioDeviceID) -> Bool {
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
