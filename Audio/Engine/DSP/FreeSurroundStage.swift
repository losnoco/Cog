//
//  FreeSurroundStage.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

import Foundation

/// FreeSurround's stereo-to-5.1 upmix (as `DSPFSurroundNode`), enabled by
/// `enableFSurround`. Anything other than stereo passes through unchanged.
///
/// The decoder works on fixed blocks of `FSurroundChunkSize` frames and its
/// output lags its input by half a block. The stage keeps a FIFO so the
/// pump's blocks may be any size, drops that half-block lag once after each
/// start, and on `drain` pushes the rest out with silence and cuts the output
/// to exactly the input's length, so nothing is padded into the middle of the
/// stream and nothing is lost at its end.
final class FreeSurroundStage: NSObject, DSPStage {
	static let blockFrames = Int(FSurroundChunkSize)
	static let latencyFrames = blockFrames / 2

	private let lock = UnfairLock()
	private var enabled: Bool

	// DSP thread only.
	private var filter: FSurroundFilter?
	private var inputFormat: StreamFormat?
	private var outputFormat: StreamFormat?
	private var pending: [Float] = []
	private var produced: [Float] = []
	private var scratch: [Float] = []
	private var latencyToDrop = 0
	private var framesIn = 0
	private var framesOut = 0

	override init() {
		enabled = UserDefaults.standard.bool(forKey: "enableFSurround")
		super.init()
		UserDefaults.standard.addObserver(self, forKeyPath: "enableFSurround", options: [], context: &Self.context)
	}

	deinit {
		UserDefaults.standard.removeObserver(self, forKeyPath: "enableFSurround", context: &Self.context)
	}

	private static var context = 0

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard context == &Self.context else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}
		let enabled = UserDefaults.standard.bool(forKey: "enableFSurround")
		lock.withLock { self.enabled = enabled }
	}

	var isActive: Bool {
		lock.withLock { enabled }
	}

	var pendingFrames: Int {
		filter == nil ? 0 : max(0, framesIn - framesOut)
	}

	var inspection: StageInspection? {
		guard let outputFormat else { return nil }
		return .freeSurround(upmixes: filter != nil, output: outputFormat)
	}

	func configure(input: StreamFormat) -> StreamFormat {
		inputFormat = input
		pending.removeAll(keepingCapacity: true)
		framesIn = 0
		framesOut = 0
		guard input.channels == 2, let filter = FSurroundFilter(sampleRate: input.sampleRate) else {
			self.filter = nil
			outputFormat = input
			return input
		}
		self.filter = filter
		let output = StreamFormat(sampleRate: input.sampleRate, channels: Int(filter.channelCount()), channelConfig: filter.channelConfig())
		outputFormat = output
		latencyToDrop = Self.latencyFrames
		scratch = [Float](repeating: 0, count: Self.blockFrames * output.channels)
		return output
	}

	func process(_ buffer: DSPBuffer) {
		guard filter != nil else { return }
		take(buffer)
		runFullBlocks()
		give(to: buffer)
	}

	func drain(_ buffer: DSPBuffer) {
		guard filter != nil, let outputFormat else { return }
		take(buffer)
		runFullBlocks()
		// Push what is left, and the decoder's lag, out with silence.
		while framesOut + produced.count / outputFormat.channels < framesIn {
			pending.append(contentsOf: repeatElement(0, count: Self.blockFrames * 2 - pending.count % (Self.blockFrames * 2)))
			runFullBlocks()
		}
		let wanted = framesIn - framesOut
		if produced.count > wanted * outputFormat.channels {
			produced.removeLast(produced.count - wanted * outputFormat.channels)
		}
		give(to: buffer)
		// A drained filter starts over, lag and all, if more input comes.
		_ = configure(input: inputFormat ?? buffer.format)
	}

	func reset() {
		if let inputFormat {
			_ = configure(input: inputFormat)
		}
	}

	// MARK: -

	private func take(_ buffer: DSPBuffer) {
		pending.append(contentsOf: buffer.samples[0..<(buffer.frames * 2)])
		framesIn += buffer.frames
	}

	private func runFullBlocks() {
		guard let filter, let outputFormat else { return }
		let blockSamples = Self.blockFrames * 2
		var offset = 0
		while pending.count - offset >= blockSamples {
			pending.withUnsafeBufferPointer { input in
				scratch.withUnsafeMutableBufferPointer { output in
					filter.process(input.baseAddress! + offset, output: output.baseAddress!, count: UInt32(Self.blockFrames))
				}
			}
			offset += blockSamples
			let drop = min(latencyToDrop, Self.blockFrames)
			latencyToDrop -= drop
			produced.append(contentsOf: scratch[(drop * outputFormat.channels)...])
		}
		if offset > 0 {
			pending.removeFirst(offset)
		}
	}

	private func give(to buffer: DSPBuffer) {
		guard let outputFormat else { return }
		let frames = produced.count / outputFormat.channels
		buffer.resize(frames: frames, format: outputFormat)
		for i in 0..<(frames * outputFormat.channels) {
			buffer.samples[i] = produced[i]
		}
		produced.removeAll(keepingCapacity: true)
		framesOut += frames
	}
}
