//
//  Feeder.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import Foundation

public protocol FeederDelegate: AnyObject {
	/// Called on the feeder thread when `track` has been decoded to its end.
	/// Return the track to play gaplessly after it, or nil to finish. May
	/// block briefly (for example to ask the main thread); the deep ring keeps
	/// playback going meanwhile.
	func feeder(_ feeder: Feeder, nextTrackAfter track: EngineTrack) -> EngineTrack?

	/// A track could not be opened and was skipped.
	func feeder(_ feeder: Feeder, couldNotOpen track: EngineTrack)
}

/// The engine's first worker thread: decodes, converts to float, resamples to
/// the output rate and writes into the deep ring, marking format changes and
/// track boundaries on the timeline.
///
/// Track boundaries tear nothing down. The next decoder is opened when the
/// current one runs out, and the persistent `StreamConverter` carries its
/// resampler straight across when the format allows.
///
/// The deep ring holds single samples (its channel count is 1) so that the
/// channel count may change between tracks; `.format` entries on the
/// timeline say how to group them into frames.
public final class Feeder {
	public typealias Opener = (EngineTrack) -> CogDecoder?

	/// Deep ring: interleaved samples at the output rate.
	public let ring: OpaquePointer
	public let timeline = Timeline()
	public let outputRate: Double
	public weak var delegate: FeederDelegate?

	private let opener: Opener
	private let converter: StreamConverter
	private let floatConverter = ChunkList(maximumDuration: 1.0)
	private let spaceAvailable = DispatchSemaphore(value: 0)
	private let idle = DispatchSemaphore(value: 0)
	private let lock = UnfairLock()

	// Shared with control threads, under `lock`.
	private var running = false
	private var pendingSeek: (track: EngineTrack, seconds: Double)?
	private var pendingReset = false
	private var thread: Thread?
	private var threadExited = DispatchSemaphore(value: 0)

	/// The decoder in use, visible to `stop()` so a blocking read can be
	/// interrupted. Under `lock`; the feeder thread is the only writer.
	private var interruptible: CogDecoder?

	// Feeder thread only.
	private var decoder: CogDecoder? {
		didSet {
			let current = decoder
			lock.withLock { interruptible = current }
		}
	}
	private var track: EngineTrack?
	private var epoch: UInt64 = 0
	private var writtenFormat: StreamFormat?
	private var trackStartPending: (track: EngineTrack, offset: Double)?
	/// The track the pending start follows, for recording the join.
	private var previousTrack: EngineTrack?
	private var finished = false
	private var waitNanoseconds: UInt64 = 0

	/// Queued boundaries the DSP thread may not have reached: where the track
	/// after `before` starts (or the stream ends), in output frames of the
	/// current epoch. A playlist change can abandon them.
	private var joins: [(frame: UInt64, before: EngineTrack)] = []

	/// Maximum channels a track may carry; sizes the deep ring.
	public static let maximumChannels = 8

	/// - Parameters:
	///   - outputRate: the rate the device runs at; everything is resampled to it.
	///   - seconds: deep ring length at `maximumChannels`; more at fewer channels.
	///   - opener: opens a track's decoder; defaults to Cog's plugin lookup.
	public init?(outputRate: Double, seconds: Double = 2.0, opener: Opener? = nil) {
		let samples = Int(outputRate * seconds) * Self.maximumChannels
		guard let ring = cog_ring_create(samples, 1) else { return nil }
		self.ring = ring
		self.outputRate = outputRate
		self.opener = opener ?? Self.openWithPlugins
		converter = StreamConverter(outputRate: outputRate)
	}

	deinit {
		stop()
		cog_ring_destroy(ring)
	}

	// MARK: - Control

	/// Starts decoding `track` from `offset` seconds.
	public func start(with track: EngineTrack, offset: Double = 0) {
		lock.withLock {
			guard !running else { return }
			running = true
			pendingSeek = (track, offset)
			threadExited = DispatchSemaphore(value: 0)
		}
		let thread = Thread { [weak self] in
			self?.run()
		}
		thread.name = "Cog Feeder"
		thread.qualityOfService = .userInitiated
		lock.withLock { self.thread = thread }
		thread.start()
	}

	/// Stops decoding and waits for the thread to finish.
	public func stop() {
		let (exited, decoder): (DispatchSemaphore?, CogDecoder?) = lock.withLock {
			guard running else { return (nil, nil) }
			running = false
			return (threadExited, interruptible)
		}
		guard let exited else { return }
		decoder?.interrupt?()
		spaceAvailable.signal()
		idle.signal()
		Self.wait(for: exited)
	}

