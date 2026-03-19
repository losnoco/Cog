import SwiftUI

private final class RubberbandPrefs: ObservableObject {
    private var isActive = true

    @Published var engine: String {
        didSet {
            guard isActive else { return }
            UserDefaults.standard.set(engine, forKey: "rubberbandEngine")
            // When switching to finer, "long" window is unavailable
            if engine == "finer" && window == "long" { window = "standard" }
        }
    }
    @Published var transients: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(transients, forKey: "rubberbandTransients") }
    }
    @Published var detector: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(detector, forKey: "rubberbandDetector") }
    }
    @Published var phase: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(phase, forKey: "rubberbandPhase") }
    }
    @Published var window: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(window, forKey: "rubberbandWindow") }
    }
    @Published var smoothing: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(smoothing, forKey: "rubberbandSmoothing") }
    }
    @Published var formant: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(formant, forKey: "rubberbandFormant") }
    }
    @Published var pitch: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(pitch, forKey: "rubberbandPitch") }
    }
    @Published var channels: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(channels, forKey: "rubberbandChannels") }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        engine     = d.string(forKey: "rubberbandEngine")     ?? "disabled"
        transients = d.string(forKey: "rubberbandTransients") ?? "crisp"
        detector   = d.string(forKey: "rubberbandDetector")   ?? "compound"
        phase      = d.string(forKey: "rubberbandPhase")      ?? "laminar"
        window     = d.string(forKey: "rubberbandWindow")     ?? "standard"
        smoothing  = d.string(forKey: "rubberbandSmoothing")  ?? "off"
        formant    = d.string(forKey: "rubberbandFormant")    ?? "shifted"
        pitch      = d.string(forKey: "rubberbandPitch")      ?? "highspeed"
        channels   = d.string(forKey: "rubberbandChannels")   ?? "apart"
    }
}

struct RubberbandPaneView: View {
    @StateObjectCompat private var prefs = RubberbandPrefs()

    private var isEnabled: Bool { prefs.engine != "disabled" && prefs.engine != "signalsmith" }
    private var isR3: Bool { prefs.engine == "finer" }

    var body: some View {
        if #available(macOS 13.0, *) {
            formContent.formStyle(.grouped)
        } else {
            formContent.padding()
        }
    }

    private var formContent: some View {
        Form {
            Picker("Engine:", selection: $prefs.engine) {
                Text("Disabled").tag("disabled")
                Divider()
                Text("Signalsmith Stretch").tag("signalsmith")
                Section {
                    Text("Faster").tag("faster")
                    Text("Finer").tag("finer")
                } header: {
                    Text("Rubber Band Engine", comment: "Engine name").bold()
                }
            }

            if (isEnabled) {
                Section {
					if(!isR3) {
						Picker("Transients:", selection: $prefs.transients) {
							Text("Crisp").tag("crisp")
							Text("Mixed").tag("mixed")
							Text("Smooth").tag("smooth")
						}
						Picker("Detector:", selection: $prefs.detector) {
							Text("Compound").tag("compound")
							Text("Percussive").tag("percussive")
							Text("Soft").tag("soft")
						}
						Picker("Phase:", selection: $prefs.phase) {
							Text("Laminar").tag("laminar")
							Text("Independent").tag("independent")
						}
					}
                    Picker("Window:", selection: $prefs.window) {
                        Text("Standard").tag("standard")
                        Text("Short").tag("short")
                        if !isR3 {
                            Text("Long").tag("long")
                        }
                    }
					if(!isR3) {
						Picker("Smoothing:", selection: $prefs.smoothing) {
							Text("Off").tag("off")
							Text("On").tag("on")
						}
					}
                    Picker("Formant:", selection: $prefs.formant) {
                        Text("Shifted").tag("shifted")
                        Text("Preserved").tag("preserved")
                    }
                    Picker("Pitch:", selection: $prefs.pitch) {
                        Text("High Speed").tag("highspeed")
                        Text("High Quality").tag("highquality")
                        Text("High Consistency").tag("highconsistency")
                    }
                    Picker("Channels:", selection: $prefs.channels) {
                        Text("Apart").tag("apart")
                        Text("Together").tag("together")
                    }
                } header: {
                    Text("Rubber Band Engine").bold()
                }
            }
        }
    }
}
