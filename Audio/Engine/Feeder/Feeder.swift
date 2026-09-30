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

	/// `track` was opened for decoding; `isSilence` if its file could not be
	/// read and ten seconds of silence stand in for it, as BufferChain did.
	func feeder(_ feeder: Feeder, opened track: EngineTrack, isSilence: Bool)
}

public extension FeederDelegate {
	func feeder(_ feeder: Feeder, opened track: EngineTrack, isSilence: Bool) {}
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

	/// Whether a next track can join this stream; called on the feeder
	/// thread with its opened decoder. One that cannot ends the stream
	/// there and waits in `takeHandoff()`. Nil admits everything.
	public var admits: ((CogDecoder) -> Bool)?

	private let opener: Opener
	private let converter: StreamConverter
	private let floatConverter = ChunkList(maximumDuration: 1.0)
	private let spaceAvailable = DispatchSemaphore(value: 0)
	private let idle = DispatchSemaphore(value: 0)
	private let lock = UnfairLock()

	// Shared with control threads, under `lock`.
	private var running = false
	private var pendingSeek: (track: EngineTrack, seconds: Double, decoder: CogDecoder?)?
	/// A next track this pipeline cannot play (it needs another DoP
	/// carrier), opened and waiting for the engine to rebuild for it.
	private var handoff: (track: EngineTrack, decoder: CogDecoder)?
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
			decoderIsLossless = (current?.properties()?["encoding"] as? String) == "lossless"
			metadataObserver = current.map { MetadataObserver(watching: $0) }
		}
	}
	/// Notices the decoder's metadata changing (from whichever thread).
	private var metadataObserver: MetadataObserver?
	/// As InputNode marked every chunk: ChunkList only looks for HDCD in
	/// lossless audio.
	private var decoderIsLossless = false
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
	///   - dsdAsDoP: pack DSD as DSD over PCM at a sixteenth of its rate,
	///     for a DoP carrier pipeline, instead of converting it to PCM.
	public init?(outputRate: Double, seconds: Double = 2.0, opener: Opener? = nil, dsdAsDoP: Bool = false) {
		let samples = Int(outputRate * seconds) * Self.maximumChannels
		guard let ring = cog_ring_create(samples, 1) else { return nil }
		self.ring = ring
		self.outputRate = outputRate
		self.opener = opener ?? Self.openWithPlugins
		converter = StreamConverter(outputRate: outputRate)
		floatConverter.setOutputDSDAsDoP(dsdAsDoP)
	}

	/// Cog's plugin lookup, as the default opener.
	public static var defaultOpener: Opener { openWithPlugins }

	deinit {
		stop()
		cog_ring_destroy(ring)
	}

	// MARK: - Control

	/// Starts decoding `track` from `offset` seconds, with `decoder` if it
	/// has already been opened.
	public func start(with track: EngineTrack, offset: Double = 0, decoder: CogDecoder? = nil) {
		lock.withLock {
			guard !running else { return }
			running = true
			pendingSeek = (track, offset, decoder)
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
		takeHandoff()?.decoder.close()
		lock.withLock { pendingSeek?.decoder?.close() }
	}

	/// The track waiting for a pipeline built for it, once the stream before
	/// it has ended.
	public func takeHandoff() -> (track: EngineTrack, decoder: CogDecoder)? {
		lock.withLock {
			defer { handoff = nil }
			return handoff
		}
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
	public func seek(to seconds: Double, in track: EngineTrack, decoder: CogDecoder? = nil) {
		lock.withLock {
			pendingSeek?.decoder?.close()
			pendingSeek = (track, seconds, decoder)
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

	private func takePendingSeek() -> (track: EngineTrack, seconds: Double, decoder: CogDecoder?)? {
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
				performSeek(to: seek.seconds, in: seek.track, with: seek.decoder)
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
		if let next = candidate, let decoder, Self.sameFile(finishedTrack.url, next.url), decoder.setTrack?(next.url) == true {
			// Another track of the same file (a cue sheet's, say): the decoder
			// carries on from where the last one ended, with no reopening or
			// seeking, as AudioPlayer arranged with `setTrack:`.
			track = next
			trackStartPending = (next, 0)
			previousTrack = finishedTrack
			announceOpened()
			return
		}
		var attempts = 0
		while let next = candidate, isRunning {
			if let nextDecoder = opener(next) {
				if let admits, !admits(nextDecoder) {
					// It needs a pipeline of its own: end this stream here.
					EngineLog.logger.info("\(next.url.lastPathComponent, privacy: .public) needs another output format; ending the stream before it")
					lock.withLock { handoff = (next, nextDecoder) }
					finish()
					return
				}
				closeDecoder()
				decoder = nextDecoder
				track = next
				// Placed when the new track's first frames arrive, once it is
				// known whether the resampler carries straight across.
				trackStartPending = (next, 0)
				previousTrack = finishedTrack
				announceOpened()
				return
			}
			delegate?.feeder(self, couldNotOpen: next)
			attempts += 1
			candidate = attempts < 16 ? delegate?.feeder(self, nextTrackAfter: next) : nil
		}
		finish()
	}

	/// Whether two track URLs name the same file, fragments (cue sheet track
	/// numbers) aside, as AudioPlayer compared them.
	static func sameFile(_ a: URL, _ b: URL) -> Bool {
		a.scheme == b.scheme && a.host == b.host && a.path == b.path && !a.path.isEmpty
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

	private func performSeek(to seconds: Double, in seekTrack: EngineTrack, with opened: CogDecoder?) {
		dropHandoff()
		epoch = cog_ring_request_flush(ring)
		converter.reset()
		floatConverter.reset()
		writtenFormat = nil
		finished = false
		joins.removeAll()
		previousTrack = nil

		// A decoder just opened is already at the start; one that has been
		// read from must be told, even to go back to 0.
		var fresh = false
		if let opened {
			closeDecoder()
			track = seekTrack
			decoder = opened
			fresh = true
			announceOpened()
		} else if seekTrack !== track || decoder == nil {
			guard reopen(seekTrack) else { return }
			fresh = true
			announceOpened()
		}

		var offset = 0.0
		if (seconds > 0 || !fresh), let decoder {
			let rate = (decoder.properties()["sampleRate"] as? NSNumber)?.doubleValue ?? 0
			if rate > 0, decoder.seek(Int(seconds * rate)) >= 0 {
				offset = seconds
			} else if !fresh {
				// Refused: start the track over rather than carry on from
				// wherever the decoder was while claiming the start.
				guard reopen(seekTrack) else { return }
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
			dropHandoff()
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

	/// Opens `seekTrack` afresh, or moves on past it if it cannot be opened.
	private func reopen(_ seekTrack: EngineTrack) -> Bool {
		closeDecoder()
		track = seekTrack
		guard let opened = opener(seekTrack) else {
			delegate?.feeder(self, couldNotOpen: seekTrack)
			advance()
			return false
		}
		decoder = opened
		return true
	}

	/// Tells the delegate the current track is open, and whether it is the
	/// silence standing in for an unreadable file (an error to flag), or
	/// real audio (clearing any error flagged on it before).
	private func announceOpened() {
		guard let track else { return }
		delegate?.feeder(self, opened: track, isSilence: decoder?.isSilence?() ?? false)
	}

	private func dropHandoff() {
		takeHandoff()?.decoder.close()
	}

	private func closeDecoder() {
		decoder?.close()
		decoder = nil
	}

	// MARK: - Conversion

	private func feed(_ chunk: AudioChunk) {
		chunk.lossless = decoderIsLossless
		if let decoder, let track, metadataObserver?.takeChange() == true {
			// Heard from the audio decoded after the change.
			var info = decoder.properties() ?? [:]
			for (key, value) in decoder.metadata() ?? [:] {
				info[key] = value
			}
			timeline.append(.info(info, track), at: converter.outputPositionOfNextInput, epoch: epoch)
		}
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
		EngineLog.logger.notice("Opened \(url.lastPathComponent, privacy: .public) with \(Self.decoderName(decoder), privacy: .public)")
		return decoder
	}

	/// The plugin doing the decoding, looking inside the wrapper that tries
	/// several in turn.
	private static func decoderName(_ decoder: CogDecoder) -> String {
		let outer = String(describing: type(of: decoder))
		// Only the wrappers have an inner decoder; asking anything else
		// would throw.
		let key: String
		switch outer {
		case "CogDecoderMulti": key = "theDecoder"
		case "CueSheetDecoder": key = "decoder"
		default: return outer
		}
		guard let inner = (decoder as? NSObject)?.value(forKey: key) as? CogDecoder else { return outer }
		return "\(decoderName(inner)) via \(outer)"
	}
}

/// Watches a decoder's `metadata` through KVO, as InputNode did. Decoders
/// may announce a change from any thread, so the change is only noted, and
/// the feeder picks it up between chunks.
private final class MetadataObserver: NSObject {
	private let decoder: NSObject?
	private let changed = LockedValue(false)
	private static var context = 0

	init(watching decoder: CogDecoder) {
		self.decoder = decoder as? NSObject
		super.init()
		self.decoder?.addObserver(self, forKeyPath: "metadata", options: [], context: &Self.context)
	}

	deinit {
		decoder?.removeObserver(self, forKeyPath: "metadata", context: &Self.context)
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard context == &Self.context else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}
		changed.withLock { $0 = true }
	}

	/// Whether the metadata changed since the last call.
	func takeChange() -> Bool {
		changed.withLock { value in
			defer { value = false }
			return value
		}
	}
}
