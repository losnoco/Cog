//
//  PlaylistView.swift
//  Cog (iOS)
//

import CogPlaylist
import SwiftUI
import UniformTypeIdentifiers

struct PlaylistView: View {
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var model: PlaylistModel
	@State private var importing = false
	@State private var addingCount = 0
	@State private var showsSettings = false
	@State private var confirmsClear = false

	var body: some View {
		List {
			ForEach(model.entries, id: \.objectID) { entry in
				Button {
					player.play(entry)
				} label: {
					EntryRow(entry: entry)
				}
				.swipeActions(edge: .leading) {
					Button(entry.queued ? "Unqueue" : "Queue", systemImage: entry.queued ? "text.badge.minus" : "text.badge.plus") {
						model.toggleQueued([entry])
					}
					.tint(.indigo)
				}
			}
			.onDelete { model.remove(at: $0) }
			.onMove { model.move(fromOffsets: $0, toOffset: $1) }
		}
		.listStyle(.plain)
		.navigationTitle("Playlist")
		.overlay {
			if model.entries.isEmpty && addingCount == 0 {
				ContentUnavailableView {
					Label("No Music", systemImage: "music.note.list")
				} description: {
					Text("Add files or folders from Files. Music copied into Cog's folder in the Files app or Finder can be added too.")
				} actions: {
					Button("Add Music") { importing = true }
						.buttonStyle(.borderedProminent)
				}
			}
		}
		.toolbar {
			ToolbarItem(placement: .topBarLeading) {
				EditButton()
			}
			ToolbarItemGroup(placement: .topBarTrailing) {
				if addingCount > 0 {
					ProgressView()
				}
				Button("Add Music", systemImage: "plus") { importing = true }
				Menu("More", systemImage: "ellipsis.circle") {
					Picker("Shuffle", systemImage: "shuffle", selection: Binding(get: { model.shuffleMode }, set: { model.shuffleMode = $0 })) {
						Text("Off").tag(PlaylistShuffleMode.off)
						Text("Albums").tag(PlaylistShuffleMode.albums)
						Text("All").tag(PlaylistShuffleMode.all)
					}
					.pickerStyle(.menu)
					Picker("Repeat", systemImage: "repeat", selection: Binding(get: { model.repeatMode }, set: { model.repeatMode = $0 })) {
						Text("Off").tag(PlaylistRepeatMode.none)
						Text("One").tag(PlaylistRepeatMode.one)
						Text("Album").tag(PlaylistRepeatMode.album)
						Text("All").tag(PlaylistRepeatMode.all)
					}
					.pickerStyle(.menu)
					Button("Add from Cog's Folder", systemImage: "folder") { addMusicFolder() }
					Divider()
					Button("Settings", systemImage: "gearshape") { showsSettings = true }
					Button("Clear Playlist", systemImage: "trash", role: .destructive) { confirmsClear = true }
						.disabled(model.entries.isEmpty)
				}
			}
		}
		.fileImporter(isPresented: $importing, allowedContentTypes: [.item, .folder], allowsMultipleSelection: true) { result in
			guard case let .success(urls) = result else { return }
			Task { await add(urls, copying: true) }
		}
		.confirmationDialog("Clear the playlist?", isPresented: $confirmsClear, titleVisibility: .visible) {
			Button("Clear Playlist", role: .destructive) {
				player.stop()
				model.removeAll()
			}
		}
		.sheet(isPresented: $showsSettings) {
			SettingsView()
		}
	}

	private func add(_ urls: [URL], copying: Bool) async {
		addingCount += 1
		defer { addingCount -= 1 }
		let files = copying ? await MusicImporter.importItems(urls) : urls
		await player.loader.add(files)
	}

	/// Everything in Cog's Music folder not already in the playlist (files
	/// put there from Finder or the Files app).
	private func addMusicFolder() {
		let known = Set(model.entries.compactMap { $0.url?.standardizedFileURL.path })
		let files = (try? FileManager.default.contentsOfDirectory(at: MusicImporter.musicFolder, includingPropertiesForKeys: nil)) ?? []
		let new = files.filter { !known.contains($0.standardizedFileURL.path) }
			.sorted { $0.lastPathComponent.localizedStandardCompare($1.lastPathComponent) == .orderedAscending }
		guard !new.isEmpty else { return }
		Task { await add(new, copying: false) }
	}
}

private struct EntryRow: View {
	@ObservedObject var entry: PlaylistEntry
	@EnvironmentObject private var player: Player

	var body: some View {
		HStack(spacing: 12) {
			status
				.frame(width: 18)
			VStack(alignment: .leading, spacing: 2) {
				Text(entry.title)
					.lineLimit(1)
					.foregroundStyle(entry.error ? .secondary : .primary)
				if let subtitle {
					Text(subtitle)
						.font(.subheadline)
						.foregroundStyle(.secondary)
						.lineLimit(1)
				}
			}
			Spacer()
			if entry.length > 0 {
				Text(formatTime(entry.length))
					.font(.subheadline.monospacedDigit())
					.foregroundStyle(.secondary)
			}
		}
		.contentShape(Rectangle())
	}

	private var subtitle: String? {
		let parts = [entry.artist, entry.album].compactMap { $0 }.filter { !$0.isEmpty }
		return parts.isEmpty ? nil : parts.joined(separator: " — ")
	}

	@ViewBuilder private var status: some View {
		if entry.current {
			Image(systemName: player.isPlaying ? "speaker.wave.2.fill" : "speaker.fill")
				.foregroundStyle(.tint)
		} else if entry.error {
			Image(systemName: "exclamationmark.triangle")
				.foregroundStyle(.secondary)
		} else if entry.queued {
			Text("\(entry.queuePosition + 1)")
				.font(.caption.monospacedDigit().bold())
				.foregroundStyle(.indigo)
		} else if entry.stopAfter {
			Image(systemName: "stop.circle")
				.foregroundStyle(.secondary)
		}
	}
}
