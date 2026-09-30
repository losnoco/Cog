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

	/// Main thread: the equalizer started or stopped being used.
	func playbackEngineBeginEqualizer(_ equalizer: CogEqualizer)
	func playbackEngineEndEqualizer(_ equalizer: CogEqualizer)
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

	/// One equalizer for the engine's lifetime: the app keeps an unretained
	/// reference to whichever equalizer it was last given.
	private let timeStretch = TimeStretchStage()
	private let freeSurround = FreeSurroundStage()
	private let hrtf = HRTFStage()
	private let visualization = VisualizationTap()

	private lazy var equalizer: EqualizerStage = {
		let equalizer = EqualizerStage()
		equalizer.onActivation = { [weak self] active in
			guard let self else { return }
			if active {
				self.host?.playbackEngineBeginEqualizer(self.equalizer)
			} else {
				self.host?.playbackEngineEndEqualizer(self.equalizer)
			}
		}
		return equalizer
	}()
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
	/// Track time at `currentStart`, and track frames per output frame from
	/// there (the tempo while time-stretching): a piecewise stretch map.
	private var currentOffset: Double = 0
	private var currentStart: UInt64 = 0
	private var currentRatio: Double = 1
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
		UserDefaults.standard.addObserver(self, forKeyPath: "volumeScaling", options: [], context: &Self.volumeScalingContext)
	}

	deinit {
		UserDefaults.standard.removeObserver(self, forKeyPath: "outputDevice", context: &Self.outputDeviceContext)
		UserDefaults.standard.removeObserver(self, forKeyPath: "volumeScaling", context: &Self.volumeScalingContext)
	}

	private static var outputDeviceContext = 0
	private static var volumeScalingContext = 0

	// MARK: - ReplayGain

	/// Tracks that may still be playing or queued, for gain updates. Weak, and
	/// shared with the feeder thread (which adds next tracks).
	private let liveTracksLock = UnfairLock()
	private var liveTracks: [WeakTrack] = []

	private struct WeakTrack {
		weak var track: EngineTrack?
	}

	private func register(_ track: EngineTrack) {
		liveTracksLock.withLock {
			liveTracks.removeAll { $0.track == nil }
			liveTracks.append(WeakTrack(track: track))
		}
	}

	private var tracksInPlay: [EngineTrack] {
		liveTracksLock.withLock { liveTracks.compactMap(\.track) }
	}

	/// New ReplayGain info for the playlist entry `userInfo`, for example
	/// once its tags have loaded after playback began. Heard within the
	/// shallow ring, with a short ramp.
	@objc public func updateReplayGain(_ rgInfo: [AnyHashable: Any]?, forTrack userInfo: Any?) {
		for track in tracksInPlay where track.belongs(to: userInfo) {
			track.update(rgInfo: rgInfo)
		}
	}

	/// The volume scaling setting changed: recompute every track's gain.
	private func volumeScalingChanged() {
		for track in tracksInPlay {
			track.update(rgInfo: track.rgInfo)
		}
	}

	public override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == &Self.volumeScalingContext {
			volumeScalingChanged()
			return
		}
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
		      let pump = Pump(feeder: feeder, outputFormat: output.format, stages: [timeStretch, freeSurround, equalizer, visualization, hrtf]),
		      let renderer = cog_renderer_create(pump.ring) else {
			return false
		}
		self.feeder = feeder
		self.pump = pump
		self.renderer = renderer
		equalizer.rearm()
		reportedUnderruns = 0
		lastBeat = nil
		output.attach(renderer)
		cog_gain_ramp_to(cog_renderer_volume(renderer), Float(volumeLevel * 0.01), 0)
		cog_gain_ramp_to(cog_renderer_transport(renderer), fadeFrames > 0 ? 0 : 1, 0)
		// Room for a seek's crossfade, allocated before the device runs.
		_ = cog_renderer_set_crossfade_frames(renderer, Int(output.format.sampleRate * Self.fadeSeconds))

		let track = EngineTrack(url: url, userInfo: userInfo, rgInfo: rgInfo)
		register(track)
		EngineLog.logger.info("Play \(url.lastPathComponent, privacy: .public) from \(seconds, format: .fixed(precision: 2)) s\(startPaused ? " paused" : "", privacy: .public): volume \(self.volumeLevel, format: .fixed(precision: 1)), fade \(self.fadeFrames) frames, track gain \(track.gain, format: .fixed(precision: 4)), rgInfo \(String(describing: rgInfo ?? [:]), privacy: .public)")
		initialTrack = track
		currentTrack = track
		currentOffset = seconds
		currentStart = 0
		currentRatio = 1
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

	/// Transport ramps, logged for diagnosing level problems.
	/// Ramps the transport gain from `from` (nil: wherever it is) to
	/// `target`, as one request to the renderer.
	private func rampTransport(_ renderer: OpaquePointer, from: Float? = nil, to target: Float, frames: UInt32) {
		let transport = cog_renderer_transport(renderer)
		EngineLog.logger.info("Transport \(from ?? cog_gain_current(transport), format: .fixed(precision: 3)) -> \(target, format: .fixed(precision: 3)) over \(frames) frames (phase \(String(describing: self.phase), privacy: .public))")
		cog_gain_ramp(transport, from ?? .nan, target, frames)
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
			rampTransport(renderer, to: 0, frames: fadeFrames)
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
			rampTransport(renderer, to: 1, frames: fadeFrames)
			phase = .playing
		default:
			return
		}
		host?.playbackEngineDidChangeStatus(.playing, userInfo: currentTrack?.userInfo)
	}

	/// Seeks within the track being heard.
	@objc public func seek(to seconds: Double) {
		guard let feeder, let renderer, let track = currentTrack else { return }
		// The renderer crossfades from what it was about to play to the new
		// position as it honours the flush; with fades off, it cuts.
		cog_renderer_set_crossfade_enabled(renderer, Self.fadesEnabled)
		seekPending = true
		amountPlayed = seconds
		feeder.seek(to: seconds, in: track)
	}

	@objc public var volume: Double {
		get { volumeLevel }
		set {
			EngineLog.logger.info("Volume \(self.volumeLevel, format: .fixed(precision: 1)) -> \(newValue, format: .fixed(precision: 1))")
			volumeLevel = newValue
			if let renderer, let rate = output?.format.sampleRate {
				cog_gain_ramp_to(cog_renderer_volume(renderer), Float(newValue * 0.01), UInt32(rate * 0.01))
			}
		}
	}

	/// The playlist changed after the next track was chosen: drop any queued
	/// track not yet playing and ask again what follows the current one.
	@objc public func resetNextStreams() {
		feeder?.resetNextTracks()
	}

	@objc public func setScrobbleThreshold(_ threshold: Double) {
		scrobbleThreshold = threshold
		scrobbleReported = false
	}

	private func tearDown() {
		monitor?.invalidate()
		monitor = nil
		if feeder != nil {
			VisualizationController.shared().reset()
		}
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
				// The fade-in starts from silence whatever the gain last was.
				rampTransport(renderer, from: fadeFrames > 0 ? 0 : 1, to: 1, frames: fadeFrames)
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
		postVisualizationLatency(pump: pump, output: output)

		let read = cog_ring_read_position(pump.ring)
		let heard = heardPosition(read: read, dry: cog_ring_readable(pump.ring) == 0, output: output)

		for (position, event) in pump.presentation.take(through: heard) {
			switch event {
			case let .trackStart(track, offset):
				trackHeard(track, offset: offset, at: position)
			case let .rate(ratio):
				// Close the segment at the old ratio and start one at the new.
				if position > currentStart {
					currentOffset += Double(position - currentStart) / output.format.sampleRate * currentRatio
					currentStart = position
				}
				currentRatio = ratio
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
			let seconds = currentOffset + Double(heard - currentStart) / output.format.sampleRate * currentRatio
			advanceAmountPlayed(to: seconds, of: track)
		}
	}

	/// Tells the spectrum and oscilloscope how far behind the device the
	/// audio they were last given is: everything written to the shallow ring
	/// but not yet heard. The full latency adds the deep ring.
	private func postVisualizationLatency(pump: Pump, output: DeviceOutput) {
		let rate = output.format.sampleRate
		let read = cog_ring_read_position(pump.ring)
		let heard = heardPosition(read: read, dry: cog_ring_readable(pump.ring) == 0, output: output)
		let written = cog_ring_write_position(pump.ring)
		let latency = Double(written > heard ? written - heard : 0) / rate
		let deep = Double(cog_ring_readable(pump.feederRing)) / Double(max(1, pump.outputFormat.channels)) / rate
		let controller = VisualizationController.shared()
		controller.postLatency(latency)
		controller.postFullLatency(latency + deep)
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
		let message = String(format: "Heartbeat: device pulled %.1f ms in %.1f ms, shallow %.1f ms, deep %.0f ms, underruns %llu; peak out %.3f, gains transport %.3f volume %.3f track %.4f",
		                     pulledMs, elapsed * 1000, shallowMs, deepMs, cog_renderer_underrun_events(renderer),
		                     cog_renderer_take_peak(renderer), cog_gain_current(cog_renderer_transport(renderer)),
		                     cog_gain_current(cog_renderer_volume(renderer)), pump.currentTrackGain)
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
			seekPending = false
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
		let next = host?.playbackEngineNextTrack(after: track.userInfo)
		if let next {
			register(next)
		}
		return next
	}

	public func feeder(_ feeder: Feeder, couldNotOpen track: EngineTrack) {
		host?.playbackEngineSetError(true, forTrack: track.userInfo)
	}
}
