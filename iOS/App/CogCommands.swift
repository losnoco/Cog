//
//  CogCommands.swift
//  Cog (iOS)
//
//  The menu bar, and its keyboard shortcuts: the macOS app's, where it has
//  one for the same thing.
//

import CogPlaylist
import SwiftUI

/// What the menus open and show, shared with the views that open and
/// show it.
@MainActor
final class AppUI: ObservableObject {
	@Published var showsNowPlaying = false
	@Published var showsEqualizer = false
	@Published var showsSpeed = false
	@Published var showsLyrics = false
	@Published var showsSettings = false
	@Published var addsMusic = false
	@Published var addsURL = false
	@Published var searching = false
	/// Counted up to scroll the playlist to what plays.
	@Published var revealsCurrent = 0
}

struct CogCommands: Commands {
	@ObservedObject var player: Player
	@ObservedObject var model: PlaylistModel
	@ObservedObject var ui: AppUI
	@AppStorage("showsVisualizer") private var showsVisualizer = false

	private var nothingPlays: Bool { model.currentEntry == nil }

	var body: some Commands {
		CommandGroup(replacing: .appSettings) {
			Button("Settings…") { ui.showsSettings = true }
				.keyboardShortcut(",")
		}

		CommandGroup(replacing: .newItem) {
			Button("Add Music…") { ui.addsMusic = true }
				.keyboardShortcut("o")
			Button("Add URL…") { ui.addsURL = true }
				.keyboardShortcut("o", modifiers: [.command, .shift])
		}

		CommandGroup(after: .textEditing) {
			Button("Find in Playlist") { ui.searching = true }
				.keyboardShortcut("f")
		}

		CommandGroup(before: .toolbar) {
			Button("Go to Current Track") { ui.revealsCurrent += 1 }
				.keyboardShortcut("l")
				.disabled(nothingPlays)
			Divider()
			Toggle("Lyrics", isOn: $ui.showsLyrics)
				.keyboardShortcut("l", modifiers: [.command, .shift])
				.disabled(nothingPlays)
			Toggle("Equalizer", isOn: $ui.showsEqualizer)
				.keyboardShortcut("e", modifiers: [.command, .option])
			Toggle("Speed", isOn: $ui.showsSpeed)
			Toggle("Visualizer", isOn: $showsVisualizer)
				.keyboardShortcut("v", modifiers: [.command, .shift])
			Divider()
		}

		CommandMenu("Playback") {
			Button(player.isPlaying ? "Pause" : "Play") { player.togglePlayPause() }
				.keyboardShortcut("p")
				.disabled(model.entries.isEmpty)
			Button("Stop") { player.stop() }
				.keyboardShortcut(".")
				.disabled(nothingPlays)
			Divider()
			Button("Next Track") { player.next() }
				.keyboardShortcut(.rightArrow)
				.disabled(nothingPlays)
			Button("Previous Track") { player.previous() }
				.keyboardShortcut(.leftArrow)
				.disabled(nothingPlays)
			Button("Next Album") { player.nextAlbum() }
				.keyboardShortcut(.rightArrow, modifiers: .option)
				.disabled(nothingPlays)
			Button("Previous Album") { player.previousAlbum() }
				.keyboardShortcut(.leftArrow, modifiers: .option)
				.disabled(nothingPlays)
			Button("Seek Forward") { player.seek(by: 5) }
				.keyboardShortcut(.rightArrow, modifiers: .shift)
				.disabled(nothingPlays)
			Button("Seek Backward") { player.seek(by: -5) }
				.keyboardShortcut(.leftArrow, modifiers: .shift)
				.disabled(nothingPlays)
			Divider()
			Toggle("Stop After Current", isOn: Binding(get: { model.currentEntry?.stopAfter == true },
			                                          set: { _ in model.toggleStopAfterCurrent() }))
				.keyboardShortcut(".", modifiers: [.command, .option])
				.disabled(nothingPlays)
			Menu("Shuffle") {
				Picker("Shuffle", selection: Binding(get: { model.shuffleMode }, set: { model.shuffleMode = $0 })) {
					Text("Off").tag(PlaylistShuffleMode.off)
					Text("Albums").tag(PlaylistShuffleMode.albums)
					Text("All").tag(PlaylistShuffleMode.all)
				}
				.pickerStyle(.inline)
				Button("Next Shuffle Mode") { model.toggleShuffle() }
					.keyboardShortcut("s", modifiers: [.command, .shift])
			}
			Menu("Repeat") {
				Picker("Repeat", selection: Binding(get: { model.repeatMode }, set: { model.repeatMode = $0 })) {
					Text("Off").tag(PlaylistRepeatMode.none)
					Text("One").tag(PlaylistRepeatMode.one)
					Text("Album").tag(PlaylistRepeatMode.album)
					Text("All").tag(PlaylistRepeatMode.all)
				}
				.pickerStyle(.inline)
				Button("Next Repeat Mode") { model.toggleRepeat() }
					.keyboardShortcut("r")
			}
		}
	}
}
