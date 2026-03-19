import SwiftUI

@objc class PreferencesWindow: NSWindow {

    private let model: PreferencesModel

    @objc init() {
        model = PreferencesModel(panes: PreferencePaneDescriptor.allPanes())
        super.init(
            contentRect: NSRect(x: 0, y: 0, width: 820, height: 480),
            styleMask: [.titled, .closable, .resizable, .miniaturizable],
            backing: .buffered,
            defer: false
        )
        isReleasedWhenClosed = false
        title = NSLocalizedString("PreferencesTitle", comment: "")
        contentView = NSHostingView(rootView: PreferencesRootView(model: model))
        center()

        // Observe "Suggest…" button in GeneralPaneView
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(handleShowPathSuggesterFromPane),
            name: Notification.Name("CogShowPathSuggester"),
            object: nil
        )
    }

    @objc private func handleShowPathSuggesterFromPane() {
        model.selectedPane?.showPathSuggesterAction?(self)
    }

    @objc func show() {
        var lastPane = UserDefaults.standard.string(forKey: "LastPreferencePane")
        // Migrate stale pane names from older versions.
        if lastPane == "Growl" { lastPane = "Notifications" }
        if lastPane == "Last.fm" || lastPane == "Scrobble" { lastPane = "General" }
        model.select(named: lastPane ?? "")
        if model.selectedTitle == nil {
            model.select(named: model.panes.first?.title ?? "")
        }
        makeKeyAndOrderFront(nil)
    }

    @objc func showPathSuggester() {
        model.select(named: "General")
        makeKeyAndOrderFront(nil)
        model.selectedPane?.showPathSuggesterAction?(self)
    }

    @objc func showRubberbandSettings() {
        model.select(named: "Rubber Band")
        makeKeyAndOrderFront(nil)
    }

    override func cancelOperation(_ sender: Any?) {
        close()
    }
}
