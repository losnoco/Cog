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
	@Environment(\.colorScheme) private var colorScheme
	/// The playing album's colors, as Now Playing has them.
	@State private var palette: ArtworkPalette?

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
		.background {
			if let palette {
				LinearGradient(colors: [palette.top, palette.bottom], startPoint: .leading, endPoint: .trailing)
					.ignoresSafeArea(edges: .bottom)
			} else {
				Rectangle().fill(.bar)
					.ignoresSafeArea(edges: .bottom)
			}
		}
		.contentShape(Rectangle())
		.tint(palette?.accent)
		.environment(\.colorScheme, palette == nil ? colorScheme : .dark)
		.animation(.easeInOut(duration: 0.6), value: palette)
		.albumPalette($palette, of: model.currentEntry)
	}
}

struct NowPlayingView: View {
	/// Beside the playlist on a large screen, rather than a sheet over it.
	var isEmbedded = false
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var model: PlaylistModel
	@Environment(\.dismiss) private var dismiss
	@Environment(\.colorScheme) private var colorScheme
	@State private var showsEqualizer = false
	@State private var showsLyrics = false
	@State private var showsSpeed = false
	@AppStorage("rubberbandEngine") private var speedEngine = "varispeed"
	@AppStorage("tempo") private var tempo = 1.0
	@AppStorage("pitch") private var pitch = 1.0
	@AppStorage("showsVisualizer") private var showsVisualizer = false
	/// The playing album's colors; nil without art, for the usual look.
	@State private var palette: ArtworkPalette?
	@EnvironmentObject private var equalizer: Equalizer

	var body: some View {
		NavigationStack {
			if isEmbedded && model.currentEntry == nil {
				ContentUnavailableView("Not Playing", systemImage: "music.note",
				                       description: Text("Choose a track in the playlist."))
			} else {
				layout
			}
		}
		// Light on the album's dark colors, in its accent; the sheets it
		// opens keep the usual look.
		.tint(palette?.accent)
		.environment(\.colorScheme, palette == nil ? colorScheme : .dark)
		.animation(.easeInOut(duration: 0.6), value: palette)
		.albumPalette($palette, of: model.currentEntry)
	}

