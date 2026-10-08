//
//  ContentView.swift
//  Cog (iOS)
//

import CogPlaylist
import SwiftUI

struct ContentView: View {
	@EnvironmentObject private var model: PlaylistModel
	@Environment(\.horizontalSizeClass) private var sizeClass
	@Environment(\.verticalSizeClass) private var verticalSizeClass
	@Environment(\.colorScheme) private var colorScheme
	@Environment(\.displayScale) private var displayScale
	@EnvironmentObject private var ui: AppUI

	var body: some View {
		// Room for both, as on an iPad or an unfolded iPhone Duo: the playlist
		// beside Now Playing, always shown. A Pro Max iPhone held sideways is
		// as regular in width, but too short for both.
		if sizeClass == .regular && verticalSizeClass == .regular {
			GeometryReader { geometry in
				let fold = Fold(in: geometry)
				// The Duo bent across, standing like a laptop: the controls go
				// to the half that lies flat, under the hands.
				let laptop = fold?.isHorizontal == true && fold?.isActive == true
				// The playlist's column, where no fold places the split.
				let column = min(max(geometry.size.width * 0.4, 340), 460)
				if #available(iOS 27.1, *) {
					// The system splits them, along the fold where there is one.
					ArrangementView {
						NowPlayingView(isEmbedded: true, showsControls: !laptop, panelsElsewhere: fold != nil)
					} secondary: {
						playlist(fold: fold, laptop: laptop)
							.splitArrangementLayoutSize(minWidth: fold == nil ? 340 : nil,
							                            idealWidth: fold == nil ? column : nil,
							                            maxWidth: fold == nil ? 460 : nil)
					}
					.arrangementViewStyle(.split)
				} else {
					// No fold before iOS 27.1.
					ColumnLayout(column: column, hairline: 1 / displayScale) {
						playlist(fold: nil, laptop: false)
						// Drawn, as a Divider outside a stack would lie across.
						Rectangle()
							.fill(.separator)
							.ignoresSafeArea()
						NowPlayingView(isEmbedded: true)
					}
				}
			}
		} else {
			compact
		}
	}

	/// The playlist beside Now Playing. On an iPhone Duo, the equalizer,
	/// speed and lyrics open over it, across the fold from Now Playing; and
	/// while the Duo stands like a laptop, the controls are in a bar at its
	/// foot.
	private func playlist(fold: Fold?, laptop: Bool) -> some View {
		let panelsShown = fold != nil
			&& (ui.showsEqualizer || ui.showsSpeed || (ui.showsLyrics && model.currentEntry != nil))
		// A column in a pane beside the fold, a row in one below it.
		let across = fold?.isHorizontal == true
		return NavigationStack {
			PlaylistView()
				.overlay {
					if panelsShown {
						PlaybackPanels(axis: across ? .horizontal : .vertical)
							.padding(16)
							.background(.background)
							.transition(.move(edge: across ? .bottom : .trailing).combined(with: .opacity))
					}
				}
				.toolbar(panelsShown ? .hidden : .automatic, for: .navigationBar)
				// Inside the stack, so the list makes room for it at its end.
				.bottomBar {
					if laptop && model.currentEntry != nil {
						NowPlayingControls(compact: true, palette: nil)
							.padding(.vertical, 12)
							.glassBacking()
							.padding(.horizontal, 16)
							.padding(.bottom, 8)
							.transition(.move(edge: .bottom).combined(with: .opacity))
					}
				}
				// Inside the stack, which an animation from outside it does
				// not reach.
				.animation(.spring(duration: 0.4), value: laptop)
				.animation(.spring(duration: 0.4), value: panelsShown)
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

private extension View {
	/// A bar along the foot, which scrolled content fades under where the
	/// system has such bars.
	@ViewBuilder func bottomBar(@ViewBuilder _ content: () -> some View) -> some View {
		if #available(iOS 26.0, *) {
			safeAreaBar(edge: .bottom, content: content)
		} else {
			safeAreaInset(edge: .bottom, content: content)
		}
	}

	/// Liquid Glass behind, where the system has it; a material otherwise.
	@ViewBuilder func glassBacking() -> some View {
		let shape = RoundedRectangle(cornerRadius: 28, style: .continuous)
		if #available(iOS 26.0, *) {
			glassEffect(.regular, in: shape)
		} else {
			background(.regularMaterial, in: shape)
		}
	}
}

/// Where an unfolded iPhone Duo's fold crosses a view, margins included:
/// nothing should straddle it.
struct Fold {
	let frame: CGRect
	/// Bent, rather than lying flat. The fold stays where it is either way.
	let isActive: Bool

	/// Across the screen, as when the Duo is turned on its side.
	var isHorizontal: Bool { frame.width > frame.height }

	/// The fold across the geometry's view, if one divides it in two. Flat
	/// as well as bent: it is there either way, so what goes on which side
	/// of it stays put as the hinge moves.
	init?(in geometry: GeometryProxy) {
		guard #available(iOS 27.1, *) else { return nil }
		guard let region = geometry.reservedRegions(kind: .division, options: .includeInactive).first else { return nil }
		let frame = region.frame
		let size = geometry.size
		// Only a fold that leaves room on both sides of it.
		let divides = frame.width > frame.height
			? frame.minY > 0 && frame.maxY < size.height
			: frame.minX > 0 && frame.maxX < size.width
		guard divides else { return nil }
		self.frame = frame
		isActive = region.isActive
	}
}

/// The playlist's column, a hairline and Now Playing in the rest, in that
/// order.
private struct ColumnLayout: Layout {
	let column: CGFloat
	/// The divider's thickness.
	let hairline: CGFloat

	func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
		proposal.replacingUnspecifiedDimensions()
	}

	func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
		guard subviews.count == 3 else { return }
		let (playlist, divider, nowPlaying) = (subviews[0], subviews[1], subviews[2])
		func place(_ subview: LayoutSubview, in rect: CGRect) {
			subview.place(at: rect.origin, proposal: ProposedViewSize(rect.size))
		}
		place(playlist, in: CGRect(x: bounds.minX, y: bounds.minY, width: column, height: bounds.height))
		place(divider, in: CGRect(x: bounds.minX + column, y: bounds.minY, width: hairline, height: bounds.height))
		let start = bounds.minX + column + hairline
		place(nowPlaying, in: CGRect(x: start, y: bounds.minY, width: bounds.maxX - start, height: bounds.height))
	}
}

/// "3:07", or "1:02:03" past the hour.
func formatTime(_ seconds: Double) -> String {
	guard seconds.isFinite, seconds > 0 else { return "0:00" }
	let total = Int(seconds.rounded(.down))
	let (hours, minutes, secs) = (total / 3600, total / 60 % 60, total % 60)
	return hours > 0 ? String(format: "%d:%02d:%02d", hours, minutes, secs) : String(format: "%d:%02d", minutes, secs)
}
