import SwiftUI

/// Backport of @StateObject for macOS 10.15 (retains object identity via @State,
/// observes changes via @ObservedObject). update() is called by SwiftUI before each
/// body evaluation to sync the two.
@MainActor @propertyWrapper
struct StateObjectCompat<ObjectType: ObservableObject>: @MainActor DynamicProperty {
    @State private var storage: ObjectType
    @ObservedObject private var observed: ObjectType

    init(wrappedValue thunk: @autoclosure @escaping () -> ObjectType) {
        let instance = thunk()
        _storage = State(initialValue: instance)
        _observed = ObservedObject(initialValue: instance)
    }

    mutating func update() {
        _observed = ObservedObject(wrappedValue: storage)
    }

    var wrappedValue: ObjectType { storage }
    var projectedValue: ObservedObject<ObjectType>.Wrapper { $observed }
}

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
                    let suggester = PathSuggester()
                    suggester.beginSuggestion(window)
                    // Observe PathSuggester window close to notify GeneralPaneView
                    if let w = suggester.window {
                        NotificationCenter.default.addObserver(
                            forName: NSWindow.willCloseNotification,
                            object: w,
                            queue: .main
                        ) { _ in
                            NotificationCenter.default.post(
                                name: .cogSandboxPathsChanged, object: nil
                            )
                        }
                    }
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
        ]
    }

    private static func paneIcon(system: String, legacy: String) -> NSImage {
        if #available(macOS 11.0, *) {
            if let img = NSImage(systemSymbolName: system, accessibilityDescription: nil) {
                return img
            }
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
