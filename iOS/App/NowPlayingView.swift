//
//  NowPlayingView.swift
//  Cog (iOS)
//

import AVKit
import CogPlaylist
import SwiftUI

extension Animation {
	/// How every control in Now Playing and the mini player changes, and how
	/// the cards come and go, so what moves together eases together.
	static let controls = Animation.spring(duration: 0.4)
}

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
				ArtworkView(entry: entry, size: 40, cornerRadius: 4, crossfades: true)
			}
			// The next track's title and artist blur in over the last's.
			ZStack(alignment: .leading) {
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
				.id(model.currentEntry?.objectID)
				.transition(.blurReplace)
			}
			.animation(.easeInOut(duration: 0.35), value: model.currentEntry?.objectID)
			Spacer()
			Button(player.isPlaying ? "Pause" : "Play", systemImage: player.isPlaying ? "pause.fill" : "play.fill") {
				player.togglePlayPause()
			}
			.contentTransition(.symbolEffect(.replace.magic(fallback: .downUp.byLayer), options: .nonRepeating))
			.animation(.controls, value: player.isPlaying)
			Button("Next", systemImage: "forward.fill") { player.next() }
		}
		.labelStyle(.iconOnly)
		.font(.title3)
		.padding(.horizontal)
		.padding(.vertical, 10)
		.modifier(MiniPlayerBackground(palette: palette))
		.tint(palette?.accent)
		.environment(\.colorScheme, palette == nil ? colorScheme : .dark)
		.albumPalette($palette, of: model.currentEntry)
	}
}

/// The mini player's background. With Liquid Glass, a capsule of it that
/// floats over the playlist, in the album's colors; before it, a bar to the
/// screen's edges.
private struct MiniPlayerBackground: ViewModifier {
	let palette: ArtworkPalette?

	func body(content: Content) -> some View {
		if #available(iOS 26, *) {
			content
				// From one album's color to the next's, as AlbumGradient fades.
				.animation(.easeInOut(duration: 0.6)) {
					$0.glassEffect(.regular.tint(palette?.bottom.opacity(0.6)).interactive(), in: .capsule)
				}
				.contentShape(.capsule)
				.contentShape(.contextMenuPreview, .capsule)
				.padding(.horizontal, 16)
				.padding(.bottom, 8)
		} else {
			content
				.background {
					// To the screen's edges: under the home indicator, and
					// beside the Dynamic Island in landscape.
					ZStack {
						Rectangle().fill(.bar)
						AlbumGradient(palette: palette, startPoint: .leading, endPoint: .trailing)
					}
					.ignoresSafeArea(edges: [.horizontal, .bottom])
				}
				.contentShape(Rectangle())
		}
	}
}

