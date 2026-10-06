//
//  LyricsView.swift
//  Cog (iOS)
//
//  The playing entry's lyrics: its own, or LRCLIB's when the lookup is on,
//  as the macOS lyrics window shows them.
//

import CogPlaylist
import SwiftUI

struct LyricsView: View {
	@ObservedObject var entry: PlaylistEntry
	@Environment(\.dismiss) private var dismiss
	@State private var text = ""

	var body: some View {
		NavigationStack {
			ScrollView {
				Text(text)
					.font(.title3)
					.frame(maxWidth: .infinity, alignment: .leading)
					.textSelection(.enabled)
					.padding()
			}
			.navigationTitle(entry.title)
			.navigationBarTitleDisplayMode(.inline)
			.toolbar {
				ToolbarItem(placement: .confirmationAction) {
					Button("Done") { dismiss() }
				}
			}
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
			// The answer, if this sheet still shows the entry it was for.
			if entry.objectID == asked { text = answer }
		}
		text = shown ?? String(localized: "No lyrics. Cog can look them up on LRCLIB: turn that on in Settings.")
	}
}
