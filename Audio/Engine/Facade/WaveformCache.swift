//
//  WaveformCache.swift
//  CogAudio
//
//  Waveforms already scanned: the last few in memory, and every complete one
//  on disk under Caches, named for the track and the file's size and date
//  (an archive's, for a track inside one), so an edited file is scanned
//  again.
//

import CryptoKit
import Foundation

public final class WaveformCache {
	public static let shared = WaveformCache()

	private static let memoryLimit = 32
	private var memory: [String: Waveform] = [:]
	private var recent: [String] = []

	/// Calls `update` on the main queue with the track's waveform: at once
	/// if it is in memory, else from disk or as a scan reaches it. If the
	/// scan reads nothing, calls `failed` instead, so the caller can try
	/// again when the track next begins. Returns the scanner, for the caller
	/// to cancel when the track changes; nil if nothing more will come. Only
	/// local files and tracks inside archives are scanned; a stream is
	/// neither. Call on the main queue.
	public func load(_ url: URL, update: @escaping (Waveform) -> Void, failed: @escaping () -> Void = {}) -> WaveformScanner? {
		guard let file = Self.localFile(url) else { return nil }
		let identity = Self.identity(url, file: file)
		if let waveform = memory[identity] {
			touch(identity)
			update(waveform)
			return nil
		}
		let scanner = WaveformScanner(url: url, cacheFile: Self.cacheFile(for: file, identity: identity))
		scanner.start(progress: update) { [weak self] waveform in
			guard let waveform else { return failed() }
			self?.remember(waveform, for: identity)
			update(waveform)
		}
		return scanner
	}

	private func remember(_ waveform: Waveform, for identity: String) {
		memory[identity] = waveform
		touch(identity)
		while recent.count > Self.memoryLimit {
			memory[recent.removeFirst()] = nil
		}
	}

	private func touch(_ identity: String) {
		recent.removeAll { $0 == identity }
		recent.append(identity)
	}

	// MARK: - Disk

	/// The file on disk the track is read from: the file itself, or for an
	/// `unpack://` URL the archive it is inside. Nil for anything else.
	static func localFile(_ url: URL) -> URL? {
		if url.isFileURL { return url }
		guard url.scheme == "unpack", let archive = archivePath(url) else { return nil }
		return URL(fileURLWithPath: archive)
	}

	/// The archive's path in an `unpack://<type>|<length>|<archive>|<file>`
	/// URL, as ArchiveContainer builds them: the length counts UTF-16 units,
	/// as NSString does.
	static func archivePath(_ url: URL) -> String? {
		let string = (url.absoluteString.removingPercentEncoding ?? url.absoluteString) as NSString
		let open = string.range(of: "|")
		guard open.location != NSNotFound else { return nil }
		let afterOpen = open.location + 1
		let close = string.range(of: "|", range: NSRange(location: afterOpen, length: string.length - afterOpen))
		guard close.location != NSNotFound,
		      let length = Int(string.substring(with: NSRange(location: afterOpen, length: close.location - afterOpen))), length > 0 else { return nil }
		let start = close.location + 1
		guard start + length < string.length, string.character(at: start + length) == UInt16(UInt8(ascii: "|")) else { return nil }
		return string.substring(with: NSRange(location: start, length: length))
	}

	/// As `PlaylistLoader.identity` has it: the resolved path, and the
	/// fragment naming a cue sheet or subsong track. A track inside an
	/// archive adds its whole URL, which names the file within.
	private static func identity(_ url: URL, file: URL) -> String {
		let path = file.standardizedFileURL.resolvingSymlinksInPath().path
		return url.isFileURL ? path + "#" + (url.fragment ?? "") : path + "\n" + url.absoluteString
	}

	private static let directory: URL? = {
		guard let caches = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first else { return nil }
		return caches.appendingPathComponent(Bundle.main.bundleIdentifier ?? "org.cogx.cog", isDirectory: true)
			.appendingPathComponent("Waveforms", isDirectory: true)
	}()

	private static func cacheFile(for file: URL, identity: String) -> URL? {
		guard let directory,
		      let attributes = try? FileManager.default.attributesOfItem(atPath: file.path),
		      let size = attributes[.size] as? NSNumber,
		      let modified = attributes[.modificationDate] as? Date else { return nil }
		let key = "\(identity)\n\(size)\n\(modified.timeIntervalSinceReferenceDate)"
		let hash = SHA256.hash(data: Data(key.utf8)).map { String(format: "%02x", $0) }.joined()
		return directory.appendingPathComponent(hash).appendingPathExtension("waveform")
	}

	static func read(_ file: URL) -> Waveform? {
		guard let data = try? Data(contentsOf: file) else { return nil }
		return Waveform(encoded: data)
	}

	static func write(_ waveform: Waveform, to file: URL) {
		do {
			try FileManager.default.createDirectory(at: file.deletingLastPathComponent(), withIntermediateDirectories: true)
			try waveform.encoded().write(to: file, options: .atomic)
		} catch {
			NSLog("Could not cache a waveform at %@: %@", file.path, error.localizedDescription)
		}
	}
}
