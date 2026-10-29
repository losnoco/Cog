//
//  WaveformScanner.swift
//  CogAudio
//
//  Decodes a whole track apart from playback, folds it to mono and measures
//  each of the `Waveform`'s buckets, in segments at once where the decoder
//  can seek. Scans run one at a time; a cancelled one stops at its next
//  block.
//

import Accelerate
import Foundation

public final class WaveformScanner {
	// The seek bar waits on it, so not at utility priority, which playback
	// would starve.
	private static let queue = DispatchQueue(label: "org.cogx.cog.waveform", qos: .userInitiated)
	/// At most this many segments are read at once.
	private static let maximumSegments = 8
	/// Shorter tracks are not worth splitting.
	private static let shortestSplitSeconds = 20.0
	/// How often a scan in progress reports what it has so far.
	private static let progressInterval: TimeInterval = 0.1
	/// A track whose length is unknown is measured in blocks this long, and
	/// shrunk to the waveform's buckets at the end.
	private static let unknownLengthBlock = 1024
	/// And is read for no longer than this, as a looping format can play
	/// forever.
	private static let unknownLengthLimit: Double = 4 * 60 * 60

	public let url: URL
	/// Where a complete scan is kept, and read back instead of scanning.
	private let cacheFile: URL?
	private let cancelled = CancellationFlag()

	public init(url: URL, cacheFile: URL? = nil) {
		self.url = url
		self.cacheFile = cacheFile
	}

	/// Scans on the waveform queue. `progress` and then `completion` are
	/// called on the main queue, unless the scan is cancelled; `completion`
	/// gets nil if the track could not be read.
	public func start(progress: @escaping (Waveform) -> Void, completion: @escaping (Waveform?) -> Void) {
		Self.queue.async { [self] in
			guard !cancelled.isSet else { return }
			var result = cacheFile.flatMap(WaveformCache.read)
			if result == nil {
				result = scan { partial in
					DispatchQueue.main.async { [self] in
						if !cancelled.isSet { progress(partial) }
					}
				}
				if let result, let cacheFile, !cancelled.isSet {
					WaveformCache.write(result, to: cacheFile)
				}
			}
			DispatchQueue.main.async { [self] in
				if !cancelled.isSet { completion(result) }
			}
		}
	}

	public func cancel() {
		cancelled.set()
	}

	// MARK: - Decoding

	/// A decoder open on the track, with the source it reads.
	private struct Opened {
		let source: CogSource
		let decoder: CogDecoder
	}

	private func open() -> Opened? {
		var opened: Opened?
		_ = Self.call(url) {
			guard let source = AudioSource.audioSource(for: url), source.open(url) else { return }
			if let decoder = AudioDecoder.audioDecoder(for: source), decoder.open(source) {
				opened = Opened(source: source, decoder: decoder)
			}
		}
		return opened
	}

	private func close(_ opened: Opened) {
		_ = Self.call(url) { opened.decoder.close() }
		withExtendedLifetime(opened.source) {}
	}

