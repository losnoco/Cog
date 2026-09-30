//
//  StreamConverter.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

@_implementationOnly import CogAudioEngineInternal
import Foundation

/// The shape of interleaved float audio flowing through the engine.
public struct StreamFormat: Equatable {
	public var sampleRate: Double
	public var channels: Int
	public var channelConfig: UInt32

	public init(sampleRate: Double, channels: Int, channelConfig: UInt32 = 0) {
		self.sampleRate = sampleRate
		self.channels = channels
		self.channelConfig = channelConfig
	}

	/// Two formats a resampler can run straight through: the channel layout
	/// may be relabelled, but the rate and channel count must not change.
	func resamplesContinuously(into other: StreamFormat) -> Bool {
		sampleRate == other.sampleRate && channels == other.channels
	}
}

/// Resamples interleaved float audio to one output rate for the whole of a
/// playback session.
///
/// One soxr instance runs across track boundaries: a new track in the same
/// rate and channel count is just more input, so the join is exactly what a
/// single uninterrupted resample would produce. Only a real format change, or
/// the end of playback, drains the resampler; the edges that creates are
/// hidden with LPC extrapolation (lvqcl's, as in `ConverterNode`), predicted
/// backward before the first frame and forward after the last, and trimmed
/// back off the output. Matching rates bypass soxr and stay bit-exact.
///
/// Not thread-safe: the feeder thread owns it.
public final class StreamConverter {
	public typealias Sink = (_ samples: UnsafeBufferPointer<Float>, _ format: StreamFormat) -> Void

	public let outputRate: Double
	private let quality: UInt

	/// The input format of the current run, or nil before the first frame and
	/// after a drain.
	public private(set) var inputFormat: StreamFormat?

	/// Output frames emitted since creation (or the last `reset`).
	public private(set) var outputFrames: UInt64 = 0

	private var resampler: soxr_t?
	private var ratio: Double = 1

	// Position bookkeeping for the current run.
	private var runInputFrames: UInt64 = 0
	private var runOutputBase: UInt64 = 0

	// LPC edge handling.
	private var primeLength = 0
	private var leadInFrames = 0
	private var leadOutDrop = 0
	private var leadInDone = false
	private var leadInDropRemaining = 0
	private var pending: [Float] = [] // input held back until the lead-in has enough to predict from
	private var history: [Float] = [] // the last `primeLength` input frames, for the lead-out
	private var extrapolateBuffer: UnsafeMutableRawPointer?
	private var extrapolateBufferSize = 0

	private var scratchIn: [Float] = []
	private var scratchOut: [Float] = []

	public init(outputRate: Double, quality: UInt = UInt(SOXR_HQ)) {
		self.outputRate = outputRate
		self.quality = quality
	}

	deinit {
		closeResampler()
		free(extrapolateBuffer)
	}

	/// The output frame at which input fed from now on begins. Place a track
	/// boundary's marker here before feeding the new track: sample k of the
	/// output sits at time k / outputRate, so the first output frame at or
	/// after the input's current time is the join.
	public var outputPositionOfNextInput: UInt64 {
		guard let format = inputFormat else { return outputFrames }
		let inputRate = UInt64(format.sampleRate.rounded())
		let outRate = UInt64(outputRate.rounded())
		guard inputRate > 0, inputRate != outRate else { return runOutputBase + runInputFrames }
		return runOutputBase + (runInputFrames * outRate + inputRate - 1) / inputRate
	}

	/// Feeds `frames` frames of `format`, scaled by `gain`, and hands every
	/// output frame produced to `sink`.
	public func process(_ samples: UnsafeBufferPointer<Float>, format: StreamFormat, gain: Float = 1, sink: Sink) {
		if let current = inputFormat, !current.resamplesContinuously(into: format) {
			drain(sink: sink)
		}
		if inputFormat == nil {
			configure(for: format)
		}
		inputFormat = format

		let channels = format.channels
		let frames = samples.count / channels
		guard frames > 0 else { return }
		runInputFrames += UInt64(frames)

		scratchIn.removeAll(keepingCapacity: true)
		scratchIn.append(contentsOf: samples)
		if gain != 1 {
			for i in scratchIn.indices {
				scratchIn[i] *= gain
			}
		}

		if resampler == nil {
			emit(scratchIn, sink: sink)
			return
		}

		rememberHistory(scratchIn, channels: channels)

		if !leadInDone {
			pending.append(contentsOf: scratchIn)
			if pending.count / channels < primeLength { return }
			startWithLeadIn(sink: sink)
			return
		}

		resample(scratchIn, sink: sink)
	}

