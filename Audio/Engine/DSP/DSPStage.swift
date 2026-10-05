//
//  DSPStage.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

import Foundation

/// A block of interleaved float audio passing through the DSP chain. Stages
/// may change its channel count (surround, headphone virtualisation) or its
/// frame count (time-stretching).
public final class DSPBuffer {
	public var samples: [Float] = []
	public var frames = 0
	public var format = StreamFormat(sampleRate: 0, channels: 0)

	public init() {}

	/// Replaces the contents with `frames` frames in `format`, reusing the
	/// storage.
	public func resize(frames: Int, format: StreamFormat) {
		self.frames = frames
		self.format = format
		let count = frames * format.channels
		if samples.count < count {
			samples.append(contentsOf: repeatElement(0, count: count - samples.count))
		}
	}
}

/// One transform the pump runs on the DSP thread, as a plain object rather
/// than a node with its own thread and buffer. Stages run in order between
/// the deep and shallow rings; settings reach them through their own
/// observers and are picked up on the DSP thread at the next block.
public protocol DSPStage: AnyObject {
	/// Whether the stage does anything with its current settings. An
	/// inactive stage is skipped, and its output format is its input format.
	var isActive: Bool { get }

	/// Prepares for `input`, returning the format the stage produces. Called
	/// before the first block, whenever the input format changes, and when
	/// the stage becomes active.
	func configure(input: StreamFormat) -> StreamFormat

	/// Processes `buffer` in place (possibly changing its frames or format).
	func process(_ buffer: DSPBuffer)

	/// Forgets filter history, as after a seek.
	func reset()

	/// Processes `buffer` and appends everything the stage still holds (its
	/// latency, or input waiting for a full block), because nothing more
	/// will follow in this format: the end of the stream, or a format change.
	func drain(_ buffer: DSPBuffer)

	/// Track time per output frame: the tempo for a time stretcher, 1 for
	/// everything else.
	var timeRatio: Double { get }

	/// Output frames the stage still owes for input it has already taken
	/// (FreeSurround's block, a stretcher's latency). Events are placed
	/// after them so they line up with the audio.
	var pendingFrames: Int { get }

	/// The settings the stage applied to the last block, for the output
	/// status; nil for a stage that does not change the audio. DSP thread.
	var inspection: StageInspection? { get }
}

public extension DSPStage {
	var timeRatio: Double { 1 }
	var pendingFrames: Int { 0 }
	var inspection: StageInspection? { nil }

	/// Stages that hold nothing back just process.
	func drain(_ buffer: DSPBuffer) {
		process(buffer)
	}
}

/// What a stage did to a block, as the output status describes it. Kept
/// small and comparable: the pump reports processing again whenever it
/// changes, so it must not change with every block.
public enum StageInspection: Equatable {
	/// The time stretcher's engine (`rubberbandEngine`), and the tempo and
	/// pitch it applied (pitch 1 for varispeed, whose pitch is its tempo).
	case timeStretch(engine: String, tempo: Double, pitch: Double)
	/// FreeSurround: `upmixes` is false while the input is not stereo, which
	/// passes through; `output` is what it produced.
	case freeSurround(upmixes: Bool, output: StreamFormat)
	/// The equalizer's preamp and its 31 band gains, in dB.
	case equalizer(preampDB: Float, gainsDB: [Float])
}
