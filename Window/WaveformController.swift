//
//  WaveformController.swift
//  Cog
//
//  The playing track's waveform, for the seek bars to draw: loaded from the
//  cache or scanned as each track begins, while the waveform seek bar is on.
//

import CogAudio
import CogPlaylist
import Foundation

@MainActor @objc final class WaveformController: NSObject {
	@objc static let shared = WaveformController()
	/// Posted on the main queue when `waveform` changes.
	@objc nonisolated static let didChangeNotification = Notification.Name("CogWaveformDidChange")

	@objc nonisolated static let enabledKey = "waveformSeekBar"
	@objc nonisolated static let logScaleKey = "waveformLogScale"
	@objc nonisolated static let rectifiedKey = "waveformRectified"

	/// The track's waveform so far; nil when off, stopped or unreadable.
	private(set) var waveform: Waveform? {
		didSet { NotificationCenter.default.post(name: Self.didChangeNotification, object: self) }
	}

	private var url: URL?
	private var scanner: WaveformScanner?
	private var enabledObservation: NSKeyValueObservation?

	private var isEnabled: Bool { UserDefaults.standard.bool(forKey: Self.enabledKey) }

	override private init() {
		super.init()
		let center = NotificationCenter.default
		center.addObserver(self, selector: #selector(playbackDidBegin(_:)), name: .CogPlaybackDidBeginNotificiation, object: nil)
		center.addObserver(self, selector: #selector(playbackDidStop(_:)), name: .CogPlaybackDidStopNotificiation, object: nil)
		UserDefaults.standard.addObserver(self, forKeyPath: Self.enabledKey, options: [], context: nil)
	}

	nonisolated override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard keyPath == Self.enabledKey else {
			return super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
		DispatchQueue.main.async {
			MainActor.assumeIsolated {
				// Turned on (or off) mid-track, the playing track is loaded
				// (or let go) at once.
				let playing = self.url
				self.url = nil
				self.load(playing)
			}
		}
	}

	@objc private func playbackDidBegin(_ notification: Notification) {
		// Posted again when a stream's title changes; only a new track counts.
		guard let entry = notification.object as? PlaylistEntry, entry.url != url else { return }
		load(entry.url)
	}

	@objc private func playbackDidStop(_ notification: Notification) {
		load(nil)
	}

	private func load(_ newURL: URL?) {
		scanner?.cancel()
		scanner = nil
		url = newURL
		if waveform != nil {
			waveform = nil
		}
		guard isEnabled, let newURL else { return }
		scanner = WaveformCache.shared.load(newURL) { [weak self] waveform in
			guard let self, self.url == newURL else { return }
			self.waveform = waveform
		}
	}
}
