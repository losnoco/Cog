//
//  WaveformModel.swift
//  Cog (iOS)
//
//  The playing track's waveform for Now Playing's seek bar, as
//  WaveformController has it on macOS: loaded from the cache or scanned as
//  each track begins, while the waveform seek bar is on.
//

import CogAudio
import Combine
import Foundation

@MainActor
final class WaveformModel: ObservableObject {
	static let enabledKey = "waveformSeekBar"
	static let logScaleKey = "waveformLogScale"
	static let rectifiedKey = "waveformRectified"

	/// The track's waveform so far; nil when off, stopped or unreadable.
	@Published private(set) var waveform: Waveform?

	private var url: URL?
	private var scanner: WaveformScanner?
	private var wasEnabled: Bool
	private var defaultsObserver: NSObjectProtocol?

	private static var isEnabled: Bool { UserDefaults.standard.bool(forKey: enabledKey) }

	init() {
		wasEnabled = Self.isEnabled
		defaultsObserver = NotificationCenter.default.addObserver(forName: UserDefaults.didChangeNotification, object: nil, queue: .main) { [weak self] _ in
			MainActor.assumeIsolated {
				// Turned on (or off) mid-track, the playing track is loaded
				// (or let go) at once.
				guard let self, Self.isEnabled != self.wasEnabled else { return }
				self.wasEnabled = Self.isEnabled
				self.load(self.url)
			}
		}
	}

	/// The track that began playing, or nil when playback stops.
	func track(_ newURL: URL?) {
		guard newURL != url else { return }
		load(newURL)
	}

	private func load(_ newURL: URL?) {
		scanner?.cancel()
		scanner = nil
		url = newURL
		waveform = nil
		guard Self.isEnabled, let newURL else { return }
		scanner = WaveformCache.shared.load(newURL) { [weak self] waveform in
			guard let self, self.url == newURL else { return }
			self.waveform = waveform
		}
	}
}
