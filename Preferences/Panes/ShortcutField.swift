import SwiftUI

/// Wraps MASShortcutView for use in SwiftUI.
/// MASShortcutView manages its own UserDefaults binding internally via `associatedUserDefaultsKey`.
struct ShortcutField: NSViewRepresentable {
    let userDefaultsKey: String

    func makeNSView(context: Context) -> MASShortcutView {
        let view = MASShortcutView()
        view.associatedUserDefaultsKey = userDefaultsKey
        return view
    }

    func updateNSView(_ nsView: MASShortcutView, context: Context) {
        nsView.associatedUserDefaultsKey = userDefaultsKey
    }
}
