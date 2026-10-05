//
//  DeviceOutput+iOS.swift
//  CogAudio
//

#if os(iOS)

import AudioToolbox
import AVFoundation
import Foundation

/// Core Audio's object and device IDs are macOS-only API; the engine keeps
/// them in its bookkeeping, where on iOS they are always `kAudioObjectUnknown`.
public typealias AudioObjectID = UInt32
public typealias AudioDeviceID = AudioObjectID
let kAudioObjectUnknown: AudioObjectID = 0

/// The device side of the engine on iOS: a RemoteIO unit whose render
/// callback pulls from a `CogRenderer`, through the same C callback as AUHAL
/// on macOS (`cog_renderer_audio_unit_render`), so the I/O thread runs no
/// Objective-C or Swift.
///
/// The API is macOS's (`DeviceOutput.swift`), so the engine drives both
/// alike. What iOS has no use for answers as a Mac device that cannot do it
/// would: there is one output, the current route, which is always "the
/// system default"; it cannot be held exclusively (`exclusiveStream` is
/// nil), so no plan asks for a device rate or DoP, and everything is
/// resampled to the session's rate.
///
/// The audio session is the app's to configure and activate
/// (`activateSession()`, before playing), as `AVAudioSession` calls block
/// for a while and must stay off the main thread. If it was not activated,
/// `start()` activates it anyway, on the calling thread.
///
/// Route changes (headphones in or out, AirPlay, a new Bluetooth device)
/// report `.format`, and the engine rebuilds if the rate, channels or plan
/// changed. Interruptions (a call, Siri) stop the unit; it starts again when
/// the interruption ends if it was running and the system says to resume.
/// After media services reset, the unit is made anew and `.format` reported,
/// which rebuilds.
///
/// On a stereo route, surround can be spatialized through `AUSpatialMixer`
/// as on macOS: binaural for headphones (with head tracking where offered),
/// or for the built-in speakers.
///
/// Control methods are for one thread at a time (the engine's main thread).
public final class DeviceOutput {
	public enum Change {
		/// The route's rate, channels or kind changed.
		case format
		/// Not reported on iOS: there is no device to choose.
		case device
	}

	/// Called on a private queue when the route changes under us.
	public var onDeviceChange: ((Change) -> Void)?

	/// iOS has no device IDs; the route is always `kAudioObjectUnknown`.
	public let deviceID = AudioDeviceID(kAudioObjectUnknown)
	/// The route is the system's choice, always.
	public let followsSystemDefault = true

	/// The format the engine must render in: interleaved frames at the
	/// session's rate, at most eight channels.
	public private(set) var format = StreamFormat(sampleRate: 0, channels: 0)

	/// Never: iOS output is always shared.
	public let isExclusive = false

	/// Whether the render format is the spatial mixer's 7.1 bed rather than
	/// the route's own channels.
	public private(set) var isSpatial = false

	/// Always float: integer output exists only for DoP, which iOS cannot do.
	public private(set) var sampleFormat = CogSampleFormat.float32

	/// What the unit takes from the renderer, in full.
	public private(set) var renderFormat = AudioStreamBasicDescription()

	public var integerRender: Bool { sampleFormat != .float32 }

	/// The current route's output, as Control Center names it.
	public var deviceName: String? {
		AVAudioSession.sharedInstance().currentRoute.outputs.first?.portName
	}

	/// The route has no Core Audio streams to describe.
	public func streamFormats(physical: Bool) -> [AudioStreamBasicDescription] { [] }

