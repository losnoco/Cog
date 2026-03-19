import SwiftUI

private final class AppearancePrefs: ObservableObject {
    private var isActive = true

    @Published var customDockIcons: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(customDockIcons, forKey: "customDockIcons") }
    }
    @Published var colorfulDockIcons: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(colorfulDockIcons, forKey: "colorfulDockIcons") }
    }
    @Published var customDockIconsPlaque: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(customDockIconsPlaque, forKey: "customDockIconsPlaque") }
    }
    @Published var spectrumSceneKit: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(spectrumSceneKit, forKey: "spectrumSceneKit") }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        customDockIcons = d.bool(forKey: "customDockIcons")
        colorfulDockIcons = d.bool(forKey: "colorfulDockIcons")
        customDockIconsPlaque = d.bool(forKey: "customDockIconsPlaque")
        spectrumSceneKit = d.object(forKey: "spectrumSceneKit") as? Bool ?? true
    }
}

struct AppearancePaneView: View {
    @StateObjectCompat private var prefs = AppearancePrefs()

    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var formContent: some View {
        Form {
            Section(header: Text("Dock Icon").bold()) {
                Toggle("Use custom dock icons", isOn: $prefs.customDockIcons)
                if #unavailable(macOS 26.0) {
                    Toggle("Colorful dock icons", isOn: $prefs.colorfulDockIcons)
                }
                Toggle("Show dock icon plaque", isOn: $prefs.customDockIconsPlaque)
                HStack {
                    Text("Stop icon:")
                    Spacer()
                    Button("Choose…") { pickDockIcon("Stop") }
                }
                HStack {
                    Text("Play icon:")
                    Spacer()
                    Button("Choose…") { pickDockIcon("Play") }
                }
                HStack {
                    Text("Pause icon:")
                    Spacer()
                    Button("Choose…") { pickDockIcon("Pause") }
                }
            }
            Section(header: Text("Spectrum").bold()) {
                Toggle("Use SceneKit renderer", isOn: $prefs.spectrumSceneKit)
                HStack {
                    Text("Bar color:")
                    Spacer()
                    ColorWellView(
                        key: "spectrumBarColor",
                        defaultColor: NSColor(srgbRed: 1.0, green: 0.5, blue: 0, alpha: 1.0)
                    )
                    .frame(width: 44, height: 22)
                }
                HStack {
                    Text("Dot color:")
                    Spacer()
                    ColorWellView(
                        key: "spectrumDotColor",
                        defaultColor: .systemRed
                    )
                    .frame(width: 44, height: 22)
                }
            }
        }
    }

    private func pickDockIcon(_ baseName: String) {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = false
        panel.canChooseFiles = true
        panel.isFloatingPanel = true
        panel.allowedFileTypes = ["jpg", "jpeg", "png", "gif", "webp", "avif", "heic"]
        guard panel.runModal() == .OK, let url = panel.url else { return }

        guard let iconData = try? Data(contentsOf: url),
              let image = NSImage(data: iconData) else { return }

        var rect = NSRect(origin: .zero, size: image.size)
        guard let cgRef = image.cgImage(forProposedRect: &rect, context: nil, hints: nil) else { return }

        let paths = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)
        let iconsDir = paths[0].appendingPathComponent("Cog").appendingPathComponent("Icons")
        try? FileManager.default.createDirectory(at: iconsDir, withIntermediateDirectories: true)

        let fileURL = iconsDir.appendingPathComponent(baseName).appendingPathExtension("png")
        let rep = NSBitmapImageRep(cgImage: cgRef)
        if let pngData = rep.representation(using: .png, properties: [:]) {
            try? pngData.write(to: fileURL)
        }

        if prefs.customDockIcons {
            NotificationCenter.default.post(
                name: Notification.Name("CogCustomDockIconsReloadNotification"),
                object: nil
            )
        }
    }
}
