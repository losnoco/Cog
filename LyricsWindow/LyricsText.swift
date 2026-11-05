//
//  LyricsText.swift
//  Cog
//
//  An entry's lyrics: its own, those of a .lrc file beside it, or LRCLIB's
//  when the lookup is on, following playback where they are timed. The
//  macOS lyrics window and iOS's Now Playing share it.
//

import CogPlaylist
import SwiftUI

/// How lyrics follow playback, which each app answers for itself.
struct LyricsPlayback {
	/// Seconds into `entry`; nil unless it is the entry playing.
	var position: @MainActor (PlaylistEntry) -> Double?
	var seek: @MainActor (Double) -> Void
}

/// The lyrics, scrolling.
struct LyricsText: View {
	static let followsKey = "lyricsFollowPlayback"

	@ObservedObject var entry: PlaylistEntry
	var font: Font = .title3
	/// Without it, timed lyrics show as plain text, as they do for any
	/// entry but the one playing.
	var playback: LyricsPlayback?
	@AppStorage(LyricsLookup.enabledKey) private var lookupEnabled = false
	@AppStorage(followsKey) private var follows = true
	@State private var text = ""
	@State private var synced: SyncedLyrics?
	/// Whether `entry` is the one playing.
	@State private var isPlaying = false

	private static let playingPollInterval: UInt64 = 250_000_000

	/// What the text depends on: a change of tags, or turning the lookup on
	/// or off, asks again.
	private struct Question: Hashable {
		var own: String?
		var file: URL?
		var title: String?
		var artist: String?
		var album: String?
		var duration: Double
		var lookupEnabled: Bool
	}

	private var question: Question {
		Question(own: entry.unsyncedlyrics, file: Self.lrcFile(of: entry), title: entry.rawTitle, artist: entry.artist,
		         album: entry.album, duration: entry.length.doubleValue, lookupEnabled: lookupEnabled)
	}

	var body: some View {
		ZStack(alignment: .topTrailing) {
			if let synced, let playback, isPlaying, follows {
				SyncedLyricsLines(lyrics: synced, entry: entry, playback: playback, font: font)
			} else {
				ScrollView {
					Text(text)
						.font(font)
						.frame(maxWidth: .infinity, alignment: .leading)
						.textSelection(.enabled)
						.padding()
				}
			}
			if synced != nil, isPlaying {
				followToggle
			}
		}
		.task(id: question) { await load(question) }
		.task(id: ObjectIdentifier(entry)) {
			while !Task.isCancelled {
				isPlaying = playback?.position(entry) != nil
				try? await Task.sleep(nanoseconds: Self.playingPollInterval)
			}
		}
	}

	/// Between following playback and the plain text, which can be selected.
	private var followToggle: some View {
		Button {
			follows.toggle()
		} label: {
			Image(systemName: follows ? "clock.fill" : "clock")
				.frame(width: 30, height: 30)
				.background(.ultraThinMaterial, in: Circle())
				.contentShape(Circle())
		}
		.buttonStyle(.plain)
		.help(Text("Synced Lyrics"))
		.accessibilityLabel(Text("Synced Lyrics"))
		.accessibilityAddTraits(follows ? .isSelected : [])
		.padding(8)
	}

	private func load(_ asked: Question) async {
		let own = asked.own ?? ""
		if SyncedLyrics(own) != nil {
			show(own)
			return
		}
		if let file = asked.file, let lrc = await Self.read(file), !lrc.isEmpty {
			guard !Task.isCancelled else { return }
			show(lrc)
			return
		}
		guard !Task.isCancelled else { return }
		if !own.isEmpty {
			show(own)
			return
		}
		let shown = LyricsLookup.shared.displayText(title: asked.title, artist: asked.artist, album: asked.album,
		                                            duration: asked.duration) { answer in
			// The answer, if this still shows what it was asked for.
			if question == asked { show(answer) }
		}
		show(shown ?? String(localized: "No lyrics. Cog can look them up on LRCLIB: turn that on in Settings."))
	}

	private func show(_ lyrics: String) {
		synced = SyncedLyrics(lyrics)
		text = SyncedLyrics.plainText(from: lyrics)
	}

	/// Where a .lrc file would be: beside the file, under its name. A track
	/// of a cue sheet or archive has none of its own.
	private static func lrcFile(of entry: PlaylistEntry) -> URL? {
		guard let url = entry.url, url.isFileURL, url.fragment == nil else { return nil }
		return url.deletingPathExtension().appendingPathExtension("lrc")
	}

	private static func read(_ file: URL) async -> String? {
		await Task.detached(priority: .userInitiated) {
			#if os(macOS)
			let broker = SandboxBroker.shared()
			let access = broker.beginFolderAccess(file)
			defer { broker.endFolderAccess(access) }
			#endif
			return try? String(contentsOf: file, encoding: .utf8)
		}.value
	}
}

/// Timed lyrics: the line being sung stands out and is kept in view, and
/// tapping a line goes there.
private struct SyncedLyricsLines: View {
	let lyrics: SyncedLyrics
	let entry: PlaylistEntry
	let playback: LyricsPlayback
	let font: Font
	/// The line being sung; nil before the first.
	@State private var current: Int?
	/// The line last brought into view.
	@State private var shown: Int?
	/// Scrolled by hand, playback leaves the view alone until this time.
	@State private var heldUntil = Date.distantPast

	private static let pollInterval: UInt64 = 100_000_000
	private static let holdAfterScrolling: TimeInterval = 4

	var body: some View {
		ScrollViewReader { proxy in
			ScrollView {
				VStack(alignment: .leading, spacing: 14) {
					ForEach(lyrics.lines) { line in
						Button {
							heldUntil = .distantPast
							playback.seek(line.time)
						} label: {
							Text(verbatim: line.text.isEmpty ? "♪" : line.text)
								.font(font)
								.foregroundStyle(line.id == current ? Color.primary : Color.secondary)
								.multilineTextAlignment(.leading)
								.frame(maxWidth: .infinity, alignment: .leading)
								.contentShape(Rectangle())
						}
						.buttonStyle(.plain)
						.id(line.id)
					}
				}
				.padding()
				.animation(.easeInOut(duration: 0.2), value: current)
			}
			.onScrolledByHand { heldUntil = Date().addingTimeInterval(Self.holdAfterScrolling) }
			.task(id: lyrics.lines) {
				while !Task.isCancelled {
					follow(proxy)
					try? await Task.sleep(nanoseconds: Self.pollInterval)
				}
			}
		}
	}

	private func follow(_ proxy: ScrollViewProxy) {
		current = playback.position(entry).flatMap(lyrics.lineIndex(at:))
		guard let current, current != shown, Date() >= heldUntil else { return }
		shown = current
		withAnimation(.easeInOut(duration: 0.3)) { proxy.scrollTo(current, anchor: .center) }
	}
}

private extension View {
	/// Calls `action` as someone scrolls, where the system says so.
	@ViewBuilder func onScrolledByHand(_ action: @escaping () -> Void) -> some View {
		if #available(macOS 15.0, iOS 18.0, *) {
			onScrollPhaseChange { _, phase in
				if phase == .tracking || phase == .interacting || phase == .decelerating {
					action()
				}
			}
		} else {
			self
		}
	}
}
