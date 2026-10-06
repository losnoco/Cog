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
	@EnvironmentObject private var locations: MusicLocations
	@EnvironmentObject private var ui: AppUI
	@State private var addingCount = 0
	@State private var confirmsClear = false
	@State private var query = ""
	@State private var urlText = ""

	/// The entries the search leaves, all of them when there is none.
	private var shown: [PlaylistEntry] {
		let terms = query.split(separator: " ").map(String.init)
		guard !terms.isEmpty else { return model.entries }
		return model.entries.filter { entry in
			let text = [entry.title, entry.artist ?? "", entry.album ?? "", entry.albumartist ?? ""].joined(separator: " ")
			return terms.allSatisfy { text.localizedStandardContains($0) }
		}
	}

	var body: some View {
		ScrollViewReader { proxy in
			List {
				ForEach(shown, id: \.objectID) { entry in
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
				.onDelete { offsets in
					// Offsets into what is shown, which a search narrows.
					let entries = shown
					model.remove(at: IndexSet(offsets.map { Int(entries[$0].index) }))
				}
				.onMove(perform: query.isEmpty ? { model.move(fromOffsets: $0, toOffset: $1) } : nil)
				if !shown.isEmpty {
					Text(summary)
						.font(.footnote)
						.foregroundStyle(.secondary)
						.frame(maxWidth: .infinity)
						.listRowSeparator(.hidden)
				}
			}
			// Go to Current Track, from the menu.
			.onChange(of: ui.revealsCurrent) {
				if let entry = model.currentEntry {
					withAnimation { proxy.scrollTo(entry.objectID, anchor: .center) }
				}
			}
		}
		.searchable(text: $query, isPresented: $ui.searching, prompt: "Title, Artist, Album")
		.listStyle(.plain)
		.navigationTitle("Playlist")
		.overlay {
			if model.entries.isEmpty && addingCount == 0 {
				ContentUnavailableView {
					Label("No Music", systemImage: "music.note.list")
				} description: {
					Text("Add files or folders from Files; they play where they are. Music copied into Cog's folder in the Files app or Finder can be added too.")
				} actions: {
					Button("Add Music") { ui.addsMusic = true }
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
				Button("Add Music", systemImage: "plus") { ui.addsMusic = true }
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
					Menu("Sort By", systemImage: "arrow.up.arrow.down") {
						Button("Title") { model.sort { $0.title.localizedStandardCompare($1.title) == .orderedAscending } }
						Button("Artist") { model.sort { Self.ordered(($0.artist ?? "", $0.album ?? "", $0.disc, $0.track), ($1.artist ?? "", $1.album ?? "", $1.disc, $1.track)) } }
						Button("Album") { model.sort { Self.ordered(($0.album ?? "", "", $0.disc, $0.track), ($1.album ?? "", "", $1.disc, $1.track)) } }
						Button("Length") { model.sort { $0.length < $1.length } }
						Button("File Name") { model.sort { $0.filename.localizedStandardCompare($1.filename) == .orderedAscending } }
					}
					Button("Add from Cog's Folder", systemImage: "folder") { addMusicFolder() }
					Button("Add URL", systemImage: "link") { ui.addsURL = true }
					Button("Reload Tags", systemImage: "arrow.clockwise") {
						Task { await player.loader.loadInfo(for: model.entries) }
					}
					.disabled(model.entries.isEmpty)
					Divider()
					Button("Equalizer", systemImage: "slider.vertical.3") { ui.showsEqualizer = true }
					Button("Settings", systemImage: "gearshape") { ui.showsSettings = true }
					Button("Clear Playlist", systemImage: "trash", role: .destructive) { confirmsClear = true }
						.disabled(model.entries.isEmpty)
				}
			}
		}
		.fileImporter(isPresented: $ui.addsMusic, allowedContentTypes: [.item, .folder], allowsMultipleSelection: true) { result in
			guard case let .success(urls) = result else { return }
			Task { await add(locations.add(urls)) }
		}
		.confirmationDialog("Clear the playlist?", isPresented: $confirmsClear, titleVisibility: .visible) {
			Button("Clear Playlist", role: .destructive) {
				player.stop()
				model.removeAll()
			}
		}
		.sheet(isPresented: $ui.showsSettings) {
			SettingsView()
		}
		.alert("Add URL", isPresented: $ui.addsURL) {
			TextField("https://", text: $urlText)
				.keyboardType(.URL)
				.textInputAutocapitalization(.never)
				.autocorrectionDisabled()
			Button("Add") {
				if let url = URL(string: urlText.trimmingCharacters(in: .whitespaces)), url.scheme != nil {
					Task { await add([url]) }
				}
				urlText = ""
			}
			Button("Cancel", role: .cancel) { urlText = "" }
		} message: {
			Text("A stream, a playlist or a file on the web.")
		}
	}

	/// "12 tracks, 48:03", for what is shown.
	private var summary: String {
		let entries = shown
		let total = entries.reduce(0) { $0 + $1.length }
		return "\(entries.count) \(entries.count == 1 ? "track" : "tracks"), \(formatTime(total))"
	}

	/// Tag order: text by Finder's rules, then disc and track as numbers.
	private static func ordered(_ a: (String, String, Int32, Int32), _ b: (String, String, Int32, Int32)) -> Bool {
		let first = a.0.localizedStandardCompare(b.0)
		if first != .orderedSame { return first == .orderedAscending }
		let second = a.1.localizedStandardCompare(b.1)
		if second != .orderedSame { return second == .orderedAscending }
		return (a.2, a.3) < (b.2, b.3)
	}

	private func add(_ urls: [URL]) async {
		addingCount += 1
		defer { addingCount -= 1 }
		await player.loader.add(urls)
	}

	/// Every track in Cog's Music folder, its subfolders included, that the
	/// playlist does not have yet (files put there from Finder or the Files
	/// app).
	private func addMusicFolder() {
		Task {
			addingCount += 1
			defer { addingCount -= 1 }
			await player.loader.add([MusicImporter.musicFolder], skippingExisting: true)
		}
	}
}

private struct EntryRow: View {
	@ObservedObject var entry: PlaylistEntry
	@EnvironmentObject private var player: Player

	var body: some View {
		HStack(spacing: 12) {
			status
				.frame(width: 18)
			ArtworkView(entry: entry, size: 40, cornerRadius: 4)
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
