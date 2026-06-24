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
    @Published var spectrumFreqMode: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(spectrumFreqMode, forKey: "spectrumFreqMode") }
    }
    @Published var spectrumProjectionMode: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(spectrumProjectionMode, forKey: "spectrumProjectionMode") }
    }
    @Published var spectrumBarColor: Color {
		didSet { guard isActive else { return }; if let data = dataFromArchivedColor(nsColor(from: spectrumBarColor) ?? NSColor(srgbRed: 1.0, green: 0.5, blue: 0, alpha: 1.0)) { UserDefaults.standard.set(data, forKey: "spectrumBarColor") } }
    }
    @Published var spectrumDotColor: Color {
		didSet { guard isActive else { return }; if let data = dataFromArchivedColor(nsColor(from: spectrumDotColor) ?? NSColor(srgbRed: 1.0, green: 0, blue: 0, alpha: 1.0)) { UserDefaults.standard.set(data, forKey: "spectrumDotColor") } }
    }


    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        customDockIcons = d.bool(forKey: "customDockIcons")
        colorfulDockIcons = d.bool(forKey: "colorfulDockIcons")
        customDockIconsPlaque = d.bool(forKey: "customDockIconsPlaque")
        spectrumSceneKit = d.object(forKey: "spectrumSceneKit") as? Bool ?? true
        spectrumFreqMode = d.object(forKey: "spectrumFreqMode") as? Bool ?? false
        spectrumProjectionMode = d.object(forKey: "spectrumProjectionMode") as? Bool ?? false
		spectrumBarColor = Self.defaultColor(forKey: "spectrumBarColor", default: Color(nsColor: NSColor(srgbRed: 1.0, green: 0.5, blue: 0, alpha: 1.0)))
		spectrumDotColor = Self.defaultColor(forKey: "spectrumDotColor", default: Color(nsColor: NSColor(srgbRed: 1.0, green: 0, blue: 0, alpha: 1.0)))
    }

    private static func defaultColor(forKey key: String, default defaultColor: Color) -> Color {
        let data = UserDefaults.standard.data(forKey: key)
        return colorFromArchivedData(data) ?? defaultColor
    }
}

struct AppearancePaneView: View {
    @StateObject private var prefs = AppearancePrefs()

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
                Toggle("Use 3D rendered spectrum", isOn: $prefs.spectrumSceneKit)
                if (prefs.spectrumSceneKit) {
                    Toggle("Use flat perspective in toolbar", isOn: $prefs.spectrumProjectionMode)
                }
                HStack {
                    Text("Bar color:")
                    Spacer()
                    ColorPicker("", selection: $prefs.spectrumBarColor)
                        .labelsHidden()
                }
                HStack {
                    Text("Dot color:")
                    Spacer()
                    ColorPicker("", selection: $prefs.spectrumDotColor)
                        .labelsHidden()
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

// MARK: - Color <-> NSColor conversion helpers

extension Color {
    init(nsColor: NSColor) {
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        var alpha: CGFloat = 0
        nsColor.getRed(&red, green: &green, blue: &blue, alpha: &alpha)
        self.init(.sRGB, red: red, green: green, blue: blue, opacity: alpha)
    }
}

func nsColor(from color: Color) -> NSColor? {
    guard let components = color.nsColorComponents else { return nil }
    return NSColor(srgbRed: components.red, green: components.green, blue: components.blue, alpha: components.alpha)
}

private extension Color {
    var nsColorComponents: (red: CGFloat, green: CGFloat, blue: CGFloat, alpha: CGFloat)? {
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        var alpha: CGFloat = 0

        // Try to extract sRGB components
        if #available(macOS 11.0, *) {
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