	/// The whole track, read in segments at once where the decoder can seek
	/// to them (each segment by a decoder of its own), else from start to
	/// end. A synthesized format renders what comes before wherever it seeks
	/// to, so it is read from start to end.
	private func scan(progress: @escaping (Waveform) -> Void) -> Waveform? {
		guard url.isFileURL, let first = open() else { return nil }

		var properties: [AnyHashable: Any] = [:]
		_ = Self.call(url) { properties = first.decoder.properties() ?? [:] }
		let track = Track(rate: (properties["sampleRate"] as? NSNumber)?.doubleValue ?? 0,
		                  totalFrames: (properties["totalFrames"] as? NSNumber)?.intValue ?? 0,
		                  lossless: (properties["encoding"] as? String) == "lossless")
		let seekable = (properties["seekable"] as? NSNumber)?.boolValue ?? false
		let synthesized = (properties["encoding"] as? String) == "synthesized"
		let longEnough = track.rate > 0 && Double(track.totalFrames) >= track.rate * Self.shortestSplitSeconds
		let segments = seekable && !synthesized && longEnough ? min(max(ProcessInfo.processInfo.activeProcessorCount, 1), Self.maximumSegments) : 1

		let accumulator = Accumulator(progressive: track.totalFrames >= Waveform.bucketCount)
		let reporter = ProgressReporter(accumulator: accumulator, interval: Self.progressInterval, report: progress)

		guard segments > 1 else {
			read(first, track: track, from: 0, to: nil, into: accumulator, reporter: reporter)
			close(first)
			return cancelled.isSet ? nil : accumulator.finalWaveform()
		}

		let failed = CancellationFlag()
		DispatchQueue.concurrentPerform(iterations: segments) { segment in
			let start = track.totalFrames * segment / segments
			let end = segment == segments - 1 ? nil : track.totalFrames * (segment + 1) / segments
			guard let opened = segment == 0 ? first : open() else { return failed.set() }
			defer { close(opened) }
			var reached = 0
			if start > 0 {
				_ = Self.call(url) { reached = opened.decoder.seek(start) }
				// Short of the start would only overlap the segment before;
				// past it, or failed, would leave a gap.
				guard reached >= 0, reached <= start + Int(track.rate) / 10 else { return failed.set() }
			}
			read(opened, track: track, from: reached, to: end, into: accumulator, reporter: reporter)
		}
		guard !cancelled.isSet else { return nil }
		if failed.isSet {
			// A segment could not be read: start to end instead.
			guard let opened = open() else { return nil }
			accumulator.reset()
			read(opened, track: track, from: 0, to: nil, into: accumulator, reporter: reporter)
			close(opened)
			guard !cancelled.isSet else { return nil }
		}
		return accumulator.finalWaveform()
	}

	/// What the decoder said of the track.
	private struct Track {
		let rate: Double
		let totalFrames: Int
		let lossless: Bool
	}

	/// Decodes from frame `start` (as the decoder counts them) until frame
	/// `end`, or the end of the track, folding each block to mono into the
	/// accumulator.
	private func read(_ opened: Opened, track: Track, from start: Int, to end: Int?, into accumulator: Accumulator, reporter: ProgressReporter) {
		let decoder = opened.decoder
		let converter = ChunkList(maximumDuration: 1.0)
		var downmix: DownmixProcessor?
		var downmixFormat: AudioStreamBasicDescription?
		var mono: [Float] = []
		// In frames at the rate the blocks come out at: DSD comes out
		// decimated, so the decoder's frames are scaled to it.
		var position = 0.0
		var framesPerBucket = 0.0
		var stop = Double.infinity

		while !cancelled.isSet {
			var chunk: AudioChunk?
			guard Self.call(url, { chunk = autoreleasepool { decoder.readAudio() } }), let chunk, chunk.frameCount() > 0 else {
				return
			}
			chunk.lossless = track.lossless
			converter.add(chunk)
			while !converter.isEmpty(), !cancelled.isSet {
				let floats = converter.removeSamples(asFloat32: 4096)
				let frames = floats.frameCount()
				guard frames > 0 else { break }
				let format = floats.format
				let channels = Int(format.mChannelsPerFrame)
				let data = floats.removeSamples(frames)

				if framesPerBucket == 0 {
					let rate = format.mSampleRate
					let scale = track.rate > 0 ? rate / track.rate : 1
					position = Double(start) * scale
					if track.totalFrames > 0 {
						let total = Double(track.totalFrames) * scale
						framesPerBucket = accumulator.isProgressive ? total / Double(Waveform.bucketCount) : Double(Self.unknownLengthBlock)
						stop = end.map { Double($0) * scale } ?? total * 1.01 + rate
					} else {
						framesPerBucket = Double(Self.unknownLengthBlock)
						stop = Self.unknownLengthLimit * rate
					}
				}

				if mono.count < frames {
					mono = [Float](repeating: 0, count: frames)
				}
				if channels > 1 {
					if downmix == nil || downmixFormat.map({ !Self.sameFormat($0, format) }) ?? true {
						let config = floats.channelConfig != 0 ? floats.channelConfig : AudioChunk.guessChannelConfig(UInt32(channels))
						downmix = DownmixProcessor(inputFormat: Self.floatFormat(rate: format.mSampleRate, channels: channels), inputConfig: config,
						                           andOutputFormat: Self.floatFormat(rate: format.mSampleRate, channels: 1), outputConfig: UInt32(AudioChannelFrontCenter))
						downmixFormat = format
					}
					data.withUnsafeBytes { input in
						mono.withUnsafeMutableBufferPointer { output in
							downmix?.process(input.baseAddress!, frameCount: frames, output: output.baseAddress!)
						}
					}
				} else {
					data.withUnsafeBytes { input in
						mono.withUnsafeMutableBytes { output in
							output.copyMemory(from: UnsafeRawBufferPointer(rebasing: input.prefix(frames * MemoryLayout<Float>.size)))
						}
					}
				}

				// Not past the segment's end, which the next segment reads.
				let counted = min(frames, max(0, Int((stop - position).rounded(.up))))
				mono.withUnsafeBufferPointer { accumulator.add(UnsafeBufferPointer(rebasing: $0.prefix(counted)), at: Int(position), framesPerBucket: framesPerBucket) }
				position += Double(frames)
				reporter.tick()
				if position >= stop { return }
			}
		}
	}

