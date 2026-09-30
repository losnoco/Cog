//
//  Pump.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import Foundation

/// Something the listener should learn about when it is heard.
public enum PresentationEvent {
	/// `track` becomes audible, `offset` seconds into it.
	case trackStart(EngineTrack, offset: Double)
	/// Everything queued has been played.
	case endOfStream
}

/// Events placed at absolute positions in the shallow ring. The shallow
/// ring's positions never reset (a flush only moves the read position
/// forward), so these need no epoch: whoever watches playback pops them once
/// the renderer has read past them and the device latency has elapsed.
public final class PresentationQueue {
	private let lock = UnfairLock()
	private var entries: [(position: UInt64, event: PresentationEvent)] = []

	public init() {}

	func append(_ event: PresentationEvent, at position: UInt64) {
		lock.withLock { entries.append((position, event)) }
	}

	/// Removes and returns the events at or before `position`, in order.
	public func take(through position: UInt64) -> [(position: UInt64, event: PresentationEvent)] {
		lock.withLock {
			let due = entries.prefix { $0.position <= position }
			entries.removeFirst(due.count)
			return Array(due)
		}
	}

	/// Drops events before `position`: their audio was flushed unheard.
	func discard(before position: UInt64) {
		lock.withLock { entries.removeAll { $0.position < position } }
	}
}

/// The engine's second worker thread. It reads the deep ring frame-accurately
/// against the feeder's timeline, runs the DSP chain, fits the channels to
/// the device, and writes the shallow ring the renderer plays from, carrying
/// track starts and the end of the stream across as presentation events.
///
/// For now the chain is empty: this is the skeleton stage 5 fills in.
public final class Pump {
	/// Shallow ring: interleaved frames in the device format.
	public let ring: OpaquePointer
	public let presentation = PresentationQueue()
	public let outputFormat: StreamFormat

	private let feeder: Feeder

	/// The deep ring this pump reads (for diagnostics).
	var feederRing: OpaquePointer { feeder.ring }
	private let lock = UnfairLock()
	private var running = false
	private var threadExited = DispatchSemaphore(value: 0)

	// Pump thread only.
	private var epoch: UInt64 = 0
	private var frames: UInt64 = 0
	private var inputFormat: StreamFormat?
	private let block = DSPBuffer()

	/// The DSP chain, in order. Each is skipped while inactive.
	private let stages: [DSPStage]
	/// The active stages and input format the chain was last configured for.
	private var configuredChain: (stages: [ObjectIdentifier], input: StreamFormat)?

	/// Channel fitting from the chain's output to the device.
	private var downmix: DownmixProcessor?
	private var fitSource: StreamFormat?

	/// The track whose frames are being read, for its ReplayGain.
	private var gainTrack: EngineTrack?
	/// The gain last applied, ramped toward the track's current gain.
	private var appliedGain: Float = 1
	private var rampTarget: Float = 1
	private var rampStep: Float = 0
	/// Gain changes within a track take this long, so they do not click.
	static let gainRampSeconds = 0.02
	private var fittedBuffer: [Float] = []

	/// Frames read and written per pass.
	static let blockFrames = 4096

	/// - Parameters:
	///   - feeder: the source of the deep ring and its timeline.
	///   - outputFormat: the device's render format.
	///   - seconds: shallow ring length; this is the delay before a DSP
	///     setting change is heard, so it is kept short.
	///   - stages: the DSP chain, run in order on every block.
	public init?(feeder: Feeder, outputFormat: StreamFormat, stages: [DSPStage] = [], seconds: Double = 0.2) {
		guard outputFormat.sampleRate == feeder.outputRate,
		      let ring = cog_ring_create(max(Int(outputFormat.sampleRate * seconds), Self.blockFrames * 2), UInt32(outputFormat.channels)) else {
			return nil
		}
		self.ring = ring
		self.feeder = feeder
		self.outputFormat = outputFormat
		self.stages = stages
	}

	deinit {
		stop()
		cog_ring_destroy(ring)
	}

	public func start() {
		let started: Bool = lock.withLock {
			guard !running else { return false }
			running = true
			threadExited = DispatchSemaphore(value: 0)
			return true
		}
		guard started else { return }
		let thread = Thread { [weak self] in
			self?.run()
		}
		thread.name = "Cog DSP"
		thread.qualityOfService = .userInteractive
		thread.start()
	}

