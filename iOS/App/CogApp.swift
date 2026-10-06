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
		// Before the engine reads them.
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
