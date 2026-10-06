//
//  MIDISettingsView.swift
//  Cog (iOS)
//
//  The MIDI plugin's settings, under the macOS app's defaults keys: the
//  synthesizer, the MIDI flavor, SpessaSynth's SoundFont and Nuked SC-55's
//  ROM set.
//

import SwiftUI
import UniformTypeIdentifiers

/// The SoundFont picked in Files, played where it is: "soundFontPath" is
/// what the plugin reads, and a bookmark keeps access to it across launches.
enum SoundFontAccess {
	private static let bookmarkKey = "soundFontBookmark"

	/// Resolves the bookmark and starts accessing the SoundFont, wherever it
	/// has moved since.
	static func restore() {
		guard let bookmark = UserDefaults.standard.data(forKey: bookmarkKey) else { return }
		var stale = false
		guard let url = try? URL(resolvingBookmarkData: bookmark, options: .withoutUI, bookmarkDataIsStale: &stale) else {
			NSLog("Could not resolve the SoundFont's bookmark")
			return
		}
		_ = url.startAccessingSecurityScopedResource()
		UserDefaults.standard.set(url.path, forKey: "soundFontPath")
		if stale, let fresh = try? url.bookmarkData(options: .minimalBookmark, includingResourceValuesForKeys: nil, relativeTo: nil) {
			UserDefaults.standard.set(fresh, forKey: bookmarkKey)
		}
	}

	static func set(_ url: URL) {
		_ = url.startAccessingSecurityScopedResource()
		UserDefaults.standard.set(try? url.bookmarkData(options: .minimalBookmark, includingResourceValuesForKeys: nil, relativeTo: nil),
		                          forKey: bookmarkKey)
		UserDefaults.standard.set(url.path, forKey: "soundFontPath")
	}

	static func reset() {
		UserDefaults.standard.removeObject(forKey: bookmarkKey)
		UserDefaults.standard.removeObject(forKey: "soundFontPath")
	}
}

struct MIDISettingsView: View {
	@AppStorage("midiPlugin") private var plugin = "Spessa"
	@AppStorage("midi.flavor") private var flavor = "default"
	@AppStorage("soundFontPath") private var soundFontPath = ""
	@State private var picksSoundFont = false
	@State private var picksRoms = false
	@State private var installedModel = SC55Roms.installedModel
	@State private var installing = false
	@State private var romError: String?

	private static let synthesizers: [(name: String, id: String)] = [
		("SpessaSynth", "Spessa"),
		("Nuked SC-55", "NukeSc55"),
		("DMX Generic", "DOOM0000"),
		("DMX Doom 1", "DOOM0001"),
		("DMX Doom 2", "DOOM0002"),
		("DMX Raptor", "DOOM0003"),
		("DMX Strife", "DOOM0004"),
		("DMXOPL", "DOOM0005"),
		("OPL3Windows", "OPL3W000"),
	]

	private static let flavors: [(name: LocalizedStringKey, id: String)] = [
		("Default (Automatic)", "default"),
		("General MIDI", "gm"),
		("General MIDI 2", "gm2"),
		("Roland SC-55", "sc55"),
		("Roland SC-88", "sc88"),
		("Roland SC-88 Pro", "sc88pro"),
		("Roland SC-8850", "sc8850"),
		("Yamaha XG", "xg"),
	]

	private static let soundFontTypes: [UTType] = ["sf2", "sf2pack", "sf3", "sf4", "sflist", "dls"]
		.compactMap { UTType(filenameExtension: $0) } + [.json]

	var body: some View {
		Form {
			Section {
				Picker("Synthesizer", selection: $plugin) {
					ForEach(Self.synthesizers, id: \.id) { Text($0.name).tag($0.id) }
				}
				Picker("MIDI Flavor", selection: $flavor) {
					ForEach(Self.flavors, id: \.id) { Text($0.name).tag($0.id) }
				}
			} footer: {
				Text("Files with a SoundFont of their own always play with SpessaSynth.")
			}

			if plugin == "Spessa" {
				Section {
					LabeledContent("SoundFont", value: soundFontPath.isEmpty
						? String(localized: "Built In")
						: (soundFontPath as NSString).lastPathComponent)
					Button("Choose SoundFont…") { picksSoundFont = true }
					if !soundFontPath.isEmpty {
						Button("Use the Built-In SoundFonts") { SoundFontAccess.reset() }
					}
				} header: {
					Text("SpessaSynth")
				} footer: {
					Text("SF2, SF3, DLS or an sflist, played where it is. Without one, Cog picks a built-in SoundFont for each file.")
				}
			}

			if plugin == "NukeSc55" {
				Section {
					LabeledContent("ROM Set", value: installedModel ?? String(localized: "None"))
					Button(installing ? "Installing…" : (installedModel == nil ? "Install ROM Set…" : "Replace ROM Set…")) {
						picksRoms = true
					}
					.disabled(installing)
					if installedModel != nil {
						Button("Remove ROM Set", role: .destructive) {
							SC55Roms.remove()
							installedModel = SC55Roms.installedModel
						}
					}
				} header: {
					Text("Nuked SC-55")
				} footer: {
					if let romError {
						Text(romError).foregroundStyle(.red)
					} else {
						Text("An SC-55mk2 or SC-55mk1 set: its files, their folder, or a zip, rar or 7z archive of them. Cog copies it in.")
					}
				}
			}
		}
		.navigationTitle("MIDI")
		// Two importers on one view: SwiftUI presents only the last, so
		// each hangs off a section-independent view of its own.
		.background {
			Color.clear.fileImporter(isPresented: $picksSoundFont, allowedContentTypes: Self.soundFontTypes) { result in
				if case let .success(url) = result {
					SoundFontAccess.set(url)
				}
			}
		}
		.fileImporter(isPresented: $picksRoms, allowedContentTypes: [.folder, .item], allowsMultipleSelection: true) { result in
			guard case let .success(urls) = result else { return }
			installing = true
			romError = nil
			Task {
				do {
					installedModel = try await SC55Roms.install(from: urls)
				} catch {
					romError = error.localizedDescription
				}
				installing = false
			}
		}
	}
}
