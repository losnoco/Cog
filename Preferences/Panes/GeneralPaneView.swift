import SwiftUI

struct SandboxPathEntry: Identifiable {
    let id: Int
    let path: String
    let valid: String
    let token: AnyObject?
}

private let httpStreamingBufferSizeOptions: [(String, Int)] = [
    ("64 KB", 0x10000),
    ("128 KB", 0x20000),
    ("256 KB", 0x40000),
    ("512 KB", 0x80000),
    ("1 MB", 0x100000),
    ("2 MB", 0x200000),
    ("4 MB", 0x400000),
    ("8 MB", 0x800000),
    ("16 MB", 0x1000000),
    ("32 MB", 0x2000000),
    ("64 MB", 0x4000000),
    ("128 MB", 0x8000000),
]

private func normalizedHTTPStreamingBufferSize(_ size: Int) -> Int {
    guard !httpStreamingBufferSizeOptions.isEmpty else { return 0x40000 }

    return httpStreamingBufferSizeOptions.min { lhs, rhs in
        abs(lhs.1 - size) < abs(rhs.1 - size)
    }?.1 ?? 0x40000
}

private final class GeneralPrefs: ObservableObject {
    private var isActive = true

    @Published var allowInsecureSSL: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(allowInsecureSSL, forKey: "allowInsecureSSL") }
    }
    @Published var httpStreamingBufferSize: Int {
        didSet { guard isActive else { return }; UserDefaults.standard.set(httpStreamingBufferSize, forKey: "httpStreamingBufferSize") }
    }
    @Published var sentryConsented: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(sentryConsented, forKey: "sentryConsented") }
    }

    @Published var suCheckAtStartup: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(suCheckAtStartup, forKey: "SUCheckAtStartup") }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        allowInsecureSSL = d.bool(forKey: "allowInsecureSSL")
        let storedHTTPStreamingBufferSize = d.object(forKey: "httpStreamingBufferSize") == nil ? 0x40000 : d.integer(forKey: "httpStreamingBufferSize")
        httpStreamingBufferSize = normalizedHTTPStreamingBufferSize(storedHTTPStreamingBufferSize)
        sentryConsented = d.bool(forKey: "sentryConsented")
        suCheckAtStartup = d.bool(forKey: "SUCheckAtStartup")
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
                Picker("HTTP/HTTPS streaming buffer:", selection: $prefs.httpStreamingBufferSize) {
                    ForEach(httpStreamingBufferSizeOptions, id: \.1) { option in
                        Text(option.0).tag(option.1)
                    }
                }
            } header: {
                Text("Network").bold()
            }
            Section {
                Toggle("Send crash reports and usage data", isOn: $prefs.sentryConsented)
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
