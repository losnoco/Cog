//
//  PlaybackEngine.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import Foundation

/// What the engine tells the player about. `AudioPlayer` implements this and
/// turns each call into its existing delegate messages.
@objc public protocol PlaybackEngineHost: AnyObject {
	/// Feeder thread: the track after the one with `userInfo`, or nil to end.
	/// May block on the main thread (the playlist decides).
	func playbackEngineNextTrack(after userInfo: Any?) -> EngineTrack?

	/// Main thread, as each track is heard.
	func playbackEngineDidBeginTrack(_ userInfo: Any?)
	func playbackEngineDidChangeStatus(_ status: CogStatus, userInfo: Any?)
	func playbackEngineDidStopNaturally(_ userInfo: Any?)
	func playbackEngineReportPlayCount(_ userInfo: Any?)
	func playbackEngineReportScrobble(_ userInfo: Any?)
	func playbackEngineSetError(_ error: Bool, forTrack userInfo: Any?)
	/// The output device changed in a way that needs playback rebuilt.
	func playbackEngineRestartAtCurrentPosition(_ userInfo: Any?)
}

/// The new engine behind `AudioPlayer`: feeder and DSP threads, the device
/// output, and the bookkeeping that turns rendered frames into track changes,
/// play counts and the playback position.
///
/// All control methods are for the main thread, which is how `AudioPlayer`
/// is driven.
@objc public final class PlaybackEngine: NSObject, FeederDelegate {
	@objc public weak var host: PlaybackEngineHost?

	/// Opens decoders; the plugins unless a test supplies its own.
	var opener: Feeder.Opener?

	/// Transport fades, as Cog's `fadeTimeMS`.
	private static let fadeSeconds = 0.2
	/// How much the shallow ring must hold before the device starts.
	private static let prebufferSeconds = 0.1

	private var output: DeviceOutput?
	private var feeder: Feeder?
	private var pump: Pump?
	private var renderer: OpaquePointer?
	private var monitor: Timer?

	private enum Phase {
		case idle
		/// Waiting for audio before starting the device (`startPaused` stays
		/// here until resumed).
		case prebuffering(paused: Bool)
		case playing
		case pausing
		case paused
	}
	private var phase = Phase.idle {
		didSet { EngineLog.logger.info("Phase \(String(describing: self.phase), privacy: .public)") }
	}

	private var volumeLevel: Double = 100

	// What is being heard.
	private var currentTrack: EngineTrack?
	private var currentOffset: Double = 0
	private var currentStart: UInt64 = 0
	private var initialTrack: EngineTrack?
	/// Whether `currentTrack` has actually been heard (a first track that
	/// fails to open never is, and must not be counted as played).
	private var currentHeard = false
	private var seekPending = false

	// Position and play counting, as OutputNode keeps them.
	@objc public private(set) var amountPlayed: Double = 0
	@objc public private(set) var amountPlayedInterval: Double = 0
	private var intervalReported = false
	private var scrobbleThreshold: Double = 0
	private var scrobbleReported = false

	@objc public override init() {
		super.init()
		// Only the output device key: UserDefaults.didChangeNotification fires
		// for every write, and the preferences window writes plenty.
		UserDefaults.standard.addObserver(self, forKeyPath: "outputDevice", options: [], context: &Self.outputDeviceContext)
	}

	deinit {
		UserDefaults.standard.removeObserver(self, forKeyPath: "outputDevice", context: &Self.outputDeviceContext)
	}

	private static var outputDeviceContext = 0

