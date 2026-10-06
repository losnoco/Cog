//
//  ContentView.swift
//  Cog (iOS)
//

import CogPlaylist
import SwiftUI

struct ContentView: View {
	@EnvironmentObject private var model: PlaylistModel
	@Environment(\.horizontalSizeClass) private var sizeClass
	@Environment(\.colorScheme) private var colorScheme
	@EnvironmentObject private var ui: AppUI

	var body: some View {
		// An iPad's full width: the playlist beside Now Playing, always shown.
		// A Pro Max iPhone held sideways is as regular in width, but too short
		// for both.
		if sizeClass == .regular && UIDevice.current.userInterfaceIdiom == .pad {
			GeometryReader { geometry in
				HStack(spacing: 0) {
					NavigationStack {
						PlaylistView()
					}
					.frame(width: min(max(geometry.size.width * 0.4, 340), 460))
					Divider()
						.ignoresSafeArea()
					NowPlayingView(isEmbedded: true)
				}
			}
		} else {
			compact
		}
	}

	/// A phone, or a narrow window: the playlist, with what plays in a bar
	/// at its foot that opens Now Playing.
	private var compact: some View {
		NavigationStack {
			PlaylistView()
				// Inside the stack, so the list makes room for it at its end.
				.safeAreaInset(edge: .bottom, spacing: 0) {
					if model.currentEntry != nil {
						MiniPlayerView()
							.onTapGesture { ui.showsNowPlaying = true }
					}
				}
				// The menus' equalizer, speed and lyrics, while Now Playing
				// (which opens its own) is closed.
				.sheet(isPresented: overPlaylist($ui.showsEqualizer)) {
					EqualizerView()
				}
				.sheet(isPresented: overPlaylist($ui.showsSpeed)) {
					SpeedView()
				}
				.sheet(isPresented: overPlaylist($ui.showsLyrics)) {
					if let entry = model.currentEntry {
						LyricsView(entry: entry)
					}
				}
		}
		.sheet(isPresented: $ui.showsNowPlaying) {
			NowPlayingView()
		}
	}

	private func overPlaylist(_ shows: Binding<Bool>) -> Binding<Bool> {
		Binding(get: { shows.wrappedValue && !ui.showsNowPlaying }, set: { shows.wrappedValue = $0 })
	}
}

/// "3:07", or "1:02:03" past the hour.
func formatTime(_ seconds: Double) -> String {
	guard seconds.isFinite, seconds > 0 else { return "0:00" }
	let total = Int(seconds.rounded(.down))
	let (hours, minutes, secs) = (total / 3600, total / 60 % 60, total % 60)
	return hours > 0 ? String(format: "%d:%02d:%02d", hours, minutes, secs) : String(format: "%d:%02d", minutes, secs)
}