	/// Flushes the resampler at the end of playback (or before a format
	/// change), extrapolating past the last frame so the final samples are as
	/// clean as the rest, then trimming the extrapolation back off.
	public func drain(sink: Sink) {
		guard let format = inputFormat else { return }
		defer {
			closeResampler()
			inputFormat = nil
		}
		guard resampler != nil else { return }
		let channels = format.channels

		if !leadInDone {
			if pending.isEmpty { return }
			startWithLeadIn(sink: sink)
		}

		// Predict leadInFrames past the end from the recorded history.
		let historyFrames = history.count / channels
		var tail = history + [Float](repeating: 0, count: leadInFrames * channels)
		tail.withUnsafeMutableBufferPointer { buffer in
			lpc_extrapolate_fwd(buffer.baseAddress!, historyFrames, min(historyFrames, primeLength), Int32(channels), Int32(LPC_ORDER), leadInFrames, &extrapolateBuffer, &extrapolateBufferSize)
		}
		let extrapolated = Array(tail[(historyFrames * channels)...])

		scratchOut.removeAll(keepingCapacity: true)
		runResampler(extrapolated, into: &scratchOut)
		flushResampler(into: &scratchOut)

		let producedFrames = scratchOut.count / channels
		let keepFrames = max(0, producedFrames - leadOutDrop)
		let keep = Array(scratchOut[0..<(keepFrames * channels)])
		emit(dropLeadIn(keep, channels: channels), sink: sink)
	}

	/// Forgets the current run without draining it, as for a seek. The next
	/// input starts a fresh run with its own lead-in.
	public func reset() {
		closeResampler()
		inputFormat = nil
		outputFrames = 0
		runOutputBase = 0
		runInputFrames = 0
	}

	// MARK: - Run lifecycle

	private func configure(for format: StreamFormat) {
		closeResampler()
		runOutputBase = outputFrames
		runInputFrames = 0
		pending.removeAll(keepingCapacity: true)
		history.removeAll(keepingCapacity: true)
		leadInDone = false
		leadInDropRemaining = 0

		guard format.sampleRate != outputRate else {
			ratio = 1
			return
		}
		ratio = outputRate / format.sampleRate

		var error: soxr_error_t?
		var ioSpec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I)
		var qualitySpec = soxr_quality_spec(quality, 0)
		var runtimeSpec = soxr_runtime_spec(0)
		resampler = soxr_create(format.sampleRate, outputRate, UInt32(format.channels), &error, &ioSpec, &qualitySpec, &runtimeSpec)
		if error != nil {
			resampler = nil
		}

