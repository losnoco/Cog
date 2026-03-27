import SwiftUI
import AudioToolbox
import AVFoundation

// MARK: - Model

private struct MIDIPlugin: Identifiable {
    let id: String   // preference key
    let name: String
    let configurable: Bool
    let builtIn: Bool
}

private final class MIDIPrefs: ObservableObject {
    private var isActive = true

    @Published var plugin: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(plugin, forKey: "midiPlugin"); updateSetupButtonEnabled() }
    }
    @Published var flavor: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(flavor, forKey: "midi.flavor") }
    }
    @Published var soundFontPath: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(soundFontPath.isEmpty ? nil : soundFontPath, forKey: "soundFontPath") }
    }
    @Published var synthDefaultSeconds: Double {
        didSet { guard isActive else { return }; UserDefaults.standard.set(synthDefaultSeconds, forKey: "synthDefaultSeconds") }
    }
    @Published var synthDefaultFadeSeconds: Double {
        didSet { guard isActive else { return }; UserDefaults.standard.set(synthDefaultFadeSeconds, forKey: "synthDefaultFadeSeconds") }
    }
    @Published var synthDefaultLoopCount: Int {
        didSet { guard isActive else { return }; UserDefaults.standard.set(synthDefaultLoopCount, forKey: "synthDefaultLoopCount") }
    }
    @Published var synthSampleRate: Int {
        didSet { guard isActive else { return }; UserDefaults.standard.set(synthSampleRate, forKey: "synthSampleRate") }
    }

    deinit { isActive = false }
    @Published var plugins: [MIDIPlugin] = []
    @Published var setupButtonEnabled: Bool = false

    init() {
        let d = UserDefaults.standard
        plugin = d.string(forKey: "midiPlugin") ?? "BASSMIDI"
        flavor = d.string(forKey: "midi.flavor") ?? "default"
        soundFontPath = d.string(forKey: "soundFontPath") ?? ""
        synthDefaultSeconds = d.object(forKey: "synthDefaultSeconds") as? Double ?? 150.0
        synthDefaultFadeSeconds = d.object(forKey: "synthDefaultFadeSeconds") as? Double ?? 8.0
        synthDefaultLoopCount = d.object(forKey: "synthDefaultLoopCount") as? Int ?? 2
        synthSampleRate = d.object(forKey: "synthSampleRate") as? Int ?? 44100
        plugins = Self.buildPluginList()
        updateSetupButtonEnabled()
    }

    private func updateSetupButtonEnabled() {
        setupButtonEnabled = plugins.first(where: { $0.id == plugin })?.configurable ?? false
    }

    private static func buildPluginList() -> [MIDIPlugin] {
        var list: [MIDIPlugin] = [
            MIDIPlugin(id: "BASSMIDI",  name: "BASSMIDI",       configurable: false, builtIn: true),
            MIDIPlugin(id: "NukeSc55",  name: "Nuked SC-55",    configurable: true,  builtIn: true),
            MIDIPlugin(id: "DOOM0000",  name: "DMX Generic",    configurable: false, builtIn: true),
            MIDIPlugin(id: "DOOM0001",  name: "DMX Doom 1",     configurable: false, builtIn: true),
            MIDIPlugin(id: "DOOM0002",  name: "DMX Doom 2",     configurable: false, builtIn: true),
            MIDIPlugin(id: "DOOM0003",  name: "DMX Raptor",     configurable: false, builtIn: true),
            MIDIPlugin(id: "DOOM0004",  name: "DMX Strife",     configurable: false, builtIn: true),
            MIDIPlugin(id: "DOOM0005",  name: "DMXOPL",         configurable: false, builtIn: true),
            MIDIPlugin(id: "OPL3W000",  name: "OPL3Windows",    configurable: false, builtIn: true),
        ]
        // Add installed AudioUnit MusicDevice plugins
        var cd = AudioComponentDescription(
            componentType: kAudioUnitType_MusicDevice,
            componentSubType: 0, componentManufacturer: 0,
            componentFlags: 0, componentFlagsMask: 0
        )
        var comp: AudioComponent? = AudioComponentFindNext(nil, &cd)
        while let c = comp {
            var tcd = AudioComponentDescription()
            AudioComponentGetDescription(c, &tcd)
            if tcd.componentManufacturer != kAudioUnitManufacturer_Apple {
                var cfName: Unmanaged<CFString>? = nil
                AudioComponentCopyName(c, &cfName)
                if let name = cfName?.takeRetainedValue() as String? {
                    let pref = fourCharCode(tcd.componentSubType) + fourCharCode(tcd.componentManufacturer)
                    list.append(MIDIPlugin(id: pref, name: name, configurable: true, builtIn: false))
                }
            }
            comp = AudioComponentFindNext(c, &cd)
        }
        return list
    }
}

private func fourCharCode(_ value: OSType) -> String {
    var v = value.bigEndian
    return withUnsafeBytes(of: &v) { bytes in
        String(bytes: bytes, encoding: .isoLatin1) ?? "????"
    }
}

