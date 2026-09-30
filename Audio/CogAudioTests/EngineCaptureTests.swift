//
//  EngineCaptureTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

import AVFoundation
@testable import CogAudio
import XCTest

final class EngineCaptureTests: XCTestCase {
	private var directory: URL!

	override func setUp() {
		super.setUp()
		directory = FileManager.default.temporaryDirectory.appendingPathComponent("EngineCaptureTests-\(UUID().uuidString)")
	}

	override func tearDown() {
		try? FileManager.default.removeItem(at: directory)
		super.tearDown()
	}

	func testRecordsAFloatWAVWithMarkers() throws {
		let format = StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))
		let capture = try XCTUnwrap(EngineCapture(point: "test", format: format, directory: directory))
		let samples = (0..<(10000 * 2)).map { Float($0 % 200) / 200 - 0.5 }
		samples.withUnsafeBufferPointer { capture.record($0.baseAddress!, frames: 6000) }
		capture.mark("halfway")
		samples.withUnsafeBufferPointer { capture.record($0.baseAddress! + 6000 * 2, frames: 4000) }
		capture.finish()

		let file = try AVAudioFile(forReading: capture.url, commonFormat: .pcmFormatFloat32, interleaved: true)
		XCTAssertEqual(file.fileFormat.sampleRate, 48000)
		XCTAssertEqual(file.fileFormat.channelCount, 2)
		XCTAssertEqual(file.length, 10000)
		let buffer = try XCTUnwrap(AVAudioPCMBuffer(pcmFormat: file.processingFormat, frameCapacity: 10000))
		try file.read(into: buffer)
		let read = Array(UnsafeBufferPointer(start: buffer.floatChannelData![0], count: 10000 * 2))
		XCTAssertEqual(read, samples, "every sample, in order")

		let markers = try String(contentsOf: capture.url.deletingPathExtension().appendingPathExtension("txt"), encoding: .utf8)
		XCTAssertEqual(markers, "6000\thalfway\n")
	}

	func testThePumpCapturesItsInputAndOutput() throws {
		let samples = SeamSignal.loopable(frames: 24000, sampleRate: 48000)
		let track = EngineTrack(url: URL(string: "memory://capture")!)
		let feeder = try XCTUnwrap(Feeder(outputRate: 48000, opener: { _ in MemoryDecoder(samples: samples, sampleRate: 48000, channels: 2) }))
		let delegate = ScriptedTracks([])
		feeder.delegate = delegate
		let format = StreamFormat(sampleRate: 48000, channels: 2, channelConfig: UInt32(AudioConfigStereo))
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: format, captureDirectory: directory))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }
		feeder.start(with: track)
		pump.start()
		var ended = false
		var buffer = [Float](repeating: 0, count: 512 * 2)
		let deadline = Date().addingTimeInterval(10)
		while !ended && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			for entry in pump.presentation.take(through: cog_ring_read_position(pump.ring)) {
				if case .endOfStream = entry.event { ended = true }
			}
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()

		let files = try FileManager.default.contentsOfDirectory(atPath: directory.path).sorted()
		let wavs = files.filter { $0.hasSuffix(".wav") }
		XCTAssertEqual(wavs.count, 2, "\(files)")
		for name in wavs {
			let file = try AVAudioFile(forReading: directory.appendingPathComponent(name))
			XCTAssertEqual(file.length, 24000, name)
		}
		let output = try XCTUnwrap(files.first { $0.contains("-output-") && $0.hasSuffix(".txt") })
		let markers = try String(contentsOf: directory.appendingPathComponent(output), encoding: .utf8)
		XCTAssertEqual(markers, "0\ttrack start memory://capture at 0.0 s\n")
	}
}
