//
//  LyricsText.swift
//  Cog
//
//  An entry's lyrics: its own, or LRCLIB's when the lookup is on. The
//  macOS lyrics window and iOS's Now Playing share it.
//

import CogPlaylist
import SwiftUI

/// The lyrics, scrolling.
struct LyricsText: View {
	@ObservedObject var entry: PlaylistEntry
	var font: Font = .title3
	@AppStorage(LyricsLookup.enabledKey) private var lookupEnabled = false
	@State private var text = ""

	/// What the text depends on: a change of tags, or turning the lookup on
	/// or off, asks again.
	private struct Question: Hashable {
		var own: String?
		var title: String?
		var artist: String?
		var album: String?
		var duration: Double
		var lookupEnabled: Bool
	}

	private var question: Question {
		Question(own: entry.unsyncedlyrics, title: entry.rawTitle, artist: entry.artist, album: entry.album,
		         duration: entry.length.doubleValue, lookupEnabled: lookupEnabled)
	}

	var body: some View {
		ScrollView {
			Text(text)
				.font(font)
				.frame(maxWidth: .infinity, alignment: .leading)
				.textSelection(.enabled)
				.padding()
		}
		.task(id: question) { load(question) }
	}

	private func load(_ asked: Question) {
		if let own = asked.own, !own.isEmpty {
			text = own
			return
		}
		let shown = LyricsLookup.shared.displayText(title: asked.title, artist: asked.artist, album: asked.album,
		                                            duration: asked.duration) { answer in
			// The answer, if this still shows what it was asked for.
			if question == asked { text = answer }
		}
		text = shown ?? String(localized: "No lyrics. Cog can look them up on LRCLIB: turn that on in Settings.")
	}
}