	public override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard context == &Self.outputDeviceContext else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}
		if Thread.isMainThread {
			outputDeviceSettingChanged()
		} else {
			DispatchQueue.main.async { self.outputDeviceSettingChanged() }
		}
	}

	private static var fadesEnabled: Bool {
		(UserDefaults.standard.object(forKey: "enableFading") as? NSNumber)?.boolValue ?? true
	}

	private var fadeFrames: UInt32 {
		guard Self.fadesEnabled, let rate = output?.format.sampleRate else { return 0 }
		return UInt32(rate * Self.fadeSeconds)
	}

	// MARK: - Control

	/// Starts `url` from `seconds`. Returns false if the device could not be
	/// opened.
	@objc public func play(_ url: URL, userInfo: Any?, rgInfo: [AnyHashable: Any]?, startPaused: Bool, seekTo seconds: Double) -> Bool {
		tearDown()

		do {
			if output == nil {
				let device = try DeviceOutput()
				device.onDeviceChange = { [weak self] change in
					DispatchQueue.main.async { self?.deviceChanged(change) }
				}
				output = device
			}
			try selectSavedDevice()
		} catch {
			output = nil
			return false
		}
		guard let output,
		      let feeder = Feeder(outputRate: output.format.sampleRate, opener: opener),
		      let pump = Pump(feeder: feeder, outputFormat: output.format),
		      let renderer = cog_renderer_create(pump.ring) else {
			return false
		}
		self.feeder = feeder
		self.pump = pump
		self.renderer = renderer
		reportedUnderruns = 0
		lastBeat = nil
		output.attach(renderer)
		cog_gain_ramp_to(cog_renderer_volume(renderer), Float(volumeLevel * 0.01), 0)
		cog_gain_ramp_to(cog_renderer_transport(renderer), fadeFrames > 0 ? 0 : 1, 0)

		let track = EngineTrack(url: url, userInfo: userInfo, rgInfo: rgInfo)
		initialTrack = track
		currentTrack = track
		currentOffset = seconds
		currentStart = 0
		currentHeard = false
		amountPlayed = seconds
		resetInterval()
		scrobbleReported = false

		feeder.delegate = self
		feeder.start(with: track, offset: seconds)
		pump.start()

		phase = .prebuffering(paused: startPaused)
		host?.playbackEngineDidChangeStatus(startPaused ? .paused : .playing, userInfo: userInfo)

		let monitor = Timer(timeInterval: 0.02, repeats: true) { [weak self] _ in self?.tick() }
		RunLoop.main.add(monitor, forMode: .common)
		self.monitor = monitor
		return true
	}

	@objc public func stop() {
		let userInfo = currentTrack?.userInfo
		tearDown()
		host?.playbackEngineDidChangeStatus(.stopped, userInfo: userInfo)
	}

	@objc public func pause() {
		guard let renderer else { return }
		switch phase {
		case .prebuffering:
			phase = .prebuffering(paused: true)
		case .playing:
			cog_gain_ramp_to(cog_renderer_transport(renderer), 0, fadeFrames)
			phase = .pausing
		default:
			return
		}
		host?.playbackEngineDidChangeStatus(.paused, userInfo: currentTrack?.userInfo)
	}

	@objc public func resume() {
		guard let renderer else { return }
		switch phase {
		case .prebuffering:
			phase = .prebuffering(paused: false)
		case .paused, .pausing:
			try? output?.start()
			cog_gain_ramp_to(cog_renderer_transport(renderer), 1, fadeFrames)
			phase = .playing
		default:
			return
		}
		host?.playbackEngineDidChangeStatus(.playing, userInfo: currentTrack?.userInfo)
	}

	/// Seeks within the track being heard.
	@objc public func seek(to seconds: Double) {
		guard let feeder, let renderer, let track = currentTrack else { return }
		// Duck briefly so the cut to the new position is not a step; the
		// transport comes back up when the new position is heard.
		cog_gain_ramp_to(cog_renderer_transport(renderer), 0, UInt32((output?.format.sampleRate ?? 48000) * 0.005))
		seekPending = true
		amountPlayed = seconds
		DispatchQueue.main.asyncAfter(deadline: .now() + .milliseconds(6)) {
			feeder.seek(to: seconds, in: track)
		}
	}

	@objc public var volume: Double {
		get { volumeLevel }
		set {
			volumeLevel = newValue
			if let renderer, let rate = output?.format.sampleRate {
				cog_gain_ramp_to(cog_renderer_volume(renderer), Float(newValue * 0.01), UInt32(rate * 0.01))
			}
		}
	}

	@objc public func setScrobbleThreshold(_ threshold: Double) {
		scrobbleThreshold = threshold
		scrobbleReported = false
	}

	private func tearDown() {
		monitor?.invalidate()
		monitor = nil
		output?.stop()
		feeder?.stop()
		pump?.stop()
		if let renderer {
			cog_renderer_destroy(renderer)
		}
		feeder = nil
		pump = nil
		renderer = nil
		currentTrack = nil
		initialTrack = nil
		seekPending = false
		phase = .idle
	}

	// MARK: - Monitoring

	private func tick() {
		guard let pump, let renderer, let output else { return }

		switch phase {
		case let .prebuffering(paused):
			let ready = cog_ring_readable(pump.ring) >= Int(output.format.sampleRate * Self.prebufferSeconds)
			if !paused && (ready || feeder.map(isFinished) == true) {
				try? output.start()
				cog_gain_ramp_to(cog_renderer_transport(renderer), 1, fadeFrames)
				phase = .playing
			}
		case .pausing:
			if cog_gain_settled(cog_renderer_transport(renderer)) {
				output.stop()
				phase = .paused
			}
		default:
			break
		}

		let underruns = cog_renderer_underrun_events(renderer)
		if underruns != reportedUnderruns {
			reportedUnderruns = underruns
			let rate = output.format.sampleRate
			let shallowMs = Double(cog_ring_readable(pump.ring)) / rate * 1000
			let deepMs = Double(cog_ring_readable(pump.feederRing)) / Double(max(1, pump.outputFormat.channels)) / rate * 1000
			EngineLog.logger.error("Underrun #\(underruns): shallow ring \(shallowMs, format: .fixed(precision: 1)) ms, deep ring about \(deepMs, format: .fixed(precision: 0)) ms")
		}

		heartbeat(pump: pump, renderer: renderer, output: output)

		let read = cog_ring_read_position(pump.ring)
		let heard = heardPosition(read: read, dry: cog_ring_readable(pump.ring) == 0, output: output)

		for (position, event) in pump.presentation.take(through: heard) {
			switch event {
			case let .trackStart(track, offset):
				trackHeard(track, offset: offset, at: position)
			case .endOfStream:
				finishTrack()
				let userInfo = currentTrack?.userInfo
				tearDown()
				host?.playbackEngineDidChangeStatus(.stopped, userInfo: userInfo)
				host?.playbackEngineDidStopNaturally(userInfo)
				return
			}
		}

		if let track = currentTrack, !seekPending, heard >= currentStart {
			let seconds = currentOffset + Double(heard - currentStart) / output.format.sampleRate
			advanceAmountPlayed(to: seconds, of: track)
		}
	}

	/// The shallow-ring position now leaving the device. While audio flows
	/// that is the read position less the device latency. When the ring runs
	/// dry the read position stops at the end, but the device still plays out
	/// what it holds, so once the latency has passed in real time everything
	/// read has been heard.
	private func heardPosition(read: UInt64, dry: Bool, output: DeviceOutput) -> UInt64 {
		let latency = UInt64(output.latencyFrames)
		guard dry, isOutputRunning else {
			dryRead = nil
			return read > latency ? read - latency : 0
		}
		if dryRead?.position != read {
			dryRead = (read, Date())
		}
		let drained = Date().timeIntervalSince(dryRead!.since) * output.format.sampleRate
		let remaining = UInt64(max(0, Double(latency) - drained))
		return read > remaining ? read - remaining : 0
	}

	private var dryRead: (position: UInt64, since: Date)?

	private var lastBeat: (time: UInt64, rendered: UInt64)?

	/// Once a second: buffer levels, and whether the device pulled as many
	/// frames as the wall clock says it should have. A shortfall there is a
	/// device-side glitch even when no ring ran dry.
	private func heartbeat(pump: Pump, renderer: OpaquePointer, output: DeviceOutput) {
		let now = EngineLog.now()
		let rendered = cog_renderer_frames_rendered(renderer)
		guard let last = lastBeat else {
			lastBeat = (now, rendered)
			return
		}
		let elapsed = Double(now - last.time) / 1_000_000_000
		guard elapsed >= 1 else { return }
		lastBeat = (now, rendered)
		guard output.isRunning else { return }
		let rate = output.format.sampleRate
		let pulledMs = Double(rendered - last.rendered) / rate * 1000
		let shortfallMs = elapsed * 1000 - pulledMs
		let shallowMs = Double(cog_ring_readable(pump.ring)) / rate * 1000
		let deepMs = Double(cog_ring_readable(pump.feederRing)) / Double(max(1, pump.outputFormat.channels)) / rate * 1000
		let message = String(format: "Heartbeat: device pulled %.1f ms in %.1f ms, shallow %.1f ms, deep %.0f ms, underruns %llu",
		                     pulledMs, elapsed * 1000, shallowMs, deepMs, cog_renderer_underrun_events(renderer))
		if abs(shortfallMs) > 30 {
			EngineLog.logger.error("\(message, privacy: .public) — device shortfall \(shortfallMs, format: .fixed(precision: 1)) ms")
		} else {
			EngineLog.logger.debug("\(message, privacy: .public)")
		}
	}
	private var reportedUnderruns: UInt64 = 0

	private var isOutputRunning: Bool {
		if case .playing = phase { return true }
		return false
	}

	private func isFinished(_ feeder: Feeder) -> Bool {
		// The whole stream fits in the prebuffer: start anyway.
		cog_ring_readable(feeder.ring) == 0 && cog_ring_readable(pump?.ring ?? feeder.ring) > 0
	}

	private func trackHeard(_ track: EngineTrack, offset: Double, at position: UInt64) {
		if track === currentTrack && (seekPending || track === initialTrack) {
			// The start of playback, or a seek within the same track.
			if seekPending, let renderer {
				seekPending = false
				cog_gain_ramp_to(cog_renderer_transport(renderer), 1, fadeFrames)
			}
			initialTrack = nil
		} else {
			finishTrack()
			currentTrack = track
			initialTrack = nil
			amountPlayed = offset
			scrobbleReported = false
			host?.playbackEngineDidBeginTrack(track.userInfo)
		}
		currentOffset = offset
		currentStart = position
		currentHeard = true
	}

	/// The heard track ended: count it if it has not been counted yet.
	private func finishTrack() {
		guard let track = currentTrack, currentHeard else { return }
		if !intervalReported {
			intervalReported = true
			host?.playbackEngineReportPlayCount(track.userInfo)
		}
		if !scrobbleReported && scrobbleThreshold > 0 {
			scrobbleReported = true
			host?.playbackEngineReportScrobble(track.userInfo)
		}
		resetInterval()
	}

	private func resetInterval() {
		amountPlayedInterval = 0
		intervalReported = false
	}

	/// OutputNode's `setAmountPlayed:` rules: small forward steps count as
	/// listening, anything else is a jump.
	private func advanceAmountPlayed(to seconds: Double, of track: EngineTrack) {
		let delta = seconds - amountPlayed
		if delta > 0 && delta < 5 {
			amountPlayed = seconds
			amountPlayedInterval += delta
			if !intervalReported && amountPlayedInterval >= 60 {
				intervalReported = true
				host?.playbackEngineReportPlayCount(track.userInfo)
			}
			if !scrobbleReported && scrobbleThreshold > 0 && amountPlayed >= scrobbleThreshold {
				scrobbleReported = true
				host?.playbackEngineReportScrobble(track.userInfo)
			}
		} else if delta != 0 {
			amountPlayed = seconds
		}
	}

	// MARK: - Devices

	/// The output device setting as last seen, to skip identical rewrites.
	private var lastSeenDeviceSetting: NSDictionary?

	private func selectSavedDevice() throws {
		guard let output else { return }
		let saved = UserDefaults.standard.dictionary(forKey: "outputDevice")
		lastSeenDeviceSetting = saved.map { $0 as NSDictionary }
		if !(try output.selectDevice(saved)) {
			UserDefaults.standard.removeObject(forKey: "outputDevice")
		}
	}

	/// The output device setting was written. Rebuild only if it now names a
	/// different device than the one in use: the preferences window rewrites
	/// the setting just by opening, as OutputCoreAudio also saw. The cheap
	/// dictionary comparison comes first, so rewrites of the same value cost
	/// no CoreAudio queries.
	private func outputDeviceSettingChanged() {
		guard feeder != nil, let output else { return }
		let saved = UserDefaults.standard.dictionary(forKey: "outputDevice")
		let savedObject = saved.map { $0 as NSDictionary }
		guard savedObject != lastSeenDeviceSetting else { return }
		lastSeenDeviceSetting = savedObject
		guard output.wouldChange(for: saved) else { return }
		EngineLog.logger.info("Output device setting now names another device; rebuilding")
		DispatchQueue.main.async { self.deviceChanged(.device) }
	}

	/// The device or its format changed: rebuild at the current position,
	/// unless nothing that matters to the render format actually changed.
	private func deviceChanged(_ change: DeviceOutput.Change) {
		guard feeder != nil, let output else { return }
		switch change {
		case .format:
			guard output.hardwareFormatDiffers() else {
				EngineLog.logger.debug("Device format notification without a change; ignored")
				return
			}
		case .device:
			let saved = UserDefaults.standard.dictionary(forKey: "outputDevice")
			guard output.wouldChange(for: saved) || !DeviceOutput.isAliveOutput(output.deviceID) else {
				EngineLog.logger.debug("Device notification without a change of device; ignored")
				return
			}
		}
		EngineLog.logger.info("Output \(String(describing: change), privacy: .public) changed; restarting at the current position")
		host?.playbackEngineRestartAtCurrentPosition(currentTrack?.userInfo)
	}

	// MARK: - FeederDelegate

	public func feeder(_ feeder: Feeder, nextTrackAfter track: EngineTrack) -> EngineTrack? {
		host?.playbackEngineNextTrack(after: track.userInfo)
	}

	public func feeder(_ feeder: Feeder, couldNotOpen track: EngineTrack) {
		host?.playbackEngineSetError(true, forTrack: track.userInfo)
	}
}
