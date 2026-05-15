import SwiftUI
import Sparkle

struct SandboxPathEntry: Identifiable {
    let id: Int
    let path: String
    let valid: String
    let token: AnyObject?
}

@MainActor
private final class GeneralPrefs: ObservableObject {
    private var isActive = true

    @Published var allowInsecureSSL: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(allowInsecureSSL, forKey: "allowInsecureSSL") }
    }
    @Published var sentryConsented: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(sentryConsented, forKey: "sentryConsented") }
    }

    @Published var automaticallyChecksForUpdates: Bool {
        didSet { guard isActive else { return }; SparkleBridge.sharedStandardUpdaterController()?.updater.automaticallyChecksForUpdates = automaticallyChecksForUpdates }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        allowInsecureSSL = d.bool(forKey: "allowInsecureSSL")
        sentryConsented = d.bool(forKey: "sentryConsented")
        automaticallyChecksForUpdates = SparkleBridge.sharedStandardUpdaterController()?.updater.automaticallyChecksForUpdates ?? false
    }
}

struct GeneralPaneView: View {
    @StateObjectCompat private var pathModel = SandboxPathModel()
    @StateObjectCompat private var prefs = GeneralPrefs()
    @State private var selectedIndices: Set<Int> = []

    private var pathEntries: [SandboxPathEntry] {
        pathModel.entries.enumerated().map { (i, dict) in
            SandboxPathEntry(
                id: i,
                path: dict["path"] as? String ?? "",
                valid: dict["valid"] as? String ?? "",
                token: dict["token"] as AnyObject?
            )
        }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("Sandbox-accessible paths:")
                .font(.headline)
                .padding([.horizontal, .top])

            if #available(macOS 12.0, *) {
                pathTable
            } else {
                pathList
            }

            actionBar

            Divider()

            if #available(macOS 13.0, *) {
                settingsForm.formStyle(.grouped)
            } else {
                settingsForm.padding()
            }
        }
        .onAppear { pathModel.refresh() }
        .onReceive(NotificationCenter.default.publisher(for: .cogSandboxPathsChanged)) { _ in
            pathModel.refresh()
            selectedIndices.removeAll()
        }
    }

    @available(macOS 12.0, *)
    private var pathTable: some View {
        Table(pathEntries, selection: $selectedIndices) {
            TableColumn("Path") { entry in
                Text(entry.path)
                    .lineLimit(1)
                    .truncationMode(.middle)
            }
            TableColumn("Status") { entry in
                Text(entry.valid)
                    .foregroundColor(.secondary)
                    .font(.caption)
            }
            .width(60)
        }
    }

    private var pathList: some View {
        List(selection: $selectedIndices) {
            ForEach(pathEntries) { entry in
                HStack {
                    Text(entry.path)
                        .lineLimit(1)
                        .truncationMode(.middle)
                    Spacer()
                    Text(entry.valid)
                        .foregroundColor(.secondary)
                        .font(.caption)
                }
                .tag(entry.id)
            }
        }
        .listStyle(PlainListStyle())
    }

    private var settingsForm: some View {
        Form {
            Section {
                Toggle("Allow insecure SSL connections", isOn: $prefs.allowInsecureSSL)
                Toggle("Send crash reports and usage data", isOn: $prefs.sentryConsented)
            }
            Section {
                Toggle(
                    "Check for updates automatically",
                    isOn: $prefs.automaticallyChecksForUpdates
                )
            }
        }
    }

    private var actionBar: some View {
        HStack(spacing: 8) {
            Button("Add…") {
                let panel = NSOpenPanel()
                panel.allowsMultipleSelection = false
                panel.canChooseDirectories = true
                panel.canChooseFiles = false
                panel.isFloatingPanel = true
                if panel.runModal() == .OK, let url = panel.url {
                    pathModel.addUrl(url)
                    selectedIndices.removeAll()
                }
            }
            Button("Delete") {
                let tokens = selectedIndices.sorted(by: >).compactMap {
                    pathEntries[safe: $0]?.token
                }
                pathModel.removeSelectedTokens(tokens)
                selectedIndices.removeAll()
            }
            .disabled(selectedIndices.isEmpty)
            Button("Delete Invalid") {
                pathModel.removeStaleEntries()
                selectedIndices.removeAll()
            }
            Button("Suggest…") {
                NotificationCenter.default.post(
                    name: Notification.Name("CogShowPathSuggester"),
                    object: nil
                )
            }
            Spacer()
            Button("Refresh") {
                pathModel.refresh()
                selectedIndices.removeAll()
            }
        }
        .padding()
    }
}

private extension Array {
    subscript(safe index: Int) -> Element? {
        indices.contains(index) ? self[index] : nil
    }
}
