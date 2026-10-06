//
//  DeviceOutput.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

#if os(macOS)

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
/// On a stereo device, surround can be spatialized (`refreshFormat(spatial:)`):
/// the engine renders a 7.1 bed into Apple's `AUSpatialMixer`, which feeds
/// the unit binaural stereo with the system's (personalized) HRTF and,
/// optionally, AirPods head tracking, or renders for speakers
/// (`spatialOutput(of:)`). macOS never spatializes an AUHAL
/// client's output itself, so this is how Cog reaches its renderer.
///
/// A device can also be held exclusively (`takeExclusive`): hog mode, its
/// stream set to the best format it offers at the rate asked for, integer
/// if it has one, and rendered into directly by an IOProc on the device
/// (`cog_renderer_device_io_proc`), since AUHAL will not drive a stream the
/// system cannot mix. The renderer then writes the device's own sample
/// words, and nothing after it converts them.
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

	/// The format the engine must render in: interleaved frames at the
	/// device's rate, at most eight channels.
	public private(set) var format = StreamFormat(sampleRate: 0, channels: 0)

	/// Whether this process holds the device exclusively (hog mode, its
	/// stream in a format of Cog's choosing), for DoP or for PCM.
	public private(set) var isExclusive = false

	/// Whether the render format is the spatial mixer's 7.1 bed rather than
	/// the device's own channels.
	public private(set) var isSpatial = false

	/// The sample words the renderer hands Core Audio, set by
	/// `refreshFormat`: float through the unit, 24-bit integer through it
	/// for a DoP carrier on a device not held, or while held, whatever the
	/// device's stream runs at.
	public private(set) var sampleFormat = CogSampleFormat.float32

	/// What Core Audio takes from the renderer, in full.
	public private(set) var renderFormat = AudioStreamBasicDescription()

	/// Whether the renderer hands over integers rather than float.
	public var integerRender: Bool { sampleFormat != .float32 }

	/// The device's name, as the device menu shows it.
	public var deviceName: String? {
		var name: Unmanaged<CFString>?
		guard Self.getProperty(deviceID, kAudioDevicePropertyDeviceNameCFString, &name) else { return nil }
		return name?.takeRetainedValue() as String?
	}

	/// The formats of the device's output streams as Core Audio reports them:
	/// the virtual format the system mixes in, or the physical format the
	/// hardware runs at.
	public func streamFormats(physical: Bool) -> [AudioStreamBasicDescription] {
		Self.outputStreams(of: deviceID).compactMap { Self.streamFormat($0, physical: physical) }
	}

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
	/// The `AUSpatialMixer` feeding the unit while spatial; kept once made.
	private var mixer: AudioComponentInstance?
	private var mixerInitialized = false
	/// The device's channels as the unit takes them (at most eight), which
	/// spatial rendering does not follow.
	private var deviceChannels = 0
	private var renderer: OpaquePointer?
	/// Drives the renderer while the device is held exclusively.
	private var ioProcID: AudioDeviceIOProcID?
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
		if let mixer {
			if mixerInitialized {
				AudioUnitUninitialize(mixer)
			}
			AudioComponentInstanceDispose(mixer)
		}
	}

	static func check(_ status: OSStatus) throws {
		if status != noErr {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(status))
		}
	}

	private func setProperty<Value>(_ property: AudioUnitPropertyID, scope: AudioUnitScope, _ value: inout Value) throws {
		try Self.check(withUnsafeMutableBytes(of: &value) { AudioUnitSetProperty(unit, property, scope, 0, $0.baseAddress, UInt32($0.count)) })
	}

	private func uninitialize() {
		if initialized {
			AudioUnitUninitialize(unit)
			initialized = false
		}
		if let mixer, mixerInitialized {
			AudioUnitUninitialize(mixer)
			mixerInitialized = false
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

	/// Re-reads the device format and sets the render format to match. Call
	/// after `onDeviceChange(.format)` before rebuilding the renderer.
	///
	/// Held exclusively, the render format is the device stream's own, and
	/// `integer` is moot. Otherwise the unit takes float or, with `integer`
	/// (a DoP carrier on a device not held, which only tests use), 24-bit
	/// integer high-aligned in 32 bits.
	///
	/// `sampleRate` overrides the rate read from the unit, which can lag a
	/// moment behind a nominal rate just set.
	///
	/// `spatial` (shared float output only) renders the 7.1 bed
	/// (`spatialFormat`) into the spatial mixer instead of the device's own
	/// channels.
	public func refreshFormat(integer: Bool = false, sampleRate: Double? = nil, spatial: Bool = false) throws {
		let wasRunning = isRunning
		if wasRunning { stop() }
		if isExclusive {
			if isSpatial {
				uninitialize()
				disconnectMixer()
			}
			try refreshExclusiveFormat(sampleRate: sampleRate)
			if wasRunning { try start() }
			return
		}
		uninitialize()
		// Bound to the device again: the unit leaves a device whose stream is
		// made non-mixable, as it was while held.
		var bound = AudioDeviceID(kAudioObjectUnknown)
		var boundSize = UInt32(MemoryLayout<AudioDeviceID>.size)
		if AudioUnitGetProperty(unit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &bound, &boundSize) != noErr || bound != deviceID {
			var device = deviceID
			try setProperty(kAudioOutputUnitProperty_CurrentDevice, scope: kAudioUnitScope_Global, &device)
		}

		guard let hardware = hardwareFormat(), hardware.mSampleRate > 0, hardware.mChannelsPerFrame > 0 else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
		}
		let channels = min(Int(hardware.mChannelsPerFrame), 8)
		// The device's nominal rate over the unit's, which can lag behind it.
		let nominal = nominalSampleRate
		let rate = sampleRate ?? (nominal > 0 ? nominal : hardware.mSampleRate)
		let spatial = spatial && !integer
		if isSpatial && !spatial {
			disconnectMixer()
		}
		let device = StreamFormat(sampleRate: rate, channels: channels, channelConfig: Self.channelConfig(channels: channels))
		if spatial {
			// The mixer gives the unit planar stereo, and takes the bed.
			var asbd = Self.planarFloatASBD(sampleRate: rate, channels: 2)
			try setProperty(kAudioUnitProperty_StreamFormat, scope: kAudioUnitScope_Input, &asbd)
			renderFormat = Pump.asbd(Self.spatialFormat(sampleRate: rate))
			var layout = AudioChannelLayout()
			layout.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo
			_ = AudioUnitSetProperty(unit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, 0, &layout, UInt32(MemoryLayout<AudioChannelLayout>.size))
		} else {
			var asbd = integer ? Self.integerASBD(device) : Pump.asbd(device)
			try setProperty(kAudioUnitProperty_StreamFormat, scope: kAudioUnitScope_Input, &asbd)
			renderFormat = asbd
			var layout = AudioChannelLayout()
			layout.mChannelLayoutTag = Self.layoutTag(channels: channels)
			// Not every device takes a layout; the stream format is what matters.
			_ = AudioUnitSetProperty(unit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, 0, &layout, UInt32(MemoryLayout<AudioChannelLayout>.size))
		}
		sampleFormat = integer ? .int24High : .float32
		configureBufferSize(sampleRate: rate)
		var mixerLatency = 0
		if spatial {
			mixerLatency = try connectMixer(sampleRate: rate)
		}
		try Self.check(AudioUnitInitialize(unit))
		initialized = true

		isSpatial = spatial
		deviceChannels = channels
		format = spatial ? Self.spatialFormat(sampleRate: rate) : device
		latencyFrames = Self.presentationLatency(of: deviceID) + mixerLatency
		if wasRunning { try start() }
	}

	// MARK: - Spatial audio

	/// The guess for a device with no choice of its own, from its transport,
	/// output channels and current output data source. Only stereo devices
	/// are spatialized: a device with more channels takes surround as it is,
	/// and a Bluetooth headset in its mono call mode a downmix. Bluetooth and
	/// USB devices are taken for headphones (Core Audio cannot tell what is
	/// plugged into a USB DAC), as is built-in output on its headphone jack
	/// or the separate headphone device of Apple silicon Macs; built-in
	/// speakers are speakers. Anything else (HDMI, DisplayPort, AirPlay,
	/// aggregate and virtual devices) is left alone unless chosen.
	static func automaticSpatialOutput(transport: UInt32, outputChannels: Int, dataSource: UInt32?) -> SpatialOutput {
		guard outputChannels == 2 else { return .off }
		switch transport {
		case kAudioDeviceTransportTypeBluetooth, kAudioDeviceTransportTypeBluetoothLE, kAudioDeviceTransportTypeUSB:
			return .headphones
		case kAudioDeviceTransportTypeBuiltIn:
			switch dataSource {
			case nil, Self.headphonesDataSource: return .headphones
			case Self.speakersDataSource: return .speakers
			default: return .off
			}
		default:
			return .off
		}
	}

	static let headphonesDataSource: UInt32 = 0x6864_706E // 'hdpn'
	static let speakersDataSource: UInt32 = 0x6973_706B // 'ispk'

	/// A device's choice (`spatialDevicesKey`), else the automatic guess.
	/// A device with other than two output channels is never spatialized.
	public static func spatialOutput(of id: AudioDeviceID) -> SpatialOutput {
		guard isStereo(id) else { return .off }
		return chosenSpatialOutput(of: id) ?? automaticSpatialOutput(of: id)
	}

	/// Whether a device has two output channels, which spatial audio needs.
	public static func isStereo(_ id: AudioDeviceID) -> Bool {
		outputChannels(of: id)?.reduce(0, +) == 2
	}

	/// The choice made for a device; nil for automatic.
	public static func chosenSpatialOutput(of id: AudioDeviceID) -> SpatialOutput? {
		guard let uid = uid(of: id) else { return nil }
		return (UserDefaults.standard.dictionary(forKey: spatialDevicesKey)?[uid] as? String).flatMap(SpatialOutput.init(rawValue:))
	}

	/// Makes a choice for a device, or (nil) leaves it automatic.
	public static func choose(_ output: SpatialOutput?, for id: AudioDeviceID) {
		guard let uid = uid(of: id) else { return }
		var choices = UserDefaults.standard.dictionary(forKey: spatialDevicesKey) ?? [:]
		choices[uid] = output?.rawValue
		UserDefaults.standard.set(choices, forKey: spatialDevicesKey)
	}

	/// The guess `automaticSpatialOutput` makes for a device.
	public static func automaticSpatialOutput(of id: AudioDeviceID) -> SpatialOutput {
		var transport: UInt32 = 0
		guard getProperty(id, kAudioDevicePropertyTransportType, &transport) else { return .off }
		var source: UInt32 = 0
		let hasSource = getProperty(id, kAudioDevicePropertyDataSource, &source, scope: kAudioDevicePropertyScopeOutput)
		let channels = outputChannels(of: id)?.reduce(0, +) ?? 0
		return automaticSpatialOutput(transport: transport, outputChannels: channels, dataSource: hasSource ? source : nil)
	}

	/// What the spatial mixer renders for on the selected device; nil when
	/// its surround is downmixed instead.
	public var spatialOutputType: AUSpatialMixerOutputType? {
		switch Self.spatialOutput(of: deviceID) {
		case .headphones:
			return .spatialMixerOutputType_Headphones
		case .speakers:
			var transport: UInt32 = 0
			let builtIn = Self.getProperty(deviceID, kAudioDevicePropertyTransportType, &transport) && transport == kAudioDeviceTransportTypeBuiltIn
			return builtIn ? .spatialMixerOutputType_BuiltInSpeakers : .spatialMixerOutputType_ExternalSpeakers
		case .off:
			return nil
		}
	}

	/// While spatial, renders for the device as it now is (headphones or
	/// speakers), live: a choice changed, or built-in output switched
	/// between its speakers and headphone jack.
	public func updateSpatialOutputType() {
		guard isSpatial, let mixer, let type = spatialOutputType else { return }
		var value = type.rawValue
		let status = AudioUnitSetProperty(mixer, kAudioUnitProperty_SpatialMixerOutputType, kAudioUnitScope_Global, 0, &value, UInt32(MemoryLayout<UInt32>.size))
		if status != noErr {
			EngineLog.logger.error("Could not change the spatial mixer's output type: \(status)")
		}
	}

	/// Turns the spatial mixer's head tracking on or off, live.
	public func setHeadTracking(_ enabled: Bool) {
		guard let mixer else { return }
		Self.setHeadTracking(enabled, on: mixer)
	}

	private static func setHeadTracking(_ enabled: Bool, on mixer: AudioComponentInstance) {
		guard #available(macOS 12.3, *) else { return }
		var value: UInt32 = enabled ? 1 : 0
		_ = AudioUnitSetProperty(mixer, kAudioUnitProperty_SpatialMixerEnableHeadTracking, kAudioUnitScope_Global, 0, &value, UInt32(MemoryLayout<UInt32>.size))
	}

	/// Sets the spatial mixer up for the 7.1 bed at `sampleRate` and connects
	/// it to the unit's input. Returns the mixer's latency in frames.
	private func connectMixer(sampleRate: Double) throws -> Int {
		let mixer = try self.mixer ?? Self.makeMixer()
		self.mixer = mixer
		func set<Value>(_ property: AudioUnitPropertyID, _ scope: AudioUnitScope, _ value: Value) throws {
			var value = value
			try Self.check(withUnsafeMutableBytes(of: &value) { AudioUnitSetProperty(mixer, property, scope, 0, $0.baseAddress, UInt32($0.count)) })
		}
		let bed = Self.spatialFormat(sampleRate: sampleRate)
		try set(kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, UInt32(1))
		// Interleaved in, as the renderer writes; the mixer only gives planar.
		try set(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, Pump.asbd(bed))
		try set(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, Self.planarFloatASBD(sampleRate: sampleRate, channels: 2))
		var layout = AudioChannelLayout()
		layout.mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelBitmap
		layout.mChannelBitmap = AudioChannelBitmap(rawValue: bed.channelConfig)
		try set(kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, layout)
		var stereo = AudioChannelLayout()
		stereo.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo
		_ = try? set(kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Output, stereo)
		try set(kAudioUnitProperty_SpatializationAlgorithm, kAudioUnitScope_Input, AUSpatializationAlgorithm.spatializationAlgorithm_UseOutputType.rawValue)
		try set(kAudioUnitProperty_SpatialMixerSourceMode, kAudioUnitScope_Input, AUSpatialMixerSourceMode.spatialMixerSourceMode_AmbienceBed.rawValue)
		try set(kAudioUnitProperty_SpatialMixerOutputType, kAudioUnitScope_Global, (spatialOutputType ?? .spatialMixerOutputType_Headphones).rawValue)
		if #available(macOS 13, *) {
			_ = try? set(kAudioUnitProperty_SpatialMixerPersonalizedHRTFMode, kAudioUnitScope_Global, AUSpatialMixerPersonalizedHRTFMode.auto.rawValue)
		}
		Self.setHeadTracking(UserDefaults.standard.bool(forKey: Self.headTrackingKey), on: mixer)
		var slice: UInt32 = 0
		var sliceSize = UInt32(MemoryLayout<UInt32>.size)
		if AudioUnitGetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &slice, &sliceSize) == noErr {
			try set(kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, slice)
		}
		try Self.check(AudioUnitInitialize(mixer))
		mixerInitialized = true

		var connection = AudioUnitConnection(sourceAudioUnit: mixer, sourceOutputNumber: 0, destInputNumber: 0)
		try setProperty(kAudioUnitProperty_MakeConnection, scope: kAudioUnitScope_Input, &connection)

		var latency: Float64 = 0
		var size = UInt32(MemoryLayout<Float64>.size)
		guard AudioUnitGetProperty(mixer, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0, &latency, &size) == noErr else { return 0 }
		return Int((latency * sampleRate).rounded())
	}

	/// Feeds the unit from the render callback again.
	private func disconnectMixer() {
		var connection = AudioUnitConnection(sourceAudioUnit: nil, sourceOutputNumber: 0, destInputNumber: 0)
		_ = try? setProperty(kAudioUnitProperty_MakeConnection, scope: kAudioUnitScope_Input, &connection)
		isSpatial = false
	}

	private static func makeMixer() throws -> AudioComponentInstance {
		var description = AudioComponentDescription(componentType: kAudioUnitType_Mixer,
		                                            componentSubType: kAudioUnitSubType_SpatialMixer,
		                                            componentManufacturer: kAudioUnitManufacturer_Apple,
		                                            componentFlags: 0,
		                                            componentFlagsMask: 0)
		guard let component = AudioComponentFindNext(nil, &description) else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_NoConnection))
		}
		var instance: AudioComponentInstance?
		try check(AudioComponentInstanceNew(component, &instance))
		return instance!
	}

	/// Held exclusively: the renderer writes the stream's own format, which
	/// `takeExclusive` chose, at the device's rate.
	private func refreshExclusiveFormat(sampleRate: Double?) throws {
		guard let stream = Self.outputStreams(of: deviceID).first, let virtual = Self.streamFormat(stream, physical: false),
		      let sample = Self.sampleFormat(of: virtual), (1...8).contains(Int(virtual.mChannelsPerFrame)) else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
		}
		let channels = Int(virtual.mChannelsPerFrame)
		let nominal = nominalSampleRate
		let rate = sampleRate ?? (nominal > 0 ? nominal : virtual.mSampleRate)
		var asbd = virtual
		asbd.mSampleRate = rate
		asbd.mFormatFlags &= ~kAudioFormatFlagIsNonMixable
		sampleFormat = sample
		renderFormat = asbd
		configureBufferSize(sampleRate: rate)
		format = StreamFormat(sampleRate: rate, channels: channels, channelConfig: Self.channelConfig(channels: channels))
		deviceChannels = channels
		latencyFrames = Self.presentationLatency(of: deviceID)
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

	// MARK: - Exclusive access

	/// The device's output stream, if the device can be held exclusively: a
	/// device chosen by name, never the followed system default (holding it
	/// would silence every other app), with one output stream of at most
	/// eight channels (the IOProc renders straight into it, with no unit to
	/// map channels), whose hog mode can be taken.
	public var exclusiveStream: AudioStreamID? {
		guard !followsSystemDefault else { return nil }
		return Self.exclusiveStream(of: deviceID)
	}

	static func exclusiveStream(of id: AudioDeviceID) -> AudioStreamID? {
		let streams = outputStreams(of: id)
		guard streams.count == 1, let channels = outputChannels(of: id), channels.count == 1, (1...8).contains(channels[0]) else {
			return nil
		}
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyHogMode, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		var settable: DarwinBoolean = false
		guard AudioObjectIsPropertySettable(id, &address, &settable) == noErr, settable.boolValue else { return nil }
		return streams[0]
	}

	public enum ExclusiveResult {
		case taken
		/// The device cannot be held: another process holds it, or it would
		/// not be handed over.
		case unavailable
		/// Held, but nothing it offers at the rate would do; now released.
		case unsupportedFormat
	}

	/// Takes the device for this process alone at `rate` (or, already held,
	/// moves it to `rate`): hog mode, mixing off, and its stream set to the
	/// best format it offers there (`exclusiveCandidates`), so no system
	/// mixer, volume or float stage sits between the renderer and the DAC.
	/// `integerBits` asks for an integer stream of at least that many bits
	/// (DoP needs 24). Call `refreshFormat` and `attach` afterwards.
	///
	/// macOS leaves a stream non-mixable when its holder dies, where no other
	/// app can play through it; so what is changed is recorded in the
	/// defaults while the device is held, and `recoverAbandonedSession`
	/// undoes it after a crash.
	public func takeExclusive(rate: Double, integerBits: Int = 0) -> ExclusiveResult {
		guard let stream = exclusiveStream else { return .unavailable }
		if !isExclusive {
			Self.recoverAbandonedSession()
			// The unit must let go of the device first; it would move to
			// another one once this stream turns non-mixable.
			stop()
			uninitialize()
			// macOS may not hand a device over while it is the system's
			// default output (or its sound-effects output), so move those
			// elsewhere for as long as it is held, as Pine Player does.
			guard moveSystemDefaultsAway() else { return .unavailable }
			guard hog() else {
				restoreSystemDefaults()
				return .unavailable
			}
			isExclusive = true
			heldDeviceUID = Self.uid(of: deviceID)
			savedPhysicalFormats = Self.outputStreams(of: deviceID).enumerated().compactMap { index, stream in
				Self.streamFormat(stream, physical: true).map { (index, stream, $0) }
			}
			var mixing: UInt32 = 1
			if Self.getProperty(deviceID, kAudioDevicePropertySupportsMixing, &mixing), mixing != 0 {
				var off: UInt32 = 0
				if Self.setProperty(deviceID, kAudioDevicePropertySupportsMixing, &off) {
					savedMixing = mixing
				}
			}
			applyDeviceVolumeSetting()
			saveSession()
		}
		// The rate goes with the stream's format. Setting the nominal rate
		// first as well, a second reconfiguration straight after the first,
		// leaves some USB DACs unable to start I/O for seconds (an SMSL failed
		// three starts in four that way, "IO is still disabled"); so that is
		// only a fallback, for a device that will not take the rate as part of
		// a format.
		guard setExclusiveFormat(stream, rate: rate, integerBits: integerBits) ||
			(setNominalSampleRate(rate) && setExclusiveFormat(stream, rate: rate, integerBits: integerBits)) else {
			EngineLog.logger.notice("The device offers no usable\(integerBits > 0 ? " integer" : "", privacy: .public) format at \(rate, format: .fixed(precision: 0)) Hz; releasing it")
			releaseExclusive()
			return .unsupportedFormat
		}
		return .taken
	}

	/// The process holding the device, or -1.
	private var hogOwner: pid_t {
		var owner: pid_t = -1
		_ = Self.getProperty(deviceID, kAudioDevicePropertyHogMode, &owner)
		return owner
	}

	/// Takes hog mode, allowing the system a moment to move other clients
	/// off the device after its default role was taken away. Setting hog
	/// mode toggles it, whatever value is written, so it is set only while
	/// nobody holds the device.
	private func hog() -> Bool {
		var owner = hogOwner
		for _ in 0..<25 {
			if owner == getpid() { return true }
			if owner == -1 {
				var pid = getpid()
				Self.setProperty(deviceID, kAudioDevicePropertyHogMode, &pid)
			}
			usleep(20000)
			owner = hogOwner
		}
		if owner == getpid() { return true }
		EngineLog.logger.notice("Could not take the device exclusively (owner \(owner))")
		return false
	}

	/// Puts back everything taking the device changed, and gives it back to
	/// the system mixer and the system its defaults.
	public func releaseExclusive() {
		guard isExclusive else { return }
		stop()
		destroyIOProc()
		restoreDeviceVolume()
		// What will not go back (a device unplugged while held, say) stays on
		// record, to be put back when next seen free: a replugged device can
		// come back in the format it was left in.
		savedPhysicalFormats = savedPhysicalFormats.filter { saved in
			var format = saved.format
			if Self.setProperty(saved.stream, kAudioStreamPropertyPhysicalFormat, &format) && Self.waitForPhysicalFormat(saved.stream, matching: saved.format) {
				return false
			}
			EngineLog.logger.error("Could not put stream \(saved.stream) back to its format; keeping it on record")
			return true
		}
		if var mixing = savedMixing {
			_ = Self.setProperty(deviceID, kAudioDevicePropertySupportsMixing, &mixing)
			savedMixing = nil
		}
		// Toggled off only if still held: set while free, it would be taken.
		if hogOwner == getpid() {
			var none: pid_t = -1
			_ = Self.setProperty(deviceID, kAudioDevicePropertyHogMode, &none)
		}
		restoreSystemDefaults()
		if savedPhysicalFormats.isEmpty {
			UserDefaults.standard.removeObject(forKey: Self.sessionKey)
		} else {
			saveSession()
		}
		savedPhysicalFormats = []
		heldPhysicalFormats = [:]
		heldDeviceUID = nil
		isExclusive = false
		sampleFormat = .float32
		EngineLog.logger.notice("Released the device")
	}

	/// What taking the device changed, to put back on release: each output
	/// stream's physical format before, by index, and mixing.
	private var savedPhysicalFormats: [(index: Int, stream: AudioStreamID, format: AudioStreamBasicDescription)] = []
	private var savedMixing: UInt32?
	/// The streams' formats as the hold set them, by index, for telling
	/// after a crash whether they are still what it left.
	private var heldPhysicalFormats: [Int: AudioStreamBasicDescription] = [:]
	/// The held device's UID, which outlives it being unplugged.
	private var heldDeviceUID: String?

	/// Sets `stream` to the first of `exclusiveCandidates` it takes whose
	/// resulting virtual format the renderer can write.
	private func setExclusiveFormat(_ stream: AudioStreamID, rate: Double, integerBits: Int) -> Bool {
		guard let current = Self.streamFormat(stream, physical: true) else { return false }
		let candidates = Self.exclusiveCandidates(Self.availablePhysicalFormats(of: stream), rate: rate,
		                                          channels: Int(current.mChannelsPerFrame), integerBits: integerBits)
		for candidate in candidates {
			var requested = candidate
			guard Self.setProperty(stream, kAudioStreamPropertyPhysicalFormat, &requested) else {
				EngineLog.logger.notice("Stream \(stream) would not take \(Self.describe(candidate), privacy: .public)")
				continue
			}
			// A mixable stream's virtual format stays float, which the system
			// converts to the integer stream: exactly, for 24 bits (and DoP).
			guard Self.waitForPhysicalFormat(stream, matching: candidate),
			      let virtual = Self.waitForVirtualFormat(stream, rate: rate, nonMixable: candidate.mFormatFlags & kAudioFormatFlagIsNonMixable != 0),
			      Self.sampleFormat(of: virtual) != nil else {
				EngineLog.logger.notice("Stream \(stream) did not settle on \(Self.describe(candidate), privacy: .public)")
				continue
			}
			EngineLog.logger.notice("Holding the device exclusively at \(rate, format: .fixed(precision: 0)) Hz: stream \(Self.describe(candidate), privacy: .public), rendering \(Self.describe(virtual), privacy: .public)")
			for (index, stream) in Self.outputStreams(of: deviceID).enumerated() {
				heldPhysicalFormats[index] = Self.streamFormat(stream, physical: true)
			}
			saveSession()
			return true
		}
		return false
	}

	/// The formats worth holding a stream at for `rate`, best first:
	/// integer the system cannot mix (the stream's virtual format is then its
	/// physical one, so the renderer writes the DAC's own words), then
	/// integer it can (the system converts float to it, exactly for 24 bits
	/// and fewer), then float. Widest integer first, whatever the source:
	/// every source up to 24 bits passes exactly through any of 24 bits or
	/// more, processed audio loses the least, and tracks of other bit depths
	/// at the same rate then need no change of format, so stay gapless.
	/// `integerBits` leaves out float and narrower integers.
	static func exclusiveCandidates(_ available: [AudioStreamRangedDescription], rate: Double, channels: Int, integerBits: Int) -> [AudioStreamBasicDescription] {
		let widths: [CogSampleFormat] = [.int32, .int24High, .int24Low, .int24Packed, .int16]
		var ranked: [(format: AudioStreamBasicDescription, rank: Int)] = []
		for ranged in available {
			var format = ranged.mFormat
			let range = ranged.mSampleRateRange
			let atRate = abs(format.mSampleRate - rate) < 1 ||
				(range.mMaximum > 0 && rate >= range.mMinimum - 1 && rate <= range.mMaximum + 1)
			guard atRate, Int(format.mChannelsPerFrame) == channels, let sample = sampleFormat(of: format) else { continue }
			let nonMixable = format.mFormatFlags & kAudioFormatFlagIsNonMixable != 0
			let rank: Int
			if sample == .float32 {
				guard integerBits == 0 else { continue }
				rank = nonMixable ? 20 : 21
			} else {
				guard Int(format.mBitsPerChannel) >= integerBits else { continue }
				rank = (nonMixable ? 0 : 10) + (widths.firstIndex(of: sample) ?? 9)
			}
			format.mSampleRate = rate
			ranked.append((format, rank))
		}
		// Stable: among equals, the device's own order.
		return ranked.enumerated().sorted { ($0.element.rank, $0.offset) < ($1.element.rank, $1.offset) }.map(\.element.format)
	}

	/// Waits up to half a second for `stream`'s physical format to read back
	/// as `format`.
	@discardableResult
	private static func waitForPhysicalFormat(_ stream: AudioStreamID, matching format: AudioStreamBasicDescription) -> Bool {
		for _ in 0..<50 {
			if let current = streamFormat(stream, physical: true), sameRepresentation(current, format), abs(current.mSampleRate - format.mSampleRate) < 1 {
				return true
			}
			usleep(10000)
		}
		return false
	}

	/// Waits up to half a second for `stream`'s virtual format to reach
	/// `rate` (and, for a non-mixable physical format, to become the same),
	/// and returns it.
	private static func waitForVirtualFormat(_ stream: AudioStreamID, rate: Double, nonMixable: Bool) -> AudioStreamBasicDescription? {
		for _ in 0..<50 {
			if let virtual = streamFormat(stream, physical: false), abs(virtual.mSampleRate - rate) < 1,
			   !nonMixable || virtual.mFormatFlags & kAudioFormatFlagIsNonMixable != 0 {
				return virtual
			}
			usleep(10000)
		}
		return nil
	}

	// MARK: - Device volume while held

	/// Volume scalars replaced by full volume, by element, to put back.
	private var replacedVolumes: [UInt32: Float32] = [:]

	/// While held: sets the device's own volume to full when
	/// `setDeviceVolumeTo100ForExclusiveOutput` asks for it, or puts back
	/// what it replaced when that is turned off. A DAC's volume control would
	/// otherwise still change the samples after Cog; devices without one are
	/// left alone.
	public func applyDeviceVolumeSetting() {
		guard isExclusive else { return }
		if UserDefaults.standard.bool(forKey: Self.fullVolumeKey) {
			guard replacedVolumes.isEmpty else { return }
			for element in Self.volumeElements(of: deviceID) {
				var volume: Float32 = 0
				var full: Float32 = 1
				if Self.getProperty(deviceID, kAudioDevicePropertyVolumeScalar, &volume, scope: kAudioDevicePropertyScopeOutput, element: element),
				   volume != 1,
				   Self.setProperty(deviceID, kAudioDevicePropertyVolumeScalar, &full, scope: kAudioDevicePropertyScopeOutput, element: element) {
					replacedVolumes[element] = volume
				}
			}
			if !replacedVolumes.isEmpty {
				EngineLog.logger.notice("Set the device's volume to full while holding it")
			}
		} else {
			restoreDeviceVolume()
		}
		saveSession()
	}

	/// Puts back volumes replaced by full volume, unless changed since.
	private func restoreDeviceVolume() {
		Self.restoreVolumes(replacedVolumes, on: deviceID)
		replacedVolumes = [:]
	}

	private static func restoreVolumes(_ volumes: [UInt32: Float32], on id: AudioDeviceID) {
		for (element, saved) in volumes {
			var current: Float32 = 0
			var volume = saved
			guard getProperty(id, kAudioDevicePropertyVolumeScalar, &current, scope: kAudioDevicePropertyScopeOutput, element: element),
			      abs(current - 1) < 0.001 else { continue }
			setProperty(id, kAudioDevicePropertyVolumeScalar, &volume, scope: kAudioDevicePropertyScopeOutput, element: element)
		}
	}

	/// The output volume controls to set: the main one if the device has a
	/// settable one, else each channel's.
	static func volumeElements(of id: AudioDeviceID) -> [UInt32] {
		func settable(_ element: UInt32) -> Bool {
			var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyVolumeScalar, mScope: kAudioDevicePropertyScopeOutput, mElement: element)
			var settable: DarwinBoolean = false
			return AudioObjectHasProperty(id, &address) && AudioObjectIsPropertySettable(id, &address, &settable) == noErr && settable.boolValue
		}
		if settable(kAudioObjectPropertyElementMain) {
			return [kAudioObjectPropertyElementMain]
		}
		let channels = outputChannels(of: id)?.reduce(0, +) ?? 0
		return channels > 0 ? (1...UInt32(channels)).filter(settable) : []
	}

	// MARK: - System defaults while held

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
		Self.restoreDefaults(movedDefaults)
		movedDefaults.removeAll()
	}

	private static func restoreDefaults(_ moved: [(selector: AudioObjectPropertySelector, original: AudioDeviceID, replacement: AudioDeviceID)]) {
		let system = AudioObjectID(kAudioObjectSystemObject)
		for moved in moved.reversed() {
			var current = AudioDeviceID(kAudioObjectUnknown)
			guard getProperty(system, moved.selector, &current), current == moved.replacement, isAliveOutput(moved.original) else { continue }
			var original = moved.original
			setProperty(system, moved.selector, &original)
		}
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

	// MARK: - Recovering from a crash while held

	/// What holding a device changed, in the defaults for as long as it is
	/// held. Devices by UID, which unlike IDs outlive a restart.
	private struct Session: Codable {
		struct Stream: Codable {
			/// Among the device's output streams.
			var index: Int
			/// The physical format before, and as the hold set it, as raw
			/// bytes.
			var before: Data
			var held: Data?
		}

		struct MovedDefault: Codable {
			var selector: UInt32
			var original: String
			var replacement: String
		}

		var device: String
		var streams: [Stream]
		var mixing: UInt32?
		var volumes: [UInt32: Float32]
		var defaults: [MovedDefault]
	}

	static let sessionKey = "exclusiveOutputSession"

	private func saveSession() {
		guard let uid = heldDeviceUID else { return }
		let session = Session(device: uid,
		                      streams: savedPhysicalFormats.map { saved in
		                      	Session.Stream(index: saved.index, before: Self.data(saved.format), held: heldPhysicalFormats[saved.index].map(Self.data))
		                      },
		                      mixing: savedMixing, volumes: replacedVolumes,
		                      defaults: movedDefaults.compactMap { moved in
		                      	guard let original = Self.uid(of: moved.original), let replacement = Self.uid(of: moved.replacement) else { return nil }
		                      	return Session.MovedDefault(selector: moved.selector, original: original, replacement: replacement)
		                      })
		if let data = try? PropertyListEncoder().encode(session) {
			UserDefaults.standard.set(data, forKey: Self.sessionKey)
		}
	}

	/// Undoes what holding a device left behind when it was not released: a
	/// crash frees hog mode, but macOS leaves the stream in the format it was
	/// given, non-mixable, where no other app can play, and the system
	/// defaults moved; a device unplugged while held can come back like that.
	/// Done once the device is here and nobody holds it (this process
	/// included), and only for what is still as the hold left it: a format
	/// or default changed since is someone's choice.
	public static func recoverAbandonedSession() {
		guard let data = UserDefaults.standard.data(forKey: sessionKey) else { return }
		guard let session = try? PropertyListDecoder().decode(Session.self, from: data) else {
			UserDefaults.standard.removeObject(forKey: sessionKey)
			return
		}
		guard let id = device(withUID: session.device) else { return }
		var owner: pid_t = -1
		_ = getProperty(id, kAudioDevicePropertyHogMode, &owner)
		// Still held by this process: nothing was abandoned.
		if owner == getpid() { return }
		defer { UserDefaults.standard.removeObject(forKey: sessionKey) }
		if owner != -1 {
			EngineLog.logger.notice("A device Cog held when it last quit is now held by process \(owner); leaving it alone")
			return
		}
		let streams = outputStreams(of: id)
		for record in session.streams where streams.indices.contains(record.index) {
			let stream = streams[record.index]
			guard var before = format(from: record.before), let current = streamFormat(stream, physical: true) else { continue }
			func same(_ a: AudioStreamBasicDescription, _ b: AudioStreamBasicDescription) -> Bool {
				sameRepresentation(a, b) && abs(a.mSampleRate - b.mSampleRate) < 1
			}
			// Held but not yet set, only a non-mixable format can be the hold's.
			let leftAsHeld = record.held.flatMap(format(from:)).map { same(current, $0) } ?? (current.mFormatFlags & kAudioFormatFlagIsNonMixable != 0)
			guard leftAsHeld, !same(current, before) else { continue }
			setProperty(stream, kAudioStreamPropertyPhysicalFormat, &before)
			waitForPhysicalFormat(stream, matching: before)
		}
		if var mixing = session.mixing {
			setProperty(id, kAudioDevicePropertySupportsMixing, &mixing)
		}
		restoreVolumes(session.volumes, on: id)
		restoreDefaults(session.defaults.compactMap { moved in
			guard let original = device(withUID: moved.original), let replacement = device(withUID: moved.replacement) else { return nil }
			return (moved.selector, original, replacement)
		})
		EngineLog.logger.notice("Put back the output device Cog held when it last quit")
	}

	private static func data(_ format: AudioStreamBasicDescription) -> Data {
		withUnsafeBytes(of: format) { Data($0) }
	}

	private static func format(from data: Data) -> AudioStreamBasicDescription? {
		guard data.count == MemoryLayout<AudioStreamBasicDescription>.size else { return nil }
		var format = AudioStreamBasicDescription()
		_ = withUnsafeMutableBytes(of: &format) { data.copyBytes(to: $0) }
		return format
	}

	static func uid(of id: AudioDeviceID) -> String? {
		var uid: Unmanaged<CFString>?
		guard getProperty(id, kAudioDevicePropertyDeviceUID, &uid) else { return nil }
		return uid?.takeRetainedValue() as String?
	}

	static func device(withUID uid: String) -> AudioDeviceID? {
		outputDevices().first { Self.uid(of: $0.id) == uid }?.id
	}

	// MARK: - Device rate

	/// Whether the device offers `rate`. A device that does not list its
	/// rates is given the benefit of the doubt, as OutputCoreAudio did.
	public func supportsSampleRate(_ rate: Double) -> Bool {
		Self.supports(rate, among: availableSampleRates)
	}

	/// The nominal rates the device lists; empty if it does not say.
	public var availableSampleRates: [AudioValueRange] {
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyAvailableNominalSampleRates,
		                                         mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(deviceID, &address, 0, nil, &size) == noErr, size > 0 else { return [] }
		var ranges = [AudioValueRange](repeating: AudioValueRange(), count: Int(size) / MemoryLayout<AudioValueRange>.size)
		guard AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &ranges) == noErr else { return [] }
		return ranges
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
	/// Held exclusively, the stream's own format must also still be the one
	/// rendered (the unit's view of a held device means nothing).
	public func hardwareFormatDiffers() -> Bool {
		if isExclusive {
			guard let stream = Self.outputStreams(of: deviceID).first, let virtual = Self.streamFormat(stream, physical: false) else { return true }
			let rate = nominalSampleRate > 0 ? nominalSampleRate : virtual.mSampleRate
			return abs(rate - format.sampleRate) >= 1 || Int(virtual.mChannelsPerFrame) != format.channels || Self.sampleFormat(of: virtual) != sampleFormat
		}
		guard let hardware = hardwareFormat() else { return true }
		let rate = nominalSampleRate > 0 ? nominalSampleRate : hardware.mSampleRate
		return abs(rate - format.sampleRate) >= 1 || min(Int(hardware.mChannelsPerFrame), 8) != deviceChannels
	}

	/// Asks again for the I/O buffer the engine wants. A rate change made
	/// elsewhere resets the device to its default, 512 frames, which at
	/// 384 kHz is too short a deadline and crackles.
	public func reassertBufferSize() {
		configureBufferSize(sampleRate: format.sampleRate)
	}

	// MARK: - Rendering

	/// Renders from `renderer`, whose ring must have `format.channels`
	/// channels and whose output format must be `sampleFormat`. The renderer
	/// must outlive this output or be replaced first.
	public func attach(_ renderer: OpaquePointer) {
		precondition(Int(cog_ring_channels(cog_renderer_ring(renderer))) == format.channels)
		let wasRunning = isRunning
		if wasRunning { stop() }
		self.renderer = renderer
		var callback = AURenderCallbackStruct(inputProc: cog_renderer_audio_unit_render,
		                                      inputProcRefCon: UnsafeMutableRawPointer(renderer))
		if isSpatial, let mixer {
			// Plain C on the I/O thread still: the mixer pulls the renderer.
			_ = AudioUnitSetProperty(mixer, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, UInt32(MemoryLayout<AURenderCallbackStruct>.size))
		} else {
			try? setProperty(kAudioUnitProperty_SetRenderCallback, scope: kAudioUnitScope_Input, &callback)
		}
		if isExclusive {
			destroyIOProc()
			var id: AudioDeviceIOProcID?
			let status = AudioDeviceCreateIOProcID(deviceID, cog_renderer_device_io_proc, UnsafeMutableRawPointer(renderer), &id)
			if status == noErr {
				ioProcID = id
			} else {
				EngineLog.logger.error("Could not create an IOProc on the device held exclusively: \(status)")
			}
		}
		if wasRunning { try? start() }
	}

	public func start() throws {
		guard let renderer, !isRunning else { return }
		// The device's sample time starts over with the unit.
		cog_renderer_forget_device_time(renderer)
		if isExclusive {
			guard let ioProcID else {
				throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioHardwareBadDeviceError))
			}
			try Self.check(AudioDeviceStart(deviceID, ioProcID))
			isRunning = true
			return
		}
		if isSpatial, let mixer, !mixerInitialized {
			try Self.check(AudioUnitInitialize(mixer))
			mixerInitialized = true
		}
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
		if let ioProcID {
			AudioDeviceStop(deviceID, ioProcID)
		} else {
			AudioOutputUnitStop(unit)
		}
		isRunning = false
	}

	private func destroyIOProc() {
		guard let id = ioProcID else { return }
		if isRunning {
			AudioDeviceStop(deviceID, id)
			isRunning = false
		}
		AudioDeviceDestroyIOProcID(deviceID, id)
		ioProcID = nil
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
		// Headphones plugged into or pulled from the jack: spatial audio
		// comes or goes with them.
		addListener(deviceID, kAudioDevicePropertyDataSource, scope: kAudioDevicePropertyScopeOutput) { [weak self] in
			self?.onDeviceChange?(.format)
		}
	}

	private func addListener(_ object: AudioObjectID, _ selector: AudioObjectPropertySelector, scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal, _ action: @escaping () -> Void) {
		var address = AudioObjectPropertyAddress(mSelector: selector, mScope: scope, mElement: kAudioObjectPropertyElementMain)
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

	/// A device's output streams.
	static func outputStreams(of id: AudioDeviceID) -> [AudioStreamID] {
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyStreams, mScope: kAudioDevicePropertyScopeOutput, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(id, &address, 0, nil, &size) == noErr, size >= MemoryLayout<AudioStreamID>.size else { return [] }
		var streams = [AudioStreamID](repeating: 0, count: Int(size) / MemoryLayout<AudioStreamID>.size)
		guard AudioObjectGetPropertyData(id, &address, 0, nil, &size, &streams) == noErr else { return [] }
		return streams
	}

	/// The channels of each of a device's output streams.
	static func outputChannels(of id: AudioDeviceID) -> [Int]? {
		var address = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyStreamConfiguration, mScope: kAudioDevicePropertyScopeOutput, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(id, &address, 0, nil, &size) == noErr, size >= MemoryLayout<UInt32>.size else { return nil }
		let list = UnsafeMutableRawPointer.allocate(byteCount: Int(size), alignment: MemoryLayout<AudioBufferList>.alignment)
		defer { list.deallocate() }
		guard AudioObjectGetPropertyData(id, &address, 0, nil, &size, list) == noErr else { return nil }
		return UnsafeMutableAudioBufferListPointer(list.assumingMemoryBound(to: AudioBufferList.self)).map { Int($0.mNumberChannels) }
	}

	/// A stream's virtual format (what its clients render, and the system
	/// mixes in) or physical format (what the hardware runs at).
	static func streamFormat(_ stream: AudioStreamID, physical: Bool) -> AudioStreamBasicDescription? {
		var asbd = AudioStreamBasicDescription()
		guard getProperty(stream, physical ? kAudioStreamPropertyPhysicalFormat : kAudioStreamPropertyVirtualFormat, &asbd), asbd.mFormatID != 0 else {
			return nil
		}
		return asbd
	}

	/// The physical formats a stream offers.
	static func availablePhysicalFormats(of stream: AudioStreamID) -> [AudioStreamRangedDescription] {
		var address = AudioObjectPropertyAddress(mSelector: kAudioStreamPropertyAvailablePhysicalFormats, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
		var size: UInt32 = 0
		guard AudioObjectGetPropertyDataSize(stream, &address, 0, nil, &size) == noErr, size > 0 else { return [] }
		var formats = [AudioStreamRangedDescription](repeating: AudioStreamRangedDescription(), count: Int(size) / MemoryLayout<AudioStreamRangedDescription>.size)
		guard AudioObjectGetPropertyData(stream, &address, 0, nil, &size, &formats) == noErr else { return [] }
		return formats
	}

	@discardableResult
	static func setProperty<Value>(_ object: AudioObjectID, _ selector: AudioObjectPropertySelector, _ value: inout Value, scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal, element: AudioObjectPropertyElement = kAudioObjectPropertyElementMain) -> Bool {
		var address = AudioObjectPropertyAddress(mSelector: selector, mScope: scope, mElement: element)
		return withUnsafeMutableBytes(of: &value) { AudioObjectSetPropertyData(object, &address, 0, nil, UInt32($0.count), $0.baseAddress!) } == noErr
	}

	@discardableResult
	static func getProperty<Value>(_ object: AudioObjectID, _ selector: AudioObjectPropertySelector, _ value: inout Value, scope: AudioObjectPropertyScope = kAudioObjectPropertyScopeGlobal, element: AudioObjectPropertyElement = kAudioObjectPropertyElementMain) -> Bool {
		var address = AudioObjectPropertyAddress(mSelector: selector, mScope: scope, mElement: element)
		return withUnsafeMutableBytes(of: &value) {
			var size = UInt32($0.count)
			return AudioObjectGetPropertyData(object, &address, 0, nil, &size, $0.baseAddress!)
		} == noErr
	}
}

#endif