	/// Runs a call into the plugins, which may raise an Objective-C
	/// exception; false if one did.
	private static func call(_ url: URL, _ body: () -> Void) -> Bool {
		if let exception = CogCatchException(body) {
			NSLog("A plugin raised %@ scanning %@: %@", exception.name.rawValue, url.absoluteString, exception.reason ?? "")
			return false
		}
		return true
	}

	private static func sameFormat(_ a: AudioStreamBasicDescription, _ b: AudioStreamBasicDescription) -> Bool {
		a.mSampleRate == b.mSampleRate && a.mChannelsPerFrame == b.mChannelsPerFrame
	}

	private static func floatFormat(rate: Double, channels: Int) -> AudioStreamBasicDescription {
		let bytesPerFrame = UInt32(MemoryLayout<Float>.size * channels)
		return AudioStreamBasicDescription(mSampleRate: rate, mFormatID: kAudioFormatLinearPCM,
		                                   mFormatFlags: kAudioFormatFlagsNativeFloatPacked,
		                                   mBytesPerPacket: bytesPerFrame, mFramesPerPacket: 1,
		                                   mBytesPerFrame: bytesPerFrame, mChannelsPerFrame: UInt32(channels),
		                                   mBitsPerChannel: 32, mReserved: 0)
	}
}

// MARK: - Measuring

/// Gathers each bucket's extremes and power, from any thread. With the
/// length known, the buckets are the waveform's own; without it, short
/// blocks that are shrunk to fit at the end.
private final class Accumulator: @unchecked Sendable {
	let isProgressive: Bool
	private let lock = NSLock()
	private var minimum: [Float] = []
	private var maximum: [Float] = []
	private var power: [Double] = []
	private var counts: [Int] = []

	init(progressive: Bool) {
		isProgressive = progressive
		reset()
	}

	func reset() {
		lock.lock()
		defer { lock.unlock() }
		let count = isProgressive ? Waveform.bucketCount : 0
		minimum = [Float](repeating: 0, count: count)
		maximum = minimum
		power = [Double](repeating: 0, count: count)
		counts = [Int](repeating: 0, count: count)
	}

