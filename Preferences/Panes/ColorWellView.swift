import SwiftUI

/// Wraps NSColorWell in SwiftUI. ColorPicker requires macOS 11+; this works on 10.15+.
/// Stores the selected color as NSKeyedArchiver'd NSData in UserDefaults.
struct ColorWellView: NSViewRepresentable {
    let key: String
    let defaultColor: NSColor

    func makeNSView(context: Context) -> NSColorWell {
        let well = NSColorWell()
        well.color = loadColor()
        well.target = context.coordinator
        well.action = #selector(Coordinator.colorChanged(_:))
        return well
    }

    func updateNSView(_ nsView: NSColorWell, context: Context) {
        context.coordinator.key = key
    }

    func makeCoordinator() -> Coordinator {
        Coordinator(key: key, defaultColor: defaultColor)
    }

    private func loadColor() -> NSColor {
        guard let data = UserDefaults.standard.data(forKey: key) else { return defaultColor }
        return colorFromArchivedData(data) ?? defaultColor
    }

    @MainActor final class Coordinator: NSObject {
        var key: String
        let defaultColor: NSColor

        init(key: String, defaultColor: NSColor) {
            self.key = key
            self.defaultColor = defaultColor
        }

        @objc func colorChanged(_ sender: NSColorWell) {
            if let data = dataFromArchivedColor(sender.color) {
                UserDefaults.standard.set(data, forKey: key)
            }
        }
    }
}

func dataFromArchivedColor(_ color: NSColor) -> Data? {
    try? NSKeyedArchiver.archivedData(withRootObject: color, requiringSecureCoding: true)
}

func colorFromArchivedData(_ data: Data) -> NSColor? {
    return try? NSKeyedUnarchiver.unarchivedObject(ofClass: NSColor.self, from: data)
}
