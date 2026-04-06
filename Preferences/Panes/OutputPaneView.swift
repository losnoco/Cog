import SwiftUI

private final class OutputPrefs: ObservableObject {
    private var isActive = true

    @Published var volumeScaling: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(volumeScaling, forKey: "volumeScaling") }
    }
    @Published var resampling: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(resampling, forKey: "resampling") }
    }
    @Published var enableHrtf: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableHrtf, forKey: "enableHrtf") }
    }
    @Published var enableFSurround: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableFSurround, forKey: "enableFSurround") }
    }
    @Published var enableHeadTracking: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableHeadTracking, forKey: "enableHeadTracking") }
    }
    @Published var volumeLimit: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(volumeLimit, forKey: "volumeLimit") }
    }
    @Published var suspendOutputOnPause: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(suspendOutputOnPause, forKey: "suspendOutputOnPause") }
    }
    @Published var enableHdcd: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableHdcd, forKey: "enableHDCD") }
    }
    @Published var halveDSDVolume: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(halveDSDVolume, forKey: "halveDSDVolume") }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        volumeScaling = d.string(forKey: "volumeScaling") ?? "albumGainWithPeak"
        resampling = d.string(forKey: "resampling") ?? "cubic"
        enableHrtf = d.bool(forKey: "enableHrtf")
        enableHeadTracking = d.bool(forKey: "enableHeadTracking")
        enableFSurround = d.bool(forKey: "enableFSurround")
        volumeLimit = d.object(forKey: "volumeLimit") as? Bool ?? true
        suspendOutputOnPause = d.object(forKey: "suspendOutputOnPause") as? Bool ?? true
        enableHdcd = d.object(forKey: "enableHDCD") as? Bool ?? true
        halveDSDVolume = d.object(forKey: "halveDSDVolume") as? Bool ?? false
    }
}

@MainActor private let volumeOptions: [(LocalizedStringKey, String, Int)] = [
    ("ReplayGain Album Gain with peak", "albumGainWithPeak", 0),
    ("ReplayGain Album Gain", "albumGain", 0),
    ("ReplayGain Track Gain with peak", "trackGainWithPeak", 0),
    ("ReplayGain Track Gain", "trackGain", 0),
    ("SoundCheck", "soundcheck", 1),
    ("Volume scale tag only", "volumeScale", 1),
    ("No volume scaling", "none", 2),
]

@MainActor private let resamplingOptions: [(LocalizedStringKey, String)] = [
    ("Zero Order Hold", "zoh"),
    ("Blep Synthesis", "blep"),
    ("Linear Interpolation", "linear"),
    ("Blam Synthesis", "blam"),
    ("Cubic Interpolation", "cubic"),
    ("Sinc Interpolation", "sinc"),
]

struct OutputPaneView: View {
    @StateObjectCompat private var prefs = OutputPrefs()
    @StateObjectCompat private var deviceModel = AudioDeviceModel()

    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var volumeScalingIsReplayGain: Bool {
        volumeOptions.first { $0.1 == prefs.volumeScaling }?.2 == 0
    }

    private var formContent: some View {
        Form {
            Picker("Output device:", selection: $deviceModel.selectedDeviceID) {
                ForEach(deviceModel.devices) { device in
                    Text(device.name).tag(device.id)
                }
            }
            Picker(volumeScalingIsReplayGain ? "Volume scaling (ReplayGain):" : "Volume scaling:",
                   selection: $prefs.volumeScaling) {
                Section {
                    ForEach(volumeOptions.filter { $0.2 == 0 }, id: \.1) { opt in
                        Text(opt.0).tag(opt.1)
                    }
                } header: {
                    Text("ReplayGain")
                }
                Section {
                    ForEach(volumeOptions.filter { $0.2 == 1 }, id: \.1) { opt in
                        Text(opt.0).tag(opt.1)
                    }
                }
                Section {
                    ForEach(volumeOptions.filter { $0.2 == 2 }, id: \.1) { opt in
                        Text(opt.0).tag(opt.1)
                    }
                }
            }
            Picker("Resampling:", selection: $prefs.resampling) {
                ForEach(resamplingOptions, id: \.1) { opt in
                    Text(opt.0).tag(opt.1)
                }
            }
            Toggle("Limit volume to prevent clipping", isOn: $prefs.volumeLimit)
            Toggle("Suspend output when paused", isOn: $prefs.suspendOutputOnPause)
            Section {
                Toggle(
                    "Enable HDCD Peak and Low Level Range Extend",
                    isOn: $prefs.enableHdcd
                )
                Toggle(
                    "Halve volume for DSD",
                    isOn: $prefs.halveDSDVolume
                )
            } header: {
                Text("Advanced audio formats").bold()
            }
            Section {
                Toggle("Enable HRTF / Binaural", isOn: $prefs.enableHrtf)
                Toggle(
                    "Enable FreeSurround decoder",
                    isOn: $prefs.enableFSurround
                )
                if #available(macOS 14.0, *) {
                    Toggle("Enable head tracking", isOn: $prefs.enableHeadTracking)
                        .disabled(!prefs.enableHrtf)
                    Button("Recenter head tracking") {
                        NotificationCenter.default.post(
                            name: Notification.Name("CogPlaybackDidResetHeadTracking"),
                            object: nil
                        )
                    }
                    .disabled(!prefs.enableHrtf || !prefs.enableHeadTracking)
                }
            } header: {
                Text("Binaural audio").bold()
            }
        }
    }
}
