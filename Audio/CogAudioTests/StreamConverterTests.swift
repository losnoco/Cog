//
//  StreamConverterTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import CogAudio
import XCTest

final class StreamConverterTests: XCTestCase {
	private let channels = SeamSignal.channels
	private let chunkSizes = [17, 1024, 576, 2048, 333, 4096]

	/// Feeds `samples` in decoder-sized chunks and collects the output.
	private func feed(_ samples: [Float], rate: Double, into converter: StreamConverter, gain: Float = 1, output: inout [Float]) {
		let frames = samples.count / channels
		let format = StreamFormat(sampleRate: rate, channels: channels)
		var position = 0
		var index = 0
		samples.withUnsafeBufferPointer { buffer in
			while position < frames {
				let count = min(chunkSizes[index % chunkSizes.count], frames - position)
				let chunk = UnsafeBufferPointer(rebasing: buffer[(position * channels)..<((position + count) * channels)])
				converter.process(chunk, format: format, gain: gain) { out, outFormat in
					XCTAssertEqual(outFormat.sampleRate, converter.outputRate)
					XCTAssertEqual(outFormat.channels, channels)
					output.append(contentsOf: out)
				}
				position += count
				index += 1
			}
		}
	}

	private func drain(_ converter: StreamConverter, output: inout [Float]) {
		converter.drain { out, _ in output.append(contentsOf: out) }
	}

	private func maxError(_ a: [Float], _ b: [Float], frames: Range<Int>) -> Float {
		var worst: Float = 0
		for frame in frames {
			for channel in 0..<channels {
				worst = max(worst, abs(a[frame * channels + channel] - b[frame * channels + channel]))
			}
		}
		return worst
	}

	// MARK: - Seamless joins

	/// Repeat-one through the persistent converter: the join must be what a
	/// single uninterrupted resample of both laps produces.
	private func assertRepeatOneIsSeamless(inputRate: Double, outputRate: Double, file: StaticString = #filePath, line: UInt = #line) {
		let frames = Int(inputRate * 2)
		let track = SeamSignal.loopable(frames: frames, sampleRate: inputRate)
		let converter = StreamConverter(outputRate: outputRate)

		var output: [Float] = []
		feed(track, rate: inputRate, into: converter, output: &output)
		let seam = converter.outputPositionOfNextInput
		feed(track, rate: inputRate, into: converter, output: &output)
		drain(converter, output: &output)

		let expectedPerTrack = Int((Double(frames) * outputRate / inputRate).rounded())
		XCTAssertEqual(Int(seam), expectedPerTrack, "seam position", file: file, line: line)
		XCTAssertEqual(output.count / channels, expectedPerTrack * 2, "total frames", file: file, line: line)
		XCTAssertEqual(converter.outputFrames, UInt64(expectedPerTrack * 2), file: file, line: line)

		let reference = SeamSignal.reference(track + track, inputRate: inputRate, outputRate: outputRate)
		let window = Int(outputRate / 20)
		let seamError = maxError(output, reference, frames: (Int(seam) - window)..<(Int(seam) + window))
		let interiorError = maxError(output, reference, frames: (Int(seam) / 2 - window)..<(Int(seam) / 2 + window))
		print("[\(Int(inputRate)) → \(Int(outputRate))] persistent converter seam error \(seamError), interior \(interiorError)")
		XCTAssertLessThan(seamError, 1e-5, "seam", file: file, line: line)
		XCTAssertLessThan(interiorError, 1e-5, "interior", file: file, line: line)
	}

	func testRepeatOne48kTo384kIsSeamless() {
		assertRepeatOneIsSeamless(inputRate: 48000, outputRate: 384000)
	}

	func testRepeatOne44kTo384kIsSeamless() {
		assertRepeatOneIsSeamless(inputRate: 44100, outputRate: 384000)
	}

	func testRepeatOne48kTo44kIsSeamless() {
		assertRepeatOneIsSeamless(inputRate: 48000, outputRate: 44100)
	}

	func testRepeatOne44kTo48kIsSeamless() {
		assertRepeatOneIsSeamless(inputRate: 44100, outputRate: 48000)
	}

