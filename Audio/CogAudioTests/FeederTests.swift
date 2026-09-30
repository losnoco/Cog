//
//  FeederTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import CogAudio
import XCTest

/// A decoder plugin that plays back interleaved float samples from memory in
/// uneven chunks, as real decoders do.
final class MemoryDecoder: NSObject, CogDecoder {
	let samples: [Float]
	let sampleRate: Double
	let channels: Int
	private var position = 0
	private var chunkIndex = 0
	private let chunkSizes = [1152, 17, 4096, 576, 2048]

	init(samples: [Float], sampleRate: Double, channels: Int) {
		self.samples = samples
		self.sampleRate = sampleRate
		self.channels = channels
	}

	static func mimeTypes() -> [Any]! { [] }
	static func fileTypes() -> [Any]! { [] }
	static func fileTypeAssociations() -> [Any]! { [] }
	static func priority() -> Float { 1 }

	func properties() -> [AnyHashable: Any]! {
		["sampleRate": sampleRate, "channels": channels, "bitsPerSample": 32, "floatingPoint": true,
		 "totalFrames": samples.count / channels, "seekable": true, "encoding": "lossless"]
	}

	func metadata() -> [AnyHashable: Any]! { [:] }

	func readAudio() -> AudioChunk! {
		let chunk = AudioChunk()
		let frames = samples.count / channels
		guard position < frames else { return chunk }
		let count = min(chunkSizes[chunkIndex % chunkSizes.count], frames - position)
		chunkIndex += 1
		let bytesPerFrame = UInt32(MemoryLayout<Float>.size * channels)
		chunk.format = AudioStreamBasicDescription(mSampleRate: sampleRate, mFormatID: kAudioFormatLinearPCM,
		                                           mFormatFlags: kAudioFormatFlagsNativeFloatPacked,
		                                           mBytesPerPacket: bytesPerFrame, mFramesPerPacket: 1,
		                                           mBytesPerFrame: bytesPerFrame, mChannelsPerFrame: UInt32(channels),
		                                           mBitsPerChannel: 32, mReserved: 0)
		samples.withUnsafeBufferPointer { buffer in
			chunk.assignSamples(buffer.baseAddress! + position * channels, frameCount: count)
		}
		position += count
		return chunk
	}

	func open(_ source: CogSource!) -> Bool { true }

	func seek(_ frame: Int) -> Int {
		position = min(max(0, frame), samples.count / channels)
		return position
	}

	func close() {}
}

/// Hands out tracks from a list and records what the feeder reports.
final class ScriptedTracks: FeederDelegate {
	var queue: [EngineTrack]
	var unopenable: [EngineTrack] = []
	private let lock = NSLock()

	init(_ queue: [EngineTrack]) {
		self.queue = queue
	}

	func feeder(_ feeder: Feeder, nextTrackAfter track: EngineTrack) -> EngineTrack? {
		lock.lock()
		defer { lock.unlock() }
		return queue.isEmpty ? nil : queue.removeFirst()
	}

	func feeder(_ feeder: Feeder, couldNotOpen track: EngineTrack) {
		lock.lock()
		unopenable.append(track)
		lock.unlock()
	}
}

final class FeederTests: XCTestCase {
	/// What the DSP thread will do: read the deep ring frame-accurately,
	/// applying each timeline entry exactly at its frame.
	private struct Consumer {
		var samples: [Float] = []
		var entries: [TimelineEntry] = []
		var channels = 0
		var frames: UInt64 = 0
		var epoch: UInt64 = 0
		var ended = false

