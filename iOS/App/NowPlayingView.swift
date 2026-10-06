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
		HStack(spacing: 16) {
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

	var body: some View {
		NavigationStack {
			VStack(spacing: 24) {
				Spacer()
				RoundedRectangle(cornerRadius: 16)
					.fill(.quaternary)
					.aspectRatio(1, contentMode: .fit)
					.overlay {
						Image(systemName: "music.note")
							.font(.system(size: 80))
							.foregroundStyle(.secondary)
					}
					.padding(.horizontal, 32)

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

				HStack(spacing: 48) {
					Button("Previous", systemImage: "backward.fill") { player.previous() }
					Button(player.isPlaying ? "Pause" : "Play", systemImage: player.isPlaying ? "pause.circle.fill" : "play.circle.fill") {
						player.togglePlayPause()
					}
					.font(.system(size: 64))
					Button("Next", systemImage: "forward.fill") { player.next() }
				}
				.labelStyle(.iconOnly)
				.font(.largeTitle)

				HStack(spacing: 40) {
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
					RoutePicker()
						.frame(width: 32, height: 32)
				}
				.labelStyle(.iconOnly)
				.font(.title2)
				Spacer()
			}
			.toolbar {
				ToolbarItem(placement: .topBarTrailing) {
					Button("Done") { dismiss() }
				}
			}
		}
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
