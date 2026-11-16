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

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        customDockIcons = d.bool(forKey: "customDockIcons")
        colorfulDockIcons = d.bool(forKey: "colorfulDockIcons")
        customDockIconsPlaque = d.bool(forKey: "customDockIconsPlaque")
    }
}

struct AppearancePaneView: View {
    @StateObject private var prefs = AppearancePrefs()
    // As @AppStorage, these follow the seek bar's menu and the View menu too.
    @AppStorage("waveformSeekBar") private var waveformSeekBar = true
    @AppStorage("waveformLogScale") private var waveformLogScale = false
    @AppStorage("waveformRectified") private var waveformRectified = true

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
            Section(header: Text("Seek Bar").bold()) {
                Toggle("Show waveform", isOn: $waveformSeekBar)
                if waveformSeekBar {
                    Toggle("Logarithmic scale", isOn: $waveformLogScale)
                    Toggle("Rectified", isOn: $waveformRectified)
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
        panel.allowedContentTypes = ContentTypes.types(forExtensions: ["jpg", "jpeg", "png", "gif", "webp", "avif", "heic"])
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

// MARK: - Color <-> NSColor conversion helpers

extension Color {
    init(nsColor: NSColor) {
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        var alpha: CGFloat = 0
        // getRed(_:green:blue:alpha:) raises for catalog and pattern colors,
        // so convert to sRGB first.
        (nsColor.usingColorSpace(.sRGB) ?? .black).getRed(&red, green: &green, blue: &blue, alpha: &alpha)
        self.init(.sRGB, red: red, green: green, blue: blue, opacity: alpha)
    }
}

func nsColor(from color: Color) -> NSColor? {
    guard let components = color.nsColorComponents else { return nil }
    return NSColor(srgbRed: components.red, green: components.green, blue: components.blue, alpha: components.alpha)
}

private extension Color {
    var nsColorComponents: (red: CGFloat, green: CGFloat, blue: CGFloat, alpha: CGFloat)? {
        // Try to extract sRGB components
        if let cgColor = self.cgColor {
            let colorSpace = CGColorSpace(name: CGColorSpace.sRGB)
            if let converted = cgColor.converted(to: colorSpace!, intent: .defaultIntent, options: nil) {
                let components = converted.components
                if let components = components, components.count >= 4 {
                    return (components[0], components[1], components[2], components[3])
                } else if let components = components, components.count == 1 {
                    // Grayscale
                    return (components[0], components[0], components[0], components[0])
                }
            }
        }
        return nil
    }
}

// MARK: - Archiving helpers (same as ColorWellView)

func dataFromArchivedColor(_ color: NSColor) -> Data? {
    try? NSKeyedArchiver.archivedData(withRootObject: color, requiringSecureCoding: true)
}

func colorFromArchivedData(_ data: Data?) -> Color? {
    guard let data = data else { return nil }
    guard let nsColor = try? NSKeyedUnarchiver.unarchivedObject(ofClass: NSColor.self, from: data) else { return nil }
    return Color(nsColor: nsColor)
}
