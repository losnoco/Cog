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
				SplitLayout(fold: fold) {
					NavigationStack {
						PlaylistView()
					}
					// The fold divides them itself. Drawn, as a Divider outside a
					// stack would lie across rather than stand.
					if fold == nil {
						Rectangle()
							.fill(.separator)
							.frame(width: 1 / displayScale)
							.ignoresSafeArea()
					}
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

/// Where an unfolded iPhone Duo's fold crosses a view, margins included:
/// nothing should straddle it.
struct Fold {
	let frame: CGRect

	/// Across the screen, as when the Duo is turned on its side.
	var isHorizontal: Bool { frame.width > frame.height }

	/// The fold across the geometry's view, if one divides it in two.
	init?(in geometry: GeometryProxy) {
		guard #available(iOS 27.1, *) else { return nil }
		guard let region = geometry.reservedRegions(kind: .division).first else { return nil }
		let frame = region.frame
		let size = geometry.size
		// Only a fold that leaves room on both sides of it.
		let divides = frame.width > frame.height
			? frame.minY > 0 && frame.maxY < size.height
			: frame.minX > 0 && frame.maxX < size.width
		guard divides else { return nil }
		self.frame = frame
	}
}

/// The playlist, a divider and Now Playing, in that order. Without a fold
/// the playlist is a column at the leading edge; beside a fold down the
/// screen each takes a half; across it, Now Playing takes the half that
/// stands, to be watched, and the playlist the half below. Only where they
/// go changes, so neither is rebuilt as the fold turns.
private struct SplitLayout: Layout {
	let fold: Fold?

	func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
		proposal.replacingUnspecifiedDimensions()
	}

	func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
		guard let playlist = subviews.first, let nowPlaying = subviews.last, subviews.count >= 2 else { return }
		func place(_ subview: LayoutSubview, in rect: CGRect) {
			subview.place(at: rect.origin, proposal: ProposedViewSize(rect.size))
		}
		if let fold {
			// The fold in the geometry reader's space, which the layout fills.
			let fold = fold.frame.offsetBy(dx: bounds.minX, dy: bounds.minY)
			if fold.width > fold.height {
				place(nowPlaying, in: CGRect(x: bounds.minX, y: bounds.minY, width: bounds.width, height: fold.minY - bounds.minY))
				place(playlist, in: CGRect(x: bounds.minX, y: fold.maxY, width: bounds.width, height: bounds.maxY - fold.maxY))
			} else {
				place(playlist, in: CGRect(x: bounds.minX, y: bounds.minY, width: fold.minX - bounds.minX, height: bounds.height))
				place(nowPlaying, in: CGRect(x: fold.maxX, y: bounds.minY, width: bounds.maxX - fold.maxX, height: bounds.height))
			}
		} else {
			let width = min(max(bounds.width * 0.4, 340), 460)
			place(playlist, in: CGRect(x: bounds.minX, y: bounds.minY, width: width, height: bounds.height))
			var dividerWidth: CGFloat = 0
			if subviews.count == 3 {
				let divider = subviews[1]
				dividerWidth = divider.sizeThatFits(ProposedViewSize(width: nil, height: bounds.height)).width
				place(divider, in: CGRect(x: bounds.minX + width, y: bounds.minY, width: dividerWidth, height: bounds.height))
			}
			let start = bounds.minX + width + dividerWidth
			place(nowPlaying, in: CGRect(x: start, y: bounds.minY, width: bounds.maxX - start, height: bounds.height))
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
