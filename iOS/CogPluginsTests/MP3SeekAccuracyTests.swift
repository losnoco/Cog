//
//  MP3SeekAccuracyTests.swift
//  CogPluginsTests
//
//  Phase 2 of Plugins/minimp3/PLAN.md: minimp3-ex seeking is sample-exact.
//  Decode a real LAME encode sequentially from sample 0 as the reference;
//  for each target, seek and read a window, and confirm the window aligns
//  with the reference at exactly that sample. A 440 Hz sine shifts phase
//  detectably with even a one-sample error, so alignment at offset 0 proves
//  zero positional error. The first couple of frames after a seek carry the
//  bit-reservoir transient, so the comparison starts past them.
//

@testable import CogAudio
import XCTest

final class MP3SeekAccuracyTests: XCTestCase {
	private var directory: URL!

	override func setUpWithError() throws {
		_ = PluginController.shared()
		directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
		try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
	}

	override func tearDownWithError() throws {
		try? FileManager.default.removeItem(at: directory)
	}

	private var plugins: CogPluginController { PluginController.shared() }

	private func fixture(_ base64: String, _ name: String) throws -> URL {
		let url = directory.appendingPathComponent(name)
		try MP3Fixtures.write(base64, to: url)
		return url
	}

	private func openDecoder(_ url: URL) throws -> CogDecoder {
		let source = try XCTUnwrap(plugins.audioSource(for: url))
		XCTAssertTrue(source.open(url))
		let decoder = try XCTUnwrap((NSClassFromString("MP3Decoder") as? NSObject.Type)?.init() as? CogDecoder)
		XCTAssertTrue(decoder.open(source), "MP3Decoder would not open \(url)")
		return decoder
	}

	/// The first channel's samples, read sequentially.
	private func channel0(_ decoder: CogDecoder, maxFrames: Int = .max) -> [Float] {
		var out: [Float] = []
		while out.count < maxFrames, let chunk = decoder.readAudio(), chunk.frameCount() > 0 {
			let data = chunk.removeSamples(chunk.frameCount())
			let channels = Int(chunk.format.mChannelsPerFrame)
			data.withUnsafeBytes { raw in
				let floats = raw.bindMemory(to: Float.self)
				var i = 0
				while i < floats.count && out.count < maxFrames {
					out.append(floats[i])
					i += channels
				}
			}
		}
		return out
	}

	private func assertSeekExact(_ url: URL, _ label: String) throws {
		let referenceDecoder = try openDecoder(url)
		let reference = channel0(referenceDecoder)
		referenceDecoder.close()
		let total = reference.count
		XCTAssertGreaterThan(total, 40000, "\(label): fixture too short")

		let decoder = try openDecoder(url)
		let skip = 2304 // two frames of bit-reservoir transient
		let window = 4096
		let targets = [0, 1, 1151, 1152, 1153, 44100, 26460, total * 2 / 3, total / 2 + 123, total - 4000]
		for target in targets {
			XCTAssertEqual(decoder.seek(target), target, "\(label): seek \(target)")
			let got = channel0(decoder, maxFrames: skip + window)
			let n = min(window, total - target - skip, got.count - skip)
			XCTAssertGreaterThanOrEqual(n, 512, "\(label): target \(target) window")
			var maxDiff: Float = 0
			for i in 0..<n {
				let diff = abs(got[skip + i] - reference[target + skip + i])
				if diff > maxDiff { maxDiff = diff }
			}
			XCTAssertLessThan(maxDiff, 1e-4, "\(label): target \(target) landed off-sample (maxDiff \(maxDiff))")
		}
		decoder.close()
	}

	func testCBRStereoSeeksToTheExactSample() throws {
		try assertSeekExact(fixture(MP3Fixtures.cbrStereo, "cbr.mp3"), "CBR stereo")
	}

	func testVBRStereoSeeksToTheExactSample() throws {
		try assertSeekExact(fixture(MP3Fixtures.vbrStereo, "vbr.mp3"), "VBR stereo")
	}

	func testCBRMonoSeeksToTheExactSample() throws {
		try assertSeekExact(fixture(MP3Fixtures.cbrMono, "mono.mp3"), "CBR mono")
	}

	func testWithoutAXingTagSeeksToTheExactSample() throws {
		try assertSeekExact(fixture(MP3Fixtures.noXingStereo, "noxing.mp3"), "no Xing")
	}
}