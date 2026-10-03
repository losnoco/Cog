import CogAudio
import CoreAudio
import SwiftUI

private final class OutputPrefs: ObservableObject {
    private var isActive = true

    @Published var volumeScaling: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(volumeScaling, forKey: "volumeScaling") }
    }
    @Published var enableSpatialAudio: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableSpatialAudio, forKey: "enableSpatialAudio") }
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
    @Published var enableFading: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableFading, forKey: "enableFading") }
    }
    @Published var enableHdcd: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableHdcd, forKey: "enableHDCD") }
    }
    @Published var halveDSDVolume: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(halveDSDVolume, forKey: "halveDSDVolume") }
    }
    @Published var enableDoP: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(enableDoP, forKey: "enableDoP") }
    }
    // The fork's key names, so the settings carry over.
    @Published var exclusiveOutput: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(exclusiveOutput, forKey: "exclusiveIntegerOutput") }
    }
    @Published var fullDeviceVolume: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(fullDeviceVolume, forKey: "setDeviceVolumeTo100ForExclusiveOutput") }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        volumeScaling = d.string(forKey: "volumeScaling") ?? "albumGainWithPeak"
        enableSpatialAudio = d.bool(forKey: "enableSpatialAudio")
        enableHeadTracking = d.bool(forKey: "enableHeadTracking")
        enableFSurround = d.bool(forKey: "enableFSurround")
        volumeLimit = d.object(forKey: "volumeLimit") as? Bool ?? true
        suspendOutputOnPause = d.object(forKey: "suspendOutputOnPause") as? Bool ?? true
        enableFading = d.object(forKey: "enableFading") as? Bool ?? true
        enableHdcd = d.object(forKey: "enableHDCD") as? Bool ?? true
        halveDSDVolume = d.object(forKey: "halveDSDVolume") as? Bool ?? false
        enableDoP = d.bool(forKey: "enableDoP")
        exclusiveOutput = d.bool(forKey: "exclusiveIntegerOutput")
        fullDeviceVolume = d.bool(forKey: "setDeviceVolumeTo100ForExclusiveOutput")
    }
}

/// The spatial audio choice for the output device selected in the pane (the
/// system default's current device while following it), kept per device by
/// `DeviceOutput`.
private final class SpatialDeviceChoice: ObservableObject {
    @Published private(set) var deviceName = ""
    @Published private(set) var automatic: DeviceOutput.SpatialOutput = .off
    @Published private(set) var isStereo = false
    /// A `SpatialOutput` raw value, or "automatic".
    @Published var choice = "automatic" {
        didSet {
            guard !loading, let deviceID else { return }
            DeviceOutput.choose(DeviceOutput.SpatialOutput(rawValue: choice), for: deviceID)
        }
    }

    private var deviceID: AudioDeviceID?
    private var loading = false

    func load(selected: Int) {
        loading = true
        defer { loading = false }
        deviceID = selected < 0 ? DeviceOutput.systemDefaultOutput() : AudioDeviceID(selected)
        guard let deviceID else {
            deviceName = ""
            isStereo = false
            choice = "automatic"
            return
        }
        deviceName = DeviceOutput.outputDevices().first { $0.id == deviceID }?.name ?? ""
        isStereo = DeviceOutput.isStereo(deviceID)
        automatic = DeviceOutput.automaticSpatialOutput(of: deviceID)
        choice = DeviceOutput.chosenSpatialOutput(of: deviceID)?.rawValue ?? "automatic"
    }

