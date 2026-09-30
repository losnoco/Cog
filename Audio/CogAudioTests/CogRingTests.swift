//
//  CogRingTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/29/26.
//

import CogAudio
import XCTest

final class CogRingTests: XCTestCase {
	private var ring: OpaquePointer!

	override func tearDown() {
		cog_ring_destroy(ring)
		ring = nil
		super.tearDown()
	}

	/// Frames whose samples encode their own index, so a reader can tell
	/// exactly which frames it received.
	private static func frames(from start: Int, count: Int, channels: Int = 2) -> [Float] {
		var samples = [Float](repeating: 0, count: count * channels)
		for frame in 0..<count {
			for channel in 0..<channels {
				samples[frame * channels + channel] = Float((start + frame) * 10 + channel)
			}
		}
		return samples
	}

	@discardableResult
	private func write(_ samples: [Float]) -> Int {
		samples.withUnsafeBufferPointer { cog_ring_write(ring, $0.baseAddress!, samples.count / Int(cog_ring_channels(ring))) }
	}

	private func read(_ count: Int) -> [Float] {
		let channels = Int(cog_ring_channels(ring))
		var samples = [Float](repeating: -1, count: count * channels)
		let got = samples.withUnsafeMutableBufferPointer { cog_ring_read(ring, $0.baseAddress, count) }
		return Array(samples[0..<(got * channels)])
	}

	func testCreationRejectsEmptyShapesAndRoundsCapacityUp() {
		XCTAssertNil(cog_ring_create(0, 2))
		XCTAssertNil(cog_ring_create(16, 0))
		ring = cog_ring_create(1000, 2)
		XCTAssertEqual(cog_ring_capacity(ring), 1024)
		XCTAssertEqual(cog_ring_channels(ring), 2)
		XCTAssertEqual(cog_ring_writable(ring), 1024)
		XCTAssertEqual(cog_ring_readable(ring), 0)
	}

	func testFramesSurviveWrappingAroundTheEnd() {
		ring = cog_ring_create(8, 2)
		XCTAssertEqual(write(Self.frames(from: 0, count: 5)), 5)
		XCTAssertEqual(read(5), Self.frames(from: 0, count: 5))
		// Starts at slot 5 of 8, so this write wraps.
		XCTAssertEqual(write(Self.frames(from: 5, count: 6)), 6)
		XCTAssertEqual(read(6), Self.frames(from: 5, count: 6))
		XCTAssertEqual(cog_ring_write_position(ring), 11)
		XCTAssertEqual(cog_ring_read_position(ring), 11)
	}

	func testWritesStopAtCapacity() {
		ring = cog_ring_create(8, 2)
		XCTAssertEqual(write(Self.frames(from: 0, count: 12)), 8)
		XCTAssertEqual(cog_ring_writable(ring), 0)
		XCTAssertEqual(write(Self.frames(from: 8, count: 1)), 0)
		XCTAssertEqual(read(20), Self.frames(from: 0, count: 8))
	}

	func testReadingIntoNothingDiscards() {
		ring = cog_ring_create(8, 2)
		write(Self.frames(from: 0, count: 6))
		XCTAssertEqual(cog_ring_read(ring, nil, 4), 4)
		XCTAssertEqual(read(8), Self.frames(from: 4, count: 2))
	}

	func testFlushDiscardsOnlyWhatWasWrittenBeforeTheRequest() {
		ring = cog_ring_create(64, 2)
		write(Self.frames(from: 0, count: 10))
		let epoch = cog_ring_request_flush(ring)
		// The producer keeps going without waiting for the consumer.
		write(Self.frames(from: 100, count: 3))
		XCTAssertLessThan(cog_ring_flush_acknowledged(ring), epoch)

		XCTAssertEqual(cog_ring_honour_flush(ring), 10)
		XCTAssertEqual(cog_ring_flush_acknowledged(ring), epoch)
		XCTAssertEqual(read(8), Self.frames(from: 100, count: 3))
		XCTAssertEqual(cog_ring_honour_flush(ring), 0, "a flush is honoured once")
	}

	func testSeveralRequestsCollapseIntoTheLatest() {
		ring = cog_ring_create(64, 2)
		write(Self.frames(from: 0, count: 4))
		cog_ring_request_flush(ring)
		write(Self.frames(from: 4, count: 4))
		let latest = cog_ring_request_flush(ring)
		write(Self.frames(from: 8, count: 2))

		XCTAssertEqual(cog_ring_honour_flush(ring), 8)
		XCTAssertEqual(cog_ring_flush_acknowledged(ring), latest)
		XCTAssertEqual(read(8), Self.frames(from: 8, count: 2))
	}

