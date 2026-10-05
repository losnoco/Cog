//
//  OutputStatusTests.swift
//  CogAudioTests
//
//  Created by Marko Jurkovic on 9/30/26.
//

@testable import CogAudio
import XCTest

final class OutputStatusTests: XCTestCase {
	private static let stereo = StreamFormat(sampleRate: 44100, channels: 2, channelConfig: UInt32(AudioConfigStereo))

	private func source(rate: Double = 44100, bits: Int, float: Bool = false, channels: Int = 2) -> SourceFormat? {
		SourceFormat(properties: ["sampleRate": rate, "bitsPerSample": bits, "floatingPoint": float, "channels": channels])
	}

	private func status(_ source: SourceFormat?, render: StreamFormat = OutputStatusTests.stereo, integer: Bool = false, _ change: (inout OutputStatus) -> Void = { _ in }) -> OutputStatus {
		var status = OutputStatus(source: source, render: render,
		                          renderFormat: integer ? DeviceOutput.integerASBD(render) : Pump.asbd(render))
		change(&status)
		return status
	}

	func testUntouchedSamplesArePassedOnAsDecoded() {
		XCTAssertEqual(status(source(bits: 16)).modifications, [])
		XCTAssertEqual(status(source(bits: 24)).modifications, [])
		XCTAssertEqual(status(source(bits: 32, float: true)).modifications, [])
	}

	func testFloat32CannotHoldEverySourceExactly() {
		XCTAssertEqual(status(source(bits: 32)).modifications, [CogAudioOutputModificationPrecision])
		XCTAssertEqual(status(source(bits: 64, float: true)).modifications, [CogAudioOutputModificationPrecision])
		// A DoP carrier pipeline renders 24-bit integers.
		XCTAssertEqual(status(source(bits: 24), integer: true).modifications, [])
		XCTAssertEqual(status(source(bits: 32, float: true), integer: true).modifications, [CogAudioOutputModificationPrecision])
	}

	func testModificationsAreListedInSignalOrder() {
		let modified = status(source(rate: 48000, bits: 32)) {
			$0.processing.appliesGain = true
			$0.processing.fitsChannels = true
			$0.stageModifications = [CogAudioOutputModificationTimeStretch, CogAudioOutputModificationEqualizer]
			$0.volume = 50
		}
		XCTAssertEqual(modified.modifications, [CogAudioOutputModificationPrecision, CogAudioOutputModificationResampling,
		                                        CogAudioOutputModificationTrackGain, CogAudioOutputModificationTimeStretch,
		                                        CogAudioOutputModificationEqualizer, CogAudioOutputModificationChannelLayout,
		                                        CogAudioOutputModificationVolume])
	}

	func testHDCDDecodingIsReported() {
		XCTAssertEqual(status(source(bits: 16)) { $0.decodesHDCD = true }.modifications, [CogAudioOutputModificationHDCD])
	}

	func testDSDIsConvertedUnlessItGoesOutAsDoP() {
		let dsd = source(rate: 2_822_400, bits: 1)
		XCTAssertEqual(status(dsd).modifications, [CogAudioOutputModificationDSDToPCM])
		let dop = status(dsd, render: StreamFormat(sampleRate: 176_400, channels: 2, channelConfig: UInt32(AudioConfigStereo)), integer: true) {
			$0.processing.passesDoP = true
			// The renderer passes DoP by the volume, too.
			$0.volume = 50
		}
		XCTAssertEqual(dop.modifications, [])
	}

