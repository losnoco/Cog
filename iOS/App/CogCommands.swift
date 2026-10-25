//
//  CogCommands.swift
//  Cog (iOS)
//
//  The menu bar, and its keyboard shortcuts: the macOS app's, where it has
//  one for the same thing.
//

import CogPlaylist
import CoreData
import SwiftUI

/// What the menus open and show, shared with the views that open and
/// show it.
@MainActor
final class AppUI: ObservableObject {
	@Published var showsNowPlaying = false
	@Published var showsEqualizer = false
	@Published var showsSpeed = false
	@Published var showsLyrics = false
	/// The details of what plays, as the macOS Info Inspector has them.
	@Published var showsInfo = false
	@Published var showsSettings = false
	@Published var addsMusic = false
	@Published var addsURL = false
	@Published var searching = false
	/// The list of playlists, to reorder, rename and delete them.
	@Published var managesPlaylists = false
	/// A playlist being named: a new one, or one renamed.
	@Published var namingPlaylist: PlaylistNaming?
	/// A playlist with tracks, waiting for its deletion to be confirmed.
	@Published var deletingPlaylist: Playlist?
	/// What was just added, however it was, or what plays when asked for:
	/// the playlist scrolls to it and highlights it for a moment.
	@Published private(set) var added: Set<NSManagedObjectID> = []
	/// Counted up with each addition, for the playlist to scroll again.
	@Published private(set) var revealsAdded = 0

	func reveal(_ entries: [PlaylistEntry]) {
		guard !entries.isEmpty else { return }
		added = Set(entries.map(\.objectID))
		revealsAdded += 1
	}

	/// Go to Current Track: what plays, in its playlist, scrolled to and
	/// highlighted.
	func revealCurrent(of model: PlaylistModel) {
		guard let entry = model.currentEntry else { return }
		if let playing = entry.playlist, playing != model.shownPlaylist { model.show(playing) }
		reveal([entry])
	}

	/// Deletes `playlist`, asking first when it has tracks; playback stops
	/// if it is the one playing.
	func delete(_ playlist: Playlist, model: PlaylistModel, player: Player, confirmed: Bool = false) {
		guard model.playlists.count > 1 else { return }
		guard confirmed || playlist.orderedEntries.isEmpty else {
			deletingPlaylist = playlist
			return
		}
		if playlist == model.playingPlaylist { player.stop() }
		model.delete(playlist)
	}
}

/// A playlist being named, in an alert: a new one, with copies of some
/// entries, or one being renamed.
enum PlaylistNaming {
	case new([PlaylistEntry])
	case rename(Playlist)
}

extension AppUI {
	/// The shown playlist's neighbor, after or before it, around the ends.
	func showPlaylist(offsetBy offset: Int, in model: PlaylistModel) {
		let playlists = model.playlists
		guard playlists.count > 1, let shown = model.shownPlaylist, let position = playlists.firstIndex(of: shown) else { return }
		model.show(playlists[(position + offset + playlists.count) % playlists.count])
	}

	func clearAdded() {
		added = []
	}
}

struct CogCommands: Commands {
	@ObservedObject var player: Player
	@ObservedObject var model: PlaylistModel
	@ObservedObject var ui: AppUI
	@AppStorage("showsVisualizer") private var showsVisualizer = false
	@AppStorage("visualizerStyle") private var visualizerStyle = VisualizerStyle.spectrum
	@AppStorage("showsPlaylistArt") private var showsPlaylistArt = true

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
			Divider()
			Button("New Playlist") { ui.namingPlaylist = .new([]) }
				.keyboardShortcut("n")
			Button("Show Next Playlist") { ui.showPlaylist(offsetBy: 1, in: model) }
				.keyboardShortcut(.tab, modifiers: .control)
				.disabled(model.playlists.count < 2)
			Button("Show Previous Playlist") { ui.showPlaylist(offsetBy: -1, in: model) }
				.keyboardShortcut(.tab, modifiers: [.control, .shift])
				.disabled(model.playlists.count < 2)
		}

		CommandGroup(after: .textEditing) {
			Button("Find in Playlist") { ui.searching = true }
				.keyboardShortcut("f")
		}

		CommandGroup(before: .toolbar) {
			Button("Go to Current Track") { ui.revealCurrent(of: model) }
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
			Picker("Visualizer Style", selection: $visualizerStyle) {
				Text("Spectrum").tag(VisualizerStyle.spectrum)
				Text("VU Meter").tag(VisualizerStyle.vuMeter)
			}
			Toggle("Album Art in Playlist", isOn: $showsPlaylistArt)
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