	private var layout: some View {
		GeometryReader { geometry in
			let landscape = geometry.size.width > geometry.size.height
			// One tree in either orientation, only its layout switching, so
			// the art, info and controls keep their identity and move
			// rather than being rebuilt.
			let layout = landscape ? AnyLayout(HStackLayout(spacing: 32)) : AnyLayout(VStackLayout(spacing: 24))
			layout {
				// Side by side the art is as tall as there is room for.
				artwork(size: landscape ? min(geometry.size.height - 32, geometry.size.width * 0.42)
					: min(geometry.size.width - 64, geometry.size.height * 0.45, isEmbedded ? 560 : 360))
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
		.background {
			if let palette {
				LinearGradient(colors: [palette.top, palette.bottom], startPoint: .top, endPoint: .bottom)
					.ignoresSafeArea()
			}
		}
		.toolbar {
			if !isEmbedded {
				ToolbarItem(placement: .topBarTrailing) {
					Button("Done") { dismiss() }
				}
			}
		}
		.toolbar(isEmbedded ? .hidden : .automatic, for: .navigationBar)
		.toolbarBackground(palette == nil ? .automatic : .hidden, for: .navigationBar)
		.sheet(isPresented: $showsEqualizer) {
			EqualizerView()
				.environment(\.colorScheme, colorScheme)
				.tint(.accentColor)
		}
		.sheet(isPresented: $showsSpeed) {
			SpeedView()
				.environment(\.colorScheme, colorScheme)
				.tint(.accentColor)
		}
		.sheet(isPresented: $showsLyrics) {
			if let entry = model.currentEntry {
				LyricsView(entry: entry)
					.environment(\.colorScheme, colorScheme)
					.tint(.accentColor)
			}
		}
	}

	/// What an on-or-off button shows: the tint when on.
	private func state(_ on: Bool) -> AnyShapeStyle {
		on ? AnyShapeStyle(.tint) : AnyShapeStyle(.secondary)
	}

	@ViewBuilder private func artwork(size: CGFloat) -> some View {
		if let entry = model.currentEntry {
			ArtworkView(entry: entry, size: max(size, 0), cornerRadius: 16)
				// The spectrum rises over the art's lower half, darkened to
				// carry it.
				.overlay(alignment: .bottom) {
					if showsVisualizer {
						SpectrumView(isPlaying: player.isPlaying, color: palette?.accent ?? .white)
							.padding(.horizontal, 12)
							.frame(height: max(size, 0) * 0.45)
							.background(LinearGradient(colors: [.clear, .black.opacity(0.6)], startPoint: .top, endPoint: .bottom))
							.clipShape(UnevenRoundedRectangle(bottomLeadingRadius: 16, bottomTrailingRadius: 16))
							.transition(.opacity)
					}
				}
				.animation(.default, value: showsVisualizer)
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

			ProgressBar(clock: player.clock, length: entry.length, tint: palette?.accent)
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
			// Lifted off the art's colors, or the plain background.
			.shadow(color: .black.opacity(palette == nil ? 0.18 : 0.35), radius: compact ? 4 : 6, y: compact ? 2 : 3)

			// Each button an equal share of the width, which eight need on a
			// phone held upright.
			HStack(spacing: 0) {
				Button("Shuffle", systemImage: "shuffle") { model.toggleShuffle() }
					.foregroundStyle(state(model.shuffleMode != .off))
					.overlay(alignment: .bottomTrailing) {
						if model.shuffleMode == .albums { badge("A") }
					}
					.frame(maxWidth: .infinity)
				Button("Repeat", systemImage: model.repeatMode == .one ? "repeat.1" : "repeat") { model.toggleRepeat() }
					.foregroundStyle(state(model.repeatMode != .none))
					.overlay(alignment: .bottomTrailing) {
						if model.repeatMode == .album { badge("A") }
					}
					.frame(maxWidth: .infinity)
				Button("Stop After This", systemImage: "stop.circle") { model.toggleStopAfterCurrent() }
					.foregroundStyle(state(model.currentEntry?.stopAfter == true))
					.frame(maxWidth: .infinity)
				Button("Lyrics", systemImage: "quote.bubble") { showsLyrics = true }
					.foregroundStyle(Color.secondary)
					.disabled(model.currentEntry == nil)
					.frame(maxWidth: .infinity)
				Button("Equalizer", systemImage: "slider.vertical.3") { showsEqualizer = true }
					.foregroundStyle(state(equalizer.isEnabled))
					.frame(maxWidth: .infinity)
				Button("Speed", systemImage: "gauge.with.needle") { showsSpeed = true }
					.foregroundStyle(state(Speed.isChanged(engine: speedEngine, tempo: tempo, pitch: pitch)))
					.frame(maxWidth: .infinity)
				Button(showsVisualizer ? "Hide Visualizer" : "Show Visualizer", systemImage: "waveform") { showsVisualizer.toggle() }
					.foregroundStyle(state(showsVisualizer))
					.frame(maxWidth: .infinity)
				RoutePicker()
					.frame(width: 32, height: 32)
					.frame(maxWidth: .infinity)
			}
			.padding(.horizontal, 12)
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
	/// The album's accent, which a UIKit slider cannot take from the
	/// environment's tint.
	let tint: Color?
	@EnvironmentObject private var player: Player
	/// The slider's value while dragged; nil follows playback.
	@State private var scrubbing: Double?

	var body: some View {
		VStack(spacing: 4) {
			ScrubbingSlider(value: Binding(get: { scrubbing ?? clock.position }, set: { scrubbing = $0 }),
			                range: 0...max(length, 1), tint: tint) { editing in
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