	/// Waits for `semaphore`. On the main thread the run loop keeps turning
	/// meanwhile: the feeder may itself be waiting on the main thread (asking
	/// for the next track), and blocking here would deadlock the two.
	static func wait(for semaphore: DispatchSemaphore) {
		guard Thread.isMainThread else {
			semaphore.wait()
			return
		}
		while semaphore.wait(timeout: .now()) == .timedOut {
			RunLoop.current.run(mode: .default, before: Date(timeIntervalSinceNow: 0.005))
		}
	}

	/// Moves playback to `seconds` into `track`, which need not be the track
	/// being decoded (it usually is the one being heard, a few seconds behind).
	/// Everything queued is discarded.
	public func seek(to seconds: Double, in track: EngineTrack) {
		lock.withLock {
			pendingSeek = (track, seconds)
		}
		spaceAvailable.signal()
		idle.signal()
	}

	/// The playlist changed after the next track was requested: forget any
	/// queued track that has not started playing, and ask again what follows
	/// the one being heard. A queued end of stream is forgotten too, so a
	/// track added while the last one plays is picked up. A track the DSP
	/// thread has already reached stays.
	public func resetNextTracks() {
		lock.withLock { pendingReset = true }
		spaceAvailable.signal()
		idle.signal()
	}

	private func takePendingReset() -> Bool {
		lock.withLock {
			defer { pendingReset = false }
			return pendingReset
		}
	}

	/// The consumer calls this after reading, so a feeder waiting for space
	/// wakes at once instead of at its next poll.
	public func consumerDidRead() {
		spaceAvailable.signal()
	}

	private var isRunning: Bool {
		lock.withLock { running }
	}

	private func takePendingSeek() -> (track: EngineTrack, seconds: Double)? {
		lock.withLock {
			defer { pendingSeek = nil }
			return pendingSeek
		}
	}

	private var hasPendingSeek: Bool {
		lock.withLock { pendingSeek != nil }
	}

	// MARK: - Thread

	private func run() {
		defer {
			closeDecoder()
			lock.withLock { threadExited }.signal()
		}

		while isRunning {
			if let seek = takePendingSeek() {
				performSeek(to: seek.seconds, in: seek.track)
				continue
			}

			if takePendingReset() {
				performReset()
				continue
			}

			if finished {
				// Nothing left to decode; wait for a seek or a stop.
				_ = idle.wait(timeout: .now() + .milliseconds(100))
				continue
			}

			let start = EngineLog.now()
			let chunk: AudioChunk? = autoreleasepool {
				decoder?.readAudio()
			}
			let decodeMs = EngineLog.milliseconds(since: start)
			if let chunk, chunk.frameCount() > 0 {
				feed(chunk)
			} else {
				advance()
			}
			// Waiting for ring space is expected; only report time spent working.
			let workMs = EngineLog.milliseconds(since: start) - Double(waitNanoseconds) / 1_000_000
			waitNanoseconds = 0
			if workMs > EngineLog.slowPass * 1000 {
				let deepMs = Double(cog_ring_readable(ring)) / 2 / outputRate * 1000
				EngineLog.logger.warning("Feeder pass worked \(workMs, format: .fixed(precision: 1)) ms (decode \(decodeMs, format: .fixed(precision: 1)) ms), deep ring about \(deepMs, format: .fixed(precision: 0)) ms")
			}
		}
	}

	/// The current decoder ran out: open the next track, or finish.
	private func advance() {
		guard let finishedTrack = track else {
			finish()
			return
		}
		var candidate = delegate?.feeder(self, nextTrackAfter: finishedTrack)
		var attempts = 0
		while let next = candidate, isRunning {
			if let nextDecoder = opener(next) {
				closeDecoder()
				decoder = nextDecoder
				track = next
				// Placed when the new track's first frames arrive, once it is
				// known whether the resampler carries straight across.
				trackStartPending = (next, 0)
				previousTrack = finishedTrack
				return
			}
			delegate?.feeder(self, couldNotOpen: next)
			attempts += 1
			candidate = attempts < 16 ? delegate?.feeder(self, nextTrackAfter: next) : nil
		}
		finish()
	}

	private func finish() {
		converter.drain { samples, format in write(samples, format: format) }
		if let track {
			joins.append((converter.outputFrames, track))
		}
		timeline.append(.endOfStream, at: converter.outputFrames, epoch: epoch)
		closeDecoder()
		track = nil
		finished = true
	}

	private func performSeek(to seconds: Double, in seekTrack: EngineTrack) {
		epoch = cog_ring_request_flush(ring)
		converter.reset()
		floatConverter.reset()
		writtenFormat = nil
		finished = false
		joins.removeAll()
		previousTrack = nil

		if seekTrack !== track || decoder == nil {
			closeDecoder()
			guard let opened = opener(seekTrack) else {
				delegate?.feeder(self, couldNotOpen: seekTrack)
				track = seekTrack
				advance()
				return
			}
			decoder = opened
			track = seekTrack
		}

		var offset = 0.0
		if seconds > 0, let decoder {
			let rate = (decoder.properties()["sampleRate"] as? NSNumber)?.doubleValue ?? 0
			if rate > 0, decoder.seek(Int(seconds * rate)) >= 0 {
				offset = seconds
			}
		}
		trackStartPending = (seekTrack, offset)
	}

