//
//  Timeline.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import Foundation

/// A track the engine plays: what to open, and what the player needs back
/// when it starts.
public final class EngineTrack: NSObject {
	public let url: URL
	public let userInfo: Any?

	private let lock = UnfairLock()
	private var storedGain: Float
	private var storedRGInfo: [AnyHashable: Any]?

	/// Linear ReplayGain (or volume scaling), applied on the DSP thread so a
	/// change is heard within the shallow ring rather than after the deep
	/// one. May change during playback (tags that load late, or a new
	/// volume scaling setting); readable from any thread.
	public var gain: Float {
		get { lock.withLock { storedGain } }
		set { lock.withLock { storedGain = newValue } }
	}

	/// The ReplayGain info the gain was computed from, if any.
	public var rgInfo: [AnyHashable: Any]? {
		lock.withLock { storedRGInfo }
	}

	public init(url: URL, userInfo: Any? = nil, gain: Float = 1) {
		self.url = url
		self.userInfo = userInfo
		storedGain = gain
	}

	/// Replaces the ReplayGain info and recomputes the gain from it and the
	/// current volume scaling setting.
	public func update(rgInfo: [AnyHashable: Any]?) {
		let gain = ReplayGain.linearGain(rgInfo: rgInfo)
		lock.withLock {
			storedRGInfo = rgInfo
			storedGain = gain
		}
	}

	/// Whether this track belongs to the playlist entry `userInfo`.
	public func belongs(to userInfo: Any?) -> Bool {
		guard let mine = self.userInfo as AnyObject?, let theirs = userInfo as AnyObject? else { return false }
		return mine === theirs
	}
}

/// Something that happens at a point in the engine's output.
public enum TimelineEvent {
	/// Frames from here on have this format.
	case format(StreamFormat)
	/// `track` becomes audible here, `offset` seconds into it (non-zero after
	/// a seek).
	case trackStart(EngineTrack, offset: Double)
	/// Nothing follows.
	case endOfStream
}

public struct TimelineEntry {
	/// Output frame at which the event takes effect, counted from the start
	/// of the epoch.
	public let frame: UInt64
	/// The ring flush epoch the frame count belongs to. A seek flushes the
	/// ring and starts a new epoch whose frames count from zero again.
	public let epoch: UInt64
	public let event: TimelineEvent
}

/// Events placed at output-frame positions, passed from the feeder to the
/// DSP thread alongside the audio itself.
///
/// Neither side is a real-time thread, so a lock is fine here.
public final class Timeline {
	private let lock = UnfairLock()
	private var entries: [TimelineEntry] = []

	public init() {}

	func append(_ event: TimelineEvent, at frame: UInt64, epoch: UInt64) {
		lock.withLock {
			entries.append(TimelineEntry(frame: frame, epoch: epoch, event: event))
		}
	}

	/// Removes and returns, in order, the entries of `epoch` at or before
	/// `frame`. Entries of earlier epochs are stale and dropped.
	public func take(through frame: UInt64, epoch: UInt64) -> [TimelineEntry] {
		lock.withLock {
			entries.removeAll { $0.epoch < epoch }
			let due = entries.prefix { $0.epoch == epoch && $0.frame <= frame }
			entries.removeFirst(due.count)
			return Array(due)
		}
	}

	/// The frame of the next entry of `epoch`, if any, so a reader can stop
	/// exactly there.
	public func nextFrame(epoch: UInt64) -> UInt64? {
		lock.withLock {
			entries.first { $0.epoch == epoch }?.frame
		}
	}

	func removeAll() {
		lock.withLock {
			entries.removeAll()
		}
	}
}
