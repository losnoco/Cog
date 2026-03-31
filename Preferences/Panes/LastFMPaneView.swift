import SwiftUI

private final class LastFMPrefs: ObservableObject {
    private var isActive = true

    @Published var enableScrobbling: Bool {
        didSet {
            guard isActive else { return }
            UserDefaults.standard.set(enableScrobbling, forKey: "enableAudioScrobbler")
        }
    }

    @Published var username: String = ""
    @Published var password: String = ""
    @Published var connectedUsername: String
    @Published var isAuthenticated: Bool
    @Published var isAuthenticating: Bool = false
    @Published var errorMessage: String?

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        enableScrobbling = d.object(forKey: "enableAudioScrobbler") as? Bool ?? false
        let username = d.string(forKey: "lastFmUsername") ?? ""
        connectedUsername = username
        isAuthenticated = !username.isEmpty
    }

    func connect() {
        guard !username.isEmpty, !password.isEmpty else { return }
        isAuthenticating = true
        errorMessage = nil

        let api = LastFMAPI(apiKey: Secrets.lastFmApiKey, apiSecret: Secrets.lastFmApiSecret)
        api.authenticateMobile(username: username, password: password) { [weak self] result in
            DispatchQueue.main.async {
                guard let self, self.isActive else { return }
                self.isAuthenticating = false
                switch result {
                case .success(let auth):
                    guard KeychainHelper.save(sessionKey: auth.sessionKey) else {
                        self.errorMessage = NSLocalizedString(
                            "Could not save credentials to Keychain.",
                            comment: "Last.FM keychain save error"
                        )
                        return
                    }
                    UserDefaults.standard.set(auth.username, forKey: "lastFmUsername")
                    self.connectedUsername = auth.username
                    self.isAuthenticated = true
                    self.username = ""
                    self.password = ""
                case .failure(let error):
                    if case LastFMAPIError.apiError(let message) = error {
                        self.errorMessage = message
                    } else {
                        self.errorMessage = NSLocalizedString(
                            "Could not connect to Last.FM. Please try again.",
                            comment: "Last.FM auth network error"
                        )
                    }
                }
            }
        }
    }

    func disconnect() {
        KeychainHelper.delete()
        UserDefaults.standard.removeObject(forKey: "lastFmUsername")
        connectedUsername = ""
        isAuthenticated = false
    }
}

struct LastFMPaneView: View {
    @StateObjectCompat private var prefs = LastFMPrefs()

    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var formContent: some View {
        Form {
            Toggle(
                NSLocalizedString("Enable Last.FM scrobbling", comment: "Last.FM pref toggle"),
                isOn: $prefs.enableScrobbling
            )

            if prefs.isAuthenticated {
                connectedView
            } else {
                disconnectedView
            }
        }
    }

    private var connectedView: some View {
        Section {
            HStack(spacing: 6) {
                Circle()
                    .fill(Color.green)
                    .frame(width: 8, height: 8)
                Text(String(
                    format: NSLocalizedString("Connected as %@", comment: "Last.FM connected status"),
                    prefs.connectedUsername
                ))
            }
            Button(NSLocalizedString("Disconnect", comment: "Last.FM disconnect button")) {
                prefs.disconnect()
            }
        }
    }

    private var disconnectedView: some View {
        Section {
            TextField(
                NSLocalizedString("Username", comment: "Last.FM username field"),
                text: $prefs.username
            )
            .textContentType(.username)

            SecureField(
                NSLocalizedString("Password", comment: "Last.FM password field"),
                text: $prefs.password
            )
            .textContentType(.password)

            HStack {
                Button(NSLocalizedString("Connect", comment: "Last.FM connect button")) {
                    prefs.connect()
                }
                .disabled(prefs.username.isEmpty || prefs.password.isEmpty || prefs.isAuthenticating)

                if prefs.isAuthenticating {
                    ProgressView()
                        .controlSize(.small)
                }
            }

            if let errorMessage = prefs.errorMessage {
                Text(errorMessage)
                    .foregroundColor(.red)
                    .font(.caption)
            }
        }
    }
}
