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
	private var downmix: DownmixProcessor?
	private var readBuffer: [Float] = []
	private var fittedBuffer: [Float] = []

	/// Frames read and written per pass.
	static let blockFrames = 4096

	/// - Parameters:
	///   - feeder: the source of the deep ring and its timeline.
	///   - outputFormat: the device's render format.
	///   - seconds: shallow ring length; this is the delay before a DSP
	///     setting change is heard, so it is kept short.
	public init?(feeder: Feeder, outputFormat: StreamFormat, seconds: Double = 0.2) {
		guard outputFormat.sampleRate == feeder.outputRate,
		      let ring = cog_ring_create(max(Int(outputFormat.sampleRate * seconds), Self.blockFrames * 2), UInt32(outputFormat.channels)) else {
			return nil
		}
		self.ring = ring
		self.feeder = feeder
		self.outputFormat = outputFormat
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
		}

		var endOfStream = false
		for entry in feeder.timeline.take(through: frames, epoch: epoch) {
			switch entry.event {
			case let .format(format):
				configure(for: format)
			case let .trackStart(track, offset):
				presentation.append(.trackStart(track, offset: offset), at: cog_ring_write_position(ring))
			case .endOfStream:
				endOfStream = true
			}
		}
		if endOfStream {
			presentation.append(.endOfStream, at: cog_ring_write_position(ring))
		}

		guard let format = inputFormat else { return false }
		let channels = format.channels

		var count = min(cog_ring_readable(deep) / channels, cog_ring_writable(ring), Self.blockFrames)
		if let next = feeder.timeline.nextFrame(epoch: epoch) {
			count = min(count, Int(next - frames))
		}
		guard count > 0 else { return false }

		readBuffer.removeAll(keepingCapacity: true)
		readBuffer.append(contentsOf: repeatElement(0, count: count * channels))
		let got = readBuffer.withUnsafeMutableBufferPointer { cog_ring_read(deep, $0.baseAddress, count * channels) } / channels
		frames += UInt64(got)
		feeder.consumerDidRead()

		emit(got)
		return true
	}

	private func configure(for format: StreamFormat) {
		inputFormat = format
		let inputConfig = format.channelConfig != 0 ? format.channelConfig : AudioChunk.guessChannelConfig(UInt32(format.channels))
		if format.channels == outputFormat.channels && inputConfig == outputFormat.channelConfig {
			downmix = nil
		} else {
			downmix = DownmixProcessor(inputFormat: Self.asbd(format), inputConfig: inputConfig,
			                           andOutputFormat: Self.asbd(outputFormat), outputConfig: outputFormat.channelConfig)
		}
	}

	/// Fits `frames` frames of `readBuffer` to the device's channel layout
	/// and writes them to the shallow ring, waiting for room.
	private func emit(_ frames: Int) {
		guard let downmix else {
			readBuffer.withUnsafeBufferPointer { write($0.baseAddress!, frames: frames) }
			return
		}
		fittedBuffer.removeAll(keepingCapacity: true)
		fittedBuffer.append(contentsOf: repeatElement(0, count: frames * outputFormat.channels))
		readBuffer.withUnsafeBufferPointer { input in
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