	public func stop() {
		let exited: DispatchSemaphore? = lock.withLock {
			guard running else { return nil }
			running = false
			return threadExited
		}
		exited?.wait()
	}

	private var isRunning: Bool {
		lock.withLock { running }
	}

	// MARK: - Thread

	private func run() {
		defer { lock.withLock { threadExited }.signal() }
		var last = EngineLog.now()
		while isRunning {
			let start = EngineLog.now()
			let gap = EngineLog.milliseconds(since: last)
			let worked = pass()
			let took = EngineLog.milliseconds(since: start)
			if gap > EngineLog.slowPass * 1000 || took > EngineLog.slowPass * 1000 {
				let shallowMs = Double(cog_ring_readable(ring)) / outputFormat.sampleRate * 1000
				EngineLog.logger.warning("DSP thread slow: \(gap, format: .fixed(precision: 1)) ms since last pass, pass took \(took, format: .fixed(precision: 1)) ms, shallow ring \(shallowMs, format: .fixed(precision: 1)) ms")
			}
			last = EngineLog.now()
			if !worked {
				// Nothing to read, or no room to write: the renderer drains the
				// shallow ring at the device rate, so a short nap suffices.
				Thread.sleep(forTimeInterval: 0.002)
			}
		}
	}

	/// Moves at most one block from the deep ring to the shallow ring.
	/// Returns false when there was nothing to do.
	private func pass() -> Bool {
		let deep = feeder.ring
		cog_ring_honour_flush(deep)
		let acknowledged = cog_ring_flush_acknowledged(deep)
		if acknowledged != epoch {
			// The feeder seeked. Frames count from zero in its new epoch, and
			// anything still queued for the device is stale.
			epoch = acknowledged
			frames = 0
			presentation.discard(before: cog_ring_write_position(ring))
			cog_ring_request_flush(ring)
			// Filter history belongs to audio that will never be heard.
			for stage in stages {
				stage.reset()
			}
		}

		var endOfStream = false
		for entry in feeder.timeline.take(through: frames, epoch: epoch) {
			switch entry.event {
			case let .format(format):
				// Stages holding audio back (FreeSurround's block) give it up
				// before the chain is reconfigured for the new format.
				drainChain()
				configure(for: format)
			case let .trackStart(track, offset):
				presentation.append(.trackStart(track, offset: offset), at: cog_ring_write_position(ring))
				// A new track's gain starts exactly on its first frame.
				gainTrack = track
				appliedGain = track.gain
				rampTarget = appliedGain
				EngineLog.logger.info("Track start: \(track.url.lastPathComponent, privacy: .public) at \(offset, format: .fixed(precision: 2)) s, gain \(track.gain, format: .fixed(precision: 4))")
			case .endOfStream:
				endOfStream = true
			}
		}
		if endOfStream {
			drainChain()
			presentation.append(.endOfStream, at: cog_ring_write_position(ring))
		}

		guard let format = inputFormat else { return false }
		let channels = format.channels

		var count = min(cog_ring_readable(deep) / channels, cog_ring_writable(ring), Self.blockFrames)
		if let next = feeder.timeline.nextFrame(epoch: epoch) {
			count = min(count, Int(next - frames))
		}
		guard count > 0 else { return false }

		block.resize(frames: count, format: format)
		let got = block.samples.withUnsafeMutableBufferPointer { cog_ring_read(deep, $0.baseAddress, count * channels) } / channels
		block.frames = got
		frames += UInt64(got)
		feeder.consumerDidRead()

		applyGain(frames: got, channels: channels)
		runStages()
		emit()
		return true
	}

	/// Pushes out whatever the configured chain still holds, as at the end of
	/// the stream or before a format change.
	private func drainChain() {
		guard let chain = configuredChain else { return }
		block.resize(frames: 0, format: chain.input)
		for stage in stages where chain.stages.contains(ObjectIdentifier(stage)) {
			stage.drain(block)
		}
		if block.frames > 0 {
			emit()
		}
	}

