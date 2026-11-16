//
//  VisualizationsPaneView.swift
//  Cog
//
//  The visualizers' settings: the spectrum, the waveform and projectM,
//  moved out of Appearance on 2026-11-15.
//

import SwiftUI

private final class VisualizationPrefs: ObservableObject {
    private var isActive = true

    @Published var spectrumSceneKit: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(spectrumSceneKit, forKey: "spectrumSceneKit") }
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
    @Published var waveformVisualizerColor: Color {
        didSet { guard isActive else { return }; if let data = dataFromArchivedColor(nsColor(from: waveformVisualizerColor) ?? NSColor(srgbRed: 1.0, green: 0.5, blue: 0, alpha: 1.0)) { UserDefaults.standard.set(data, forKey: "waveformVisualizerColor") } }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        spectrumSceneKit = d.object(forKey: "spectrumSceneKit") as? Bool ?? true
        spectrumProjectionMode = d.object(forKey: "spectrumProjectionMode") as? Bool ?? false
        spectrumBarColor = Self.defaultColor(forKey: "spectrumBarColor", default: Color(nsColor: NSColor(srgbRed: 1.0, green: 0.5, blue: 0, alpha: 1.0)))
        spectrumDotColor = Self.defaultColor(forKey: "spectrumDotColor", default: Color(nsColor: NSColor(srgbRed: 1.0, green: 0, blue: 0, alpha: 1.0)))
        waveformVisualizerColor = Self.defaultColor(forKey: "waveformVisualizerColor", default: Color(nsColor: NSColor(srgbRed: 1.0, green: 0.5, blue: 0, alpha: 1.0)))
    }

    private static func defaultColor(forKey key: String, default defaultColor: Color) -> Color {
        colorFromArchivedData(UserDefaults.standard.data(forKey: key)) ?? defaultColor
    }
}

struct VisualizationsPaneView: View {
    @StateObject private var prefs = VisualizationPrefs()
    @ObservedObject private var presetPack = ProjectMPresetPack.shared
    // As @AppStorage, this follows the waveform pane's click and menu too.
    @AppStorage("waveformVisualizerMode") private var waveformVisualizerMode = WaveformView.Mode.oscilloscope

    // As @AppStorage, these reach the open projectM pane at once.
    @AppStorage("projectMShuffle") private var projectMShuffle = true
    @AppStorage("projectMPresetDuration") private var projectMPresetDuration = 30.0
    @AppStorage("projectMPresetFolder") private var projectMPresetFolder = ""
    @AppStorage("projectMSeizureWarningSuppressed") private var projectMSeizureWarningSuppressed = false
    @AppStorage("projectMCrossfade") private var projectMCrossfade = 3.0
    @AppStorage("projectMHardCuts") private var projectMHardCuts = false
    @AppStorage("projectMHardCutThreshold") private var projectMHardCutThreshold = 2.0
    @AppStorage("projectMDetail") private var projectMDetail = ProjectMDetail.standard.rawValue
    @AppStorage("projectMResolution") private var projectMResolution = 1
    @AppStorage("projectMFrameRate") private var projectMFrameRate = 30

