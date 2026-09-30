//
//  ConverterSeamTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import XCTest

/// Drives the real `ConverterNode` across a track boundary, the way two
/// consecutive `BufferChain`s do: each track gets its own converter, with its
/// own soxr instance and LPC edge extrapolation. The joined output is compared
/// with one uninterrupted soxr pass over the concatenated input, which is what
/// a seamless join would sound like.
final class ConverterSeamTests: XCTestCase {
	private static let channels = SeamSignal.channels
	private static let owner = NSObject()

	// MARK: - Harness

	private static func floatFormat(sampleRate: Double) -> AudioStreamBasicDescription {
		let bytesPerFrame = UInt32(MemoryLayout<Float>.size * channels)
		return AudioStreamBasicDescription(mSampleRate: sampleRate,
		                                   mFormatID: kAudioFormatLinearPCM,
		                                   mFormatFlags: kAudioFormatFlagsNativeFloatPacked,
		                                   mBytesPerPacket: bytesPerFrame,
		                                   mFramesPerPacket: 1,
		                                   mBytesPerFrame: bytesPerFrame,
		                                   mChannelsPerFrame: UInt32(channels),
		                                   mBitsPerChannel: 32,
		                                   mReserved: 0)
	}

	/// Runs one track through a fresh `ConverterNode`, feeding it in the uneven
	/// chunk sizes a decoder produces, and returns everything it emits.
	private static func convertTrack(_ samples: [Float], inputRate: Double, outputRate: Double) throws -> [Float] {
		let inputFormat = floatFormat(sampleRate: inputRate)
		let source = try XCTUnwrap(Node(controller: owner, previous: nil))
		let converter = try XCTUnwrap(ConverterNode(controller: owner, previous: source))
		XCTAssertTrue(converter.setup(withInputFormat: inputFormat, withInputConfig: 0, outputFormat: floatFormat(sampleRate: outputRate), isLossless: true))

		let chunkSizes = [1024, 576, 2048, 333, 4096, 17]
		let frames = samples.count / channels
		var position = 0
		var index = 0
		while position < frames {
			let count = min(chunkSizes[index % chunkSizes.count], frames - position)
			let chunk = AudioChunk()
			chunk.format = inputFormat
			samples.withUnsafeBufferPointer { buffer in
				chunk.assignSamples(buffer.baseAddress! + position * channels, frameCount: count)
			}
			source.write(chunk)
			position += count
			index += 1
		}
		source.setEndOfStream(true)

		var output: [Float] = []
		var emptyCalls = 0
		while emptyCalls < 4 {
			guard let chunk = converter.convert(), chunk.frameCount() > 0 else {
				if source.buffer().isEmpty() {
					emptyCalls += 1
				}
				continue
			}
			emptyCalls = 0
			let data = chunk.removeSamples(chunk.frameCount())
			data.withUnsafeBytes { raw in
				output.append(contentsOf: raw.bindMemory(to: Float.self))
			}
		}
		converter.setShouldContinue(false)
		return output
	}

	private struct SeamReport {
		var trackFrames: [Int]
		var expectedTrackFrames: Int
		var seamError: Float
		var tailError: Float
		var headError: Float
		var disturbedMilliseconds: Double
		var interiorError: Float
	}

