//
//  PlaylistsView.swift
//  Cog (iOS)
//
//  The playlists: switched between from the playlist's title, and listed
//  to reorder, rename and delete them. One plays (marked with a speaker)
//  while any one is shown.
//

import CogPlaylist
import SwiftUI

/// The playlist title's menu: every playlist to show, and what can be done
/// to them.
struct PlaylistsMenu: View {
	@EnvironmentObject private var model: PlaylistModel
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var ui: AppUI

	var body: some View {
		Picker("Playlist", selection: Binding(get: { model.shownPlaylist?.uuid }, set: { uuid in
			if let playlist = model.playlists.first(where: { $0.uuid == uuid }) { model.show(playlist) }
		})) {
			ForEach(model.playlists, id: \.objectID) { playlist in
				PlaylistLabel(playlist: playlist)
					.tag(playlist.uuid)
			}
		}
		Divider()
		Button("New Playlist", systemImage: "plus") { ui.namingPlaylist = .new([]) }
		if let shown = model.shownPlaylist {
			Button("Rename Playlist…", systemImage: "pencil") { ui.namingPlaylist = .rename(shown) }
			Button("Manage Playlists…", systemImage: "list.bullet") { ui.managesPlaylists = true }
			Button("Delete Playlist…", systemImage: "trash", role: .destructive) {
				ui.delete(shown, model: model, player: player)
			}
			.disabled(model.playlists.count < 2)
		}
	}
}

/// A playlist's name, with a speaker when it is the one playing.
struct PlaylistLabel: View {
	@ObservedObject var playlist: Playlist
	@EnvironmentObject private var model: PlaylistModel

	var body: some View {
		Label(playlist.name ?? "", systemImage: playlist == model.playingPlaylist ? "speaker.wave.2.fill" : "music.note.list")
	}
}

/// Every playlist, to show one, reorder, rename or delete them.
struct PlaylistsView: View {
	@EnvironmentObject private var model: PlaylistModel
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var ui: AppUI
	@Environment(\.dismiss) private var dismiss

	var body: some View {
		NavigationStack {
			List {
				ForEach(model.playlists, id: \.objectID) { playlist in
					Button {
						model.show(playlist)
						dismiss()
					} label: {
						HStack {
							PlaylistLabel(playlist: playlist)
								.foregroundStyle(.primary)
							Spacer()
							if playlist == model.shownPlaylist {
								Image(systemName: "checkmark")
									.foregroundStyle(.tint)
									.accessibilityLabel(Text("Shown", comment: "Playlists list: marks the playlist on screen"))
							}
						}
					}
					.swipeActions {
						if model.playlists.count > 1 {
							Button("Delete", systemImage: "trash", role: .destructive) {
								ui.delete(playlist, model: model, player: player)
							}
						}
						Button("Rename", systemImage: "pencil") { ui.namingPlaylist = .rename(playlist) }
							.tint(.indigo)
					}
					.contextMenu {
						Button("Rename…", systemImage: "pencil") { ui.namingPlaylist = .rename(playlist) }
						Button("Delete…", systemImage: "trash", role: .destructive) {
							ui.delete(playlist, model: model, player: player)
						}
						.disabled(model.playlists.count < 2)
					}
				}
				.onMove { model.movePlaylists(fromOffsets: $0, toOffset: $1) }
			}
			.navigationTitle("Playlists")
			.navigationBarTitleDisplayMode(.inline)
			.toolbar {
				ToolbarItem(placement: .topBarLeading) {
					EditButton()
				}
				ToolbarItemGroup(placement: .topBarTrailing) {
					Button("New Playlist", systemImage: "plus") { ui.namingPlaylist = .new([]) }
					Button("Done") { dismiss() }
						.fontWeight(.semibold)
				}
			}
			.playlistPrompts(isActive: true)
		}
	}
}

extension View {
	/// The alerts for naming a playlist and confirming its deletion. Only
	/// the view on top presents them (the list's sheet, over the playlist).
	func playlistPrompts(isActive: Bool) -> some View {
		modifier(PlaylistPrompts(isActive: isActive))
	}
}

private struct PlaylistPrompts: ViewModifier {
	let isActive: Bool
	@EnvironmentObject private var model: PlaylistModel
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var ui: AppUI
	@State private var name = ""

	private var title: LocalizedStringKey {
		if case .rename = ui.namingPlaylist { return "Rename Playlist" }
		return "New Playlist"
	}

	private var confirmation: LocalizedStringKey {
		if case .rename = ui.namingPlaylist { return "Rename" }
		return "Create"
	}

	func body(content: Content) -> some View {
		content
			.alert(title, isPresented: Binding(get: { isActive && ui.namingPlaylist != nil }, set: { if !$0 { ui.namingPlaylist = nil } })) {
				TextField(Playlist.defaultName, text: $name)
				Button(confirmation) { commit() }
				Button("Cancel", role: .cancel) {}
			}
			.onChange(of: ui.namingPlaylist != nil) {
				if case let .rename(playlist) = ui.namingPlaylist {
					name = playlist.name ?? ""
				} else {
					name = ""
				}
			}
			.confirmationDialog(deletionTitle, isPresented: Binding(get: { isActive && ui.deletingPlaylist != nil }, set: { if !$0 { ui.deletingPlaylist = nil } }), titleVisibility: .visible) {
				Button("Delete Playlist", role: .destructive) {
					if let playlist = ui.deletingPlaylist {
						ui.delete(playlist, model: model, player: player, confirmed: true)
					}
				}
			} message: {
				Text("Its \(ui.deletingPlaylist?.orderedEntries.count ?? 0) tracks are removed with it.", comment: "Deleting a playlist: what goes with it")
			}
	}

	private var deletionTitle: String {
		String(localized: "Delete “\(ui.deletingPlaylist?.name ?? "")”?", comment: "Deleting a playlist: the alert's title, with its name")
	}

	private func commit() {
		switch ui.namingPlaylist {
		case let .new(entries)?:
			model.newPlaylist(named: name, with: entries)
		case let .rename(playlist)?:
			model.rename(playlist, to: name)
		case nil:
			break
		}
		ui.namingPlaylist = nil
	}
}