	/// Adds mono samples that begin `position` frames into the track.
	func add(_ samples: UnsafeBufferPointer<Float>, at position: Int, framesPerBucket: Double) {
		guard let base = samples.baseAddress, !samples.isEmpty else { return }
		lock.lock()
		defer { lock.unlock() }
		var offset = 0
		while offset < samples.count {
			let here = position + offset
			var bucket = Int(Double(here) / framesPerBucket)
			let bucketEnd: Int
			if isProgressive {
				bucket = min(bucket, Waveform.bucketCount - 1)
				bucketEnd = bucket == Waveform.bucketCount - 1 ? Int.max : Int((Double(bucket + 1) * framesPerBucket).rounded(.up))
			} else {
				while bucket >= minimum.count {
					minimum.append(0)
					maximum.append(0)
					power.append(0)
					counts.append(0)
				}
				bucketEnd = Int((Double(bucket + 1) * framesPerBucket).rounded(.up))
			}
			let length = max(1, min(samples.count - offset, bucketEnd - here))
			var low: Float = 0
			var high: Float = 0
			var squares: Float = 0
			vDSP_minv(base + offset, 1, &low, vDSP_Length(length))
			vDSP_maxv(base + offset, 1, &high, vDSP_Length(length))
			vDSP_svesq(base + offset, 1, &squares, vDSP_Length(length))
			minimum[bucket] = min(minimum[bucket], low)
			maximum[bucket] = max(maximum[bucket], high)
			power[bucket] += Double(squares)
			counts[bucket] += length
			offset += length
		}
	}

	/// The complete waveform, or nil if nothing was read: a decoder that
	/// failed at once has nothing to show, and should not be cached as a
	/// flat line.
	func finalWaveform() -> Waveform? {
		lock.lock()
		let read = counts.contains { $0 > 0 }
		lock.unlock()
		return read ? waveform(final: true) : nil
	}

	/// What has been gathered. Final, buckets nothing reached (where a
	/// segment fell a little short, or the track did) take their
	/// neighbour's values, so the waveform is whole.
	func waveform(final: Bool) -> Waveform {
		lock.lock()
		defer { lock.unlock() }
		var waveform = Waveform()
		let n = minimum.count
		guard n > 0 else { return waveform }
		if isProgressive {
			for i in 0..<n where counts[i] > 0 {
				waveform.minimum[i] = minimum[i]
				waveform.maximum[i] = maximum[i]
				waveform.rms[i] = Float((power[i] / Double(counts[i])).squareRoot())
				waveform.scanned[i] = true
			}
			if final, let firstScanned = waveform.scanned.firstIndex(of: true) {
				var source = firstScanned
				for i in 0..<n {
					if waveform.scanned[i] {
						source = i
					} else {
						waveform.minimum[i] = waveform.minimum[source]
						waveform.maximum[i] = waveform.maximum[source]
						waveform.rms[i] = waveform.rms[source]
						waveform.scanned[i] = true
					}
				}
			}
			return waveform
		}
		guard final else { return waveform }
		// Shrink the blocks to the buckets, or stretch them if there are
		// fewer than buckets.
		for i in 0..<Waveform.bucketCount {
			let first = i * n / Waveform.bucketCount
			let last = max(first + 1, (i + 1) * n / Waveform.bucketCount)
			var low: Float = 0
			var high: Float = 0
			var sum: Double = 0
			var frames = 0
			for block in first..<min(last, n) {
				low = min(low, minimum[block])
				high = max(high, maximum[block])
				sum += power[block]
				frames += counts[block]
			}
			waveform.minimum[i] = low
			waveform.maximum[i] = high
			waveform.rms[i] = frames > 0 ? Float((sum / Double(frames)).squareRoot()) : 0
			waveform.scanned[i] = true
		}
		return waveform
	}
}

/// Reports what the accumulator has every so often, from whichever segment
/// gets there first.
private final class ProgressReporter: @unchecked Sendable {
	private let accumulator: Accumulator
	private let interval: TimeInterval
	private let report: (Waveform) -> Void
	private let lock = NSLock()
	private var last = Date()

	init(accumulator: Accumulator, interval: TimeInterval, report: @escaping (Waveform) -> Void) {
		self.accumulator = accumulator
		self.interval = interval
		self.report = report
	}

	func tick() {
		guard accumulator.isProgressive, lock.try() else { return }
		let now = Date()
		let due = now.timeIntervalSince(last) >= interval
		if due { last = now }
		lock.unlock()
		if due { report(accumulator.waveform(final: false)) }
	}
}

/// A flag set from one thread and read on another.
private final class CancellationFlag: @unchecked Sendable {
	private let lock = NSLock()
	private var value = false

	var isSet: Bool {
		lock.lock()
		defer { lock.unlock() }
		return value
	}

	func set() {
		lock.lock()
		value = true
		lock.unlock()
	}
}