		/// Reads whatever is available; returns false once the stream ended.
		mutating func poll(_ feeder: Feeder) -> Bool {
			let ring = feeder.ring
			cog_ring_honour_flush(ring)
			let acknowledged = cog_ring_flush_acknowledged(ring)
			if acknowledged != epoch {
				// A seek: frame counting restarts in the new epoch.
				epoch = acknowledged
				frames = 0
				samples.removeAll()
				entries.removeAll()
				ended = false
			}

			for entry in feeder.timeline.take(through: frames, epoch: epoch) {
				entries.append(entry)
				switch entry.event {
				case let .format(format): channels = format.channels
				case .endOfStream: ended = true
				case .trackStart: break
				}
			}
			if ended && cog_ring_readable(ring) == 0 { return false }
			guard channels > 0 else { return true }

			var frameLimit = cog_ring_readable(ring) / channels
			if let next = feeder.timeline.nextFrame(epoch: epoch) {
				frameLimit = min(frameLimit, Int(next - frames))
			}
			guard frameLimit > 0 else { return true }
			var block = [Float](repeating: 0, count: frameLimit * channels)
			let got = block.withUnsafeMutableBufferPointer { cog_ring_read(ring, $0.baseAddress, frameLimit * channels) }
			samples.append(contentsOf: block[0..<got])
			frames += UInt64(got / channels)
			feeder.consumerDidRead()
			return true
		}
	}

	private func drainFeeder(_ feeder: Feeder, timeout: TimeInterval = 30) -> Consumer {
		var consumer = Consumer()
		let deadline = Date().addingTimeInterval(timeout)
		while consumer.poll(feeder) {
			if Date() > deadline {
				XCTFail("feeder did not finish")
				break
			}
		}
		return consumer
	}

	private func track(_ name: String, _ decoder: MemoryDecoder, gain: Float = 1) -> (EngineTrack, MemoryDecoder) {
		(EngineTrack(url: URL(string: "memory://\(name)")!, gain: gain), decoder)
	}

	private func makeFeeder(outputRate: Double, decoders: [(EngineTrack, MemoryDecoder)]) -> Feeder {
		let byURL = Dictionary(decoders.map { ($0.0.url, $0.1) }, uniquingKeysWith: { a, _ in a })
		return Feeder(outputRate: outputRate) { track in
			guard let source = byURL[track.url] else { return nil }
			// A fresh decoder per open, like the plugins.
			return MemoryDecoder(samples: source.samples, sampleRate: source.sampleRate, channels: source.channels)
		}!
	}

	private func trackStarts(_ consumer: Consumer) -> [(frame: UInt64, url: URL, offset: Double)] {
		consumer.entries.compactMap { entry in
			if case let .trackStart(track, offset) = entry.event { return (entry.frame, track.url, offset) }
			return nil
		}
	}

	// MARK: - Tests

	func testRepeatOneThroughTheFeederIsSeamless() {
		let samples = SeamSignal.loopable(frames: 96000, sampleRate: 48000)
		let lap = track("lap", MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2))
		let feeder = makeFeeder(outputRate: 384000, decoders: [lap])
		let delegate = ScriptedTracks([lap.0])
		feeder.delegate = delegate
		feeder.start(with: lap.0)
		let consumer = drainFeeder(feeder)
		feeder.stop()

		XCTAssertEqual(consumer.samples.count / 2, 768000 * 2)
		let starts = trackStarts(consumer)
		XCTAssertEqual(starts.map(\.frame), [0, 768000])
		if case .endOfStream? = consumer.entries.last?.event {} else { XCTFail("ends with endOfStream") }
		XCTAssertEqual(consumer.entries.last?.frame, 1_536_000)