    static func label(_ output: DeviceOutput.SpatialOutput) -> LocalizedStringKey {
        switch output {
        case .headphones: return "Headphones"
        case .speakers: return "Speakers"
        case .off: return "Off"
        }
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

struct OutputPaneView: View {
    @StateObject private var prefs = OutputPrefs()
    @StateObject private var deviceModel = AudioDeviceModel()
    @StateObject private var spatialDevice = SpatialDeviceChoice()

    var body: some View {
        Group {
            if #available(macOS 13.0, *) {
                formContent.formStyle(.grouped)
            } else {
                formContent.padding()
            }
        }
        .onAppear { spatialDevice.load(selected: deviceModel.selectedDeviceID) }
        .onChange(of: deviceModel.selectedDeviceID) { spatialDevice.load(selected: $0) }
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
            Toggle("Limit volume to prevent clipping", isOn: $prefs.volumeLimit)
            Toggle("Suspend output when paused", isOn: $prefs.suspendOutputOnPause)
            Toggle("Fade playback transitions", isOn: $prefs.enableFading)
            Section {
                Toggle("Use exclusive mode when supported", isOn: $prefs.exclusiveOutput)
                Toggle(
                    "Set device volume to 100% for exclusive output",
                    isOn: $prefs.fullDeviceVolume
                )
                .disabled(!prefs.exclusiveOutput)
                .help("Turns the device's own volume control, if it has one, all the way up while Cog holds the device, and back down afterwards, so that it does not change the samples either. Check the level first: Cog's volume and your amplifier's are then all that is left.")
                Text("Needs a specific output device, not the system default. While playing, Cog takes the device for itself, so other apps cannot play through it; runs it at each track's sample rate; and sends it integer samples when it takes them, unchanged when nothing in Cog alters the sound. A device that cannot be taken plays shared, as before.")
                    .font(.caption)
                    .foregroundColor(.secondary)
            } header: {
                Text("Output ownership").bold()
            }
            Section {
                Toggle(
                    "Enable HDCD Peak and Low Level Range Extend",
                    isOn: $prefs.enableHdcd
                )
                Toggle(
                    "Halve volume for DSD",
                    isOn: $prefs.halveDSDVolume
                )
                Toggle(
                    "Send DSD to the DAC as DoP (DSD over PCM)",
                    isOn: $prefs.enableDoP
                )
                .help("Only for DACs that decode DoP; others play it as noise, and there is no way to tell which kind is connected. Needs a specific output device, not the system default: Cog takes it for itself while playing DSD. Otherwise, DSD is converted to PCM.")
            } header: {
                Text("Advanced audio formats").bold()
            }
            Section {
                Toggle("Spatialize surround on stereo devices", isOn: $prefs.enableSpatialAudio)
                    .help("Surround plays through Apple's spatial audio on headphones, with your personalized spatial audio profile, or on stereo speakers. Stereo plays as it is, unless FreeSurround upmixes it. Devices with more channels take surround as it is.")
                Picker(selection: $spatialDevice.choice) {
                    Text("Automatic (\(Text(SpatialDeviceChoice.label(spatialDevice.automatic))))").tag("automatic")
                    Text("Headphones").tag(DeviceOutput.SpatialOutput.headphones.rawValue)
                    Text("Speakers").tag(DeviceOutput.SpatialOutput.speakers.rawValue)
                    Text("Off").tag(DeviceOutput.SpatialOutput.off.rawValue)
                } label: {
                    Text("\(spatialDevice.deviceName) is:")
                }
                .disabled(!prefs.enableSpatialAudio || !spatialDevice.isStereo)
                .help(spatialDevice.isStereo
                      ? "What is connected to this device, which Cog cannot always tell: Bluetooth and USB devices are taken for headphones, and the Mac's own speakers for speakers. Off downmixes surround instead."
                      : "Only stereo devices are spatialized; this one takes surround as it is.")
                if #available(macOS 12.3, *) {
                    Toggle("Head tracking", isOn: $prefs.enableHeadTracking)
                        .disabled(!prefs.enableSpatialAudio)
                        .help("Keeps the sound in place as you turn your head, on AirPods that support it.")
                }
                Toggle(
                    "Enable FreeSurround decoder",
                    isOn: $prefs.enableFSurround
                )
                .help("Upmixes stereo to 5.1, for surround speakers or for spatial audio on headphones.")
            } header: {
                Text("Spatial audio").bold()
            }
        }
    }
}
