import SwiftUI

@available(macOS 13.0, *)
@MainActor
private final class RemoteControlPrefs: ObservableObject {
    private var isActive = true

    @Published var enabled: Bool {
        didSet {
            guard isActive else { return }
            UserDefaults.standard.set(enabled, forKey: RemoteControlBootstrap.enabledKey)
            RemoteControlBootstrap.shared.applyEnabledChange(enabled)
        }
    }

    deinit { isActive = false }

    init() {
        enabled = UserDefaults.standard.bool(forKey: RemoteControlBootstrap.enabledKey)
    }
}

@available(macOS 13.0, *)
struct RemoteControlPaneAvailableView: View {
    @StateObject private var prefs = RemoteControlPrefs()
    @ObservedObject private var status = RemoteControlBootstrap.shared.statusModel

    var body: some View {
        Form {
            Section {
                Toggle(NSLocalizedString("Allow remote control via MCP", comment: "Remote Control preference"), isOn: $prefs.enabled)
            } footer: {
                Text(NSLocalizedString("Lets MCP clients like Claude control playback and the playlist. Add the command below to your MCP client (stdio); it only connects to Cog on this Mac and works only while this is enabled.", comment: "Remote Control preference"))
                    .foregroundColor(.secondary)
            }

            Section {
                if status.isRunning, let command = status.clientCommand {
                    HStack {
                        Text(NSLocalizedString("MCP client command", comment: "Remote Control preference"))
                        Text(command)
                            .textSelection(.enabled)
                            .font(.system(.body, design: .monospaced))
                            .lineLimit(1)
                            .truncationMode(.middle)
                        Button {
                            NSPasteboard.general.clearContents()
                            NSPasteboard.general.setString(command, forType: .string)
                        } label: {
                            Image(systemName: "doc.on.doc")
                        }
                        .buttonStyle(.borderless)
                        .help(NSLocalizedString("Copy command", comment: "Remote Control preference"))
                    }
                } else if prefs.enabled {
                    Text(NSLocalizedString("Server not running.", comment: "Remote Control preference"))
                        .foregroundColor(.secondary)
                }

                if let error = status.lastError {
                    Text(error)
                        .foregroundColor(.red)
                }
            }
        }
        .formStyle(.grouped)
    }
}
