//
//  UnfairLock.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import os

/// `os_unfair_lock` at a stable heap address, which Swift requires: a lock
/// stored inline in a class or struct may be moved or copied.
///
/// For the engine's worker and control threads only. Never take it on the
/// real-time render thread.
final class UnfairLock {
	private let pointer: UnsafeMutablePointer<os_unfair_lock>

	init() {
		pointer = .allocate(capacity: 1)
		pointer.initialize(to: os_unfair_lock())
	}

	deinit {
		pointer.deinitialize(count: 1)
		pointer.deallocate()
	}

	@inline(__always)
	func withLock<Result>(_ body: () throws -> Result) rethrows -> Result {
		os_unfair_lock_lock(pointer)
		defer { os_unfair_lock_unlock(pointer) }
		return try body()
	}
}