    /// The theme folders of the presets in use, and those left out.
    @State private var themes: [String] = []
    @State private var excludedThemes: Set<String> = Set(UserDefaults.standard.projectMExcludedThemes)

    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var formContent: some View {
        Form {
            Section(header: Text("Spectrum").bold()) {
                Toggle("Use 3D rendered spectrum", isOn: $prefs.spectrumSceneKit)
                if prefs.spectrumSceneKit {
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
            Section(header: Text("Waveform").bold()) {
                Picker("Style:", selection: $waveformVisualizerMode) {
                    Text("Oscilloscope").tag(WaveformView.Mode.oscilloscope)
                    Text("Envelope").tag(WaveformView.Mode.envelope)
                }
                HStack {
                    Text("Color:")
                    Spacer()
                    ColorPicker("", selection: $prefs.waveformVisualizerColor)
                        .labelsHidden()
                }
            }
            Section(header: Text("projectM").bold()) {
                Toggle("Warn about flashing before opening", isOn: Binding(get: { !projectMSeizureWarningSuppressed },
                                                                        set: { projectMSeizureWarningSuppressed = !$0 }))
                HStack {
                    Text("Presets:")
                    Spacer()
                    Text(projectMPresetFolder.isEmpty
                        ? String(localized: "Included", comment: "projectM setting: the presets that come with Cog")
                        : URL(fileURLWithPath: projectMPresetFolder).lastPathComponent)
                        .foregroundColor(.secondary)
                        .lineLimit(1)
                        .truncationMode(.middle)
                    Button("Choose…") { pickProjectMFolder() }
                    if !projectMPresetFolder.isEmpty {
                        Button("Reset") { projectMPresetFolder = "" }
                    }
                }
                HStack {
                    Text("Cream of the Crop:")
                    Spacer()
                    presetPackStatus
                }
                Picker("Detail:", selection: $projectMDetail) {
                    Text("Low", comment: "projectM detail: coarse, for older Macs").tag(ProjectMDetail.low.rawValue)
                    Text("Standard", comment: "projectM detail").tag(ProjectMDetail.standard.rawValue)
                    Text("High", comment: "projectM detail: fine, costs more GPU").tag(ProjectMDetail.high.rawValue)
                }
                Picker("Resolution:", selection: $projectMResolution) {
                    Text("Full", comment: "projectM resolution").tag(1)
                    Text("Half", comment: "projectM resolution: half the width and height").tag(2)
                    Text("Quarter", comment: "projectM resolution: a quarter of the width and height").tag(4)
                }
                Picker("Frame rate:", selection: $projectMFrameRate) {
                    Text("Display’s", comment: "projectM frame rate: as fast as the display refreshes").tag(0)
                    ForEach([60, 50, 48, 30, 25, 24], id: \.self) { rate in
                        Text(String(format: NSLocalizedString("%d fps", comment: "projectM frame rate; the argument is frames per second"), rate)).tag(rate)
                    }
                }
            }
            Section(header: Text("projectM Transitions").bold()) {
                Toggle("Shuffle presets", isOn: $projectMShuffle)
                Stepper(value: $projectMPresetDuration, in: 5...600, step: 5) {
                    Text("Change preset every \(Int(projectMPresetDuration)) seconds",
                         comment: "projectM setting; the argument is a number of seconds")
                }
                HStack {
                    Text("Crossfade:")
                    Slider(value: $projectMCrossfade, in: 0...10, step: 0.5)
                    Text(String(format: NSLocalizedString("%.1f s", comment: "A duration in seconds, with one decimal"), projectMCrossfade))
                        .monospacedDigit()
                        .frame(width: 44, alignment: .trailing)
                }
                Toggle("Cut to a new preset on strong beats", isOn: $projectMHardCuts)
                if projectMHardCuts {
                    // projectM's value is how much louder a beat must be;
                    // the slider runs the other way, toward more cuts.
                    HStack {
                        Text("Cuts:")
                        Text("Fewer", comment: "projectM beat cuts slider: fewer cuts")
                            .foregroundColor(.secondary)
                        Slider(value: Binding(get: { 5 - projectMHardCutThreshold },
                                              set: { projectMHardCutThreshold = 5 - $0 }), in: 1...4)
                        Text("More", comment: "projectM beat cuts slider: more cuts")
                            .foregroundColor(.secondary)
                    }
                }
            }
            if !themes.isEmpty {
                Section(header: Text("projectM Themes").bold(),
                        footer: Text("The preset folders to show. Unchecking one leaves its presets out.",
                                     comment: "projectM themes setting footer")) {
                    ForEach(themes, id: \.self) { theme in
                        Toggle(theme, isOn: Binding(get: { !excludedThemes.contains(theme) },
                                                    set: { setTheme(theme, included: $0) }))
                    }
                }
            }
        }
        .onAppear(perform: loadThemes)
        .onChange(of: projectMPresetFolder) { _ in loadThemes() }
    }

    /// The full preset pack: download it, or use it once it is here.
    @ViewBuilder private var presetPackStatus: some View {
        switch presetPack.state {
        case .downloading, .unpacking:
            ProgressView()
                .controlSize(.small)
            Text(presetPack.state == .downloading
                ? String(localized: "Downloading…", comment: "projectM preset pack status")
                : String(localized: "Unpacking…", comment: "projectM preset pack status"))
                .foregroundColor(.secondary)
        case .failed(let message):
            Text(message)
                .foregroundColor(.red)
                .lineLimit(2)
            Button("Try Again") { presetPack.install() }
        case .idle:
            if !presetPack.isInstalled {
                Text("9,795 presets, 1.5 MB", comment: "projectM preset pack size")
                    .foregroundColor(.secondary)
                Button("Download") { presetPack.install() }
            } else if projectMPresetFolder == ProjectMPresetPack.folder.path {
                Text("In use", comment: "projectM preset pack status: it is the presets folder")
                    .foregroundColor(.secondary)
            } else {
                Button("Use") { projectMPresetFolder = ProjectMPresetPack.folder.path }
            }
        }
    }

    /// The top-level folders of the presets in use: the included ones, or
    /// the chosen folder's.
    private func loadThemes() {
        var root = Bundle.main.url(forResource: "projectM", withExtension: nil)?.appendingPathComponent("presets", isDirectory: true)
        var access: UnsafeRawPointer?
        if !projectMPresetFolder.isEmpty {
            let folder = URL(fileURLWithPath: projectMPresetFolder, isDirectory: true)
            access = SandboxBroker.shared().beginFolderAccess(folder)
            root = folder
        }
        defer {
            if let access {
                SandboxBroker.shared().endFolderAccess(access)
            }
        }
        guard let root,
              let items = try? FileManager.default.contentsOfDirectory(at: root, includingPropertiesForKeys: [.isDirectoryKey],
                                                                      options: [.skipsHiddenFiles]) else {
            themes = []
            return
        }
        themes = items.filter { (try? $0.resourceValues(forKeys: [.isDirectoryKey]).isDirectory) == true }
            .map(\.lastPathComponent)
            .sorted { $0.localizedStandardCompare($1) == .orderedAscending }
    }

    private func setTheme(_ theme: String, included: Bool) {
        if included {
            excludedThemes.remove(theme)
        } else {
            excludedThemes.insert(theme)
        }
        UserDefaults.standard.set(excludedThemes.sorted(), forKey: "projectMExcludedThemes")
    }

    /// A folder of `.milk` presets, such as the full Cream of the Crop pack,
    /// used in place of the included ones. Its textures are searched first.
    private func pickProjectMFolder() {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = true
        panel.canChooseFiles = false
        panel.isFloatingPanel = true
        panel.message = String(localized: "Choose a folder of projectM presets.", comment: "projectM presets folder panel")
        if !projectMPresetFolder.isEmpty {
            panel.directoryURL = URL(fileURLWithPath: projectMPresetFolder)
        }
        guard panel.runModal() == .OK, let url = panel.url else { return }
        // Kept, so the pane can read it after a relaunch.
        SandboxBroker.shared().addFolderIfMissing(url)
        projectMPresetFolder = url.path
    }
}
