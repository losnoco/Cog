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
}
