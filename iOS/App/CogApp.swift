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

	init() {
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
		_player = StateObject(wrappedValue: Player(model: PlaylistModel(store: store)))
	}

	var body: some Scene {
		WindowGroup {
			ContentView()
				.environmentObject(player)
				.environmentObject(player.model)
		}
	}
}