	/// The most frames the unit asks the renderer for at once.
	public var maximumFramesPerSlice: Int {
		var frames: UInt32 = 0
		var size = UInt32(MemoryLayout<UInt32>.size)
		guard AudioUnitGetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &frames, &size) == noErr else {
			return Self.maximumSlice
		}
		return Int(frames)
	}

	/// Frames between the renderer handing audio over and it being heard.
	public private(set) var latencyFrames = 0

	public private(set) var isRunning = false

	private var unit: AudioComponentInstance
	private var initialized = false
	/// The `AUSpatialMixer` feeding the unit while spatial; kept once made.
	private var mixer: AudioComponentInstance?
	private var mixerInitialized = false
	/// The route's channels as the unit takes them (at most eight), which
	/// spatial rendering does not follow.
	private var deviceChannels = 0
	private var renderer: OpaquePointer?
	/// Media services were reset: the unit is new and must be set up again.
	private var mediaServicesReset = false
	/// Whether the unit was running when an interruption stopped it.
	private var interruptedWhileRunning = false
	/// Counts `stop()` calls, so a resume that waited for the session can
	/// tell the engine stopped (and maybe destroyed the renderer) meanwhile.
	private var stops = 0
	private let notificationQueue = OperationQueue()
	private var observers: [NSObjectProtocol] = []

	/// RemoteIO asks for 4096 frames at a time while the screen is locked;
	/// a smaller slice limit fails those renders with
	/// `kAudioUnitErr_TooManyFramesToProcess` and plays silence.
	static let maximumSlice = 4096

	public init() throws {
		unit = try Self.makeRemoteIO()
		notificationQueue.name = "Cog DeviceOutput notifications"
		notificationQueue.maxConcurrentOperationCount = 1
		installObservers()
	}

	deinit {
		removeObservers()
		stop()
		uninitialize()
		AudioComponentInstanceDispose(unit)
		if let mixer {
			AudioComponentInstanceDispose(mixer)
		}
	}

	static func check(_ status: OSStatus) throws {
		if status != noErr {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(status))
		}
	}

	private static func makeRemoteIO() throws -> AudioComponentInstance {
		var description = AudioComponentDescription(componentType: kAudioUnitType_Output,
		                                            componentSubType: kAudioUnitSubType_RemoteIO,
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

	// MARK: - Audio session

	/// Whether `activateSession()` has run since the last deactivation.
	private static let sessionLock = UnfairLock()
	nonisolated(unsafe) private static var sessionActive = false

	/// Sets the shared audio session up for music playback (`.playback`,
	/// long-form audio, so the system offers AirPlay routing as it does for
	/// Music; the I/O buffer `bufferMilliseconds` asks for) and activates it,
	/// interrupting other apps' audio. Off the main thread, as these calls
	/// block; for the app to await before playing.
	public static func activateSession() async throws {
		try await Task.detached(priority: .userInitiated) {
			try configureAndActivateSession()
		}.value
	}

	/// Deactivates the session, letting interrupted apps resume. For the
	/// app once playback has stopped, not merely paused.
	public static func deactivateSession() async {
		await Task.detached(priority: .utility) {
			do {
				try AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
				sessionLock.withLock { sessionActive = false }
			} catch {
				EngineLog.logger.error("Could not deactivate the audio session: \(error.localizedDescription, privacy: .public)")
			}
		}.value
	}

	private static func configureAndActivateSession() throws {
		let session = AVAudioSession.sharedInstance()
		try session.setCategory(.playback, mode: .default, policy: .longFormAudio)
		// Surround reaches routes that take it (AirPlay to a home theater,
		// say) as surround.
		try? session.setSupportsMultichannelContent(true)
		try? session.setPreferredIOBufferDuration(bufferMilliseconds / 1000)
		try session.setActive(true)
		sessionLock.withLock { sessionActive = true }
		EngineLog.logger.notice("Audio session active: \(session.sampleRate, format: .fixed(precision: 0)) Hz, \(session.outputNumberOfChannels) channels, I/O buffer \(session.ioBufferDuration * 1000, format: .fixed(precision: 1)) ms, route \(session.currentRoute.outputs.map(\.portType.rawValue).joined(separator: ", "), privacy: .public)")
	}

	/// Activates the session if the app has not, as `start()` must have it.
	private static func ensureSessionActive() throws {
		guard !sessionLock.withLock({ sessionActive }) else { return }
		EngineLog.logger.notice("Activating the audio session on demand; the app should call activateSession() first")
		try configureAndActivateSession()
	}

	// MARK: - Device selection

	/// There is no device to choose on iOS: the setting is ignored, and
	/// always "found" so the engine leaves it alone.
	@discardableResult
	public func selectDevice(_ description: [String: Any]?) throws -> Bool {
		if format.channels == 0 {
			try refreshFormat()
		}
		return true
	}

	/// Never: the route is not the app's to change.
	public func wouldChange(for description: [String: Any]?) -> Bool { false }

	/// The route always exists, if only as the built-in speaker.
	public static func isAliveOutput(_ id: AudioDeviceID) -> Bool { true }

	/// Re-reads the route's format and sets the render format to match. Call
	/// after `onDeviceChange(.format)` before rebuilding the renderer.
	///
	/// `integer` and `sampleRate` are for DoP and exclusive output on macOS,
	/// which iOS plans never ask for; a rate other than the session's is
	/// converted by RemoteIO.
	///
	/// `spatial` renders the 7.1 bed (`spatialFormat`) into the spatial mixer
	/// instead of the route's own channels.
	public func refreshFormat(integer: Bool = false, sampleRate: Double? = nil, spatial: Bool = false) throws {
		let wasRunning = isRunning
		if wasRunning { stop() }
		uninitialize()
		if mediaServicesReset {
			try rebuildUnits()
		}

		let session = AVAudioSession.sharedInstance()
		let channels = min(max(session.outputNumberOfChannels, 1), 8)
		let rate = sampleRate ?? session.sampleRate
		guard rate > 0 else {
			throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
		}
		if isSpatial && !spatial {
			disconnectMixer()
		}
		let device = StreamFormat(sampleRate: rate, channels: channels, channelConfig: Self.channelConfig(channels: channels))
		var slice = UInt32(Self.maximumSlice)
		_ = AudioUnitSetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &slice, UInt32(MemoryLayout<UInt32>.size))
		if spatial {
			// The mixer gives the unit planar stereo, and takes the bed.
			var asbd = Self.planarFloatASBD(sampleRate: rate, channels: 2)
			try setProperty(kAudioUnitProperty_StreamFormat, scope: kAudioUnitScope_Input, &asbd)
			renderFormat = Pump.asbd(Self.spatialFormat(sampleRate: rate))
			var layout = AudioChannelLayout()
			layout.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo
			_ = AudioUnitSetProperty(unit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, 0, &layout, UInt32(MemoryLayout<AudioChannelLayout>.size))
		} else {
			var asbd = Pump.asbd(device)
			try setProperty(kAudioUnitProperty_StreamFormat, scope: kAudioUnitScope_Input, &asbd)
			renderFormat = asbd
			var layout = AudioChannelLayout()
			layout.mChannelLayoutTag = Self.layoutTag(channels: channels)
			// Not every route takes a layout; the stream format is what matters.
			_ = AudioUnitSetProperty(unit, kAudioUnitProperty_AudioChannelLayout, kAudioUnitScope_Input, 0, &layout, UInt32(MemoryLayout<AudioChannelLayout>.size))
		}
		sampleFormat = .float32
		var mixerLatency = 0
		if spatial {
			mixerLatency = try connectMixer(sampleRate: rate)
		}
		try Self.check(AudioUnitInitialize(unit))
		initialized = true

		isSpatial = spatial
		deviceChannels = channels
		format = spatial ? Self.spatialFormat(sampleRate: rate) : device
		latencyFrames = Int(((session.outputLatency + session.ioBufferDuration) * rate).rounded()) + mixerLatency
		EngineLog.logger.info("Rendering \(Self.describe(self.renderFormat), privacy: .public) for \(self.deviceName ?? "no route", privacy: .public), latency \(self.latencyFrames) frames")
		if wasRunning { try start() }
	}

	/// After media services reset: every unit made before is dead.
	private func rebuildUnits() throws {
		AudioComponentInstanceDispose(unit)
		unit = try Self.makeRemoteIO()
		initialized = false
		if let mixer {
			AudioComponentInstanceDispose(mixer)
			self.mixer = nil
			mixerInitialized = false
		}
		isSpatial = false
		mediaServicesReset = false
		// The engine attaches its new renderer after this; the old one may
		// already be gone.
		renderer = nil
	}

	// MARK: - Spatial audio

	/// The kind of output the route is: headphones (wired, Bluetooth, USB
	/// or AirPods), the built-in speaker, or anything else (AirPlay, HDMI,
	/// CarPlay), which is left alone.
	static func automaticSpatialOutput(portType: AVAudioSession.Port?, outputChannels: Int) -> SpatialOutput {
		guard outputChannels == 2, let portType else { return .off }
		switch portType {
		case .headphones, .bluetoothA2DP, .bluetoothLE, .usbAudio:
			return .headphones
		case .builtInSpeaker:
			return .speakers
		default:
			return .off
		}
	}

	/// The current route's one output, if it has exactly one.
	private static var currentOutput: AVAudioSessionPortDescription? {
		let outputs = AVAudioSession.sharedInstance().currentRoute.outputs
		return outputs.count == 1 ? outputs[0] : nil
	}

	/// A route's choice (`spatialDevicesKey`, by port UID), else the
	/// automatic guess. A route with other than two channels is never
	/// spatialized.
	public static func currentSpatialOutput() -> SpatialOutput {
		let session = AVAudioSession.sharedInstance()
		guard session.outputNumberOfChannels == 2 else { return .off }
		return chosenSpatialOutput() ?? automaticSpatialOutput(portType: currentOutput?.portType, outputChannels: 2)
	}

	/// The choice made for the current route; nil for automatic.
	public static func chosenSpatialOutput() -> SpatialOutput? {
		guard let uid = currentOutput?.uid else { return nil }
		return (UserDefaults.standard.dictionary(forKey: spatialDevicesKey)?[uid] as? String).flatMap(SpatialOutput.init(rawValue:))
	}

	/// Makes a choice for the current route, or (nil) leaves it automatic.
	public static func choose(_ output: SpatialOutput?) {
		guard let uid = currentOutput?.uid else { return }
		var choices = UserDefaults.standard.dictionary(forKey: spatialDevicesKey) ?? [:]
		choices[uid] = output?.rawValue
		UserDefaults.standard.set(choices, forKey: spatialDevicesKey)
	}

	/// What the spatial mixer renders for on the route; nil when its
	/// surround is downmixed instead.
	public var spatialOutputType: AUSpatialMixerOutputType? {
		switch Self.currentSpatialOutput() {
		case .headphones:
			return .spatialMixerOutputType_Headphones
		case .speakers:
			return Self.currentOutput?.portType == .builtInSpeaker ? .spatialMixerOutputType_BuiltInSpeakers : .spatialMixerOutputType_ExternalSpeakers
		case .off:
			return nil
		}
	}

	/// While spatial, renders for the route as it now is, live.
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
		_ = try? set(kAudioUnitProperty_SpatialMixerPersonalizedHRTFMode, kAudioUnitScope_Global, AUSpatialMixerPersonalizedHRTFMode.auto.rawValue)
		Self.setHeadTracking(UserDefaults.standard.bool(forKey: Self.headTrackingKey), on: mixer)
		try set(kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, UInt32(Self.maximumSlice))
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

	// MARK: - Exclusive access (not on iOS)

	/// Never: nothing can be held exclusively on iOS.
	public var exclusiveStream: AudioObjectID? { nil }

	public enum ExclusiveResult {
		case taken
		case unavailable
		case unsupportedFormat
	}

	public func takeExclusive(rate: Double, integerBits: Int = 0) -> ExclusiveResult { .unavailable }

	public func releaseExclusive() {}

	public func applyDeviceVolumeSetting() {}

	public static func recoverAbandonedSession() {}

	// MARK: - Device rate

	/// The session's rate; iOS offers no list of rates.
	public var availableSampleRates: [AudioValueRange] { [] }

	public func supportsSampleRate(_ rate: Double) -> Bool {
		abs(rate - nominalSampleRate) < 1
	}

	public var nominalSampleRate: Double {
		AVAudioSession.sharedInstance().sampleRate
	}

	/// Only succeeds for the rate the session already runs at: the session
	/// takes a preferred rate, which is the app's to ask for.
	public func setNominalSampleRate(_ rate: Double) -> Bool {
		supportsSampleRate(rate)
	}

	/// Whether the route's rate or channel count no longer matches the
	/// render format, or the unit must be made again.
	public func hardwareFormatDiffers() -> Bool {
		if mediaServicesReset { return true }
		let session = AVAudioSession.sharedInstance()
		return abs(session.sampleRate - format.sampleRate) >= 1 || min(max(session.outputNumberOfChannels, 1), 8) != deviceChannels
	}

	/// The session keeps its preferred I/O buffer across route changes.
	public func reassertBufferSize() {}

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
		if isSpatial, let mixer {
			// Plain C on the I/O thread still: the mixer pulls the renderer.
			_ = AudioUnitSetProperty(mixer, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, UInt32(MemoryLayout<AURenderCallbackStruct>.size))
		} else {
			try? setProperty(kAudioUnitProperty_SetRenderCallback, scope: kAudioUnitScope_Input, &callback)
		}
		if wasRunning { try? start() }
	}

	public func start() throws {
		guard let renderer, !isRunning else { return }
		try Self.ensureSessionActive()
		// The unit's sample time starts over with it.
		cog_renderer_forget_device_time(renderer)
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
		interruptedWhileRunning = false
	}

	/// Stops the unit. The renderer stays attached, so `start()` resumes.
	/// The session stays active: the app deactivates it.
	public func stop() {
		// The engine may destroy the renderer next: an interruption ending
		// later must not start the unit on it.
		interruptedWhileRunning = false
		stops += 1
		guard isRunning else { return }
		AudioOutputUnitStop(unit)
		isRunning = false
	}

	// MARK: - Session notifications

	private func installObservers() {
		let center = NotificationCenter.default
		let session = AVAudioSession.sharedInstance()
		observers.append(center.addObserver(forName: AVAudioSession.routeChangeNotification, object: session, queue: notificationQueue) { [weak self] note in
			let reason = (note.userInfo?[AVAudioSessionRouteChangeReasonKey] as? UInt).flatMap(AVAudioSession.RouteChangeReason.init(rawValue:))
			EngineLog.logger.info("Audio route changed (reason \(reason?.rawValue ?? 0)): \(session.currentRoute.outputs.map(\.portType.rawValue).joined(separator: ", "), privacy: .public)")
			switch reason {
			case .newDeviceAvailable, .oldDeviceUnavailable, .override, .categoryChange, .routeConfigurationChange, .wakeFromSleep:
				self?.onDeviceChange?(.format)
			default:
				break
			}
		})
		observers.append(center.addObserver(forName: AVAudioSession.interruptionNotification, object: session, queue: notificationQueue) { [weak self] note in
			guard let raw = note.userInfo?[AVAudioSessionInterruptionTypeKey] as? UInt,
			      let type = AVAudioSession.InterruptionType(rawValue: raw) else { return }
			DispatchQueue.main.async { self?.interruption(type, options: note.userInfo?[AVAudioSessionInterruptionOptionKey] as? UInt ?? 0) }
		})
		observers.append(center.addObserver(forName: AVAudioSession.mediaServicesWereResetNotification, object: session, queue: notificationQueue) { [weak self] _ in
			EngineLog.logger.error("Media services were reset; rebuilding the output")
			Self.sessionLock.withLock { Self.sessionActive = false }
			DispatchQueue.main.async {
				guard let self else { return }
				self.mediaServicesReset = true
				self.isRunning = false
				self.initialized = false
				self.mixerInitialized = false
				self.onDeviceChange?(.format)
			}
		})
	}

	private func removeObservers() {
		for observer in observers {
			NotificationCenter.default.removeObserver(observer)
		}
		observers.removeAll()
	}

	/// Main thread. The system stopped the unit (a call, an alarm, another
	/// app taking the session); when it ends with leave to resume, the unit
	/// starts again where the renderer left off. Until then the engine sees
	/// a device that pulls nothing, so the position holds.
	private func interruption(_ type: AVAudioSession.InterruptionType, options: UInt) {
		switch type {
		case .began:
			EngineLog.logger.notice("Audio session interrupted")
			Self.sessionLock.withLock { Self.sessionActive = false }
			if isRunning {
				interruptedWhileRunning = true
				AudioOutputUnitStop(unit)
				isRunning = false
			}
		case .ended:
			let resume = AVAudioSession.InterruptionOptions(rawValue: options).contains(.shouldResume)
			EngineLog.logger.notice("Audio session interruption ended\(resume ? ", resuming" : "", privacy: .public)")
			guard interruptedWhileRunning, resume else { return }
			interruptedWhileRunning = false
			let stopsBefore = stops
			Task { @MainActor in
				do {
					try await Self.activateSession()
					guard self.stops == stopsBefore else { return }
					try self.start()
				} catch {
					EngineLog.logger.error("Could not resume after the interruption: \(error.localizedDescription, privacy: .public)")
				}
			}
		@unknown default:
			break
		}
	}
}

#endif