	/// Plays `track` twice in a row, as repeat-one does, through two converters.
	private func measureLoop(inputRate: Double, outputRate: Double, seconds: Double = 2.0) throws -> SeamReport {
		let frames = Int(inputRate * seconds)
		let track = SeamSignal.loopable(frames: frames, sampleRate: inputRate)

		let first = try Self.convertTrack(track, inputRate: inputRate, outputRate: outputRate)
		let second = try Self.convertTrack(track, inputRate: inputRate, outputRate: outputRate)
		let joined = first + second
		let reference = SeamSignal.reference(track + track, inputRate: inputRate, outputRate: outputRate)

		let channels = Self.channels
		let seam = first.count / channels
		let window = Int(outputRate / 20.0) // 50 ms either side of the join
		func maxError(_ range: Range<Int>) -> Float {
			var worst: Float = 0
			for frame in range where frame < joined.count / channels && frame < reference.count / channels {
				for channel in 0..<channels {
					worst = max(worst, abs(joined[frame * channels + channel] - reference[frame * channels + channel]))
				}
			}
			return worst
		}

		let seamRange = max(0, seam - window)..<(seam + window)
		var disturbed = seamRange.filter { maxError($0..<($0 + 1)) > 1e-4 }
		if disturbed.isEmpty { disturbed = [seam] }
		let interiorRange = (seam / 2 - window)..<(seam / 2 + window)
		return SeamReport(trackFrames: [first.count / channels, second.count / channels],
		                  expectedTrackFrames: Int((Double(frames) * outputRate / inputRate).rounded()),
		                  seamError: maxError(seamRange),
		                  tailError: maxError(seamRange.lowerBound..<seam),
		                  headError: maxError(seam..<seamRange.upperBound),
		                  disturbedMilliseconds: Double(disturbed.last! - disturbed.first! + 1) * 1000.0 / outputRate,
		                  interiorError: maxError(interiorRange))
	}

	// MARK: - Tests

	private func assertLoopJoins(inputRate: Double, outputRate: Double, file: StaticString = #filePath, line: UInt = #line) throws {
		let report = try measureLoop(inputRate: inputRate, outputRate: outputRate)
		let label = "\(Int(inputRate)) → \(Int(outputRate))"
		print("[\(label)] track frames \(report.trackFrames) (expected \(report.expectedTrackFrames)), seam error \(report.seamError) (tail \(report.tailError), head \(report.headError), >1e-4 over \(String(format: "%.2f", report.disturbedMilliseconds)) ms), interior error \(report.interiorError)")

		// A per-track converter must neither drop nor add frames: any
		// difference here is heard as a gap or a jump at every seam.
		for count in report.trackFrames {
			XCTAssertEqual(count, report.expectedTrackFrames, "\(label): frames per track", file: file, line: line)
		}
		// The interior must match the reference to float precision, or the
		// comparison itself is misaligned.
		XCTAssertLessThan(report.interiorError, 1e-4, "\(label): interior error", file: file, line: line)
		// The per-track converter cannot know the next track's first sample,
		// so the output frames between the last input sample of one track and
		// the first of the next are interpolated toward the LPC guess instead.
		// That error is confined to about one input sample period before the
		// join and grows with the upsampling ratio (roughly 3e-4 at 44.1k out,
		// 1.4e-2 at 48k → 384k, 2.9e-2 at 44.1k → 384k). These bounds guard
		// against regressions: a dropped, repeated or misaligned frame shows up
		// as a much larger error or a much wider disturbance.
		XCTAssertLessThan(report.seamError, 5e-2, "\(label): seam error", file: file, line: line)
		XCTAssertLessThan(report.disturbedMilliseconds, 0.25, "\(label): extent of the seam disturbance", file: file, line: line)

		// A seamless join matches the continuous reference everywhere. Only a
		// resampler that carries its state across the track boundary can do
		// that; the per-track ConverterNode is expected to miss.
		XCTExpectFailure("Per-track resampling interpolates toward an extrapolated next sample; see Audio/Engine/PLAN.md, principle 2", strict: false) {
			XCTAssertLessThan(report.seamError, 1e-4, "\(label): seamless join", file: file, line: line)
		}
	}

	func testLoop48kTo44k() throws {
		try assertLoopJoins(inputRate: 48000, outputRate: 44100)
	}

	func testLoop48kTo96k() throws {
		try assertLoopJoins(inputRate: 48000, outputRate: 96000)
	}

	func testLoop48kTo192k() throws {
		try assertLoopJoins(inputRate: 48000, outputRate: 192000)
	}

	func testLoop48kTo384k() throws {
		try assertLoopJoins(inputRate: 48000, outputRate: 384000)
	}

	func testLoop44kTo48k() throws {
		try assertLoopJoins(inputRate: 44100, outputRate: 48000)
	}

	func testLoop44kTo384k() throws {
		try assertLoopJoins(inputRate: 44100, outputRate: 384000)
	}
}