		let reference = SeamSignal.reference(samples + samples, inputRate: 48000, outputRate: 384000)
		var worst: Float = 0
		for i in (768000 - 20000) * 2..<(768000 + 20000) * 2 {
			worst = max(worst, abs(consumer.samples[i] - reference[i]))
		}
		XCTAssertLessThan(worst, 1e-5, "seam")
	}

	func testARateChangeBetweenTracksKeepsBothWholeAndMarksTheJoin() {
		let first = track("a", MemoryDecoder(samples: SeamSignal.loopable(frames: 88200, sampleRate: 44100), sampleRate: 44100, channels: 2))
		let second = track("b", MemoryDecoder(samples: SeamSignal.loopable(frames: 96000, sampleRate: 48000), sampleRate: 48000, channels: 2))
		let feeder = makeFeeder(outputRate: 96000, decoders: [first, second])
		let delegate = ScriptedTracks([second.0])
		feeder.delegate = delegate
		feeder.start(with: first.0)
		let consumer = drainFeeder(feeder)
		feeder.stop()

		XCTAssertEqual(consumer.samples.count / 2, 384000)
		XCTAssertEqual(trackStarts(consumer).map(\.frame), [0, 192000])
		XCTAssertEqual(trackStarts(consumer).map(\.url), [first.0.url, second.0.url])
	}

	func testAChannelChangeIsMarkedWhereItHappens() {
		let stereo = track("stereo", MemoryDecoder(samples: SeamSignal.loopable(frames: 48000, sampleRate: 48000), sampleRate: 48000, channels: 2))
		let monoSamples = Array(SeamSignal.loopable(frames: 48000, sampleRate: 48000).enumerated().filter { $0.offset % 2 == 0 }.map(\.element))
		let mono = track("mono", MemoryDecoder(samples: monoSamples, sampleRate: 48000, channels: 1))
		let feeder = makeFeeder(outputRate: 96000, decoders: [stereo, mono])
		let delegate = ScriptedTracks([mono.0])
		feeder.delegate = delegate
		feeder.start(with: stereo.0)
		let consumer = drainFeeder(feeder)
		feeder.stop()

		let formats: [(UInt64, Int)] = consumer.entries.compactMap { entry in
			if case let .format(format) = entry.event { return (entry.frame, format.channels) }
			return nil
		}
		XCTAssertEqual(formats.map(\.0), [0, 96000])
		XCTAssertEqual(formats.map(\.1), [2, 1])
		XCTAssertEqual(trackStarts(consumer).map(\.frame), [0, 96000])
		XCTAssertEqual(consumer.samples.count, 96000 * 2 + 96000)
	}

	func testSeekingDiscardsQueuedAudioAndRestartsTheCount() {
		let samples = SeamSignal.loopable(frames: 480000, sampleRate: 48000) // 10 s
		let long = track("long", MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2))
		let feeder = makeFeeder(outputRate: 48000, decoders: [long])
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		feeder.start(with: long.0)

		// Let the ring fill, then seek to 7 s.
		var consumer = Consumer()
		while consumer.frames < 1000 { _ = consumer.poll(feeder) }
		feeder.seek(to: 7, in: long.0)
		let deadline = Date().addingTimeInterval(30)
		while consumer.poll(feeder), Date() < deadline {}
		feeder.stop()

		let starts = trackStarts(consumer)
		XCTAssertEqual(starts.count, 1)
		XCTAssertEqual(starts.first?.frame, 0)
		XCTAssertEqual(starts.first?.offset, 7)
		// Matching rates are bit-exact, so the audio is exactly the last 3 s.
		XCTAssertEqual(consumer.samples.count, 144000 * 2)
		XCTAssertEqual(Array(consumer.samples.prefix(8)), Array(samples[(336000 * 2)..<(336000 * 2 + 8)]))
	}

	func testAnUnopenableTrackIsSkipped() {
		let good = track("good", MemoryDecoder(samples: SeamSignal.loopable(frames: 4800, sampleRate: 48000), sampleRate: 48000, channels: 2))
		let missing = EngineTrack(url: URL(string: "memory://missing")!)
		let feeder = makeFeeder(outputRate: 48000, decoders: [good])
		let delegate = ScriptedTracks([missing, good.0])
		feeder.delegate = delegate
		feeder.start(with: good.0)
		let consumer = drainFeeder(feeder)
		feeder.stop()

		XCTAssertEqual(delegate.unopenable.map(\.url), [missing.url])
		XCTAssertEqual(trackStarts(consumer).map(\.frame), [0, 4800])
		XCTAssertEqual(consumer.samples.count, 9600 * 2)
	}
}
