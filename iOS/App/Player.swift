//
//  Player.swift
//  Cog (iOS)
//
//  Playback for the app: the playlist (CogPlaylist) feeding CogAudio's
//  AudioPlayer, as PlaybackController does on macOS, with what iOS adds:
//  the audio session, Now Playing and the remote commands, and pausing
//  when headphones go.
//

import AVFoundation
import CogAudio
import CogPlaylist
import Combine
import MediaPlayer

/// The playing entry's position, apart from Player: it changes four times a
/// second, and only what shows it should redraw that often.
@MainActor
final class PlaybackClock: ObservableObject {
	@Published var position: Double = 0
}

@MainActor
final class Player: NSObject, ObservableObject {
	let model: PlaylistModel
	let loader: PlaylistLoader
	// -init returns id, so Swift sees it as failable; it never fails.
	private let audioPlayer: AudioPlayer = AudioPlayer()!

	@Published private(set) var status: CogStatus = .stopped
	/// Seconds into the entry playing, refreshed while it plays, in `clock`
	/// (which views showing it observe, rather than the whole player).
	let clock = PlaybackClock()
	private(set) var position: Double {
		get { clock.position }
		set { clock.position = newValue }
	}

	private var positionTimer: Timer?
	private var routeObserver: NSObjectProtocol?

	init(model: PlaylistModel) {
		self.model = model
		loader = PlaylistLoader(model: model)
		super.init()
		audioPlayer.setDelegate(self)
		audioPlayer.setVolume(100)
		model.onPlaylistChange = { [weak self] in self?.audioPlayer.resetNextStreams() }
		setUpRemoteCommands()
		routeObserver = NotificationCenter.default.addObserver(forName: AVAudioSession.routeChangeNotification, object: nil, queue: .main) { [weak self] note in
			// Headphones out: pause, as every iOS player does.
			let reason = (note.userInfo?[AVAudioSessionRouteChangeReasonKey] as? UInt).flatMap(AVAudioSession.RouteChangeReason.init(rawValue:))
			guard reason == .oldDeviceUnavailable else { return }
			MainActor.assumeIsolated { self?.pause() }
		}
	}

	var currentEntry: PlaylistEntry? { model.currentEntry }
	var isPlaying: Bool { status == .playing }

	// MARK: - Control

	func play(_ entry: PlaylistEntry, from seconds: Double = 0) {
		guard let url = entry.url else { return }
		model.setCurrent(entry)
		position = seconds
		Task {
			do {
				try await DeviceOutput.activateSession()
			} catch {
				NSLog("Could not activate the audio session: \(error)")
			}
			audioPlayer.play(url, withUserInfo: entry, withRGInfo: entry.replayGainInfo, startPaused: false, andSeekTo: seconds)
		}
	}

	func togglePlayPause() {
		switch status {
		case .playing: pause()
		case .paused: resume()
		default:
			if let entry = model.currentEntry ?? model.entries.first { play(entry) }
		}
	}

	func pause() {
		guard status == .playing else { return }
		audioPlayer.pause()
	}

	func resume() {
		guard status == .paused else { return }
		Task {
			try? await DeviceOutput.activateSession()
			audioPlayer.resume()
		}
	}

	func stop() {
		audioPlayer.stop()
	}

	func next() {
		guard let entry = model.nextEntry(after: model.currentEntry, ignoreRepeatOne: true) else { return }
		play(entry)
	}

	/// Back to the start of the entry a few seconds in, as other players do;
	/// otherwise the entry before.
	func previous() {
		if position > 3, model.currentEntry != nil {
			seek(to: 0)
			return
		}
		guard let entry = model.previousEntry(before: model.currentEntry, ignoreRepeatOne: true) else { return }
		play(entry)
	}

	func seek(to seconds: Double) {
		position = seconds
		audioPlayer.seek(toTime: seconds)
		updateNowPlaying()
	}

	// MARK: - Position

	private func startPositionTimer() {
		guard positionTimer == nil else { return }
		let timer = Timer(timeInterval: 0.25, repeats: true) { [weak self] _ in
			MainActor.assumeIsolated {
				guard let self else { return }
				self.position = self.audioPlayer.amountPlayed()
			}
		}
		RunLoop.main.add(timer, forMode: .common)
		positionTimer = timer
	}

	private func stopPositionTimer() {
		positionTimer?.invalidate()
		positionTimer = nil
	}

	// MARK: - Now Playing

