import SwiftUI

private final class PlaylistPrefs: ObservableObject {
    private var isActive = true

    @Published var openingFilesBehavior: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(openingFilesBehavior, forKey: "openingFilesBehavior") }
    }
    @Published var openingFilesAlteredBehavior: String {
        didSet { guard isActive else { return }; UserDefaults.standard.set(openingFilesAlteredBehavior, forKey: "openingFilesAlteredBehavior") }
    }
    @Published var readCueSheetsInFolders: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(readCueSheetsInFolders, forKey: "readCueSheetsInFolders") }
    }
    @Published var readPlaylistsInFolders: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(readPlaylistsInFolders, forKey: "readPlaylistsInFolders") }
    }
    @Published var addOtherFilesInFolders: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(addOtherFilesInFolders, forKey: "addOtherFilesInFolders") }
    }
    @Published var resumePlaybackOnStartup: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(resumePlaybackOnStartup, forKey: "resumePlaybackOnStartup") }
    }
    @Published var resetPlaylistOnQuit: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(resetPlaylistOnQuit, forKey: "resetPlaylistOnQuit") }
    }
    @Published var quitOnNaturalStop: Bool {
        didSet { guard isActive else { return }; UserDefaults.standard.set(quitOnNaturalStop, forKey: "quitOnNaturalStop") }
    }

    deinit { isActive = false }

    init() {
        let d = UserDefaults.standard
        openingFilesBehavior = d.string(forKey: "openingFilesBehavior") ?? "enqueueAndPlay"
        openingFilesAlteredBehavior = d.string(forKey: "openingFilesAlteredBehavior") ?? "enqueue"
        readCueSheetsInFolders = d.bool(forKey: "readCueSheetsInFolders")
        readPlaylistsInFolders = d.bool(forKey: "readPlaylistsInFolders")
        addOtherFilesInFolders = d.bool(forKey: "addOtherFilesInFolders")
        resumePlaybackOnStartup = d.bool(forKey: "resumePlaybackOnStartup")
        resetPlaylistOnQuit = d.bool(forKey: "resetPlaylistOnQuit")
        quitOnNaturalStop = d.bool(forKey: "quitOnNaturalStop")
    }
}

private let behaviorOptions: [(label: LocalizedStringKey, value: String)] = [
    ("Clear Playlist and Play", "clearAndPlay"),
    ("Enqueue", "enqueue"),
    ("Enqueue and Play", "enqueueAndPlay"),
]

struct PlaylistPaneView: View {
    @StateObjectCompat private var prefs = PlaylistPrefs()

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
                Picker("Normally:", selection: $prefs.openingFilesBehavior) {
                    ForEach(behaviorOptions, id: \.value) { opt in
                        Text(opt.label).tag(opt.value)
                    }
                }
                Picker("With modifiers:", selection: $prefs.openingFilesAlteredBehavior) {
                    ForEach(behaviorOptions, id: \.value) { opt in
                        Text(opt.label).tag(opt.value)
                    }
                }

            } header: {
                Text("When opening files").bold()
            } footer: {
                Text("Modifiers are either ⇧ or ⌃⌘")
            }

            Section {
                Toggle("Read cue sheets", isOn: $prefs.readCueSheetsInFolders)
                Toggle("Read playlists", isOn: $prefs.readPlaylistsInFolders)
                Toggle("Add other files in same folder", isOn: $prefs.addOtherFilesInFolders)
            } header: {
                Text("When adding folders").bold()
            }

            Toggle("Resume playback on startup", isOn: $prefs.resumePlaybackOnStartup)
            Toggle("Reset playlist on quit", isOn: $prefs.resetPlaylistOnQuit)
            Toggle("Quit when playback completes", isOn: $prefs.quitOnNaturalStop)
        }
    }
}