	/// Runs the active stages over `block`, reconfiguring the chain when the
	/// input format or the set of active stages changes.
	private func runStages() {
		let active = stages.filter(\.isActive)
		let identifiers = active.map { ObjectIdentifier($0) }
		if configuredChain?.stages != identifiers || configuredChain?.input != block.format {
			var format = block.format
			for stage in active {
				format = stage.configure(input: format)
			}
			configuredChain = (identifiers, block.format)
		}
		for stage in active {
			stage.process(block)
		}
	}

	/// Scales the block in `readBuffer` by the current track's gain. A
	/// change within a track ramps linearly over `gainRampSeconds`, however
	/// many blocks that spans.
	private func applyGain(frames: Int, channels: Int) {
		let target = gainTrack?.gain ?? 1
		if target != rampTarget {
			EngineLog.logger.info("Track gain changed: \(self.appliedGain, format: .fixed(precision: 4)) -> \(target, format: .fixed(precision: 4))")
			rampTarget = target
			let rampFrames = max(1, outputFormat.sampleRate * Self.gainRampSeconds)
			rampStep = (target - appliedGain) / Float(rampFrames)
		}
		if appliedGain == 1 && target == 1 { return }

		var level = appliedGain
		block.samples.withUnsafeMutableBufferPointer { samples in
			for frame in 0..<frames {
				if level != target {
					level += rampStep
					if (rampStep > 0 && level > target) || (rampStep < 0 && level < target) || rampStep == 0 {
						level = target
					}
				}
				let base = frame * channels
				for channel in 0..<channels {
					samples[base + channel] *= level
				}
			}
		}
		appliedGain = level
		publishedGain.withLock { $0 = level }
	}

	/// The track gain as last applied, for diagnostics.
	private let publishedGain = LockedValue<Float>(1)
	var currentTrackGain: Float { publishedGain.withLock { $0 } }

	private func configure(for format: StreamFormat) {
		inputFormat = format
	}

	/// Sets up channel fitting from `format` (the chain's output) to the
	/// device's layout.
	private func configureFit(from format: StreamFormat) {
		fitSource = format
		let inputConfig = format.channelConfig != 0 ? format.channelConfig : AudioChunk.guessChannelConfig(UInt32(format.channels))
		if format.channels == outputFormat.channels && inputConfig == outputFormat.channelConfig {
			downmix = nil
		} else {
			downmix = DownmixProcessor(inputFormat: Self.asbd(format), inputConfig: inputConfig,
			                           andOutputFormat: Self.asbd(outputFormat), outputConfig: outputFormat.channelConfig)
		}
	}

	/// Fits `block` to the device's channel layout and writes it to the
	/// shallow ring, waiting for room.
	private func emit() {
		if block.format != fitSource {
			configureFit(from: block.format)
		}
		let frames = block.frames
		guard let downmix else {
			block.samples.withUnsafeBufferPointer { write($0.baseAddress!, frames: frames) }
			return
		}
		fittedBuffer.removeAll(keepingCapacity: true)
		fittedBuffer.append(contentsOf: repeatElement(0, count: frames * outputFormat.channels))
		block.samples.withUnsafeBufferPointer { input in
			fittedBuffer.withUnsafeMutableBufferPointer { output in
				downmix.process(input.baseAddress!, frameCount: frames, output: output.baseAddress!)
			}
		}
		fittedBuffer.withUnsafeBufferPointer { write($0.baseAddress!, frames: frames) }
	}

	private func write(_ samples: UnsafePointer<Float>, frames: Int) {
		var written = 0
		while written < frames {
			written += cog_ring_write(ring, samples + written * outputFormat.channels, frames - written)
			if written < frames {
				if !isRunning { return }
				Thread.sleep(forTimeInterval: 0.002)
			}
		}
	}

	static func asbd(_ format: StreamFormat) -> AudioStreamBasicDescription {
		let bytesPerFrame = UInt32(MemoryLayout<Float>.size * format.channels)
		return AudioStreamBasicDescription(mSampleRate: format.sampleRate, mFormatID: kAudioFormatLinearPCM,
		                                   mFormatFlags: kAudioFormatFlagsNativeFloatPacked,
		                                   mBytesPerPacket: bytesPerFrame, mFramesPerPacket: 1,
		                                   mBytesPerFrame: bytesPerFrame, mChannelsPerFrame: UInt32(format.channels),
		                                   mBitsPerChannel: 32, mReserved: 0)
	}
}