	func testRepeatOne48kTo96kIsSeamless() {
		assertRepeatOneIsSeamless(inputRate: 48000, outputRate: 96000)
	}

	// MARK: - Bypass, gain, format changes

	func testMatchingRatesPassThroughBitExactly() {
		let track = SeamSignal.loopable(frames: 48000, sampleRate: 48000)
		let converter = StreamConverter(outputRate: 48000)
		var output: [Float] = []
		feed(track, rate: 48000, into: converter, output: &output)
		drain(converter, output: &output)
		XCTAssertEqual(output, track)
	}

	func testReplayGainScalesTheOutput() {
		let track = SeamSignal.loopable(frames: 48000, sampleRate: 48000)
		var unity: [Float] = []
		var halved: [Float] = []
		let a = StreamConverter(outputRate: 96000)
		feed(track, rate: 48000, into: a, output: &unity)
		drain(a, output: &unity)
		let b = StreamConverter(outputRate: 96000)
		feed(track, rate: 48000, into: b, gain: 0.5, output: &halved)
		drain(b, output: &halved)
		XCTAssertEqual(unity.count, halved.count)
		XCTAssertLessThan(maxError(unity.map { $0 * 0.5 }, halved, frames: 0..<(unity.count / channels)), 1e-6)
	}

	/// A rate change drains the old run and starts a new one; each track
	/// still comes out at exactly its own length, and the marker lands on the
	/// join.
	func testARateChangeKeepsEveryTracksLength() {
		let first = SeamSignal.loopable(frames: 88200, sampleRate: 44100)
		let second = SeamSignal.loopable(frames: 96000, sampleRate: 48000)
		let converter = StreamConverter(outputRate: 96000)
		var output: [Float] = []
		feed(first, rate: 44100, into: converter, output: &output)
		XCTAssertEqual(converter.outputPositionOfNextInput, 192000)
		feed(second, rate: 48000, into: converter, output: &output)
		drain(converter, output: &output)
		XCTAssertEqual(output.count / channels, 384000)
		XCTAssertFalse(output.contains { !$0.isFinite })

		// Each side of the join is the edge of its own run, so it matches that
		// track resampled alone; nothing is lost or smeared across the join.
		let firstAlone = SeamSignal.reference(first, inputRate: 44100, outputRate: 96000)
		let secondAlone = SeamSignal.reference(second, inputRate: 48000, outputRate: 96000)
		let interiorFirst = maxError(Array(output[0..<(192000 * channels)]), firstAlone, frames: 50000..<60000)
		let interiorSecond = maxError(Array(output[(192000 * channels)...]), secondAlone, frames: 50000..<60000)
		XCTAssertLessThan(interiorFirst, 1e-5)
		XCTAssertLessThan(interiorSecond, 1e-5)
	}

	func testDrainingAnIdleConverterDoesNothing() {
		let converter = StreamConverter(outputRate: 96000)
		var output: [Float] = []
		drain(converter, output: &output)
		XCTAssertTrue(output.isEmpty)
		XCTAssertNil(converter.inputFormat)
	}

	/// A run shorter than the LPC prime length still comes out whole.
	func testAVeryShortRunStillComesOutWhole() {
		let track = SeamSignal.loopable(frames: 300, sampleRate: 48000)
		let converter = StreamConverter(outputRate: 384000)
		var output: [Float] = []
		feed(track, rate: 48000, into: converter, output: &output)
		drain(converter, output: &output)
		XCTAssertEqual(output.count / channels, 2400)
	}

	/// A seek resets the run; the output after it is a fresh, complete run.
	func testResetStartsAFreshRun() {
		let track = SeamSignal.loopable(frames: 48000, sampleRate: 48000)
		let converter = StreamConverter(outputRate: 96000)
		var discarded: [Float] = []
		feed(Array(track[0..<(10000 * channels)]), rate: 48000, into: converter, output: &discarded)
		converter.reset()
		XCTAssertEqual(converter.outputFrames, 0)
		var output: [Float] = []
		feed(track, rate: 48000, into: converter, output: &output)
		drain(converter, output: &output)
		XCTAssertEqual(output.count / channels, 96000)
	}
}
