//
//  ContentView.swift
//  Cog (iOS)
//

import CogPlaylist
import SwiftUI

struct ContentView: View {
	@EnvironmentObject private var player: Player
	@State private var showsNowPlaying = false

	var body: some View {
		NavigationStack {
			PlaylistView()
		}
		.safeAreaInset(edge: .bottom) {
			if player.currentEntry != nil {
				MiniPlayerView()
					.onTapGesture { showsNowPlaying = true }
			}
		}
		.sheet(isPresented: $showsNowPlaying) {
			NowPlayingView()
		}
	}
}

/// "3:07", or "1:02:03" past the hour.
func formatTime(_ seconds: Double) -> String {
	guard seconds.isFinite, seconds > 0 else { return "0:00" }
	let total = Int(seconds.rounded(.down))
	let (hours, minutes, secs) = (total / 3600, total / 60 % 60, total % 60)
	return hours > 0 ? String(format: "%d:%02d:%02d", hours, minutes, secs) : String(format: "%d:%02d", minutes, secs)
}
