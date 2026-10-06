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
			// To the screen's edges: under the home indicator, and beside the
			// Dynamic Island in landscape.
			ZStack {
				Rectangle().fill(.bar)
				AlbumGradient(palette: palette, startPoint: .leading, endPoint: .trailing)
			}
			.ignoresSafeArea(edges: [.horizontal, .bottom])
		}
		.contentShape(Rectangle())
		.tint(palette?.accent)
		.environment(\.colorScheme, palette == nil ? colorScheme : .dark)
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
	@EnvironmentObject private var ui: AppUI
	@AppStorage("rubberbandEngine") private var speedEngine = "varispeed"
	@AppStorage("tempo") private var tempo = 1.0
	@AppStorage("pitch") private var pitch = 1.0
	@AppStorage("showsVisualizer") private var showsVisualizer = false
	/// The playing album's colors; nil without art, for the usual look.
	@State private var palette: ArtworkPalette?
	/// How tall Now Playing is, which says how much room the cards have.
	@State private var height: CGFloat = 0
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
		.albumPalette($palette, of: model.currentEntry)
	}

	private var layout: some View {
		VStack(spacing: 0) {
			main
			// Beside the playlist, the equalizer, speed and lyrics open in the
			// room below, rather than in sheets over everything.
			if cardsFit && (ui.showsEqualizer || ui.showsSpeed || ui.showsLyrics) {
				panels
					.transition(.move(edge: .bottom).combined(with: .opacity))
			}
		}
		.animation(.spring(duration: 0.4), value: ui.showsEqualizer)
		.animation(.spring(duration: 0.4), value: ui.showsSpeed)
		.animation(.spring(duration: 0.4), value: ui.showsLyrics)
		.onGeometryChange(for: CGFloat.self) { $0.size.height } action: { height = $0 }
		.background {
			AlbumGradient(palette: palette, startPoint: .top, endPoint: .bottom)
				.ignoresSafeArea()
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
		.sheet(isPresented: sheet($ui.showsEqualizer)) {
			EqualizerView()
				.environment(\.colorScheme, colorScheme)
				.tint(.accentColor)
		}
		.sheet(isPresented: sheet($ui.showsSpeed)) {
			SpeedView()
				.environment(\.colorScheme, colorScheme)
				.tint(.accentColor)
		}
		.sheet(isPresented: sheet($ui.showsLyrics)) {
			if let entry = model.currentEntry {
				LyricsView(entry: entry)
					.environment(\.colorScheme, colorScheme)
					.tint(.accentColor)
			}
		}
	}

	/// The art, what plays and the controls, in whatever room is left.
	private var main: some View {
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
		}
	}

	/// A sheet's binding: shown only where the cards are not.
	private func sheet(_ shows: Binding<Bool>) -> Binding<Bool> {
		Binding(get: { shows.wrappedValue && !cardsFit }, set: { shows.wrappedValue = $0 })
	}

	/// The cards' height: up to 440 points, leaving the art and controls
	/// the 300 they need side by side.
	private var cardHeight: CGFloat {
		min(440, height - 300)
	}

	/// Beside the playlist, in a window tall enough for cards worth having;
	/// otherwise the equalizer, speed and lyrics open in sheets.
	private var cardsFit: Bool {
		isEmbedded && cardHeight >= 220
	}

	/// The equalizer, speed and lyrics as cards, side by side.
	private var panels: some View {
		HStack(alignment: .top, spacing: 16) {
			if ui.showsEqualizer {
				panel("Equalizer", close: { ui.showsEqualizer = false }) {
					Toggle("Equalizer", isOn: $equalizer.isEnabled)
						.labelsHidden()
				} trailing: {
					Button("Flat") { equalizer.flatten() }
						.disabled(!equalizer.isEnabled)
				} content: {
					ScrollView {
						EqualizerControls()
							.padding([.horizontal, .bottom])
					}
				}
			}
			if ui.showsSpeed {
				panel("Speed", close: { ui.showsSpeed = false }) {
					EmptyView()
				} trailing: {
					EmptyView()
				} content: {
					SpeedControls()
						.scrollContentBackground(.hidden)
				}
			}
			if ui.showsLyrics, let entry = model.currentEntry {
				panel("Lyrics", close: { ui.showsLyrics = false }) {
					EmptyView()
				} trailing: {
					EmptyView()
				} content: {
					LyricsText(entry: entry)
				}
			}
		}
		.frame(height: max(cardHeight - 16, 0))
		.padding([.horizontal, .bottom], 16)
	}

	/// A card: its title centered, what it offers on either side of it, and
	/// a close button at the end.
	private func panel<Leading: View, Trailing: View, Content: View>(
		_ title: LocalizedStringKey, close: @escaping () -> Void, @ViewBuilder leading: () -> Leading,
		@ViewBuilder trailing: () -> Trailing, @ViewBuilder content: () -> Content
	) -> some View {
		VStack(spacing: 0) {
			ZStack {
				Text(title)
					.font(.headline)
				HStack(spacing: 16) {
					leading()
					Spacer()
					trailing()
					Button("Close", systemImage: "xmark.circle.fill", action: close)
						.labelStyle(.iconOnly)
						.font(.title2)
						.foregroundStyle(.secondary)
				}
			}
			.padding(.horizontal)
			.padding(.vertical, 12)
			content()
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
		.background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 20))
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

			ProgressBar(clock: player.clock, length: entry.length.doubleValue, tint: palette?.accent)
		}
	}

	/// Transport, then the modes and the rest; smaller in landscape.
	private func controls(compact: Bool) -> some View {
		VStack(spacing: compact ? 12 : 24) {
			HStack(spacing: compact ? 40 : 48) {
				HoldButton("Previous", systemImage: "backward.fill", action: { player.previous() }) { held in
					held ? player.beginRewind() : player.endRewind()
				}
				.accessibilityAction(named: "Seek Backward") { player.seek(by: -5) }
				Button(player.isPlaying ? "Pause" : "Play", systemImage: player.isPlaying ? "pause.circle.fill" : "play.circle.fill") {
					player.togglePlayPause()
				}
				.font(.system(size: compact ? 48 : 64))
				HoldButton("Next", systemImage: "forward.fill", action: { player.next() }) { held in
					held ? player.beginFastForward() : player.endFastForward()
				}
				.accessibilityAction(named: "Seek Forward") { player.seek(by: 5) }
			}
			.font(compact ? .title : .largeTitle)
			// Lifted off the art's colors, or the plain background.
			.shadow(color: .black.opacity(palette == nil ? 0.18 : 0.35), radius: compact ? 4 : 6, y: compact ? 2 : 3)

			// Each button an equal share of the width. What is used less sits
			// in the menu, where it is named rather than only drawn.
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
				Button("Lyrics", systemImage: "quote.bubble") { ui.showsLyrics.toggle() }
					.foregroundStyle(Color.secondary)
					.disabled(model.currentEntry == nil)
					.frame(maxWidth: .infinity)
				moreMenu
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

	/// Stop after this, the equalizer, speed and the visualizer. Lit while
	/// any of the first three changes what plays, as their buttons were.
	private var moreMenu: some View {
		let speedChanged = Speed.isChanged(engine: speedEngine, tempo: tempo, pitch: pitch)
		let stopsAfter = model.currentEntry?.stopAfter == true
		return Menu {
			Toggle("Stop After This", systemImage: "stop.circle",
			       isOn: Binding(get: { stopsAfter }, set: { _ in model.toggleStopAfterCurrent() }))
				.disabled(model.currentEntry == nil)
			Divider()
			Button { ui.showsEqualizer.toggle() } label: {
				Label("Equalizer", systemImage: "slider.vertical.3")
				Text(equalizer.isEnabled ? "On" : "Off")
			}
			Button { ui.showsSpeed.toggle() } label: {
				Label("Speed", systemImage: "gauge.with.needle")
				Text(speedChanged ? "On" : "Off")
			}
			Toggle("Visualizer", systemImage: "waveform", isOn: $showsVisualizer)
		} label: {
			Label("More", systemImage: "ellipsis.circle")
				.labelStyle(.iconOnly)
		}
		// The items named, though the row around shows only icons.
		.labelStyle(.titleAndIcon)
		.foregroundStyle(state(equalizer.isEnabled || speedChanged || stopsAfter))
	}

	private func badge(_ text: String) -> some View {
		Text(text)
			.font(.system(size: 9, weight: .bold))
			.offset(x: 6, y: 4)
	}
}

/// A button that acts when tapped, and does something else for as long as
/// it is held instead: Next fast-forwards, Previous rewinds.
private struct HoldButton: View {
	let title: LocalizedStringKey
	let systemImage: String
	let action: () -> Void
	let hold: (Bool) -> Void
	/// Held long enough, and not yet let go.
	@State private var holding = false
	/// The tap that ends a hold, which must not act as well.
	@State private var swallowsTap = false
	@State private var waiting: Task<Void, Never>?

	init(_ title: LocalizedStringKey, systemImage: String, action: @escaping () -> Void, hold: @escaping (Bool) -> Void) {
		self.title = title
		self.systemImage = systemImage
		self.action = action
		self.hold = hold
	}

	var body: some View {
		Button(title, systemImage: systemImage) {
			if swallowsTap {
				swallowsTap = false
			} else {
				action()
			}
		}
		// Whether the release or the tap arrives first, each flag says what
		// is left to do.
		.buttonStyle(PressReporting { pressed in
			if pressed {
				swallowsTap = false
				waiting = Task {
					try? await Task.sleep(for: .seconds(0.4))
					guard !Task.isCancelled else { return }
					holding = true
					swallowsTap = true
					hold(true)
				}
			} else {
				waiting?.cancel()
				if holding {
					holding = false
					hold(false)
				}
			}
		})
		.sensoryFeedback(.impact, trigger: holding) { _, now in now }
	}
}

/// The plain look of a tinted button, telling when it is pressed and let go.
private struct PressReporting: ButtonStyle {
	let report: (Bool) -> Void

	func makeBody(configuration: Configuration) -> some View {
		configuration.label
			.foregroundStyle(.tint)
			.opacity(configuration.isPressed ? 0.4 : 1)
			.onChange(of: configuration.isPressed) { _, pressed in report(pressed) }
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

/// An album's colors as a gradient. Changing from one album's to the next's
/// fades; appearing, or a track without art, does not. Only the colors
/// animate, never the frame: an implicit animation would also move it, as
/// the layout settles while the sheet opens.
private struct AlbumGradient: View {
	let palette: ArtworkPalette?
	let startPoint: UnitPoint
	let endPoint: UnitPoint
	@State private var shown: ArtworkPalette?

	var body: some View {
		LinearGradient(colors: [shown?.top ?? .clear, shown?.bottom ?? .clear], startPoint: startPoint, endPoint: endPoint)
			.opacity(shown == nil ? 0 : 1)
			.onAppear { shown = palette }
			.onChange(of: palette) { old, new in
				if old != nil, new != nil {
					withAnimation(.easeInOut(duration: 0.6)) { shown = new }
				} else {
					shown = new
				}
			}
	}
}
