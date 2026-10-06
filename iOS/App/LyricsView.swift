//
//  LyricsView.swift
//  Cog (iOS)
//
//  The playing entry's lyrics: its own, or LRCLIB's when the lookup is on,
//  as the macOS lyrics window shows them.
//

import CogPlaylist
import SwiftUI

/// The sheet that holds the lyrics, where there is no room for them
/// beside Now Playing.
struct LyricsView: View {
	@ObservedObject var entry: PlaylistEntry
	@Environment(\.dismiss) private var dismiss

	var body: some View {
		NavigationStack {
			LyricsText(entry: entry)
				.navigationTitle(entry.title)
				.navigationBarTitleDisplayMode(.inline)
				.toolbar {
					ToolbarItem(placement: .confirmationAction) {
						Button("Done") { dismiss() }
					}
				}
		}
	}
}

/// The lyrics, scrolling.
struct LyricsText: View {
	@ObservedObject var entry: PlaylistEntry
	@State private var text = ""

	var body: some View {
		ScrollView {
			Text(text)
				.font(.title3)
				.frame(maxWidth: .infinity, alignment: .leading)
				.textSelection(.enabled)
				.padding()
		}
		.task(id: entry.objectID) { load() }
	}

	private func load() {
		if let own = entry.unsyncedlyrics, !own.isEmpty {
			text = own
			return
		}
		let asked = entry.objectID
		let shown = LyricsLookup.shared.displayText(title: entry.rawTitle, artist: entry.artist, album: entry.album,
		                                            duration: entry.length) { answer in
			// The answer, if this still shows the entry it was for.
			if entry.objectID == asked { text = answer }
		}
		text = shown ?? String(localized: "No lyrics. Cog can look them up on LRCLIB: turn that on in Settings.")
	}
}
