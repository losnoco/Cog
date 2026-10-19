//
//  LyricsView.swift
//  Cog (iOS)
//
//  The sheet that holds the playing entry's lyrics. LyricsText, which shows
//  them, is shared with the macOS lyrics window.
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
