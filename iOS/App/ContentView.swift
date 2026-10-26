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
	/// The playlist's pane's width beside Now Playing, safe areas included.
	@State private var playlistPaneWidth: CGFloat = 0
	/// Standing like a laptop, the Duo's lower half shows only the controls,
	/// larger, rather than the playlist with a bar of them.
	@AppStorage("hidesPlaylistStanding") private var hidesPlaylistStanding = false
	/// `hidesPlaylistStanding`, changed here in a transaction of its own:
	/// one stored in the defaults arrives outside the one that changed it,
	/// and the seek bar's move across the fold would come unanimated.
	@State private var standingExpanded = UserDefaults.standard.bool(forKey: "hidesPlaylistStanding")
	/// The standing controls' bar's height, margins included, which the
	/// playlist keeps clear at its end.
	@State private var standingBarHeight: CGFloat = 0
	/// The room at the playlist's foot that its stack's bars take, and the
	/// pane's own there, without them: the difference is the bars'.
	@State private var playlistFootInset: CGFloat = 0
	@State private var paneFootInset: CGFloat = 0

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
				// ArrangementView is in the iOS 27.1 SDK (SwiftUI 8.0.85); built
				// with an older one, there are only columns.
				#if canImport(SwiftUI, _version: 8.0.85)
				if #available(iOS 27.1, *) {
					// The system splits them, along the fold where there is one.
					ArrangementView {
						// The seek bar goes down with the controls when they
						// take the lower half.
						NowPlayingView(isEmbedded: true, showsControls: !laptop,
						               showsProgress: !(laptop && standingExpanded && model.currentEntry != nil),
						               panelsElsewhere: fold != nil)
					} secondary: {
						playlist(fold: fold, laptop: laptop)
							.splitArrangementLayoutSize(minWidth: fold == nil ? 340 : nil,
							                            idealWidth: fold == nil ? column : nil,
							                            maxWidth: fold == nil ? 460 : nil)
					}
					.arrangementViewStyle(.split)
				} else {
					columns(column)
				}
				#else
				columns(column)
				#endif
			}
		} else {
			compact
		}
	}

	/// The playlist's column, a hairline and Now Playing: before iOS 27.1,
	/// which has no fold.
	private func columns(_ column: CGFloat) -> some View {
		ColumnLayout(column: column, hairline: 1 / displayScale) {
			playlist(fold: nil, laptop: false)
			// Drawn, as a Divider outside a stack would lie across.
			Rectangle()
				.fill(.separator)
				.ignoresSafeArea()
			NowPlayingView(isEmbedded: true)
		}
	}

	/// The playlist beside Now Playing. On an iPhone Duo, the equalizer,
	/// speed, lyrics and Info open in sheets over it, across the fold from
	/// Now Playing, which stays in view and in reach; and while the Duo
	/// stands like a laptop, the controls are in a bar at its foot, or, if
	/// asked, in its place.
	private func playlist(fold: Fold?, laptop: Bool) -> some View {
		let standing = laptop && model.currentEntry != nil
		// Standing like a laptop, the half lying flat may give the playlist
		// up to larger controls: the bar of them grows over it. The playlist
		// stays underneath, scrolled where it was, for when it comes back.
		let controlsOnly = standing && standingExpanded
		return ZStack(alignment: .bottom) {
			NavigationStack {
				PlaylistView()
					// Inside the stack, so the list makes room at its end for
					// the bar drawn over it, and fades under it.
					.bottomBar {
						if standing {
							Color.clear
								.frame(height: standingBarHeight)
						}
					}
					// Inside the stack, which an animation from outside it does
					// not reach.
					.animation(.controls, value: laptop)
					// Outside its own bar: the room the stack's bars take at
					// the foot, the search field's among them.
					.onGeometryChange(for: CGFloat.self) { $0.safeAreaInsets.bottom } action: {
						playlistFootInset = $0
					}
			}
			.accessibilityHidden(controlsOnly)
			// Outside the stack, so the bar can grow to the whole pane.
			if standing {
				StandingControls(expanded: controlsOnly, foldHeight: fold?.frame.height ?? 0) {
					withAnimation(.controls) { standingExpanded.toggle() }
					hidesPlaylistStanding = standingExpanded
				}
				.onGeometryChange(for: CGFloat.self) { $0.size.height } action: { height in
					if !controlsOnly { standingBarHeight = height }
				}
				// Over the stack, but above its bars at the foot, as a bar of
				// its own would be; grown, over the whole pane.
				.padding(.bottom, controlsOnly ? 0 : max(playlistFootInset - paneFootInset, 0))
				.animation(.controls, value: controlsOnly)
				.transition(.move(edge: .bottom).combined(with: .opacity))
			}
		}
		.animation(.controls, value: standing)
		.onGeometryChange(for: CGFloat.self) { $0.safeAreaInsets.bottom } action: { paneFootInset = $0 }
		// The pane's whole width, to the screen's edge: a sheet placed on
		// the trailing side runs to that edge, over the Duo's vertical status
		// bar, keeping its own content clear of it.
		.onGeometryChange(for: CGFloat.self) { $0.size.width + $0.safeAreaInsets.trailing } action: {
			playlistPaneWidth = $0
		}
		.modifier(PaneSheets(fold: fold, paneWidth: fold?.isHorizontal == false ? playlistPaneWidth : nil))
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
							.contextMenu {
								Button("Go to Current Track", systemImage: "scope") { ui.revealCurrent(of: model) }
							}
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

/// The equalizer, speed, lyrics and Info in sheets from the pane across an
/// iPhone Duo's fold from Now Playing. Half height to begin with, and the
/// playlist and Now Playing still take touches around them.
private struct PaneSheets: ViewModifier {
	let fold: Fold?
	/// The playlist's pane's width, to the screen's edge, beside a fold.
	let paneWidth: CGFloat?
	@EnvironmentObject private var model: PlaylistModel
	@EnvironmentObject private var ui: AppUI

	func body(content: Content) -> some View {
		content
			.sheet(isPresented: shows($ui.showsEqualizer)) {
				EqualizerView()
					.paneSheet(width: paneWidth)
			}
			.sheet(isPresented: shows($ui.showsSpeed)) {
				SpeedView()
					.paneSheet(width: paneWidth)
			}
			.sheet(isPresented: shows($ui.showsLyrics)) {
				if let entry = model.currentEntry {
					LyricsView(entry: entry)
						.paneSheet(width: paneWidth)
				}
			}
			.sheet(isPresented: shows($ui.showsInfo)) {
				if let entry = model.currentEntry {
					// Now Playing shows the art, across the fold.
					InfoView(entry: entry, showsArtwork: false)
						.paneSheet(width: paneWidth)
				}
			}
	}

	private func shows(_ flag: Binding<Bool>) -> Binding<Bool> {
		Binding(get: { fold != nil && flag.wrappedValue }, set: { flag.wrappedValue = $0 })
	}
}

private extension View {
	/// A sheet over the playlist's half: with the fold running beside it, on
	/// the trailing side and as wide as the pane, `width`, less a margin on
	/// either side like the one the system keeps from the screen's edge;
	/// half height, the lower half, with the fold across.
	@ViewBuilder func paneSheet(width: CGFloat?) -> some View {
		let sheet = presentationDetents([.medium, .large])
			.presentationBackgroundInteraction(.enabled)
		if let width, #available(iOS 27.0, *) {
			sheet
				.presentationPlacement(.trailing)
				.presentationSizing(PaneSheetSizing(width: max(width - 16, 0)))
		} else {
			sheet
		}
	}

	/// A bar along the foot, which scrolled content fades under where the
	/// system has such bars.
	@ViewBuilder func bottomBar(@ViewBuilder _ content: () -> some View) -> some View {
		if #available(iOS 26.0, *) {
			safeAreaBar(edge: .bottom, content: content)
		} else {
			safeAreaInset(edge: .bottom, content: content)
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
		// The fold's regions are in the iOS 27.1 SDK.
		#if canImport(SwiftUI, _version: 8.0.85)
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
		#else
		return nil
		#endif
	}
}

/// A sheet as wide as the pane it covers, as tall as its detents allow.
private struct PaneSheetSizing: PresentationSizing {
	let width: CGFloat

	func proposedSize(for root: PresentationSizingRoot, context: PresentationSizingContext) -> ProposedViewSize {
		ProposedViewSize(width: width, height: nil)
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
