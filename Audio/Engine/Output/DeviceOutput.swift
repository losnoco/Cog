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

	/// Whether this process holds the device exclusively (hog mode, mixing
	/// off), for DoP.
	public private(set) var isExclusive = false

	/// Whether the unit takes 24-bit integer (high-aligned in 32 bits) rather
	/// than float: the DoP carrier format, set by `refreshFormat(integer:)`.
	public private(set) var integerRender = false

	/// The most frames the unit asks the renderer for at once.
	public var maximumFramesPerSlice: Int {
		var frames: UInt32 = 0
		var size = UInt32(MemoryLayout<UInt32>.size)
		guard AudioUnitGetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &frames, &size) == noErr else {
			return 4096
		}
		return Int(frames)
	}

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
		releaseExclusive()
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
		releaseExclusive()
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

	/// Re-reads the device format and sets the render format to match, as
	/// float or (for a DoP carrier) 24-bit integer. Call after
	/// `onDeviceChange(.format)` before rebuilding the renderer.
	///
	/// `sampleRate` overrides the rate read from the unit, which can lag a
	/// moment behind a nominal rate just set.
	public func refreshFormat(integer: Bool = false, sampleRate: Double? = nil) throws {
		let wasRunning = isRunning
		if wasRunning { stop() }
		uninitialize()

		guard let hardware = hardwareFormat(), hardware.mSampleRate > 0, hardware.mChannelsPerFrame > 0 else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
		}
		let channels = min(Int(hardware.mChannelsPerFrame), 8)
		// The device's nominal rate over the unit's, which can lag behind it.
		let nominal = nominalSampleRate
		let rate = sampleRate ?? (nominal > 0 ? nominal : hardware.mSampleRate)
		let render = StreamFormat(sampleRate: rate, channels: channels, channelConfig: Self.channelConfig(channels: channels))
		// Held exclusively, the device's stream is integer and the unit's
		// input matches it word for word; otherwise 24-bit high-aligned.
		var asbd = integer ? (isExclusive ? Self.int32ASBD(render) : Self.integerASBD(render)) : Pump.asbd(render)
		try setProperty(kAudioUnitProperty_StreamFormat, scope: kAudioUnitScope_Input, &asbd)
		integerRender = integer
		var layout = AudioChannelLayout()
		layout.mChannelLayoutTag = Self.layoutTag(channels: channels)
		// Not every device takes a layout; the stream format is what matters.
		_ = AudioUnitSetProperty(unit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, 0, &layout, UInt32(MemoryLayout<AudioChannelLayout>.size))
		configureBufferSize(sampleRate: rate)
		try Self.check(AudioUnitInitialize(unit))
		initialized = true

		format = render
		latencyFrames = Self.presentationLatency(of: deviceID)
		if wasRunning { try start() }
	}

	/// The I/O buffer to ask for, in milliseconds (hidden setting
	/// `outputBufferMilliseconds`).
	static var bufferMilliseconds: Double {
		let setting = UserDefaults.standard.double(forKey: "outputBufferMilliseconds")
		return setting > 0 ? setting : 20
	}

	/// Asks the device for an I/O buffer of about `bufferMilliseconds`.
	///
	/// Devices default to 512 frames whatever their rate, which at 384 kHz is
	/// a 1.33 ms deadline: any scheduling hiccup (WindowServer compositing a
	/// newly shown view is enough) overloads the I/O cycle, and CoreAudio
	/// logs "Overload possibly due to client timeout". A player has no use
	/// for that little latency, so trade it for headroom. The request is
	/// clamped to what the device allows; the render unit must accept slices
	/// that large too.
	private func configureBufferSize(sampleRate: Double) {
		var range = AudioValueRange()
		var frames = UInt32(max(512, (sampleRate * Self.bufferMilliseconds / 1000).rounded()))
		if Self.getProperty(deviceID, kAudioDevicePropertyBufferFrameSizeRange, &range, scope: kAudioDevicePropertyScopeOutput), range.mMaximum > 0 {
			frames = UInt32(min(max(Double(frames), range.mMinimum), range.mMaximum))
		}
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyBufferFrameSize, mScope: kAudioDevicePropertyScopeOutput, mElement: kAudioObjectPropertyElementMain)
		var requested = frames
		let status = AudioObjectSetPropertyData(deviceID, &address, 0, nil, UInt32(MemoryLayout<UInt32>.size), &requested)
		var slice = max(frames, 4096)
		_ = AudioUnitSetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &slice, UInt32(MemoryLayout<UInt32>.size))

		var actual: UInt32 = 0
		_ = Self.getProperty(deviceID, kAudioDevicePropertyBufferFrameSize, &actual, scope: kAudioDevicePropertyScopeOutput)
		EngineLog.logger.info("I/O buffer: asked for \(frames) frames (\(Double(frames) / sampleRate * 1000, format: .fixed(precision: 1)) ms), status \(status), device now \(actual) frames")
	}

	/// 24-bit samples high-aligned in 32-bit words, as DoP DACs expect: no
	/// float conversion can then disturb the carrier.
	static func integerASBD(_ format: StreamFormat) -> AudioStreamBasicDescription {
		let bytesPerFrame = UInt32(MemoryLayout<Int32>.size * format.channels)
		return AudioStreamBasicDescription(mSampleRate: format.sampleRate, mFormatID: kAudioFormatLinearPCM,
		                                   mFormatFlags: kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsAlignedHigh | kAudioFormatFlagsNativeEndian,
		                                   mBytesPerPacket: bytesPerFrame, mFramesPerPacket: 1,
		                                   mBytesPerFrame: bytesPerFrame, mChannelsPerFrame: UInt32(format.channels),
		                                   mBitsPerChannel: 24, mReserved: 0)
	}

	static func int32ASBD(_ format: StreamFormat) -> AudioStreamBasicDescription {
		var asbd = integerASBD(format)
		asbd.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian
		asbd.mBitsPerChannel = 32
		return asbd
	}

	// MARK: - Exclusive access

	/// Takes the device for this process alone, as DoP needs on macOS: hog
	/// mode, mixing off, and the device's stream set to integer at `rate`, so
	/// no system mixer, volume or float stage sits between the renderer and
	/// the DAC. Only for a device chosen by name, never the system default,
	/// which would silence every other app. Call `refreshFormat` afterwards.
	public func takeExclusive(rate: Double) -> Bool {
		guard !followsSystemDefault else { return false }
		if !isExclusive {
			// macOS will not hand a device over while it is the system's
			// default output (or its sound-effects output), so move those
			// elsewhere for as long as it is held, as Pine Player does.
			guard moveSystemDefaultsAway() else { return false }
			guard hog() else {
				restoreSystemDefaults()
				return false
			}
			isExclusive = true
			var mixing: UInt32 = 0
			if !Self.setProperty(deviceID, kAudioDevicePropertySupportsMixing, &mixing) {
				EngineLog.logger.notice("The device would not turn mixing off")
			}
		}
		setIntegerPhysicalFormat(rate: rate)
		EngineLog.logger.notice("Holding the device exclusively at \(rate, format: .fixed(precision: 0)) Hz")
		return true
	}

	/// Takes hog mode, allowing the system a moment to move other clients
	/// off the device after its default role was taken away.
	private func hog() -> Bool {
		var owner: pid_t = -1
		for _ in 0..<25 {
			var pid = getpid()
			let set = Self.setProperty(deviceID, kAudioDevicePropertyHogMode, &pid)
			if Self.getProperty(deviceID, kAudioDevicePropertyHogMode, &owner), owner == getpid() {
				return true
			}
			if !set && owner != -1 { break }
			usleep(20000)
		}
		EngineLog.logger.notice("Could not take the device exclusively (owner \(owner))")
		return false
	}

	/// Gives the device back to the system mixer, and the system its
	/// defaults.
	public func releaseExclusive() {
		guard isExclusive else { return }
		var mixing: UInt32 = 1
		_ = Self.setProperty(deviceID, kAudioDevicePropertySupportsMixing, &mixing)
		var none: pid_t = -1
		_ = Self.setProperty(deviceID, kAudioDevicePropertyHogMode, &none)
		isExclusive = false
		restoreSystemDefaults()
		EngineLog.logger.notice("Released the device")
	}

	/// System defaults moved off the device while it is held, to put back.
	private var movedDefaults: [(selector: AudioObjectPropertySelector, original: AudioDeviceID, replacement: AudioDeviceID)] = []

	private static let defaultSelectors = [kAudioHardwarePropertyDefaultOutputDevice, kAudioHardwarePropertyDefaultSystemOutputDevice]

	private func moveSystemDefaultsAway() -> Bool {
		let system = AudioObjectID(kAudioObjectSystemObject)
		for selector in Self.defaultSelectors {
			var current = AudioDeviceID(kAudioObjectUnknown)
			guard Self.getProperty(system, selector, &current), current == deviceID else { continue }
			guard var replacement = Self.fallbackOutput(excluding: deviceID) else {
				EngineLog.logger.notice("The device is the system default and there is no other output to move the default to")
				restoreSystemDefaults()
				return false
			}
			guard Self.setProperty(system, selector, &replacement) else {
				EngineLog.logger.notice("Could not move the system default output off the device")
				restoreSystemDefaults()
				return false
			}
			movedDefaults.append((selector, current, replacement))
			EngineLog.logger.notice("Moved a system default output to device \(replacement) while holding the device")
		}
		return true
	}

	/// Puts back the defaults moved away, unless they were changed since.
	private func restoreSystemDefaults() {
		let system = AudioObjectID(kAudioObjectSystemObject)
		for moved in movedDefaults.reversed() {
			var current = AudioDeviceID(kAudioObjectUnknown)
			guard Self.getProperty(system, moved.selector, &current), current == moved.replacement, Self.isAliveOutput(moved.original) else { continue }
			var original = moved.original
			Self.setProperty(system, moved.selector, &original)
		}
		movedDefaults.removeAll()
	}

	/// Another output the system default can move to: built-in speakers if
	/// there are any, else the first that can be a default.
	static func fallbackOutput(excluding excluded: AudioDeviceID) -> AudioDeviceID? {
		let candidates = outputDevices().map(\.id).filter { id in
			var canBeDefault: UInt32 = 0
			return id != excluded && getProperty(id, kAudioDevicePropertyDeviceCanBeDefaultDevice, &canBeDefault, scope: kAudioDevicePropertyScopeOutput) && canBeDefault != 0
		}
		let builtIn = candidates.first { id in
			var transport: UInt32 = 0
			return getProperty(id, kAudioDevicePropertyTransportType, &transport) && transport == kAudioDeviceTransportTypeBuiltIn
		}
		return builtIn ?? candidates.first
	}

	/// Sets the output streams to the widest integer format they offer at
	/// `rate` with the render channel count (32 bits, else 24).
	private func setIntegerPhysicalFormat(rate: Double) {
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyStreams, mScope: kAudioObjectPropertyScopeOutput, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(deviceID, &address, 0, nil, &size) == noErr, size > 0 else { return }
		var streams = [AudioStreamID](repeating: 0, count: Int(size) / MemoryLayout<AudioStreamID>.size)
		guard AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &streams) == noErr else { return }
		for stream in streams {
			var formatsAddress = AudioObjectPropertyAddress(mSelector: kAudioStreamPropertyAvailablePhysicalFormats, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
			var formatsSize: UInt32 = 0
			guard AudioObjectGetPropertyDataSize(stream, &formatsAddress, 0, nil, &formatsSize) == noErr, formatsSize > 0 else { continue }
			var formats = [AudioStreamRangedDescription](repeating: AudioStreamRangedDescription(), count: Int(formatsSize) / MemoryLayout<AudioStreamRangedDescription>.size)
			guard AudioObjectGetPropertyData(stream, &formatsAddress, 0, nil, &formatsSize, &formats) == noErr else { continue }
			let candidates = formats.map(\.mFormat).filter {
				$0.mFormatID == kAudioFormatLinearPCM && $0.mFormatFlags & kAudioFormatFlagIsFloat == 0 &&
					abs($0.mSampleRate - rate) < 1 && Int($0.mChannelsPerFrame) == format.channels && $0.mBitsPerChannel >= 24
			}
			guard var best = candidates.max(by: { $0.mBitsPerChannel < $1.mBitsPerChannel }) else {
				EngineLog.logger.notice("Stream \(stream) offers no integer format at \(rate, format: .fixed(precision: 0)) Hz")
				continue
			}
			if !Self.setProperty(stream, kAudioStreamPropertyPhysicalFormat, &best) {
				EngineLog.logger.notice("Stream \(stream) would not take \(best.mBitsPerChannel)-bit integer")
			}
		}
	}

	// MARK: - Device rate

	/// Whether the device offers `rate`. A device that does not list its
	/// rates is given the benefit of the doubt, as OutputCoreAudio did.
	public func supportsSampleRate(_ rate: Double) -> Bool {
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyAvailableNominalSampleRates,
		                                         mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(deviceID, &address, 0, nil, &size) == noErr, size > 0 else { return true }
		var ranges = [AudioValueRange](repeating: AudioValueRange(), count: Int(size) / MemoryLayout<AudioValueRange>.size)
		guard AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &ranges) == noErr else { return true }
		return ranges.contains { rate >= $0.mMinimum - 1 && rate <= $0.mMaximum + 1 }
	}

	/// The device's nominal rate.
	public var nominalSampleRate: Double {
		var rate: Float64 = 0
		_ = Self.getProperty(deviceID, kAudioDevicePropertyNominalSampleRate, &rate)
		return rate
	}

	/// Sets the device's nominal rate (for everything using the device, as
	/// Audio MIDI Setup would) and waits up to half a second for it to take.
	/// Call `refreshFormat` afterwards.
	public func setNominalSampleRate(_ rate: Double) -> Bool {
		if abs(nominalSampleRate - rate) < 1 { return true }
		guard supportsSampleRate(rate) else { return false }
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyNominalSampleRate,
		                                         mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		var requested = Float64(rate)
		guard AudioObjectSetPropertyData(deviceID, &address, 0, nil, UInt32(MemoryLayout<Float64>.size), &requested) == noErr else {
			return false
		}
		for _ in 0..<50 {
			if abs(nominalSampleRate - rate) < 1 { return true }
			usleep(10000)
		}
		return false
	}

	/// Whether the device's rate or channel count no longer matches the
	/// render format.
	/// The rate is the device's own nominal rate: the unit's view of it lags a
	/// change made elsewhere (Audio MIDI Setup), and a change read too early
	/// would be missed.
	public func hardwareFormatDiffers() -> Bool {
		guard let hardware = hardwareFormat() else { return true }
		let rate = nominalSampleRate > 0 ? nominalSampleRate : hardware.mSampleRate
		return abs(rate - format.sampleRate) >= 1 || min(Int(hardware.mChannelsPerFrame), 8) != format.channels
	}

	/// Asks again for the I/O buffer the engine wants. A rate change made
	/// elsewhere resets the device to its default, 512 frames, which at
	/// 384 kHz is too short a deadline and crackles.
	public func reassertBufferSize() {
		configureBufferSize(sampleRate: format.sampleRate)
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
		guard let renderer, !isRunning else { return }
		// The device's sample time starts over with the unit.
		cog_renderer_forget_device_time(renderer)
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
	static func setProperty<Value>(_ object: AudioObjectID, _ selector: AudioObjectPropertySelector, _ value: inout Value, scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal) -> Bool {
		var address = AudioObjectPropertyAddress(mSelector: selector, mScope: scope, mElement: kAudioObjectPropertyElementMain)
		return AudioObjectSetPropertyData(object, &address, 0, nil, UInt32(MemoryLayout<Value>.size), &value) == noErr
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