		primeLength = Self.primeLength(for: format.sampleRate)
		(leadInFrames, leadOutDrop) = Self.paddingLengths(inputRate: format.sampleRate, outputRate: outputRate)
	}

	private func startWithLeadIn(sink: Sink) {
		guard let channels = inputFormat?.channels else { return }
		let frames = pending.count / channels
		var buffer = [Float](repeating: 0, count: leadInFrames * channels) + pending
		buffer.withUnsafeMutableBufferPointer { pointer in
			lpc_extrapolate_bkwd(pointer.baseAddress! + leadInFrames * channels, frames, min(frames, primeLength), Int32(channels), Int32(LPC_ORDER), leadInFrames, &extrapolateBuffer, &extrapolateBufferSize)
		}
		pending.removeAll(keepingCapacity: true)
		leadInDone = true
		leadInDropRemaining = leadOutDrop
		resample(buffer, sink: sink)
	}

	private func closeResampler() {
		if let resampler {
			soxr_delete(resampler)
		}
		resampler = nil
	}

	// MARK: - Resampling

	private func resample(_ input: [Float], sink: Sink) {
		guard let channels = inputFormat?.channels else { return }
		scratchOut.removeAll(keepingCapacity: true)
		runResampler(input, into: &scratchOut)
		emit(dropLeadIn(scratchOut, channels: channels), sink: sink)
	}

	private func runResampler(_ input: [Float], into output: inout [Float]) {
		guard let resampler, let channels = inputFormat?.channels else { return }
		var offset = 0
		let frames = input.count / channels
		input.withUnsafeBufferPointer { inputPointer in
			while offset < frames {
				let remaining = frames - offset
				let capacity = Int((Double(remaining) * ratio).rounded(.up)) + Int(soxr_delay(resampler).rounded(.up)) + 64
				let start = output.count
				output.append(contentsOf: repeatElement(0, count: capacity * channels))
				var inputDone = 0
				var outputDone = 0
				output.withUnsafeMutableBufferPointer { outputPointer in
					_ = soxr_process(resampler, inputPointer.baseAddress! + offset * channels, remaining, &inputDone, outputPointer.baseAddress! + start, capacity, &outputDone)
				}
				output.removeLast((capacity - outputDone) * channels)
				offset += inputDone
				if inputDone == 0 && outputDone == 0 { break }
			}
		}
	}

	private func flushResampler(into output: inout [Float]) {
		guard let resampler, let channels = inputFormat?.channels else { return }
		// A flushed soxr cannot take more input; the caller discards it.
		while true {
			let capacity = max(1024, Int(soxr_delay(resampler).rounded(.up)) + 64)
			let start = output.count
			output.append(contentsOf: repeatElement(0, count: capacity * channels))
			var inputDone = 0
			var outputDone = 0
			output.withUnsafeMutableBufferPointer { outputPointer in
				_ = soxr_process(resampler, nil, 0, &inputDone, outputPointer.baseAddress! + start, capacity, &outputDone)
			}
			output.removeLast((capacity - outputDone) * channels)
			if outputDone == 0 { break }
		}
	}

	/// Removes the output of the backward extrapolation from the front of the
	/// run's output, however many calls it spans.
	private func dropLeadIn(_ output: [Float], channels: Int) -> [Float] {
		guard leadInDropRemaining > 0 else { return output }
		let frames = output.count / channels
		let drop = min(frames, leadInDropRemaining)
		leadInDropRemaining -= drop
		return Array(output[(drop * channels)...])
	}

	private func rememberHistory(_ input: [Float], channels: Int) {
		history.append(contentsOf: input)
		let excess = history.count - primeLength * channels
		if excess > 0 {
			history.removeFirst(excess)
		}
	}

	private func emit(_ samples: [Float], sink: Sink) {
		guard let format = inputFormat, !samples.isEmpty else { return }
		outputFrames += UInt64(samples.count / format.channels)
		let outputFormat = StreamFormat(sampleRate: outputRate, channels: format.channels, channelConfig: format.channelConfig)
		samples.withUnsafeBufferPointer { sink($0, outputFormat) }
	}

	// MARK: - Edge lengths

	/// About 1/20 s of input to predict from, as in `ConverterNode`.
	static func primeLength(for sampleRate: Double) -> Int {
		let length = min(max(Int(sampleRate / 20), 1024), 16384)
		return max(length, 2 * Int(LPC_ORDER) + 1)
	}

	/// lvqcl's `samples_len`: equal durations in both rates, about 1/20 s,
	/// scaled down to fit 8192 frames, so the padding in and the trim out
	/// cover exactly the same time.
	static func paddingLengths(inputRate: Double, outputRate: Double, n: UInt64 = 20, m: UInt64 = 8192) -> (input: Int, output: Int) {
		var r1 = UInt64(inputRate.rounded())
		var r2 = UInt64(outputRate.rounded())
		func gcd(_ a: UInt64, _ b: UInt64) -> UInt64 { b == 0 ? a : gcd(b, a % b) }
		let v = gcd(r1, r2)
		guard v > 0 else { return (Int(r1), Int(r2)) }
		r1 /= v
		r2 /= v
		var scale = (v + n - 1) / n
		let largest = max(r1, r2)
		if largest * scale > m {
			scale = m / largest
		}
		scale = max(scale, 1)
		return (Int(r1 * scale), Int(r2 * scale))
	}
}