struct NowPlayingView: View {
	/// Beside the playlist on a large screen, rather than a sheet over it.
	var isEmbedded = false
	@EnvironmentObject private var player: Player
	@EnvironmentObject private var model: PlaylistModel
	@Environment(\.dismiss) private var dismiss
	@Environment(\.colorScheme) private var colorScheme
	@Environment(\.verticalSizeClass) private var verticalSizeClass
	@EnvironmentObject private var ui: AppUI
	@AppStorage("rubberbandEngine") private var speedEngine = "varispeed"
	@AppStorage("tempo") private var tempo = 1.0
	@AppStorage("pitch") private var pitch = 1.0
	@AppStorage("showsVisualizer") private var showsVisualizer = false
	/// The playing album's colors; nil without art, for the usual look.
	@State private var palette: ArtworkPalette?
	/// How tall Now Playing is, which says how much room the cards have.
	@State private var height: CGFloat = 0
	/// The details of what plays, as the macOS Info Inspector has them.
	@State private var showsInfo = false
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
				// On a phone on its side the art and controls take the whole
				// height, centered in it: the Done button floats over the
				// corner above the controls rather than taking a bar's worth
				// of room, and the home indicator's strip is used too.
				.ignoresSafeArea(.container, edges: !isEmbedded && verticalSizeClass == .compact ? .vertical : [])
			// Beside the playlist, the equalizer, speed and lyrics open in the
			// room below, rather than in sheets over everything.
			if cardsFit && (ui.showsEqualizer || ui.showsSpeed || ui.showsLyrics || showsInfo) {
				panels
					.transition(.move(edge: .bottom).combined(with: .opacity))
			}
		}
		.animation(.controls, value: ui.showsEqualizer)
		.animation(.controls, value: ui.showsSpeed)
		.animation(.controls, value: ui.showsLyrics)
		.animation(.controls, value: showsInfo)
		.onGeometryChange(for: CGFloat.self) { $0.size.height } action: { height = $0 }
		.background {
			AlbumGradient(palette: palette, startPoint: .top, endPoint: .bottom)
				.ignoresSafeArea()
		}
		.toolbar {
			if !isEmbedded {
				ToolbarItem(placement: .topBarLeading) {
					Button("Info", systemImage: "info.circle") { showsInfo = true }
						.disabled(model.currentEntry == nil)
				}
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
		// Beside the playlist, a card, or a sheet where cards do not fit;
		// over it, pushed in Now Playing's own stack. (A zoom out of the art
		// hid the whole sheet while it ran.)
		.sheet(isPresented: Binding(get: { showsInfo && isEmbedded && !cardsFit }, set: { showsInfo = $0 })) {
			if let entry = model.currentEntry {
				InfoView(entry: entry)
					.environment(\.colorScheme, colorScheme)
					.tint(.accentColor)
			}
		}
		.navigationDestination(isPresented: Binding(get: { showsInfo && !isEmbedded }, set: { showsInfo = $0 })) {
			if let entry = model.currentEntry {
				InfoView(entry: entry, presentation: .page, rowBackground: palette == nil ? nil : .white.opacity(0.1))
					// Opaque from the first frame, not faded in as AlbumGradient
					// is: the push slides in a whole page.
					.background {
						if let palette {
							LinearGradient(colors: [palette.top, palette.bottom], startPoint: .top, endPoint: .bottom)
								.background(.black)
								.ignoresSafeArea()
						}
					}
			}
		}
		.onChange(of: model.currentEntry == nil) { _, ended in
			if ended { showsInfo = false }
		}
	}

	/// The art, what plays and the controls, in whatever room is left.
	private var main: some View {
		GeometryReader { geometry in
			// Side by side the art is as tall as there is room for; stacked,
			// it leaves room below for what plays and the controls.
			let besideSize = min(geometry.size.height - 32, geometry.size.width * 0.42)
			let stackedSize = min(geometry.size.width - 64, geometry.size.height * 0.45, isEmbedded ? 560 : 360)
			// Whichever shows the larger art: a phone on its side goes side by
			// side, but Now Playing beside the playlist only a little wider
			// than tall, as with More Space in landscape, stays stacked.
			let landscape = besideSize > stackedSize
			// One tree in either orientation, only its layout switching, so
			// the art, info and controls keep their identity and move
			// rather than being rebuilt.
			let layout = landscape ? AnyLayout(HStackLayout(spacing: 32)) : AnyLayout(VStackLayout(spacing: 24))
			layout {
				artwork(size: landscape ? besideSize : stackedSize)
				VStack(spacing: landscape ? 16 : 24) {
					info
					// Smaller only on a phone on its side. Beside the playlist
					// they keep their size when a card turns the layout: a new
					// font would replace Play's and Repeat's glyphs on the
					// symbol effect's own clock, out of step with the move.
					controls(compact: landscape && !isEmbedded)
				}
				// Stacked in a wide pane, the slider and the buttons kept to a
				// width they can be taken in at a glance.
				.frame(maxWidth: landscape ? .infinity : 720)
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
			if showsInfo, let entry = model.currentEntry {
				panel("Info", close: { showsInfo = false }) {
					EmptyView()
				} trailing: {
					EmptyView()
				} content: {
					InfoView(entry: entry, presentation: .card, rowBackground: palette == nil ? nil : .white.opacity(0.1))
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
						SpectrumView(isPlaying: player.isPlaying, barColor: (palette?.accent ?? .white).opacity(0.8))
							.padding(.horizontal, 12)
							.frame(height: max(size, 0) * 0.45)
							.background(LinearGradient(colors: [.clear, .black.opacity(0.6)], startPoint: .top, endPoint: .bottom))
							.clipShape(UnevenRoundedRectangle(bottomLeadingRadius: 16, bottomTrailingRadius: 16))
							.transition(.opacity)
					}
				}
				.animation(.controls, value: showsVisualizer)
				.shadow(radius: 12, y: 4)
				// Beside the playlist, held: what plays, found in it.
				.contextMenu {
					if isEmbedded {
						Button("Go to Current Track", systemImage: "scope") { ui.revealCurrent(of: model) }
					}
				}
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

			ProgressBar(clock: player.clock, waveform: player.waveform, length: entry.length.doubleValue, tint: palette?.accent)
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
				// The circle stays; only the glyph inside turns into the other.
				.contentTransition(.symbolEffect(.replace.magic(fallback: .downUp.byLayer), options: .nonRepeating))
				.animation(.controls, value: player.isPlaying)
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
				Button { model.toggleShuffle() } label: {
					Label("Shuffle", systemImage: "shuffle")
						.overlay(alignment: .topTrailing) {
							if model.shuffleMode == .albums { ModeBadge(text: "A") }
						}
				}
				.foregroundStyle(state(model.shuffleMode != .off))
				.accessibilityValue(shuffleName(model.shuffleMode))
				// Each change shown: the color, and the badge growing in or out.
				.animation(.controls, value: model.shuffleMode)
				// Held, any mode at once, rather than tapping round to it.
				.contextMenu {
					Picker("Shuffle", selection: Binding(get: { model.shuffleMode }, set: { model.shuffleMode = $0 })) {
						ForEach([PlaylistShuffleMode.off, .albums, .all], id: \.self) { Text(shuffleName($0)).tag($0) }
					}
				}
				.frame(maxWidth: .infinity)
				Button { model.toggleRepeat() } label: {
					Label("Repeat", systemImage: model.repeatMode == .one ? "repeat.1" : "repeat")
						.overlay(alignment: .topTrailing) {
							if model.repeatMode == .album { ModeBadge(text: "A") }
						}
				}
				.foregroundStyle(state(model.repeatMode != .none))
				.accessibilityValue(repeatName(model.repeatMode))
				// The 1 drawn in or out of the arrows as well.
				.contentTransition(.symbolEffect(.replace))
				.animation(.controls, value: model.repeatMode)
				.contextMenu {
					Picker("Repeat", selection: Binding(get: { model.repeatMode }, set: { model.repeatMode = $0 })) {
						ForEach([PlaylistRepeatMode.none, .one, .album, .all], id: \.self) { Text(repeatName($0)).tag($0) }
					}
				}
				.frame(maxWidth: .infinity)
				Button("Lyrics", systemImage: "quote.bubble") { ui.showsLyrics.toggle() }
					.foregroundStyle(Color.secondary)
					.disabled(model.currentEntry == nil)
					.frame(maxWidth: .infinity)
				// Over the playlist, Info is in the toolbar.
				if isEmbedded {
					Button("Info", systemImage: "info.circle") { showsInfo.toggle() }
						.foregroundStyle(Color.secondary)
						.disabled(model.currentEntry == nil)
						.frame(maxWidth: .infinity)
				}
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

	private func shuffleName(_ mode: PlaylistShuffleMode) -> LocalizedStringKey {
		switch mode {
		case .off: "Off"
		case .albums: "Albums"
		case .all: "All"
		}
	}

	private func repeatName(_ mode: PlaylistRepeatMode) -> LocalizedStringKey {
		switch mode {
		case .none: "Off"
		case .one: "One"
		case .album: "Album"
		case .all: "All"
		}
	}
}

/// A mode's letter in a filled circle on the corner of its button's icon,
/// inside the button so it is drawn, pressed and dimmed with it.
private struct ModeBadge: View {
	let text: String

	var body: some View {
		Text(text)
			.font(.system(size: 9, weight: .heavy))
			.foregroundStyle(.background)
			.frame(minWidth: 14, minHeight: 14)
			.background(.tint, in: .circle)
			.offset(x: 8, y: -6)
			.transition(.scale.combined(with: .opacity))
			// The button's value names the mode.
			.accessibilityHidden(true)
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
	@ObservedObject var waveform: WaveformModel
	let length: Double
	/// The album's accent, which a UIKit slider cannot take from the
	/// environment's tint.
	let tint: Color?
	@EnvironmentObject private var player: Player
	/// The slider's value while dragged; nil follows playback.
	@State private var scrubbing: Double?
	@AppStorage(WaveformModel.enabledKey) private var showWaveform = true
	@AppStorage(WaveformModel.logScaleKey) private var logScale = false
	@AppStorage(WaveformModel.rectifiedKey) private var rectified = true

	var body: some View {
		VStack(spacing: 4) {
			ScrubbingSlider(value: Binding(get: { scrubbing ?? clock.position }, set: { scrubbing = $0 }),
			                range: 0...max(length, 1), tint: tint, waveform: showWaveform ? waveform.waveform : nil,
			                logScale: logScale, rectified: rectified) { editing in
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
			// The waveform settings, on the times rather than the slider,
			// where holding the thumb before a drag would open them.
			.contentShape(Rectangle())
			.contextMenu {
				Toggle("Show Waveform", isOn: $showWaveform)
				Toggle("Logarithmic Scale", isOn: $logScale)
					.disabled(!showWaveform)
				Toggle("Rectified", isOn: $rectified)
					.disabled(!showWaveform)
			}
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
