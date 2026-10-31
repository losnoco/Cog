//
//  PreviewPlayer.swift
//  CogQuickLook
//
//  Plays the file Quick Look previews through CogAudio's AudioPlayer, with
//  the plugins of the app the extension is in, and reads its tags and
//  length for the preview to show.
//

import AppKit
import CogAudio
import Combine

@MainActor
final class PreviewPlayer: NSObject, ObservableObject {
	// -init returns id, so Swift sees it as failable; it never fails.
	private let audioPlayer: AudioPlayer = AudioPlayer()!
	private var url: URL?
	private var positionTimer: Timer?

	@Published private(set) var title = ""
	@Published private(set) var subtitle = ""
	@Published private(set) var artwork: NSImage?
	/// Seconds; 0 when the file does not say.
	@Published private(set) var duration: Double = 0
	@Published var position: Double = 0
	@Published private(set) var status: CogStatus = .stopped
	@Published private(set) var failed = false
	/// The seek bar is being dragged, so playback does not move it.
	var scrubbing = false

	override init() {
		super.init()
		Self.registerDefaults()
		audioPlayer.setDelegate(self)
		audioPlayer.setVolume(100)
	}

	/// The macOS app's defaults (AppController's and PlaybackController's
	/// initDefaults) that the engine and the plugins read, as the iOS app
	/// registers them. The extension cannot read the app's settings, and
	/// without these the synth formats get a length, fade and sample rate of
	/// zero, and end as soon as they start.
	private static func registerDefaults() {
		UserDefaults.standard.register(defaults: [
			"volumeScaling": "albumGainWithPeak",
			"resampling": "cubic",
			"midiPlugin": "Spessa",
			"midi.flavor": "default",
			"synthDefaultSeconds": 150.0,
			"synthDefaultFadeSeconds": 8.0,
			"synthDefaultLoopCount": 2,
			"synthSampleRate": 44100,
			"suspendOutputOnPause": true,
			"enableFading": true,
			"enableHDCD": false,
			"tempo": 1.0,
			"pitch": 1.0,
			"speedLock": true,
			"rubberbandEngine": "varispeed",
			"rubberbandTransients": "crisp",
			"rubberbandDetector": "compound",
			"rubberbandPhase": "laminar",
			"rubberbandWindow": "standard",
			"rubberbandSmoothing": "off",
			"rubberbandFormant": "shifted",
			"rubberbandPitch": "highspeed",
			"rubberbandChannels": "apart",
		])
	}

	/// What the preview shows of the file.
	private struct TrackInfo: Sendable {
		var title: String?
		var subtitle = ""
		var artwork: Data?
		/// Seconds; 0 when the file does not say.
		var duration: Double = 0
	}

	/// Reads the file's tags and length, then plays it.
	func load(_ url: URL) async {
		self.url = url
		title = url.lastPathComponent
		let info = await Task.detached { Self.read(url) }.value
		if let value = info.title { title = value }
		subtitle = info.subtitle
		artwork = info.artwork.flatMap { NSImage(data: $0) }
		duration = info.duration
		play(from: 0)
	}

	nonisolated private static func read(_ url: URL) -> TrackInfo {
		_ = PluginController.shared()
		let metadata = AudioMetadataReader.metadata(for: url) as? [String: Any] ?? [:]
		let properties = AudioPropertiesReader.properties(for: url) as? [String: Any] ?? [:]

		// Keys come in either case, and values as a string or several.
		var tags: [String: Any] = [:]
		for (key, value) in metadata { tags[key.lowercased()] = value }
		func tag(_ key: String) -> String? {
			switch tags[key] {
			case let string as String: string.isEmpty ? nil : string
			case let strings as [String]: strings.first { !$0.isEmpty }
			default: nil
			}
		}
		var info = TrackInfo()
		info.title = tag("title")
		info.subtitle = [tag("artist"), tag("album")].compactMap { $0 }.joined(separator: " — ")
		info.artwork = tags["albumart"] as? Data

		let frames = (properties["totalFrames"] as? NSNumber)?.doubleValue ?? 0
		let rate = (properties["sampleRate"] as? NSNumber)?.doubleValue ?? 0
		info.duration = frames > 0 && rate > 0 ? frames / rate : 0
		return info
	}

	func togglePlayPause() {
		switch status {
		case .playing: audioPlayer.pause()
		case .paused: audioPlayer.resume()
		default: play(from: position)
		}
	}

	func seek(to seconds: Double) {
		if status == .stopped {
			play(from: seconds)
		} else {
			audioPlayer.seek(toTime: seconds)
		}
		position = seconds
	}

	/// Stops for good: the preview is going away.
	func stop() {
		stopPositionTimer()
		audioPlayer.stop()
		// The player holds its delegate strongly.
		audioPlayer.setDelegate(nil)
	}

	private func play(from seconds: Double) {
		guard let url else { return }
		failed = false
		audioPlayer.play(url, withUserInfo: url as NSURL, withRGInfo: nil, startPaused: false, andSeekTo: seconds)
	}

	private func startPositionTimer() {
		guard positionTimer == nil else { return }
		let timer = Timer(timeInterval: 0.25, repeats: true) { [weak self] _ in
			MainActor.assumeIsolated {
				guard let self, !self.scrubbing else { return }
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
}

// MARK: - AudioPlayerDelegate

/// AudioPlayer calls these on the main thread, every one of them, waiting
/// for willEndStream's answer (setNextStream) before it goes on.
extension PreviewPlayer {
	@objc func audioPlayer(_ player: AudioPlayer, willEndStream userInfo: Any?) {
		// One file, played once.
		player.setNextStream(nil)
	}

	@objc func audioPlayer(_ player: AudioPlayer, didChangeStatus status: Any?, userInfo: Any?) {
		MainActor.assumeIsolated {
			let value = (status as? NSNumber)?.intValue ?? CogStatus.stopped.rawValue
			self.status = CogStatus(rawValue: value) ?? .stopped
			if self.status == .playing {
				startPositionTimer()
			} else {
				stopPositionTimer()
				if !scrubbing { position = audioPlayer.amountPlayed() }
			}
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, didStopNaturally userInfo: Any?) {
		MainActor.assumeIsolated {
			position = 0
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, restartPlaybackAtCurrentPosition userInfo: Any?) {
		MainActor.assumeIsolated {
			let paused = status == .paused
			play(from: audioPlayer.amountPlayed())
			if paused { audioPlayer.pause() }
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, setError status: NSNumber, toTrack userInfo: Any?) {
		MainActor.assumeIsolated {
			failed = status.boolValue
		}
	}

	@objc func audioPlayer(_ player: AudioPlayer, didBeginStream userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, pushInfo info: [AnyHashable: Any], toTrack userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, updatePosition userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, reportPlayCountForTrack userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, reportScrobbleForTrack userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, sustainHDCD userInfo: Any?) {}
	@objc func audioPlayer(_ player: AudioPlayer, displayEqualizer equalizer: OpaquePointer?) {}
	@objc func audioPlayer(_ player: AudioPlayer, refreshEqualizer equalizer: OpaquePointer?) {}
	@objc func audioPlayer(_ player: AudioPlayer, removeEqualizer equalizer: OpaquePointer?) {}
}
