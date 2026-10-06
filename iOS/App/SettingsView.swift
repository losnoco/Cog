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
	@AppStorage("enableLrclib") private var lrclib = false

	var body: some View {
		NavigationStack {
			Form {
				MusicLocationsSection()
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
				Section {
					Toggle("Look Up Lyrics", isOn: $lrclib)
				} header: {
					Text("Lyrics")
				} footer: {
					Text("For tracks without lyrics of their own, Cog asks LRCLIB (lrclib.net), sending the title, artist, album and length.")
				}
				LastFMSection()
				ListenBrainzSection()
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

/// What Cog may play outside its own folder, and taking that back.
struct MusicLocationsSection: View {
	@EnvironmentObject private var locations: MusicLocations

	var body: some View {
		Section {
			ForEach(locations.locations) { location in
				VStack(alignment: .leading, spacing: 2) {
					Text(location.name)
					Text(location.path)
						.font(.caption)
						.foregroundStyle(.secondary)
						.lineLimit(1)
						.truncationMode(.head)
				}
			}
			.onDelete { offsets in
				offsets.map { locations.locations[$0] }.forEach(locations.remove)
			}
		} header: {
			Text("Music Locations")
		} footer: {
			Text(locations.locations.isEmpty
				? "Files and folders you add from Files play where they are, and show up here."
				: "Files and folders Cog plays where they are. Removing one leaves its tracks in the playlist, unplayable.")
		}
	}
}

/// Last.fm: signing in (which needs the build's API key) and scrobbling.
struct LastFMSection: View {
	@AppStorage("lastFmUsername") private var username = ""
	@AppStorage("enableAudioScrobbler") private var enabled = false
	@State private var name = ""
	@State private var password = ""
	@State private var signingIn = false
	@State private var error: String?

	private var hasKey: Bool { !Secrets.lastFmApiKey.isEmpty && !Secrets.lastFmApiSecret.isEmpty }

	var body: some View {
		Section {
			if !username.isEmpty {
				LabeledContent("Signed In As", value: username)
				Toggle("Scrobble", isOn: $enabled)
				Button("Sign Out", role: .destructive) {
					_ = KeychainHelper.delete()
					username = ""
				}
			} else {
				TextField("Username", text: $name)
					.textContentType(.username)
					.textInputAutocapitalization(.never)
					.autocorrectionDisabled()
				SecureField("Password", text: $password)
					.textContentType(.password)
				Button(signingIn ? "Signing In…" : "Sign In") { signIn() }
					.disabled(name.isEmpty || password.isEmpty || signingIn)
			}
		} header: {
			Text("Last.fm")
		} footer: {
			if !hasKey {
				Text("This build has no Last.fm API key. Put yours in Xcode-config/Secrets.xcconfig (see Secrets.template.xcconfig) and build again.")
			} else if let error {
				Text(error).foregroundStyle(.red)
			}
		}
		.disabled(!hasKey)
	}

	private func signIn() {
		signingIn = true
		error = nil
		LastFMAPI(apiKey: Secrets.lastFmApiKey, apiSecret: Secrets.lastFmApiSecret).authenticateMobile(username: name, password: password) { result in
			Task { @MainActor in
				signingIn = false
				switch result {
				case .success(let auth):
					guard KeychainHelper.save(sessionKey: auth.sessionKey) else {
						error = String(localized: "Could not save credentials to the Keychain.")
						return
					}
					username = auth.username
					enabled = true
					name = ""
					password = ""
				case .failure(let failure):
					if case LastFMAPIError.apiError(let message) = failure {
						error = message
					} else {
						error = String(localized: "Could not connect to Last.fm. Please try again.")
					}
				}
			}
		}
	}
}

/// ListenBrainz: a user token, then listens submitted (and queued while
/// offline).
struct ListenBrainzSection: View {
	@AppStorage("listenBrainzUsername") private var username = ""
	@AppStorage("enableListenBrainz") private var enabled = false
	@State private var token = ""
	@State private var connecting = false
	@State private var error: String?

	var body: some View {
		Section {
			if !username.isEmpty {
				LabeledContent("Connected As", value: username)
				Toggle("Submit Listens", isOn: $enabled)
				Button("Disconnect", role: .destructive) {
					ListenBrainzScrobbler.shared.disconnect()
					username = ""
				}
			} else {
				SecureField("User Token", text: $token)
					.textInputAutocapitalization(.never)
					.autocorrectionDisabled()
				Button(connecting ? "Connecting…" : "Connect") { connect() }
					.disabled(token.isEmpty || connecting)
			}
		} header: {
			Text("ListenBrainz")
		} footer: {
			if let error {
				Text(error).foregroundStyle(.red)
			} else if username.isEmpty {
				Text("Your token is on listenbrainz.org, under Settings.")
			}
		}
	}

	private func connect() {
		connecting = true
		error = nil
		Task {
			do {
				username = try await ListenBrainzScrobbler.shared.connect(token: token.trimmingCharacters(in: .whitespaces))
				token = ""
			} catch {
				self.error = String(localized: "ListenBrainz did not accept that token.")
			}
			connecting = false
		}
	}
}
