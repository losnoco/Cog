//
//  OutputMeter.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-22.
//

import Foundation
import CogAudio

/// The one reader of the output's levels. Each `SignalMetrics` takes the
/// meter's snapshot, so two views reading on their own would each see only
/// part of the audio; they subscribe here instead, and metering is on while
/// anyone is subscribed.
@MainActor
final class OutputMeter {
	static let shared = OutputMeter()

	/// Meters refresh at about the engine's own tick.
	private static let refreshInterval = 1.0 / 20

	private var subscribers: [ObjectIdentifier: (SignalMetrics?) -> Void] = [:]
	private weak var player: AudioPlayer?
	private var timer: Timer?

	/// Starts calling `handler` with each reading until `unsubscribe`.
	/// Subscribing again with the same owner replaces its handler.
	func subscribe(_ owner: AnyObject, player: AudioPlayer?, handler: @escaping (SignalMetrics?) -> Void) {
		if let player {
			self.player = player
		}
		subscribers[ObjectIdentifier(owner)] = handler
		guard timer == nil, let player = self.player else { return }
		player.meteringEnabled = true
		let timer = Timer(timeInterval: Self.refreshInterval, repeats: true) { [weak self] _ in
			MainActor.assumeIsolated { self?.refresh() }
		}
		RunLoop.main.add(timer, forMode: .common)
		self.timer = timer
		refresh()
	}

	/// Stops calling the owner's handler, last calling it with nil. Metering
	/// stops with the last subscriber.
	func unsubscribe(_ owner: AnyObject) {
		guard let handler = subscribers.removeValue(forKey: ObjectIdentifier(owner)) else { return }
		handler(nil)
		if subscribers.isEmpty {
			timer?.invalidate()
			timer = nil
			player?.meteringEnabled = false
		}
	}

	private func refresh() {
		let metrics = player?.signalMetrics()
		for handler in subscribers.values {
			handler(metrics)
		}
	}
}
