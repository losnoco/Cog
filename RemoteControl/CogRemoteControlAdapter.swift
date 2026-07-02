//
//  CogRemoteControlAdapter.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-07-01.
//

import Cocoa

/// Implements the CogRemoteControlTarget boundary protocol against Cog's own
/// controllers. Metadata that lives in PlaylistEntry's Objective-C category
/// (title/artist/album/length/url) is read via KVC, since that category's
/// header can't be imported into Swift.
@available(macOS 13.0, *)
@MainActor final class CogRemoteControlAdapter: NSObject, CogRemoteControlTarget {
	private var appController: AppController? {
		NSApp.delegate as? AppController
	}

	private var playback: PlaybackController? {
		appController?.playbackController
	}

	private var playlist: PlaylistController? {
		appController?.playlistController
	}

	// MARK: - Transport

	func remotePlay() {
		guard let playback else { return }
		switch playback.playbackStatus {
		case .paused:
			playback.resume(nil)
		case .stopped, .stopping:
			playback.play(nil)
		default:
			break
		}
	}

	func remotePause() {
		playback?.pauseResume(nil)
	}

	func remoteResume() {
		guard let playback, playback.playbackStatus == .paused else { return }
		playback.resume(nil)
	}

	func remoteStop() {
		playback?.stop(nil)
	}

	func remoteNext() {
		playback?.next(nil)
	}

	func remotePrevious() {
		playback?.prev(nil)
	}

	// MARK: - Now playing

	func remoteNowPlaying() -> [String: Any]? {
		guard let playback,
		      let entry = playlist?.value(forKey: "currentEntry") as? PlaylistEntry
		else { return nil }

		let status: String
		switch playback.playbackStatus {
		case .playing:
			status = "playing"
		case .paused:
			status = "paused"
		default:
			return nil
		}

		var info: [String: Any] = [
			"status": status,
			"position": playback.position(),
			"seekable": playback.seekable(),
		]
		info["title"] = entry.value(forKey: "title") as? String
		info["artist"] = entry.value(forKey: "artist") as? String
		info["album"] = entry.value(forKey: "album") as? String
		info["length"] = (entry.value(forKey: "length") as? NSNumber)?.doubleValue

		return info
	}

	// MARK: - Volume

	func remoteVolume() -> Double {
		playback?.volume() ?? 0
	}

	func remoteSetVolume(_ volume: Double) {
		playback?.setVolume(volume)
	}

	// MARK: - Position

	func remotePosition() -> NSNumber? {
		guard let playback,
		      playback.playbackStatus == .playing || playback.playbackStatus == .paused
		else { return nil }
		return playback.position() as NSNumber
	}

	func remoteSeek(to position: Double) {
		appController?.clickSeek(position)
	}

	func remoteIsSeekable() -> Bool {
		playback?.seekable() ?? false
	}

	// MARK: - Shuffle / Repeat

	func remoteShuffleMode() -> String {
		switch playlist?.shuffle() {
		case .albums:
			return "albums"
		case .all:
			return "all"
		default:
			return "off"
		}
	}

	func remoteSetShuffleMode(_ mode: String) {
		let value: ShuffleMode
		switch mode {
		case "albums":
			value = .albums
		case "all":
			value = .all
		default:
			value = .off
		}
		playlist?.setShuffle(value)
	}

	func remoteRepeatMode() -> String {
		switch playlist?.repeat() {
		case .repeatOne:
			return "one"
		case .repeatAlbum:
			return "album"
		case .repeatAll:
			return "all"
		default:
			return "none"
		}
	}

	func remoteSetRepeatMode(_ mode: String) {
		let value: RepeatMode
		switch mode {
		case "one":
			value = .repeatOne
		case "album":
			value = .repeatAlbum
		case "all":
			value = .repeatAll
		default:
			value = .noRepeat
		}
		playlist?.setRepeat(value)
	}

	// MARK: - Playlist

	private var entries: [PlaylistEntry] {
		(playlist?.arrangedObjects as? [PlaylistEntry]) ?? []
	}

	func remoteListPlaylist() -> [[String: Any]] {
		entries.enumerated().map { i, entry in
			var info: [String: Any] = [
				"index": i,
				"queued": entry.queued,
				"current": entry.current,
			]
			info["title"] = entry.value(forKey: "title") as? String
			info["artist"] = entry.value(forKey: "artist") as? String
			info["album"] = entry.value(forKey: "album") as? String
			info["length"] = (entry.value(forKey: "length") as? NSNumber)?.doubleValue
			info["url"] = (entry.value(forKey: "url") as? URL)?.absoluteString
			return info
		}
	}

	func remoteAdd(toPlaylist urls: [String]) -> Int {
		guard let loader = appController?.playlistLoader else { return 0 }

		let parsed = urls.compactMap { string -> URL? in
			if string.contains("://") {
				return URL(string: string)
			}
			return URL(fileURLWithPath: string)
		}
		guard !parsed.isEmpty else { return 0 }

		return loader.addURLs(parsed, sort: false)?.count ?? 0
	}

	func remoteRemoveFromPlaylist(at index: Int) -> Bool {
		playlist?.removeEntry(at: index) ?? false
	}

	func remotePlayPlaylistEntry(at index: Int) -> Bool {
		guard index >= 0, index < entries.count else { return false }
		playback?.playEntry(at: index)
		return true
	}

	func remoteToggleQueue(at index: Int) -> NSNumber? {
		guard index >= 0, index < entries.count else { return nil }
		let entry = entries[index]
		guard let playlist, playlist.toggleQueueForEntry(at: index) else { return nil }
		return NSNumber(value: entry.queued)
	}
}
