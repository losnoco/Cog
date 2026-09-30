//
//  EngineCapture.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

import Foundation

/// Records the audio passing one point of the engine to a 32-bit float WAV,
/// for finding where a click or pop comes from. Off unless the hidden
/// `engineCaptureDirectory` setting names a folder.
///
/// The engine thread only copies into a ring; a background thread writes
/// the file, so recording cannot make the engine late. Should the writer
/// fall behind, frames are dropped and counted, never waited for. Markers
/// (track starts) go to a text file beside the WAV, by frame.
final class EngineCapture {
	/// The folder captures go to, if capturing is on.
	static var directory: URL? {
		guard let path = UserDefaults.standard.string(forKey: "engineCaptureDirectory"), !path.isEmpty else { return nil }
		return URL(fileURLWithPath: (path as NSString).expandingTildeInPath, isDirectory: true)
	}

	let format: StreamFormat
	let url: URL
	private let ring: OpaquePointer
	private let file: FileHandle
	private let markers: FileHandle
	/// Frames handed to the capture, written or dropped: the frame numbering
	/// markers use. Engine thread only.
	private(set) var frames: UInt64 = 0
	private let dropped = LockedValue<UInt64>(0)
	private let lock = UnfairLock()
	private var running = true
	private let finished = DispatchSemaphore(value: 0)
	private var dataBytes: UInt64 = 0

	/// Starts a capture named `point` (with the time and format) in
	/// `directory`, or nil if the file cannot be created.
	init?(point: String, format: StreamFormat, directory: URL) {
		self.format = format
		try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
		let stamp = Self.stampFormatter.string(from: Date())
		let base = "\(stamp)-\(point)-\(Int(format.sampleRate))Hz-\(format.channels)ch"
		url = directory.appendingPathComponent(base + ".wav")
		let markersURL = directory.appendingPathComponent(base + ".txt")
		guard FileManager.default.createFile(atPath: url.path, contents: Self.header(format: format, dataBytes: 0)),
		      FileManager.default.createFile(atPath: markersURL.path, contents: nil),
		      let file = FileHandle(forWritingAtPath: url.path),
		      let markers = FileHandle(forWritingAtPath: markersURL.path),
		      let ring = cog_ring_create(Int(format.sampleRate * 4), UInt32(format.channels)) else {
			return nil
		}
		self.file = file
		self.markers = markers
		self.ring = ring
		file.seekToEndOfFile()
		let thread = Thread { [weak self] in self?.drain() }
		thread.name = "Cog capture \(point)"
		thread.qualityOfService = .utility
		thread.start()
		EngineLog.logger.notice("Capturing \(point, privacy: .public) to \(self.url.path, privacy: .public)")
	}

	deinit {
		finish()
		cog_ring_destroy(ring)
	}

	/// Engine thread: records `count` interleaved frames.
	func record(_ samples: UnsafePointer<Float>, frames count: Int) {
		let written = cog_ring_write(ring, samples, count)
		if written < count {
			dropped.withLock { $0 += UInt64(count - written) }
		}
		frames += UInt64(count)
	}

	/// Engine thread: notes `label` at the next frame recorded, or `frame` if
	/// given.
	func mark(_ label: String, at frame: UInt64? = nil) {
		let line = "\(frame ?? frames)\t\(label)\n"
		markers.write(Data(line.utf8))
	}

	/// Stops, writes out what is left and completes the WAV header.
	func finish() {
		let wasRunning: Bool = lock.withLock {
			defer { running = false }
			return running
		}
		guard wasRunning else { return }
		finished.wait()
		file.seek(toFileOffset: 0)
		file.write(Self.header(format: format, dataBytes: dataBytes))
		file.closeFile()
		markers.closeFile()
		let lost = dropped.withLock { $0 }
		EngineLog.logger.notice("Captured \(self.dataBytes / UInt64(4 * self.format.channels)) frames to \(self.url.lastPathComponent, privacy: .public)\(lost > 0 ? ", \(lost) dropped" : "", privacy: .public)")
	}

	private func drain() {
		let channels = format.channels
		var buffer = [Float](repeating: 0, count: 8192 * channels)
		while true {
			let stopping = lock.withLock { !running }
			let got = buffer.withUnsafeMutableBufferPointer { cog_ring_read(ring, $0.baseAddress!, 8192) }
			if got > 0 {
				buffer.withUnsafeBytes { raw in
					file.write(Data(raw.prefix(got * channels * 4)))
				}
				dataBytes += UInt64(got * channels * 4)
			} else if stopping {
				break
			} else {
				Thread.sleep(forTimeInterval: 0.01)
			}
		}
		finished.signal()
	}

	private static let stampFormatter: DateFormatter = {
		let formatter = DateFormatter()
		formatter.dateFormat = "yyyyMMdd-HHmmss"
		return formatter
	}()

	/// A 44-byte WAVE header for IEEE float samples.
	static func header(format: StreamFormat, dataBytes: UInt64) -> Data {
		var data = Data()
		func append<T: FixedWidthInteger>(_ value: T) {
			withUnsafeBytes(of: value.littleEndian) { data.append(contentsOf: $0) }
		}
		let channels = UInt16(format.channels)
		let rate = UInt32(format.sampleRate)
		let bytes = UInt32(min(dataBytes, UInt64(UInt32.max - 36)))
		data.append(contentsOf: Array("RIFF".utf8))
		append(UInt32(36) + bytes)
		data.append(contentsOf: Array("WAVEfmt ".utf8))
		append(UInt32(16))
		append(UInt16(3)) // IEEE float
		append(channels)
		append(rate)
		append(rate * UInt32(channels) * 4)
		append(channels * 4)
		append(UInt16(32))
		data.append(contentsOf: Array("data".utf8))
		append(bytes)
		return data
	}
}
