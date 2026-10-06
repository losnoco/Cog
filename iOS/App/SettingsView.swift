//
//  SettingsView.swift
//  Cog (iOS)
//
//  The settings iOS has a use for so far, under the defaults keys the
//  macOS app uses, which CogAudio and the plugins read.
//

import SwiftUI

struct SettingsView: View {
	@Environment(\.dismiss) private var dismiss
	@AppStorage("enableFading") private var fading = true
	@AppStorage("volumeScaling") private var replayGain = "albumGainWithPeak"
	@AppStorage("alwaysStopAfterCurrent") private var stopAfterEach = false
	@AppStorage("enableSpatialAudio") private var spatialAudio = false
	@AppStorage("enableHeadTracking") private var headTracking = false

	var body: some View {
		NavigationStack {
			Form {
				Section("Playback") {
					Toggle("Fade on Pause and Seek", isOn: $fading)
					Toggle("Stop After Each Track", isOn: $stopAfterEach)
				}
				Section {
					Picker("ReplayGain", selection: $replayGain) {
						Text("Album, Preventing Clipping").tag("albumGainWithPeak")
						Text("Album").tag("albumGain")
						Text("Track, Preventing Clipping").tag("trackGainWithPeak")
						Text("Track").tag("trackGain")
						Text("Sound Check Only").tag("soundcheck")
						Text("Off").tag("none")
					}
				} footer: {
					Text("Album gain falls back to track gain when a file has no album gain.")
				}
				Section {
					Toggle("Spatialize Surround", isOn: $spatialAudio)
					Toggle("Head Tracking", isOn: $headTracking)
						.disabled(!spatialAudio)
				} header: {
					Text("Spatial Audio")
				} footer: {
					Text("Surround tracks are spatialized on headphones and the built-in speaker.")
				}
			}
			.navigationTitle("Settings")
			.navigationBarTitleDisplayMode(.inline)
			.toolbar {
				ToolbarItem(placement: .confirmationAction) {
					Button("Done") { dismiss() }
				}
			}
		}
	}
}