	func testFlushAfterTheConsumerCaughtUpDiscardsNothing() {
		ring = cog_ring_create(16, 2)
		write(Self.frames(from: 0, count: 4))
		_ = read(4)
		cog_ring_request_flush(ring)
		XCTAssertEqual(cog_ring_honour_flush(ring), 0)
		XCTAssertEqual(cog_ring_read_position(ring), 4)
	}

	// MARK: - Two threads

	/// One producer and one consumer, both moving odd-sized blocks through a
	/// small ring as fast as they can. Every frame must arrive once, in order.
	func testConcurrentTransferIsLosslessAndOrdered() {
		ring = cog_ring_create(1024, 2)
		let total = 2_000_000
		let producerDone = expectation(description: "producer")
		let ring = self.ring!

		Thread.detachNewThread {
			var next = 0
			var block = [Float](repeating: 0, count: 777 * 2)
			while next < total {
				let count = min(1 + (next * 7919) % 777, total - next)
				for frame in 0..<count {
					block[frame * 2] = Float(bitPattern: UInt32(truncatingIfNeeded: next + frame))
					block[frame * 2 + 1] = Float(bitPattern: ~UInt32(truncatingIfNeeded: next + frame))
				}
				var offset = 0
				while offset < count {
					offset += block.withUnsafeBufferPointer { cog_ring_write(ring, $0.baseAddress! + offset * 2, count - offset) }
				}
				next += count
			}
			producerDone.fulfill()
		}

		var expected = 0
		var mismatches = 0
		var block = [Float](repeating: 0, count: 613 * 2)
		while expected < total {
			let got = block.withUnsafeMutableBufferPointer { cog_ring_read(ring, $0.baseAddress, 1 + expected % 613) }
			for frame in 0..<got {
				let index = UInt32(truncatingIfNeeded: expected + frame)
				if block[frame * 2].bitPattern != index || block[frame * 2 + 1].bitPattern != ~index {
					mismatches += 1
				}
			}
			expected += got
		}
		wait(for: [producerDone], timeout: 30)
		XCTAssertEqual(mismatches, 0)
		XCTAssertEqual(cog_ring_read_position(ring), UInt64(total))
	}

	/// The producer requests flushes while writing. The consumer must only
	/// ever move forward through the sequence, and every jump must land on
	/// a position the producer flushed to.
	func testConcurrentFlushesOnlySkipForward() {
		ring = cog_ring_create(512, 1)
		let total = 1_000_000
		let producerDone = expectation(description: "producer")
		let ring = self.ring!
		let targets = NSMutableSet()
		let targetsLock = NSLock()

		Thread.detachNewThread {
			var next = 0
			var block = [Float](repeating: 0, count: 300)
			while next < total {
				let count = min(300, total - next)
				for frame in 0..<count {
					block[frame] = Float(bitPattern: UInt32(next + frame))
				}
				var offset = 0
				while offset < count {
					offset += block.withUnsafeBufferPointer { cog_ring_write(ring, $0.baseAddress! + offset, count - offset) }
				}
				next += count
				if next % 30_000 == 0 {
					targetsLock.lock()
					targets.add(next)
					targetsLock.unlock()
					cog_ring_request_flush(ring)
				}
			}
			producerDone.fulfill()
		}

		var last = -1
		var backwards = 0
		var unexplainedJumps = 0
		var flushes = 0
		var block = [Float](repeating: 0, count: 256)
		while last < total - 1 {
			if cog_ring_honour_flush(ring) > 0 {
				flushes += 1
			}
			let got = block.withUnsafeMutableBufferPointer { cog_ring_read(ring, $0.baseAddress, 256) }
			for frame in 0..<got {
				let value = Int(block[frame].bitPattern)
				if value <= last {
					backwards += 1
				} else if value != last + 1 {
					targetsLock.lock()
					let known = targets.contains(value)
					targetsLock.unlock()
					if !known {
						unexplainedJumps += 1
					}
				}
				last = value
			}
		}
		wait(for: [producerDone], timeout: 30)
		XCTAssertEqual(backwards, 0)
		XCTAssertEqual(unexplainedJumps, 0)
		print("[CogRing] \(flushes) flushes honoured with discards")
	}
}
