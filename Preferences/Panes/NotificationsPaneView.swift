import SwiftUI

private final class NotificationsPrefs: ObservableObject {
    private var isActive = true

    @Published var enable: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enable, forKey: "notifications.enable") }
    }
    @Published var itunesStyle: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(itunesStyle, forKey: "notifications.itunes-style") }
    }
    @Published var showAlbumArt: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(showAlbumArt, forKey: "notifications.show-album-art") }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        enable = d.object(forKey: "notifications.enable") as? Bool ?? true
        itunesStyle = d.object(forKey: "notifications.itunes-style") as? Bool ?? true
        showAlbumArt = d.object(forKey: "notifications.show-album-art") as? Bool ?? true
    }
}

struct NotificationsPaneView: View {
    @StateObjectCompat private var prefs = NotificationsPrefs()

    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var formContent: some View {
        Form {
            Toggle("Enable Notifications", isOn: $prefs.enable)
            Toggle("Show Album Art", isOn: $prefs.showAlbumArt)
        }
    }
}
