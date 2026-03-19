import SwiftUI

private let shortcuts: [(label: LocalizedStringKey, key: String)] = [
    ("Play / Pause",    "cogPlayShortcutV2"),
    ("Next",            "cogNextShortcutV2"),
    ("Previous",        "cogPrevShortcutV2"),
    ("Spam",            "cogSpamShortcutV2"),
    ("Fade",            "cogFadeShortcutV2"),
    ("Seek Backward",   "cogSeekBackwardShortcutV2"),
    ("Seek Forward",    "cogSeekForwardShortcutV2"),
]

struct HotKeyPaneView: View {
    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var formContent: some View {
        Form {
            ForEach(shortcuts, id: \.key) { item in
                HStack {
                    Text(item.label)
                    Spacer()
                    ShortcutField(userDefaultsKey: item.key)
                        .frame(width: 160, height: 24)
                }
            }
            HStack {
                Spacer()
                Button("Reset All") { resetAllShortcuts() }
            }
        }
    }

    private func resetAllShortcuts() {
        let defaults = UserDefaults.standard
        for item in shortcuts {
            defaults.removeObject(forKey: item.key)
        }
    }
}
