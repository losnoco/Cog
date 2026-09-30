//
//  FreeSurroundStageTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

@testable import CogAudio
import XCTest

final class FreeSurroundStageTests: XCTestCase {
	private let stereo = StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))

	override func setUp() {
		super.setUp()
		UserDefaults.standard.set(true, forKey: "enableFSurround")
	}

	override func tearDown() {
		UserDefaults.standard.removeObject(forKey: "enableFSurround")
		super.tearDown()
	}

	/// A 440 Hz tone the same in both channels, so it steers to the centre.
	private func tone(frames: Int) -> [Float] {
		var samples = [Float](repeating: 0, count: frames * 2)
		for frame in 0..<frames {
			let value = Float(0.25 * sin(2 * .pi * 440 * Double(frame) / 48000))
			samples[frame * 2] = value
			samples[frame * 2 + 1] = value
		}
		return samples
	}

	/// Feeds `input` in awkward block sizes, then drains, as the pump does.
	private func run(_ stage: FreeSurroundStage, _ input: [Float], format: StreamFormat) -> (samples: [Float], format: StreamFormat) {
		let output = stage.configure(input: format)
		let sizes = [1000, 4096, 17, 3000, 777]
		let buffer = DSPBuffer()
		var result: [Float] = []
		var position = 0
		var index = 0
		let frames = input.count / format.channels
		while position < frames {
			let count = min(sizes[index % sizes.count], frames - position)
			buffer.resize(frames: count, format: format)
			for i in 0..<(count * format.channels) {
				buffer.samples[i] = input[position * format.channels + i]
			}
			stage.process(buffer)
			result.append(contentsOf: buffer.samples[0..<(buffer.frames * buffer.format.channels)])
			position += count
			index += 1
		}
		buffer.resize(frames: 0, format: format)
		stage.drain(buffer)
		result.append(contentsOf: buffer.samples[0..<(buffer.frames * buffer.format.channels)])
		return (result, output)
	}

	func testStereoBecomesSurroundOfExactlyTheSameLength() {
		let stage = FreeSurroundStage()
		XCTAssertTrue(stage.isActive)
		let (samples, format) = run(stage, tone(frames: 30000), format: stereo)
		XCTAssertEqual(format.channels, 6)
		XCTAssertEqual(samples.count / 6, 30000, "no frames lost or added")
	}

	/// The half-block lag is removed: the centre channel lines up with the
	/// input rather than trailing it by 2048 frames.
	func testTheDecodersLagIsRemoved() throws {
		let stage = FreeSurroundStage()
		// Noise-like and the same in both channels: one true alignment, and
		// steered to the centre. (A tone repeats and cannot show a lag.)
		let noise = SeamSignal.loopable(frames: 30000, sampleRate: 48000)
		var input = [Float](repeating: 0, count: 60000)
		for frame in 0..<30000 {
			input[frame * 2] = noise[frame * 2]
			input[frame * 2 + 1] = noise[frame * 2]
		}
		let (samples, format) = run(stage, input, format: stereo)
		let centre = Int(AudioChunk.channelIndex(fromConfig: format.channelConfig, forFlag: UInt32(AudioChannelFrontCenter)))
		let channel = stride(from: centre, to: samples.count, by: 6).map { samples[$0] }
		let reference = stride(from: 0, to: input.count, by: 2).map { input[$0] }

		var bestLag = 0
		var best = -Float.infinity
		for lag in -3000...3000 {
			var sum: Float = 0
			for i in stride(from: 8000, to: 20000, by: 3) {
				sum += channel[i] * reference[i - lag]
			}
			if sum > best {
				best = sum
				bestLag = lag
			}
		}
		XCTAssertLessThan(abs(bestLag), 8, "aligned with the input")
		XCTAssertGreaterThan(channel[10000...20000].map(abs).max()!, 0.05, "the centre carries the tone")
	}

	func testMonoPassesThroughUntouched() {
		let stage = FreeSurroundStage()
		let mono = StreamFormat(sampleRate: 48000, channels: 1, channelConfig: UInt32(AudioConfigMono))
		let input = (0..<5000).map { Float($0 % 100) / 100 }
		let (samples, format) = run(stage, input, format: mono)
		XCTAssertEqual(format, mono)
		XCTAssertEqual(samples, input)
	}

	/// Through the pump to a stereo device: the upmix is folded back down,
	/// and the drain at the end of the stream keeps the track whole.
	func testThePumpKeepsAWholeTrackWithSurroundInTheChain() throws {
		let track = (EngineTrack(url: URL(string: "memory://fs")!), MemoryDecoder(samples: tone(frames: 30000), sampleRate: 48000, channels: 2))
		let feeder = try XCTUnwrap(Feeder(outputRate: 48000) { _ in
			MemoryDecoder(samples: track.1.samples, sampleRate: 48000, channels: 2)
		})
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: stereo, stages: [FreeSurroundStage()]))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: track.0)
		pump.start()
		var played: [Float] = []
		var ended = false
		var buffer = [Float](repeating: 0, count: 1024)
		let deadline = Date().addingTimeInterval(20)
		while !ended && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			played.append(contentsOf: buffer[0..<(got * 2)])
			for (_, event) in pump.presentation.take(through: cog_ring_read_position(pump.ring)) {
				if case .endOfStream = event { ended = true }
			}
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()
		XCTAssertTrue(ended)
		XCTAssertEqual(played.count / 2, 30000)
	}
}
