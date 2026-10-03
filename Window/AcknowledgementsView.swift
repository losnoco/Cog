//
//  AcknowledgementsView.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-02.
//

import SwiftUI

/// Third-party attributions, generated into Acknowledgements.json by
/// Scripts/build-acknowledgements.py from Acknowledgements/components.json.
struct Acknowledgements: Decodable {
    let groups: [String]
    let licenses: [String: String]
    let components: [Component]

    struct Component: Decodable, Identifiable {
        let group: String
        let name: String
        let version: String?
        let url: String?
        let license: String?
        let holders: [String]?
        let note: String?
        let texts: [String]

        var id: String { name }
    }

    static func load() -> Acknowledgements? {
        guard let url = Bundle.main.url(forResource: "Acknowledgements", withExtension: "json"),
              let data = try? Data(contentsOf: url) else { return nil }
        return try? JSONDecoder().decode(Acknowledgements.self, from: data)
    }
}

struct AcknowledgementsView: View {
    private let acknowledgements = Acknowledgements.load()

    @State private var selection: String?
    @State private var query = ""

    var body: some View {
        HStack(spacing: 0) {
            VStack(spacing: 0) {
                TextField("Search", text: $query)
                    .textFieldStyle(.roundedBorder)
                    .padding(8)
                List(selection: $selection) {
                    ForEach(acknowledgements?.groups ?? [], id: \.self) { group in
                        let components = matching(in: group)
                        if !components.isEmpty {
                            Section(header: Text(title(forGroup: group))) {
                                ForEach(components) { component in
                                    Text(component.name).tag(component.id)
                                }
                            }
                        }
                    }
                }
                .listStyle(.sidebar)
            }
            .frame(width: 260)

            Divider()

            if let component = selectedComponent {
                ComponentDetail(component: component, licenses: acknowledgements?.licenses ?? [:])
                    .id(component.id)
            } else {
                Text("Select a component to see its license.")
                    .foregroundColor(.secondary)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .frame(minWidth: 560, minHeight: 360)
        .onAppear {
            if selection == nil {
                selection = acknowledgements?.components.first?.id
            }
        }
    }

    private var selectedComponent: Acknowledgements.Component? {
        acknowledgements?.components.first { $0.id == selection }
    }

    private func matching(in group: String) -> [Acknowledgements.Component] {
        let components = acknowledgements?.components.filter { $0.group == group } ?? []
        let query = query.trimmingCharacters(in: .whitespaces)
        guard !query.isEmpty else { return components }
        return components.filter { component in
            ([component.name, component.license ?? ""] + (component.holders ?? []))
                .contains { $0.localizedCaseInsensitiveContains(query) }
        }
    }

    private func title(forGroup group: String) -> LocalizedStringKey {
        switch group {
        case "audio": return "Audio Engine and DSP"
        case "codecs": return "Codecs and Formats"
        case "chiptune": return "Game Music and Trackers"
        case "midi": return "MIDI and Synthesis"
        case "app": return "Application Components"
        case "swift": return "Swift Packages"
        case "contributors": return "Other Contributions"
        default: return LocalizedStringKey(group)
        }
    }
}

private struct ComponentDetail: View {
    let component: Acknowledgements.Component
    let licenses: [String: String]

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 10) {
                Text(component.name)
                    .font(.title2.bold())
                    .textSelection(.enabled)

                if let version = component.version {
                    Text("Version \(version)")
                        .foregroundColor(.secondary)
                }

                if let holders = component.holders, !holders.isEmpty {
                    Text(holders.joined(separator: "\n"))
                        .textSelection(.enabled)
                }

                if let license = component.license {
                    HStack(alignment: .firstTextBaseline, spacing: 4) {
                        Text("License:").foregroundColor(.secondary)
                        if license == "Not stated in source" {
                            Text("Not stated in source")
                        } else {
                            Text(verbatim: license)
                        }
                    }
                }

                if let note = component.note {
                    Text(verbatim: note)
                        .font(.callout)
                        .foregroundColor(.secondary)
                        .fixedSize(horizontal: false, vertical: true)
                        .textSelection(.enabled)
                }

                if let string = component.url, let url = URL(string: string) {
                    Link(string, destination: url)
                }

                ForEach(component.texts, id: \.self) { key in
                    if let text = licenses[key] {
                        Text(verbatim: text)
                            .font(.system(.callout, design: .monospaced))
                            .textSelection(.enabled)
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .padding(10)
                            // Unlabeled: Cog's own Color(nsColor:) flattens to sRGB, which would
                            // lose the light/dark adaptation of this system color.
                            .background(Color(NSColor.textBackgroundColor))
                            .cornerRadius(6)
                    }
                }
            }
            .padding(16)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
    }
}

final class AcknowledgementsWindowController: NSWindowController {
    static let shared = AcknowledgementsWindowController()

    init() {
        super.init(window: nil)
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
    }

    override func loadWindow() {
        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 760, height: 540),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: true
        )
        window.title = NSLocalizedString("Acknowledgements", comment: "Acknowledgements window title")
        window.isReleasedWhenClosed = false
        window.contentView = NSHostingView(rootView: AcknowledgementsView())
        window.center()
        window.setFrameAutosaveName("Acknowledgements")
        self.window = window
    }

    override func showWindow(_ sender: Any?) {
        // Without a nib, NSWindowController never calls loadWindow() itself.
        if window == nil {
            loadWindow()
        }
        super.showWindow(sender)
    }
}

#Preview {
    AcknowledgementsView()
}
