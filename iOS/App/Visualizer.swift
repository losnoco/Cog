//
//  Visualizer.swift
//  Cog (iOS)
//
//  What rises over the art in Now Playing: the spectrum, or the VU meter
//  macOS shows in its own window.
//

import SwiftUI

/// What the visualizer shows while it is on.
enum VisualizerStyle: String {
	case spectrum, vuMeter
}

/// The VU meter over the art. The output is metered while it is on screen
/// and the app is in front.
struct NowPlayingVUMeter: View {
	@EnvironmentObject private var player: Player
	@Environment(\.scenePhase) private var scenePhase
	@StateObject private var model = VUMeterModel()
	@State private var isOnScreen = false

	var body: some View {
		VUMeterView(model: model)
			// Its bars' labels and track read on the darkened art.
			.environment(\.colorScheme, .dark)
			.onAppear { isOnScreen = true }
			.onDisappear { isOnScreen = false }
			.onChange(of: isOnScreen && scenePhase == .active, initial: true) { _, metering in
				if metering {
					OutputMeter.shared.subscribe(model, player: player.audioPlayer) { [weak model] metrics in
						model?.update(metrics)
					}
				} else {
					OutputMeter.shared.unsubscribe(model)
				}
			}
	}
}
