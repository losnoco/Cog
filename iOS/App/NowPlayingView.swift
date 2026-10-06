//
//  NowPlayingView.swift
//  Cog (iOS)
//

import AVKit
import CogPlaylist
import SwiftUI

/// The bar above the tab of whatever plays; tapped, it opens Now Playing.
struct MiniPlayerView: View {
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var model: PlaylistModel

	var body: some View {
		HStack(spacing: 12) {
			if let entry = model.currentEntry {
				ArtworkView(entry: entry, size: 40, cornerRadius: 4)
			}
			VStack(alignment: .leading, spacing: 2) {
				Text(model.currentEntry?.title ?? "")
					.font(.subheadline.weight(.semibold))
					.lineLimit(1)
				if let artist = model.currentEntry?.artist {
					Text(artist)
						.font(.caption)
						.foregroundStyle(.secondary)
						.lineLimit(1)
				}
			}
			Spacer()
			Button(player.isPlaying ? "Pause" : "Play", systemImage: player.isPlaying ? "pause.fill" : "play.fill") {
				player.togglePlayPause()
			}
			Button("Next", systemImage: "forward.fill") { player.next() }
		}
		.labelStyle(.iconOnly)
		.font(.title3)
		.padding(.horizontal)
		.padding(.vertical, 10)
		.background(.bar)
		.contentShape(Rectangle())
	}
}

struct NowPlayingView: View {
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var model: PlaylistModel
	@Environment(\.dismiss) private var dismiss
	@State private var showsEqualizer = false
	@State private var showsLyrics = false
	@EnvironmentObject private var equalizer: Equalizer

	var body: some View {
		NavigationStack {
			GeometryReader { geometry in
				let landscape = geometry.size.width > geometry.size.height
				// One tree in either orientation, only its layout switching, so
				// the art, info and controls keep their identity and move
				// rather than being rebuilt.
				let layout = landscape ? AnyLayout(HStackLayout(spacing: 32)) : AnyLayout(VStackLayout(spacing: 24))
				layout {
					// Side by side the art is as tall as there is room for.
					artwork(size: landscape ? min(geometry.size.height - 32, geometry.size.width * 0.42)
						: min(geometry.size.width - 64, geometry.size.height * 0.45, 360))
					VStack(spacing: landscape ? 16 : 24) {
						info
						controls(compact: landscape)
					}
					.frame(maxWidth: landscape ? .infinity : nil)
				}
				.padding(.horizontal, landscape ? 24 : 0)
				.frame(width: geometry.size.width, height: geometry.size.height)
				.animation(.default, value: landscape)
			}
			.toolbar {
				ToolbarItem(placement: .topBarTrailing) {
					Button("Done") { dismiss() }
				}
			}
			.sheet(isPresented: $showsEqualizer) {
				EqualizerView()
			}
			.sheet(isPresented: $showsLyrics) {
				if let entry = model.currentEntry {
					LyricsView(entry: entry)
				}
			}
		}
	}

	@ViewBuilder private func artwork(size: CGFloat) -> some View {
		if let entry = model.currentEntry {
			ArtworkView(entry: entry, size: max(size, 0), cornerRadius: 16)
				.shadow(radius: 12, y: 4)
		}
	}

	/// Title, artist and album, and the position.
	@ViewBuilder private var info: some View {
		if let entry = model.currentEntry {
			VStack(spacing: 4) {
				Text(entry.title)
					.font(.title2.bold())
					.lineLimit(2)
				Text([entry.artist, entry.album].compactMap { $0 }.joined(separator: " — "))
					.foregroundStyle(.secondary)
					.lineLimit(1)
			}
			.multilineTextAlignment(.center)
			.padding(.horizontal)

			ProgressBar(clock: player.clock, length: entry.length)
		}
	}

	/// Transport, then the modes and the rest; smaller in landscape.
	private func controls(compact: Bool) -> some View {
		VStack(spacing: compact ? 12 : 24) {
			HStack(spacing: compact ? 40 : 48) {
				Button("Previous", systemImage: "backward.fill") { player.previous() }
				Button(player.isPlaying ? "Pause" : "Play", systemImage: player.isPlaying ? "pause.circle.fill" : "play.circle.fill") {
					player.togglePlayPause()
				}
				.font(.system(size: compact ? 48 : 64))
				Button("Next", systemImage: "forward.fill") { player.next() }
			}
			.font(compact ? .title : .largeTitle)

			HStack(spacing: compact ? 32 : 40) {
				Button("Shuffle", systemImage: "shuffle") { model.toggleShuffle() }
					.foregroundStyle(model.shuffleMode == .off ? Color.secondary : Color.accentColor)
					.overlay(alignment: .bottomTrailing) {
						if model.shuffleMode == .albums { badge("A") }
					}
				Button("Repeat", systemImage: model.repeatMode == .one ? "repeat.1" : "repeat") { model.toggleRepeat() }
					.foregroundStyle(model.repeatMode == .none ? Color.secondary : Color.accentColor)
					.overlay(alignment: .bottomTrailing) {
						if model.repeatMode == .album { badge("A") }
					}
				Button("Stop After This", systemImage: "stop.circle") { model.toggleStopAfterCurrent() }
					.foregroundStyle(model.currentEntry?.stopAfter == true ? Color.accentColor : Color.secondary)
				Button("Lyrics", systemImage: "quote.bubble") { showsLyrics = true }
					.foregroundStyle(Color.secondary)
					.disabled(model.currentEntry == nil)
				Button("Equalizer", systemImage: "slider.vertical.3") { showsEqualizer = true }
					.foregroundStyle(equalizer.isEnabled ? Color.accentColor : Color.secondary)
				RoutePicker()
					.frame(width: 32, height: 32)
			}
			.font(compact ? .title3 : .title2)
		}
		.labelStyle(.iconOnly)
	}

	private func badge(_ text: String) -> some View {
		Text(text)
			.font(.system(size: 9, weight: .bold))
			.offset(x: 6, y: 4)
	}
}

/// The position slider: the one part of Now Playing that follows the clock.
private struct ProgressBar: View {
	@ObservedObject var clock: PlaybackClock
	let length: Double
	@EnvironmentObject private var player: Player
	/// The slider's value while dragged; nil follows playback.
	@State private var scrubbing: Double?

	var body: some View {
		VStack(spacing: 4) {
			Slider(value: Binding(get: { scrubbing ?? clock.position }, set: { scrubbing = $0 }),
			       in: 0...max(length, 1)) { editing in
				if !editing, let target = scrubbing {
					player.seek(to: target)
					scrubbing = nil
				}
			}
			.disabled(length <= 0)
			HStack {
				Text(formatTime(scrubbing ?? clock.position))
				Spacer()
				Text("-" + formatTime(max(0, length - (scrubbing ?? clock.position))))
			}
			.font(.caption.monospacedDigit())
			.foregroundStyle(.secondary)
		}
		.padding(.horizontal, 24)
	}
}

/// AirPlay and Bluetooth output, as the system offers them.
private struct RoutePicker: UIViewRepresentable {
	func makeUIView(context: Context) -> AVRoutePickerView {
		AVRoutePickerView()
	}

	func updateUIView(_ uiView: AVRoutePickerView, context: Context) {}
}