@MainActor private let flavorOptions: [(LocalizedStringKey, String)] = [
    ("Default (auto)", "default"),
    ("General MIDI", "gm"),
    ("General MIDI 2", "gm2"),
    ("Roland SC-55", "sc55"),
    ("Roland SC-88", "sc88"),
    ("Roland SC-88 Pro", "sc88pro"),
    ("Roland SC-8850", "sc8850"),
    ("Yamaha XG", "xg"),
]

private let sampleRateOptions: [Int] = [22050, 44100, 48000, 88200, 96000, 176400, 192000]

// MARK: - View

struct MIDIPaneView: View {
    @StateObjectCompat private var prefs = MIDIPrefs()

    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var formContent: some View {
        Form {

            Section {
                Picker("Synthesis plugin:", selection: $prefs.plugin) {
                    Section {
                        ForEach(prefs.plugins.filter(\.builtIn)) { p in
                            Text(p.name).tag(p.id)
                        }
                    } header: {
                        Text("Built-in to Cog")
                    }
                    if prefs.plugins.contains(where: { !$0.builtIn }) {
                        Section {
                            ForEach(prefs.plugins.filter { !$0.builtIn }) { p in
                                Text(p.name).tag(p.id)
                            }
                        } header: {
                            Text("Audio Units")
                        }
                    }
                }
                Button("Configure…") { configurePlugin() }
                    .disabled(!prefs.setupButtonEnabled)
                Picker("MIDI flavor:", selection: $prefs.flavor) {
                    ForEach(flavorOptions, id: \.1) { opt in
                        Text(opt.0).tag(opt.1)
                    }
                }
                FormRow("SoundFont:") {
                    Text(prefs.soundFontPath.isEmpty ? "" : URL(fileURLWithPath: prefs.soundFontPath).lastPathComponent)
                        .lineLimit(1)
                        .truncationMode(.middle)
                        .foregroundColor(.secondary)
                    Button("Choose…") { pickSoundFont() }
                }
            } header: {
                Text("MIDI").bold()
            }

            Picker("Sample rate:", selection: $prefs.synthSampleRate) {
                ForEach(sampleRateOptions, id: \.self) { rate in
                    Text("\(rate) Hz")
                        .frame(alignment: .trailing)
                        .tag(rate)
                }
            }
            FormRow("Default play time:") {
                HStack {
                    TextField("", value: $prefs.synthDefaultSeconds, formatter: timeIntervalFormatter())
                        .frame(width: 80)
                        .multilineTextAlignment(.trailing)
                    Text("HH:MM:SS")
                }
            }
            FormRow("Default fade time:") {
                TextField("", value: $prefs.synthDefaultFadeSeconds, formatter: timeIntervalFormatter())
                    .frame(width: 80)
                    .multilineTextAlignment(.trailing)
                Text("HH:MM:SS")
            }
            FormRow("Default loop count:") {
                TextField("", value: $prefs.synthDefaultLoopCount, formatter: NumberFormatter())
                    .frame(width: 80)
                    .multilineTextAlignment(.trailing)
            }
        }
    }

    private func pickSoundFont() {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = false
        panel.canChooseFiles = true
        panel.isFloatingPanel = true
        panel.allowedFileTypes = ["sf2", "sf2pack", "sflist", "sf3"]  // deprecated in 12, still works
        if !prefs.soundFontPath.isEmpty {
            panel.directoryURL = URL(fileURLWithPath: prefs.soundFontPath).deletingLastPathComponent()
        }
        guard panel.runModal() == .OK, let url = panel.url else { return }
        prefs.soundFontPath = url.path
    }

    private func configurePlugin() {
        // Delegate configuration to the ObjC side
		MIDIConfigHost.setupPlugin()
    }
}

private struct FormRow<Content: View>: View {
    let label: LocalizedStringKey
    let content: Content

    init(_ label: LocalizedStringKey, @ViewBuilder content: () -> Content) {
        self.label = label
        self.content = content()
    }

    var body: some View {
        if #available(macOS 13.0, *) {
            LabeledContent(label) { content }
        } else {
            VStack(alignment: .leading, spacing: 4) {
                Text(label)
                content
            }
        }
    }
}

private final class timeIntervalFormatter: DateComponentsFormatter, @unchecked Sendable {
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	override init() {
		super.init()
		self.allowedUnits = [.hour, .minute, .second]
		self.allowsFractionalUnits = false
		self.zeroFormattingBehavior = .dropLeading
		self.collapsesLargestUnit = false
		self.unitsStyle = .positional;
	}
	override func string(for obj: Any?) -> String? {
		if obj is Double {
			return super.string(from: obj as! TimeInterval)
		} else if obj is DateComponents {
			return super.string(for: obj)
		} else {
			return ""
		}
	}
	override func getObjectValue(_ obj: AutoreleasingUnsafeMutablePointer<AnyObject?>?, for string: String, errorDescription error: AutoreleasingUnsafeMutablePointer<NSString?>?) -> Bool {
		let components = string.split(separator: ":")
		var totalSeconds = 0
		var multiplier = 1

		for component in components.reversed() {
			if let value = Int(component) {
				totalSeconds += value * multiplier
			}
			multiplier *= 60
		}

		obj?.pointee = totalSeconds as AnyObject

		return true
	}
}

