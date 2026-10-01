import SwiftUI

@MainActor private final class ListenBrainzPrefs: ObservableObject {
    private var isActive = true

    @Published var enableScrobbling: Bool {
        didSet {
            guard isActive else { return }
            UserDefaults.standard.set(enableScrobbling, forKey: ListenBrainzScrobbler.enabledKey)
            if enableScrobbling {
                ListenBrainzScrobbler.shared.flush()
            }
        }
    }
    @Published var apiAddress: String {
        didSet {
            guard isActive else { return }
            let trimmed = apiAddress.trimmingCharacters(in: .whitespaces)
            if trimmed.isEmpty {
                UserDefaults.standard.removeObject(forKey: ListenBrainzScrobbler.rootKey)
            } else {
                UserDefaults.standard.set(trimmed, forKey: ListenBrainzScrobbler.rootKey)
            }
        }
    }

    @Published var token: String = ""
    @Published var connectedUsername: String
    @Published var isAuthenticated: Bool
    @Published var isAuthenticating: Bool = false
    @Published var errorMessage: String?
    @Published var pendingCount: Int

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        enableScrobbling = d.bool(forKey: ListenBrainzScrobbler.enabledKey)
        apiAddress = d.string(forKey: ListenBrainzScrobbler.rootKey) ?? ListenBrainzAPI.defaultRoot
        let username = d.string(forKey: ListenBrainzScrobbler.usernameKey) ?? ""
        connectedUsername = username
        isAuthenticated = !username.isEmpty && ListenBrainzScrobbler.shared.token != nil
        pendingCount = ListenBrainzScrobbler.shared.pendingCount
    }

    func connect() {
        let token = token.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !token.isEmpty else { return }
        isAuthenticating = true
        errorMessage = nil

        Task {
            defer { self.isAuthenticating = false }
            do {
                let name = try await ListenBrainzScrobbler.shared.connect(token: token)
                guard self.isActive else { return }
                self.connectedUsername = name
                self.isAuthenticated = true
                self.enableScrobbling = true
                self.token = ""
            } catch ListenBrainzError.unauthorized {
                self.errorMessage = NSLocalizedString(
                    "ListenBrainz does not know that token.",
                    comment: "ListenBrainz token rejected while connecting"
                )
            } catch {
                self.errorMessage = NSLocalizedString(
                    "Could not connect to ListenBrainz. Please try again.",
                    comment: "ListenBrainz connect network error"
                )
            }
        }
    }

    func disconnect() {
        ListenBrainzScrobbler.shared.disconnect()
        connectedUsername = ""
        isAuthenticated = false
    }

    func tokenWasRejected() {
        connectedUsername = ""
        isAuthenticated = false
        errorMessage = NSLocalizedString(
            "ListenBrainz rejected the token. Connect again.",
            comment: "ListenBrainz token rejected while sending"
        )
    }

    func refreshPending() {
        pendingCount = ListenBrainzScrobbler.shared.pendingCount
    }
}

struct ListenBrainzPaneView: View {
    @StateObject private var prefs = ListenBrainzPrefs()

    var body: some View {
        Group {
            if #available(macOS 13.0, *) {
                formContent.formStyle(.grouped)
            } else {
                formContent.padding()
            }
        }
        .onReceive(NotificationCenter.default.publisher(for: ListenBrainzScrobbler.tokenRejected)) { _ in
            prefs.tokenWasRejected()
        }
        .onReceive(NotificationCenter.default.publisher(for: ListenBrainzScrobbler.queueChanged)) { _ in
            prefs.refreshPending()
        }
    }

    private var formContent: some View {
        Form {
            Toggle(
                NSLocalizedString("Enable ListenBrainz scrobbling", comment: "ListenBrainz pref toggle"),
                isOn: $prefs.enableScrobbling
            )

            if prefs.isAuthenticated {
                connectedView
            } else {
                disconnectedView
            }

            if prefs.pendingCount > 0 {
                Text(String(
                    format: NSLocalizedString("Listens waiting to be sent: %d", comment: "ListenBrainz queue size"),
                    prefs.pendingCount
                ))
                .foregroundColor(.secondary)
            }

            Section {
                TextField(
                    NSLocalizedString("API address", comment: "ListenBrainz server address field"),
                    text: $prefs.apiAddress
                )
                Text(NSLocalizedString(
                    "Servers that speak the ListenBrainz API, such as Maloja, work too.",
                    comment: "ListenBrainz server note"
                ))
                .font(.caption)
                .foregroundColor(.secondary)
            } header: {
                Text(NSLocalizedString("Server", comment: "ListenBrainz server section")).bold()
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
            SecureField(
                NSLocalizedString("User token", comment: "ListenBrainz token field"),
                text: $prefs.token
            )
            .onSubmit { prefs.connect() }

            Link(
                NSLocalizedString("Find your token on ListenBrainz", comment: "ListenBrainz settings link"),
                destination: URL(string: "https://listenbrainz.org/settings/")!
            )

            HStack {
                Button(NSLocalizedString("Connect", comment: "Last.FM connect button")) {
                    prefs.connect()
                }
                .disabled(prefs.token.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty || prefs.isAuthenticating)

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
