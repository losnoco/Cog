//
//  WaveformCache.swift
//  CogAudio
//
//  Waveforms already scanned: the last few in memory, and every complete one
//  on disk under Caches, named for the track and the file's size and date,
//  so an edited file is scanned again.
//

import CryptoKit
import Foundation

public final class WaveformCache {
	public static let shared = WaveformCache()

	private static let memoryLimit = 32
	private var memory: [String: Waveform] = [:]
	private var recent: [String] = []

	/// Calls `update` on the main queue with the track's waveform: at once
	/// if it is in memory, else from disk or as a scan reaches it. Returns
	/// the scanner, for the caller to cancel when the track changes; nil if
	/// nothing more will come. Call on the main queue.
	public func load(_ url: URL, update: @escaping (Waveform) -> Void) -> WaveformScanner? {
		guard url.isFileURL else { return nil }
		let identity = Self.identity(url)
		if let waveform = memory[identity] {
			touch(identity)
			update(waveform)
			return nil
		}
		let scanner = WaveformScanner(url: url, cacheFile: Self.cacheFile(for: url, identity: identity))
		scanner.start(progress: update) { [weak self] waveform in
			guard let waveform else { return }
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

	/// As `PlaylistLoader.identity` has it: the resolved path, and the
	/// fragment naming a cue sheet or subsong track.
	private static func identity(_ url: URL) -> String {
		url.standardizedFileURL.resolvingSymlinksInPath().path + "#" + (url.fragment ?? "")
	}

	private static let directory: URL? = {
		guard let caches = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first else { return nil }
		return caches.appendingPathComponent(Bundle.main.bundleIdentifier ?? "org.cogx.cog", isDirectory: true)
			.appendingPathComponent("Waveforms", isDirectory: true)
	}()

	private static func cacheFile(for url: URL, identity: String) -> URL? {
		guard let directory,
		      let attributes = try? FileManager.default.attributesOfItem(atPath: url.path),
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
