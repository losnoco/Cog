//
//  SC55Roms.swift
//  Cog (iOS)
//
//  The ROM set Nuked SC-55 emulates, installed as the macOS app installs it
//  (Preferences/MIDIConfig.mm): every file picked, in a folder or a zip, rar
//  or 7z archive, is known by its SHA-256, and a complete set is copied
//  under the names the emulator asks for into Application Support's
//  Cog/Roms/Nuked-SC55, where the MIDI plugin reads it.
//

import CryptoKit
import Foundation

enum SC55Roms {
	enum ImportError: LocalizedError {
		case incomplete

		var errorDescription: String? {
			String(localized: "That is not a complete SC-55mk2 or SC-55mk1 ROM set.")
		}
	}

	private struct Rom {
		let name: String
		let model: String
	}

	/// The files of each set Nuked SC-55 can run, by SHA-256.
	private static let known: [String: Rom] = [
		"8a1eb33c7599b746c0c50283e4349a1bb1773b5c0ec0e9661219bf6c067d2042": Rom(name: "rom1.bin", model: "SC-55mk2"),
		"a4c9fd821059054c7e7681d61f49ce6f42ed2fe407a7ec1ba0dfdc9722582ce0": Rom(name: "rom2.bin", model: "SC-55mk2"),
		"b0b5f865a403f7308b4be8d0ed3ba2ed1c22db881b8a8326769dea222f6431d8": Rom(name: "rom_sm.bin", model: "SC-55mk2"),
		"c6429e21b9b3a02fbd68ef0b2053668433bee0bccd537a71841bc70b8874243b": Rom(name: "waverom1.bin", model: "SC-55mk2"),
		"5b753f6cef4cfc7fcafe1430fecbb94a739b874e55356246a46abe24097ee491": Rom(name: "waverom2.bin", model: "SC-55mk2"),
		"7e1bacd1d7c62ed66e465ba05597dcd60dfc13fc23de0287fdbce6cf906c6544": Rom(name: "sc55_rom1.bin", model: "SC-55mk1"),
		"effc6132d68f7e300aaef915ccdd08aba93606c22d23e580daf9ea6617913af1": Rom(name: "sc55_rom2.bin", model: "SC-55mk1"),
		"5655509a531804f97ea2d7ef05b8fec20ebf46216b389a84c44169257a4d2007": Rom(name: "sc55_waverom1.bin", model: "SC-55mk1"),
		"c655b159792d999b90df9e4fa782cf56411ba1eaa0bb3ac2bdaf09e1391006b1": Rom(name: "sc55_waverom2.bin", model: "SC-55mk1"),
		"334b2d16be3c2362210fdbec1c866ad58badeb0f84fd9bf5d0ac599baf077cc2": Rom(name: "sc55_waverom3.bin", model: "SC-55mk1"),
	]

	static var folder: URL {
		FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
			.appendingPathComponent("Cog/Roms/Nuked-SC55", isDirectory: true)
	}

	/// The model of the set installed, if one is.
	static var installedModel: String? {
		let names = Set((try? FileManager.default.contentsOfDirectory(atPath: folder.path)) ?? [])
		return Set(known.values.map(\.model)).sorted().first { model in
			known.values.filter { $0.model == model }.allSatisfy { names.contains($0.name) }
		}
	}

	/// Installs the complete set among `urls` (files, folders, archives) in
	/// place of the one installed, and returns its model.
	static func install(from urls: [URL]) async throws -> String {
		try await Task.detached(priority: .userInitiated) {
			var found: [String: (rom: Rom, data: Data)] = [:]
			for url in urls {
				let scoped = url.startAccessingSecurityScopedResource()
				defer { if scoped { url.stopAccessingSecurityScopedResource() } }
				for data in contents(of: url) {
					let hash = SHA256.hash(data: data).map { String(format: "%02x", $0) }.joined()
					if let rom = known[hash] { found[hash] = (rom, data) }
				}
			}
			// The first model whose every file turned up.
			guard let model = Set(found.values.map(\.rom.model)).sorted().first(where: { model in
				known.values.filter { $0.model == model }.count == found.values.filter { $0.rom.model == model }.count
			}) else { throw ImportError.incomplete }

			try? FileManager.default.removeItem(at: folder)
			try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
			for (rom, data) in found.values where rom.model == model {
				try data.write(to: folder.appendingPathComponent(rom.name))
			}
			return model
		}.value
	}

	static func remove() {
		try? FileManager.default.removeItem(at: folder)
	}

	/// Each file's data: a folder's, recursively; an archive's members; or
	/// the file's own.
	private static func contents(of url: URL) -> [Data] {
		var isDirectory: ObjCBool = false
		guard FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory) else { return [] }
		if isDirectory.boolValue {
			let files = FileManager.default.enumerator(at: url, includingPropertiesForKeys: nil, options: [.skipsHiddenFiles])?
				.compactMap { $0 as? URL } ?? []
			return files.flatMap { file -> [Data] in
				var isFolder: ObjCBool = false
				FileManager.default.fileExists(atPath: file.path, isDirectory: &isFolder)
				return isFolder.boolValue ? [] : contents(of: file)
			}
		}
		if ["zip", "rar", "7z"].contains(url.pathExtension.lowercased()) {
			return archiveMembers(url)
		}
		// A ROM is a few megabytes at most; anything far larger is not one.
		guard let size = try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize, size <= 8 << 20 else { return [] }
		return (try? Data(contentsOf: url)).map { [$0] } ?? []
	}

	private static func archiveMembers(_ url: URL) -> [Data] {
		var fex: OpaquePointer?
		guard fex_open(&fex, url.path) == nil, let fex else { return [] }
		defer { fex_close(fex) }
		var members: [Data] = []
		while fex_done(fex) == 0 {
			var bytes: UnsafeRawPointer?
			if fex_data(fex, &bytes) == nil, let bytes {
				members.append(Data(bytes: bytes, count: Int(fex_size(fex))))
			}
			if fex_next(fex) != nil { break }
		}
		return members
	}
}