	func testAnUndescribedSourceCannotBeJudged() {
		XCTAssertNil(SourceFormat(properties: ["sampleRate": 44100]))
		let unknown = status(nil)
		XCTAssertNil(unknown.modifications)
		let userInfo = unknown.userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])
		XCTAssertNil(userInfo[CogAudioOutputSourceFormatKey])
		XCTAssertNil(userInfo[CogAudioOutputModificationsKey])
		XCTAssertNil(userInfo[CogAudioOutputDeviceNameKey])
		XCTAssertNotNil(userInfo[CogAudioOutputRenderFormatKey])
	}

	/// The amounts go only with the modifications they explain.
	func testModificationsCarryTheirAmounts() throws {
		let source = SourceFormat(properties: ["sampleRate": 44100, "bitsPerSample": 16, "floatingPoint": false, "channels": 6,
		                                       "codec": "FLAC", "encoding": "lossless"])
		let modified = status(source) {
			$0.trackGain = 0.5
			$0.processing.appliesGain = true
			$0.processing.output = StreamFormat(sampleRate: 44100, channels: 6)
			$0.processing.fitsChannels = true
			$0.volume = 75
		}
		let userInfo = modified.userInfo(deviceName: "Speakers", virtualFormats: [], physicalFormats: [])
		XCTAssertEqual(userInfo[CogAudioOutputSourceCodecKey] as? String, "FLAC")
		XCTAssertEqual(userInfo[CogAudioOutputSourceEncodingKey] as? String, "lossless")
		XCTAssertEqual(userInfo[CogAudioOutputDeviceNameKey] as? String, "Speakers")
		XCTAssertEqual(try XCTUnwrap(userInfo[CogAudioOutputTrackGainKey] as? Double), -6.02, accuracy: 0.01)
		XCTAssertEqual(userInfo[CogAudioOutputFittedChannelsKey] as? Int, 6)
		XCTAssertEqual(userInfo[CogAudioOutputVolumeKey] as? Double, 75)

		let untouched = status(source).userInfo(deviceName: "Speakers", virtualFormats: [], physicalFormats: [])
		XCTAssertNil(untouched[CogAudioOutputTrackGainKey])
		XCTAssertNil(untouched[CogAudioOutputFittedChannelsKey])
		XCTAssertNil(untouched[CogAudioOutputVolumeKey])
	}

	func testTheResamplerIsDescribedWhereverPCMReachesIt() throws {
		let resampled = status(source(rate: 48000, bits: 16)).userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])
		let resampler = try XCTUnwrap(resampled[CogAudioOutputResamplerKey] as? [String: Any])
		XCTAssertEqual(resampler[CogAudioOutputStageActiveKey] as? Bool, true)
		XCTAssertEqual(resampler[CogAudioOutputStageInputRateKey] as? Double, 48000)
		XCTAssertEqual(resampler[CogAudioOutputStageOutputRateKey] as? Double, 44100)

		let bypassed = status(source(bits: 16)).userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])
		XCTAssertEqual((bypassed[CogAudioOutputResamplerKey] as? [String: Any])?[CogAudioOutputStageActiveKey] as? Bool, false,
		               "equal rates bypass soxr, bit-exact")

		// DSD64 is decimated to 352.8 kHz PCM before the resampler.
		let dsd = status(source(rate: 2_822_400, bits: 1)).userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])
		XCTAssertEqual((dsd[CogAudioOutputResamplerKey] as? [String: Any])?[CogAudioOutputStageInputRateKey] as? Double, 352_800)

		let dop = status(source(rate: 2_822_400, bits: 1)) { $0.processing.passesDoP = true }
		XCTAssertNil(dop.userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])[CogAudioOutputResamplerKey])
	}

	func testEachStageCarriesItsSettings() throws {
		let surround = StreamFormat(sampleRate: 44100, channels: 6, channelConfig: 0x3F)
		var gains = [Float](repeating: 0, count: 31)
		gains[0] = 2
		let staged = status(source(bits: 16)) {
			$0.processing.inspections = [.timeStretch(engine: "finer", tempo: 1.25, pitch: 1),
			                             .freeSurround(upmixes: true, output: surround),
			                             .equalizer(preampDB: -3, gainsDB: gains)]
			$0.gainSource = .album
			$0.gainPeakLimited = true
			$0.bufferFrames = 882
			$0.latencyFrames = 1500
		}
		let userInfo = staged.userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])
		let stretch = try XCTUnwrap(userInfo[CogAudioOutputTimeStretchKey] as? [String: Any])
		XCTAssertEqual(stretch[CogAudioOutputStageEngineKey] as? String, "finer")
		XCTAssertEqual(stretch[CogAudioOutputStageTempoKey] as? Double, 1.25)
		let upmix = try XCTUnwrap(userInfo[CogAudioOutputFreeSurroundKey] as? [String: Any])
		XCTAssertEqual(upmix[CogAudioOutputStageChannelsKey] as? Int, 6)
		XCTAssertEqual(upmix[CogAudioOutputStageChannelConfigKey] as? UInt32, 0x3F)
		let equalizer = try XCTUnwrap(userInfo[CogAudioOutputEqualizerKey] as? [String: Any])
		XCTAssertEqual(equalizer[CogAudioOutputStagePreampKey] as? Double, -3)
		XCTAssertEqual((equalizer[CogAudioOutputStageBandGainsKey] as? [Double])?.first, 2)
		XCTAssertEqual((equalizer[CogAudioOutputStageBandFrequenciesKey] as? [Double])?.count, 31)
		XCTAssertEqual(userInfo[CogAudioOutputTrackGainSourceKey] as? String, "album")
		XCTAssertEqual(userInfo[CogAudioOutputTrackGainPeakLimitedKey] as? Bool, true)
		XCTAssertEqual(userInfo[CogAudioOutputBufferFramesKey] as? Int, 882)
		XCTAssertEqual(userInfo[CogAudioOutputLatencyFramesKey] as? Int, 1500)
		XCTAssertNil(userInfo[CogAudioOutputSpatialKey])
	}

	/// A setting changed within a stage is news, though the same stages run.
	func testChangedSettingsMakeANewStatus() {
		let slower = status(source(bits: 16)) { $0.processing.inspections = [.timeStretch(engine: "finer", tempo: 1.25, pitch: 1)] }
		let faster = status(source(bits: 16)) { $0.processing.inspections = [.timeStretch(engine: "finer", tempo: 1.5, pitch: 1)] }
		XCTAssertNotEqual(slower, faster)
		XCTAssertNotEqual(status(source(bits: 16)), status(source(bits: 16)) { $0.headTracking = true })
	}

	func testSpatialRenderingIsDescribed() throws {
		let spatial = status(source(bits: 16, channels: 6)) {
			$0.stageModifications = [CogAudioOutputModificationSpatialAudio]
			$0.spatialRendering = .spatialMixerOutputType_Headphones
			$0.headTracking = true
		}
		let described = try XCTUnwrap(spatial.userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])[CogAudioOutputSpatialKey] as? [String: Any])
		XCTAssertEqual(described[CogAudioOutputStageSpatialOutputKey] as? String, "headphones")
		XCTAssertEqual(described[CogAudioOutputStageHeadTrackingKey] as? Bool, true)

		let refused = status(source(bits: 16, channels: 6)) { $0.spatialRefused = true }
		XCTAssertEqual(refused.userInfo(deviceName: nil, virtualFormats: [], physicalFormats: [])[CogAudioOutputSpatialRefusedKey] as? Bool, true)
	}

	func testMetricsGiveEachChannelItsLevel() {
		var snapshot = CogMeterSnapshot()
		snapshot.frames = 100
		snapshot.peak.0 = 0.5
		snapshot.sumOfSquares.0 = 100 * 0.25
		snapshot.peak.1 = 0.1
		snapshot.sumOfSquares.1 = 100 * 0.01
		let metrics = SignalMetrics(snapshot: snapshot, channels: 2, channelConfig: UInt32(AudioConfigStereo), clippedSamples: 3, underruns: 1,
		                            discontinuities: 0, shallowBufferSeconds: 0.2, deepBufferSeconds: 10, outputLatencySeconds: 0.03)
		XCTAssertEqual(metrics.peaks.map(\.floatValue), [0.5, 0.1])
		XCTAssertEqual(metrics.rms[0].doubleValue, 0.5, accuracy: 1e-9)
		XCTAssertEqual(metrics.rms[1].doubleValue, 0.1, accuracy: 1e-9)
		XCTAssertEqual(metrics.clippedSamples, 3)

		let idle = SignalMetrics(snapshot: CogMeterSnapshot(), channels: 2, channelConfig: 0, clippedSamples: 0, underruns: 0,
		                         discontinuities: 0, shallowBufferSeconds: 0, deepBufferSeconds: 0, outputLatencySeconds: 0)
		XCTAssertTrue(idle.peaks.isEmpty, "nothing metered, no levels")
	}

	func testFormatsTravelAsObjectiveCValues() {
		let value = OutputStatus.value(Pump.asbd(Self.stereo))
		var size = 0
		NSGetSizeAndAlignment(value.objCType, &size, nil)
		XCTAssertEqual(size, MemoryLayout<AudioStreamBasicDescription>.size)

		var format = AudioStreamBasicDescription()
		value.getValue(&format, size: MemoryLayout<AudioStreamBasicDescription>.size)
		XCTAssertEqual(format.mSampleRate, 44100)
		XCTAssertEqual(format.mChannelsPerFrame, 2)
		XCTAssertEqual(format.mBitsPerChannel, 32)
		XCTAssertNotEqual(format.mFormatFlags & kAudioFormatFlagIsFloat, 0)
	}

	/// The pump reports what it does to the audio from where that is heard,
	/// and the feeder records what each track decodes to.
	func testThePumpReportsProcessingWhereItChanges() throws {
		let stereo = SeamSignal.loopable(frames: 44100, sampleRate: 44100)
		let mono = stereo.enumerated().filter { $0.offset % 2 == 0 }.map(\.element)
		let first = EngineTrack(url: URL(string: "memory://stereo")!)
		let second = EngineTrack(url: URL(string: "memory://mono")!, gain: 0.5)
		let decoded = [first.url: (samples: stereo, channels: 2), second.url: (samples: mono, channels: 1)]
		let feeder = try XCTUnwrap(Feeder(outputRate: 44100) { track in
			guard let source = decoded[track.url] else { return nil }
			return MemoryDecoder(samples: source.samples, sampleRate: 44100, channels: source.channels)
		})
		let delegate = ScriptedTracks([second])
		feeder.delegate = delegate
		let pump = try XCTUnwrap(Pump(feeder: feeder, outputFormat: Self.stereo))
		let renderer = try XCTUnwrap(cog_renderer_create(pump.ring))
		defer { cog_renderer_destroy(renderer) }

		feeder.start(with: first)
		pump.start()
		var processing: [(position: UInt64, processing: Pump.Processing)] = []
		var ended = false
		var buffer = [Float](repeating: 0, count: 512 * 2)
		let deadline = Date().addingTimeInterval(30)
		while !ended && Date() < deadline {
			let got = buffer.withUnsafeMutableBufferPointer { cog_renderer_render(renderer, $0.baseAddress!, 512) }
			for entry in pump.presentation.take(through: cog_ring_read_position(pump.ring)) {
				switch entry.event {
				case let .processing(reported): processing.append((entry.position, reported))
				case .endOfStream: ended = true
				default: break
				}
			}
			if got == 0 { Thread.sleep(forTimeInterval: 0.001) }
		}
		pump.stop()
		feeder.stop()

		XCTAssertTrue(ended)
		XCTAssertEqual(processing.map(\.position), [0, 44100])
		XCTAssertEqual(processing.map(\.processing.fitsChannels), [false, true])
		XCTAssertEqual(processing.map(\.processing.output.channels), [2, 1])
		XCTAssertEqual(processing.map(\.processing.appliesGain), [false, true])
		XCTAssertEqual(first.sourceProperties?["channels"] as? Int, 2)
		XCTAssertEqual(second.sourceProperties?["channels"] as? Int, 1)
	}
}
