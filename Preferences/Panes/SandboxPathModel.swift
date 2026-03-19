import Foundation

/// Wraps SandboxPathBehaviorController (ObjC, in main target) for use in SwiftUI.
@MainActor
final class SandboxPathModel: ObservableObject {
    /// Each entry has "path" (String), "valid" (String), "token" (AnyObject).
    @Published var entries: [[String: Any]] = []

    private var controller: SandboxPathBehaviorController?

    init() {
        controller = SandboxPathBehaviorController()
        refresh()
    }

    func refresh() {
        controller?.refresh()
        reloadEntries()
    }

    func addUrl(_ url: URL) {
        controller?.add(url as URL)
        reloadEntries()
    }

    func removeSelectedTokens(_ tokens: [AnyObject]) {
        for token in tokens {
            controller?.removeToken(token)
        }
        reloadEntries()
    }

    func removeStaleEntries() {
        controller?.removeStaleEntries()
        reloadEntries()
    }

    private func reloadEntries() {
        entries = (controller?.arrangedObjects as? [[String: Any]]) ?? []
    }
}
