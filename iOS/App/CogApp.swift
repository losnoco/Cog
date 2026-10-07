//
//  CogApp.swift
//  Cog (iOS)
//

import CogAudio
import CogPlaylist
import SwiftUI

@main
struct CogApp: App {
	@StateObject private var player: Player
	@StateObject private var locations: MusicLocations
	@StateObject private var ui = AppUI()

	init() {
		// Before the engine and the plugins read them.
		Self.registerDefaults()
		Speed.registerDefaults()
		// The plugins register now rather than on the first play.
		_ = PluginController.shared()
		let store: PlaylistStore
		do {
			store = try PlaylistStore()
		} catch {
			// A store that will not open is unusable; start one in memory so
			// the app still plays, and say why in the log.
			NSLog("Could not open the playlist store: \(error)")
			store = try! PlaylistStore(inMemory: true)
		}
		PlaylistController.container = store.container
		let model = PlaylistModel(store: store)
		MusicImporter.relocateMovedContainer(in: model)
		let locations = MusicLocations()
		locations.restore(in: model)
		SoundFontAccess.restore()
		_locations = StateObject(wrappedValue: locations)
		_player = StateObject(wrappedValue: Player(model: model))
	}

	/// The macOS app's defaults (AppController's initDefaults) that the
	/// engine and the plugins read. Without them the synth formats get a
	/// length, fade and sample rate of zero, and end as soon as they start.
	private static func registerDefaults() {
		UserDefaults.standard.register(defaults: [
			"volumeScaling": "albumGainWithPeak",
			"resampling": "cubic",
			"midiPlugin": "Spessa",
			"midi.flavor": "default",
			"httpStreamingBufferSize": 0x40000,
			"enableLrclib": false,
			"lrclibUrl": "https://lrclib.net",
			"synthDefaultSeconds": 150.0,
			"synthDefaultFadeSeconds": 8.0,
			"synthDefaultLoopCount": 2,
			"synthSampleRate": 44100,
			"alwaysStopAfterCurrent": false,
			"suspendOutputOnPause": true,
			"enableFading": true,
		])
	}

	var body: some Scene {
		WindowGroup {
			ContentView()
				.environmentObject(player)
				.environmentObject(player.model)
				.environmentObject(player.equalizer)
				.environmentObject(locations)
				.environmentObject(ui)
		}
		.commands {
			CogCommands(player: player, model: player.model, ui: ui)
		}
	}
}
