import SwiftUI

struct PreferencePaneDescriptor: Identifiable {
    var id: String { title }
    /// Stable key used for persistence (LastPreferencePane) and selection — not translated.
    let title: String
    /// Display name shown in the sidebar.
    let localizedTitle: String
    let icon: NSImage
    let body: AnyView
    var showPathSuggesterAction: ((NSWindow) -> Void)? = nil

    @MainActor static func allPanes() -> [PreferencePaneDescriptor] {
        [
            PreferencePaneDescriptor(
                title: "Playlist",
                localizedTitle: NSLocalizedString("Playlist", comment: "Preference pane title"),
                icon: paneIcon(system: "music.note.list", legacy: "playlist"),
                body: AnyView(PlaylistPaneView())
            ),
            PreferencePaneDescriptor(
                title: "Hot Keys",
                localizedTitle: NSLocalizedString("Hot Keys", comment: "Preference pane title"),
                icon: paneIcon(system: "keyboard", legacy: "hot_keys"),
                body: AnyView(HotKeyPaneView())
            ),
            PreferencePaneDescriptor(
                title: "Output",
                localizedTitle: NSLocalizedString("Output", comment: "Preference pane title"),
                icon: paneIcon(system: "hifispeaker.2.fill", legacy: "output"),
                body: AnyView(OutputPaneView())
            ),
            PreferencePaneDescriptor(
                title: "General",
                localizedTitle: NSLocalizedString("General", comment: "Preference pane title"),
                icon: paneIcon(system: "gearshape.fill", legacy: "general"),
                body: AnyView(GeneralPaneView()),
                showPathSuggesterAction: { window in
                    PathSuggesterPresenter.show(from: window)
                }
            ),
            PreferencePaneDescriptor(
                title: "Notifications",
                localizedTitle: NSLocalizedString("Notifications", comment: "Preference pane title"),
                icon: paneIcon(system: "bell.fill", legacy: "growl"),
                body: AnyView(NotificationsPaneView())
            ),
            PreferencePaneDescriptor(
                title: "Appearance",
                localizedTitle: NSLocalizedString("Appearance", comment: "Preference pane title"),
                icon: paneIcon(system: "paintpalette.fill", legacy: "appearance"),
                body: AnyView(AppearancePaneView())
            ),
            PreferencePaneDescriptor(
                title: "Synthesis",
                localizedTitle: NSLocalizedString("Synthesis", comment: "Preference pane title"),
                icon: paneIcon(system: "pianokeys", legacy: "midi"),
                body: AnyView(MIDIPaneView())
            ),
            PreferencePaneDescriptor(
                title: "Rubber Band",
                localizedTitle: NSLocalizedString("Rubber Band", comment: "Preference pane title"),
                icon: paneIcon(system: "deskclock", legacy: "rubberband"),
                body: AnyView(RubberbandPaneView())
            ),
            PreferencePaneDescriptor(
                title: "Last.FM",
                localizedTitle: NSLocalizedString("Last.FM", comment: "Preference pane title"),
                icon: paneIcon(system: "music.note", legacy: "lastfm"),
                body: AnyView(LastFMPaneView())
            ),
            PreferencePaneDescriptor(
                title: "ListenBrainz",
                localizedTitle: NSLocalizedString("ListenBrainz", comment: "Preference pane title"),
                icon: paneIcon(system: "brain", legacy: "lastfm"),
                body: AnyView(ListenBrainzPaneView())
            ),
            PreferencePaneDescriptor(
                title: "Remote Control",
                localizedTitle: NSLocalizedString("Remote Control", comment: "Preference pane title"),
                icon: paneIcon(system: "network", legacy: "general"),
                body: AnyView(RemoteControlPaneView())
            ),
        ]
    }

    private static func paneIcon(system: String, legacy: String) -> NSImage {
        if let img = NSImage(systemSymbolName: system, accessibilityDescription: nil) {
            return img
        }
        if let img = Bundle.main.image(forResource: legacy) {
            return img
        }
        return NSImage()
    }
}

extension Notification.Name {
    static let cogSandboxPathsChanged = Notification.Name("CogSandboxPathsChanged")
}

/// Keeps the path suggester alive while its window is open. Nothing else
/// holds its window controller, and once that goes, so do the list feeding
/// its table and the controller that saves the folders chosen.
@MainActor
enum PathSuggesterPresenter {
    private static var suggester: PathSuggester?
    private static var closeObserver: NSObjectProtocol?

    static func show(from window: NSWindow) {
        if let suggester {
            // Already open: suggest afresh, from what is granted now.
            suggester.beginSuggestion(window)
            return
        }
        let suggester = PathSuggester()
        self.suggester = suggester
        suggester.beginSuggestion(window)
        closeObserver = NotificationCenter.default.addObserver(
            forName: NSWindow.willCloseNotification,
            object: suggester.window,
            queue: .main
        ) { _ in
            MainActor.assumeIsolated {
                if let closeObserver {
                    NotificationCenter.default.removeObserver(closeObserver)
                }
                closeObserver = nil
                self.suggester = nil
                // The General pane lists what was granted.
                NotificationCenter.default.post(name: .cogSandboxPathsChanged, object: nil)
            }
        }
    }
}
