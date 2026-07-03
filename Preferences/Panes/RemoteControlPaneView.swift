import SwiftUI

/// Dispatcher for the Remote Control pane: the real UI needs the
/// CogRemoteControl framework (macOS 13+); older systems get a notice.
struct RemoteControlPaneView: View {
    var body: some View {
        if #available(macOS 13.0, *) {
            RemoteControlPaneAvailableView()
        } else {
            VStack(spacing: 8) {
                Text("Remote Control requires macOS 13 or later.")
                    .font(.headline)
                Text("MCP clients can only control Cog on macOS 13+.")
                    .foregroundColor(.secondary)
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .padding()
        }
    }
}
