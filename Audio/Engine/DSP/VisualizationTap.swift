//
//  VisualizationTap.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

import Foundation

/// Feeds the spectrum and oscilloscope (as `VisualizationNode`): a
/// pass-through stage that folds each block to mono with the same
/// `DownmixProcessor`, resamples it to 44.1 kHz and posts it to the shared
/// `VisualizationController`. The engine tells the controller how far behind
/// the device the posted audio is, so the display lines up with what is
/// heard.
final class VisualizationTap: DSPStage {
	static let visualizationRate = 44100.0

	private let controller = VisualizationController.shared()
	private var inputFormat: StreamFormat?
	private var downmix: DownmixProcessor?
	private var resampler: soxr_t?
	private var mono: [Float] = []
	private var resampled: [Float] = []

	deinit {
		if let resampler {
			soxr_delete(resampler)
		}
	}

	var isActive: Bool { true }

	func configure(input: StreamFormat) -> StreamFormat {
		guard input != inputFormat else { return input }
		inputFormat = input
		controller.postSampleRate(Self.visualizationRate)

		let monoFormat = StreamFormat(sampleRate: input.sampleRate, channels: 1, channelConfig: UInt32(AudioChannelFrontCenter))
		let inputConfig = input.channelConfig != 0 ? input.channelConfig : AudioChunk.guessChannelConfig(UInt32(input.channels))
		downmix = input.channels == 1 ? nil : DownmixProcessor(inputFormat: Pump.asbd(input), inputConfig: inputConfig,
		                                                    andOutputFormat: Pump.asbd(monoFormat), outputConfig: monoFormat.channelConfig)
		if let resampler {
			soxr_delete(resampler)
			self.resampler = nil
		}
		if input.sampleRate != Self.visualizationRate {
			var error: soxr_error_t?
			var ioSpec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I)
			var qualitySpec = soxr_quality_spec(UInt(SOXR_QQ), 0)
			resampler = soxr_create(input.sampleRate, Self.visualizationRate, 1, &error, &ioSpec, &qualitySpec, nil)
		}
		return input
	}

	func process(_ buffer: DSPBuffer) {
		let frames = buffer.frames
		guard frames > 0 else { return }

		if mono.count < frames {
			mono = [Float](repeating: 0, count: frames)
		}
		if let downmix {
			buffer.samples.withUnsafeBufferPointer { input in
				mono.withUnsafeMutableBufferPointer { output in
					downmix.process(input.baseAddress!, frameCount: frames, output: output.baseAddress!)
				}
			}
		} else {
			for i in 0..<frames {
				mono[i] = buffer.samples[i]
			}
		}

		guard let resampler else {
			mono.withUnsafeBufferPointer { controller.postVisPCM($0.baseAddress!, amount: Int32(frames)) }
			return
		}
		let capacity = Int((Double(frames) * Self.visualizationRate / buffer.format.sampleRate).rounded(.up)) + 256
		if resampled.count < capacity {
			resampled = [Float](repeating: 0, count: capacity)
		}
		var inputDone = 0
		var outputDone = 0
		mono.withUnsafeBufferPointer { input in
			resampled.withUnsafeMutableBufferPointer { output in
				_ = soxr_process(resampler, input.baseAddress!, frames, &inputDone, output.baseAddress!, capacity, &outputDone)
			}
		}
		if outputDone > 0 {
			resampled.withUnsafeBufferPointer { controller.postVisPCM($0.baseAddress!, amount: Int32(outputDone)) }
		}
	}

	func reset() {}
}