	private func setUpRemoteCommands() {
		let center = MPRemoteCommandCenter.shared()
		center.playCommand.addTarget { [weak self] _ in
			MainActor.assumeIsolated { self?.togglePlayPause() }
			return .success
		}
		center.pauseCommand.addTarget { [weak self] _ in
			MainActor.assumeIsolated { self?.pause() }
			return .success
		}
		center.togglePlayPauseCommand.addTarget { [weak self] _ in
			MainActor.assumeIsolated { self?.togglePlayPause() }
			return .success
		}
		center.nextTrackCommand.addTarget { [weak self] _ in
			MainActor.assumeIsolated { self?.next() }
			return .success
		}
		center.previousTrackCommand.addTarget { [weak self] _ in
			MainActor.assumeIsolated { self?.previous() }
			return .success
		}
		center.changePlaybackPositionCommand.addTarget { [weak self] event in
			guard let event = event as? MPChangePlaybackPositionCommandEvent else { return .commandFailed }
			MainActor.assumeIsolated { self?.seek(to: event.positionTime) }
			return .success
		}
	}

	private func updateNowPlaying() {
		guard let entry = model.currentEntry, status != .stopped else {
			MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
			MPNowPlayingInfoCenter.default().playbackState = .stopped
			return
		}
		var info: [String: Any] = [
			MPMediaItemPropertyTitle: entry.title,
			MPMediaItemPropertyPlaybackDuration: entry.length,
			MPNowPlayingInfoPropertyElapsedPlaybackTime: position,
			MPNowPlayingInfoPropertyPlaybackRate: status == .playing ? 1.0 : 0.0,
		]
		if let artist = entry.artist { info[MPMediaItemPropertyArtist] = artist }
		if let album = entry.album { info[MPMediaItemPropertyAlbumTitle] = album }
		MPNowPlayingInfoCenter.default().nowPlayingInfo = info
		MPNowPlayingInfoCenter.default().playbackState = status == .playing ? .playing : .paused
	}
}

// MARK: - AudioPlayerDelegate

/// AudioPlayer calls these on the main thread, waiting for willEndStream's
/// answer (setNextStream) before it goes on.
extension Player {
	@objc func audioPlayer(_ player: AudioPlayer, willEndStream userInfo: Any?) {
		MainActor.assumeIsolated {
			let entry = userInfo as? PlaylistEntry
			if entry?.stopAfter == true {
				player.setNextStream(nil)
				return
			}
			guard let next = model.nextEntry(after: entry), let url = next.url else {
				player.setNextStream(nil)
				return
			}
			player.setNextStream(url, withUserInfo: next, withRGInfo: next.replayGainInfo)
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, didBeginStream userInfo: Any?) {
		MainActor.assumeIsolated {
			model.setCurrent(userInfo as? PlaylistEntry)
			position = 0
			updateNowPlaying()
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, didChangeStatus status: Any?, userInfo: Any?) {
		MainActor.assumeIsolated {
			let value = (status as? NSNumber)?.intValue ?? CogStatus.stopped.rawValue
			self.status = CogStatus(rawValue: value) ?? .stopped
			if self.status == .playing {
				startPositionTimer()
			} else {
				stopPositionTimer()
				position = audioPlayer.amountPlayed()
			}
			if self.status == .stopped {
				Task { await DeviceOutput.deactivateSession() }
			}
			updateNowPlaying()
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, didStopNaturally userInfo: Any?) {
		MainActor.assumeIsolated {
			position = 0
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, restartPlaybackAtCurrentPosition userInfo: Any?) {
		MainActor.assumeIsolated {
			guard let entry = userInfo as? PlaylistEntry ?? model.currentEntry else { return }
			let paused = status == .paused
			play(entry, from: audioPlayer.amountPlayed())
			if paused { pause() }
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, pushInfo info: [AnyHashable: Any], toTrack userInfo: Any?) {
		MainActor.assumeIsolated {
			guard let entry = userInfo as? PlaylistEntry else { return }
			// A stream's title as it changes.
			entry.setMetadata(info as? [String: Any] ?? [:])
			updateNowPlaying()
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, setError status: NSNumber, toTrack userInfo: Any?) {
		MainActor.assumeIsolated {
			(userInfo as? PlaylistEntry)?.error = status.boolValue
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, updatePosition userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, reportPlayCountForTrack userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, reportScrobbleForTrack userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, sustainHDCD userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, displayEqualizer equalizer: OpaquePointer?) {}
	@objc func audioPlayer(_ player: AudioPlayer, refreshEqualizer equalizer: OpaquePointer?) {}
	@objc func audioPlayer(_ player: AudioPlayer, removeEqualizer equalizer: OpaquePointer?) {}
}