	/// Abandons the earliest queued join the DSP thread has not reached, and
	/// decodes onward from the track before it again. The DSP thread decides,
	/// under the timeline's lock, whether it is still ahead of the join.
	private func performReset() {
		for (index, join) in joins.enumerated() {
			timeline.requestAbandon(from: join.frame, epoch: epoch)
			var accepted: Bool?
			let deadline = Date().addingTimeInterval(2)
			while accepted == nil && isRunning && Date() < deadline {
				Thread.sleep(forTimeInterval: 0.002)
				accepted = timeline.abandonAccepted()
			}
			guard accepted == true else { continue }

			EngineLog.logger.info("Abandoned the queued track at frame \(join.frame); asking again what follows \(join.before.url.lastPathComponent, privacy: .public)")
			joins.removeSubrange(index...)
			closeDecoder()
			floatConverter.reset()
			// A fresh run numbered from the join; the DSP thread discards what
			// was written after it before reading on.
			converter.reset(outputFrames: join.frame)
			writtenFormat = nil
			trackStartPending = nil
			finished = false
			track = join.before
			advance()
			return
		}
	}

	private func closeDecoder() {
		decoder?.close()
		decoder = nil
	}

	// MARK: - Conversion

	private func feed(_ chunk: AudioChunk) {
		floatConverter.add(chunk)
		while !floatConverter.isEmpty() {
			let floats = floatConverter.removeSamples(asFloat32: 4096)
			guard floats.frameCount() > 0 else { break }
			let asbd = floats.format
			let format = StreamFormat(sampleRate: asbd.mSampleRate, channels: Int(asbd.mChannelsPerFrame), channelConfig: floats.channelConfig)
			let frames = floats.frameCount()
			let data = floats.removeSamples(frames)

			if let pending = trackStartPending {
				placeTrackStart(pending.track, offset: pending.offset, before: format)
				trackStartPending = nil
			}

			// ReplayGain is applied on the DSP thread, where a change is heard
			// within the shallow ring instead of after the deep one.
			data.withUnsafeBytes { raw in
				let samples = raw.bindMemory(to: Float.self)
				converter.process(samples, format: format) { out, outFormat in
					write(out, format: outFormat)
				}
			}
			if hasPendingSeek || !isRunning { return }
		}
	}

	/// Marks where `track` becomes audible. Across a same-format boundary that
	/// is the exact output position of its first input frame; across a format
	/// change the old run is drained first and the track starts where it ends.
	private func placeTrackStart(_ track: EngineTrack, offset: Double, before format: StreamFormat) {
		if let current = converter.inputFormat, !current.resamplesContinuously(into: format) {
			converter.drain { samples, outFormat in write(samples, format: outFormat) }
		}
		let frame = converter.outputPositionOfNextInput
		if let before = previousTrack {
			joins.append((frame, before))
		}
		previousTrack = nil
		timeline.append(.trackStart(track, offset: offset), at: frame, epoch: epoch)
	}

	/// Writes output samples into the deep ring, waiting for room.
	private func write(_ samples: UnsafeBufferPointer<Float>, format: StreamFormat) {
		let frames = samples.count / format.channels
		if writtenFormat != format {
			// The converter has already counted these frames as emitted.
			timeline.append(.format(format), at: converter.outputFrames - UInt64(frames), epoch: epoch)
			writtenFormat = format
		}

		var offset = 0
		while offset < samples.count {
			let written = cog_ring_write(ring, samples.baseAddress! + offset, samples.count - offset)
			offset += written
			if offset < samples.count {
				if hasPendingSeek || !isRunning { return }
				let waitStart = EngineLog.now()
				_ = spaceAvailable.wait(timeout: .now() + .milliseconds(20))
				waitNanoseconds += EngineLog.now() - waitStart
			}
		}
	}

	// MARK: - Opening

	private static func openWithPlugins(_ track: EngineTrack) -> CogDecoder? {
		var url = track.url
		var source = AudioSource.audioSource(for: url)
		if source == nil || source?.open(url) != true {
			// As BufferChain does: an unreadable track becomes ten seconds of
			// silence rather than stopping playback.
			url = URL(string: "silence://10")!
			source = AudioSource.audioSource(for: url)
			guard source?.open(url) == true else { return nil }
		}
		guard let source, let decoder = AudioDecoder.audioDecoder(for: source), decoder.open(source) else {
			return nil
		}
		return decoder
	}
}
