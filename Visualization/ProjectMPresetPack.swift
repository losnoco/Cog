//
//  ProjectMPresetPack.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-11-15.
//

import CryptoKit
import Foundation

/// The whole Cream of the Crop preset pack, downloaded from Cog's CDN as a
/// 7z, checked, and unpacked with File_Extractor into Application Support,
/// where the sandbox always lets the app read it.
@MainActor
final class ProjectMPresetPack: ObservableObject {
	static let shared = ProjectMPresetPack()

	enum State: Equatable {
		case idle, downloading, unpacking
		case failed(String)
	}

	@Published private(set) var state: State = .idle
	@Published private(set) var isInstalled: Bool

	/// The pack at commit 0180df21f5e0bd39b9060cc5de420ed2f1f9e509 of
	/// projectM-visualizer/presets-cream-of-the-crop, solid LZMA2.
	nonisolated static let url = URL(string: "https://cogcdn.cog.losno.co/cream-of-the-crop-lzma2.7z")!
	nonisolated static let sha256 = "0a8e9cddcc008668adaad62e674f8c25cb989489ab5fe0b3d135500a84cc0443"

	nonisolated static var folder: URL {
		let support = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
		return support.appendingPathComponent("Cog/projectM/Cream of the Crop", isDirectory: true)
	}

	private init() {
		isInstalled = FileManager.default.fileExists(atPath: Self.folder.path)
	}

	/// Downloads and unpacks the pack, then makes it the presets folder.
	func install() {
		switch state {
		case .downloading, .unpacking:
			return
		case .idle, .failed:
			break
		}
		state = .downloading
		Task {
			do {
				let (archive, response) = try await URLSession.shared.download(from: Self.url)
				defer { try? FileManager.default.removeItem(at: archive) }
				guard (response as? HTTPURLResponse)?.statusCode == 200 else {
					throw PackError.download
				}
				state = .unpacking
				try await Task.detached(priority: .userInitiated) {
					try Self.unpack(archive)
				}.value
				isInstalled = true
				state = .idle
				UserDefaults.standard.set(Self.folder.path, forKey: "projectMPresetFolder")
			} catch {
				state = .failed(error.localizedDescription)
			}
		}
	}

	enum PackError: LocalizedError {
		case download, checksum, unpack(String)

		var errorDescription: String? {
			switch self {
			case .download:
				NSLocalizedString("The presets could not be downloaded.", comment: "projectM preset pack error")
			case .checksum:
				NSLocalizedString("The downloaded presets were damaged.", comment: "projectM preset pack error")
			case .unpack(let reason):
				String(format: NSLocalizedString("The presets could not be unpacked: %@", comment: "projectM preset pack error; the argument is the reason"), reason)
			}
		}
	}

	/// Checks the archive against the known hash, then unpacks it beside the
	/// pack's folder and swaps it in, so a failure leaves any old copy intact.
	nonisolated static func unpack(_ archive: URL) throws {
		let data = try Data(contentsOf: archive, options: .mappedIfSafe)
		let digest = SHA256.hash(data: data).map { String(format: "%02x", $0) }.joined()
		guard digest == sha256 else { throw PackError.checksum }

		let fileManager = FileManager.default
		let parent = folder.deletingLastPathComponent()
		try fileManager.createDirectory(at: parent, withIntermediateDirectories: true)
		let staging = parent.appendingPathComponent(".unpacking-\(UUID().uuidString)", isDirectory: true)
		try fileManager.createDirectory(at: staging, withIntermediateDirectories: true)
		do {
			try extract(archive, into: staging)
			if fileManager.fileExists(atPath: folder.path) {
				try fileManager.removeItem(at: folder)
			}
			try fileManager.moveItem(at: staging, to: folder)
		} catch {
			try? fileManager.removeItem(at: staging)
			throw error
		}
	}

	private nonisolated static func extract(_ archive: URL, into destination: URL) throws {
		fex_init()
		var opened: OpaquePointer?
		if let error = fex_open(&opened, archive.path) {
			throw PackError.unpack(String(cString: error))
		}
		guard let fex = opened else { throw PackError.unpack("fex_open") }
		defer { fex_close(fex) }

		while fex_done(fex) == 0 {
			let name = String(cString: fex_name(fex))
			// Only plain relative paths; nothing may land outside the folder.
			let components = name.split(separator: "/").map(String.init)
			guard !name.hasPrefix("/"), !components.isEmpty,
			      !components.contains(where: { $0 == ".." || $0 == "." }) else {
				throw PackError.unpack(name)
			}
			if let error = fex_stat(fex) {
				throw PackError.unpack(String(cString: error))
			}
			var bytes: UnsafeRawPointer?
			if let error = fex_data(fex, &bytes) {
				throw PackError.unpack(String(cString: error))
			}
			let file = components.reduce(destination) { $0.appendingPathComponent($1) }
			try FileManager.default.createDirectory(at: file.deletingLastPathComponent(), withIntermediateDirectories: true)
			let contents = bytes.map { Data(bytes: $0, count: Int(fex_size(fex))) } ?? Data()
			try contents.write(to: file)
			if let error = fex_next(fex) {
				throw PackError.unpack(String(cString: error))
			}
		}
	}
}
