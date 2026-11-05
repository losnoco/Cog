# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Cog is an audio player for macOS (AppKit, Objective-C with a growing share of Swift and SwiftUI) and for
iOS/iPadOS (SwiftUI). Both are built from this repository and share one audio engine, one set of format
plugins and one playlist model. The Mac and iOS apps are released separately. The Mac app supports macOS 12+
(Remote Control needs 13+); the iOS app supports iOS/iPadOS 18+.

## Setup (once per checkout)

```sh
git submodule update --init --recursive
git config core.hooksPath .githooks          # pre-commit rejects any committed DEVELOPMENT_TEAM
(cd ThirdParty && tar xvf libraries.tar.xz)  # macOS prebuilt libs; redo whenever the tarball changes
iOS/prepare.sh                               # iOS only: builds ThirdParty/*/ios/ (needs brew cmake, ninja)
```

- `Scripts/extract_libraries.sh` is the smarter form of the `tar` step. It skips work when
  `ThirdParty/libraries.updated` is newer than the tarball. In Debug it also lays
  `libraries-debug-overlay.tar.xz` on top, and it re-extracts when you switch between Debug and Release.
  Xcode Cloud runs it from `ci_scripts/ci_post_clone.sh`.
- Instead of running `iOS/prepare.sh`, you can download the `ios-libraries` artifact of the "Check if Cog builds
  for iOS" workflow and unpack it into place.
- `Xcode-config/DEVELOPMENT_TEAM.xcconfig` is gitignored and needs two lines: `DEVELOPMENT_TEAM = …` and
  `PROVISIONING_PROFILE_SUPPORTED = NO`. The second is needed because the `group.org.cogx.cog` App Group is
  registered to the upstream team. `Xcode-config/Shared.xcconfig` explains both.
- Never put a team ID, `CODE_SIGN_IDENTITY` or `CODE_SIGN_STYLE` into a `project.pbxproj`. The pre-commit hook
  checks for team IDs.
- Last.fm keys are optional and go in `Xcode-config/Secrets.xcconfig` (copy `Secrets.template.xcconfig`).
  `Scripts/generate-swift-secrets.sh` turns them into the gitignored `Generated/Secrets.swift` at build time.

## Build and test

The CI workflows (`.github/workflows/debug.yml` on a self-hosted Mac runner, and `ios.yml`) are the
reference invocations:

```sh
# macOS, App Store edition (universal)
xcodebuild -project Cog.xcodeproj -scheme Cog -configuration Debug \
  -arch x86_64 -arch arm64 ONLY_ACTIVE_ARCH=NO CODE_SIGNING_ALLOWED=NO build
# macOS, direct edition: adds Sparkle and the donation menu
xcodebuild -project Cog.xcodeproj -scheme "Cog Direct" -configuration Release-Direct CODE_SIGNING_ALLOWED=NO build
# iOS
xcodebuild -project iOS/MobileCog.xcodeproj -scheme Cog -configuration Debug \
  -destination generic/platform=iOS CODE_SIGNING_ALLOWED=NO build
```

- A fresh Xcode needs `xcodebuild -downloadComponent MetalToolchain` before it can build the visualizer's Metal
  shaders.
- The two editions differ only by configuration. `Shared.xcconfig` excludes `Sparkle.framework` and
  `Direct.xcstrings` and sets `COG_SPARKLE = NO`. `Direct.xcconfig` (the `Release-Direct` configuration) puts
  them back. Code that only the direct edition has must stay behind `COG_SPARKLE`.
- `Scripts/build.sh` is a plain Release build. `DiskImage/` holds the DMG and release scripts for the direct
  edition.

### Tests

| Project | Scheme | Tests | Runs on |
| --- | --- | --- | --- |
| `Audio/CogAudio.xcodeproj` | `CogAudio` | `Audio/CogAudioTests`: rings, render, feeder, pump, DSP stages, device and exclusive output, ReplayGain, cue tracks, metadata | macOS |
| `CogPlaylist/CogPlaylist.xcodeproj` | `CogPlaylist` | `CogPlaylist/Tests`: store, model, playback order, expander, entry metadata | macOS and iOS |
| `iOS/MobileCog.xcodeproj` | `CogPlugins` | `iOS/CogPluginsTests`: finds the real plugins, decodes and plays through them; also covers `PlaylistLoader` | iOS Simulator |

```sh
xcodebuild test -project Audio/CogAudio.xcodeproj -scheme CogAudio -destination 'platform=macOS'
xcodebuild test -project Audio/CogAudio.xcodeproj -scheme CogAudio -destination 'platform=macOS' \
  -only-testing:CogAudioTests/<TestClass>/<testMethod>
```

The main `Cog` app target has no unit tests.

After changing shared code (CogAudio, `Frameworks/`, `Plugins/`, CogPlaylist, `Utils/` headers that plugins
include), build both the macOS and the iOS app. Each platform breaks in ways the other doesn't show.

## Architecture

### Projects and dependencies

`Cog.xcodeproj` is the macOS app and references about 180 sub-projects as target dependencies:

- `Audio/CogAudio.xcodeproj`: the engine framework.
- `CogPlaylist/CogPlaylist.xcodeproj`: the AppKit-free playlist framework.
- One project per plugin under `Plugins/`.
- The libraries those plugins wrap, under `Frameworks/` (GME, vgmstream, OpenMPT, libsidplayfp, munt, mGBA, …).
  Several are git submodules of kode54 forks.
- vgmstream no longer uses the separate `libatrac9` and `g719` libraries, which have been dropped from both
  vgmstream and Cog. Its bundled decoders in `Frameworks/vgmstream/vgmstream/src/coding/libs/` (`atrac9_*`,
  `g719_*`) replace them.

Prebuilt third-party libraries (FFmpeg, FLAC, Opus, Vorbis, soxr, Rubber Band, TagLib, mpg123, …) live in
`ThirdParty/` as tarballs plus headers. Each library's `README.md` there names the release and patches it
uses.

### Plugins (`Plugins/`, `Audio/Plugin.h`, `Audio/PluginController.mm`)

Every format is a plugin class conforming to protocols in `Audio/Plugin.h`:

- `CogSource`: a byte stream for a URL scheme (`FileSource`, `HTTPSource`, `ArchiveSource`, `HLS`).
- `CogContainer`: expands one file into track URLs, usually with a `#fragment` per subsong (`CueSheet`, `M3u`,
  `Pls`, archives, multi-song chiptune files). The optional `urlsForContainerURL:trackInfo:` returns each
  track's metadata from the same pass, so each track isn't reopened.
- `CogDecoder`: `open:` a source, then `readAudio` returns `AudioChunk`s. It also has `seek:` and the optional
  `interrupt` (unblock a pending read promptly) and `setTrack:` (reuse an open decoder for another cue track).
- `CogMetadataReader`, `CogPropertiesReader`, `CogMetadataWriter`.

Each plugin declares file extensions, MIME types and a `priority`, a float that may exceed 1.0. Most are
0.0 to 2.0; the CueSheet plugin's 16.0 is the highest so far, so it wins any extension it claims.

`PluginController` builds the extension and MIME-type tables. When more than one plugin claims an extension,
the `Cog*Multi` wrappers in `CogPluginMulti.m` try them in priority order. So a plugin that takes an extension
without checking the file's header should declare a low priority.

On macOS, plugins are `.bundle`s copied into the app's PlugIns folder. On iOS, all plugins are compiled into a
single `CogPlugins.framework`, and `PluginController` finds their classes among those of the loaded images.

### Audio engine (`Audio/Engine/`, Swift plus a small C core)

`Audio/Engine/PLAN.md` is the design document, and it also records what each stage found (including
measurements). Read it before changing playback. The engine replaced the old per-track `Audio/Chain/`
node-thread engine, which has been deleted. Don't reintroduce its patterns: per-track pipelines, teardown at
track changes, locks on the render thread.

```
decoder (ObjC plugin) → Feeder thread → deep SPSC ring → DSP thread → shallow SPSC ring → render callback (C)
```

- **Feeder** (`Feeder/`, `Converter/StreamConverter.swift`):
  - Opens the next decoder itself when the current one ends.
  - Converts to float, handles HDCD and DSD decimation, and fits the channel layout.
  - Resamples with one soxr instance that persists for the whole playback session. Equal input and output rates
    bypass it bit-exactly.
  - Applies ReplayGain and writes seam markers.
- **DSP** (`DSP/`, through `Pump/`): in-place stages in a fixed order: time-stretch (Rubber Band or Signalsmith),
  FreeSurround, EQ, then the visualization tap. Stages report `pendingFrames`, so that track-start
  announcements line up with what is actually heard.
- **Render** (`Core/CogRender.c`, `Core/CogRing.c`) must stay real-time safe: no locks, allocation, ARC,
  Objective-C messaging or autorelease pools. On an underrun it fills with silence and counts the event. Ring
  sizes are computed in seconds, not samples, so 384 kHz output gets the same time cushion as 44.1 kHz.
- **Seams**:
  - Nothing is torn down at a track boundary.
  - Notifications are positional. A seam is a frame index, and `didBeginStream`, play counts and scrobbles are
    published once the played-frame counter passes it.
  - Seek and stop flush through an acknowledged epoch.
- **Output** (`Output/DeviceOutput*.swift`): AUHAL with device-change handling, plus exclusive (hog) mode and
  DoP on macOS. iOS has neither and resamples everything to the audio session's rate.
- **Facade** (`Facade/PlaybackEngine.swift`) implements the Objective-C `AudioPlayer` API (`Audio/AudioPlayer.h`),
  which is unchanged. The app only ever talks to `AudioPlayer`.
  - `CogAudioOutputStatusDidChangeNotification` describes the signal path as heard: source format, render
    format, device, and each modification in signal order. The Signal Inspector shows it.
  - The waveform seek bar's data comes from `Facade/Waveform*.swift`.
- `Support/EngineCapture.swift` and `EngineLog.swift` are the diagnostic hooks, used by the seam and capture
  tests.

### Playlist and persistence

- One Core Data model, `DataModel.xcdatamodeld`, has the entities `Playlist`, `PlaylistEntry`, `AlbumArtwork`,
  `PlayCount`, `LyricsCache`, `PendingListen` (the ListenBrainz queue) and `SandboxToken`. Multiple playlists
  shipped in model version 2.
  - Adding attributes means a new model version that lightweight migration can handle.
  - `CogPlaylist`'s `PlaylistStore` owns the managed object model, so the entity classes aren't generated twice.
- `CogPlaylist/` builds for both platforms. It contains:
  - `PlaylistStore`: the store.
  - `PlaybackOrder`: shuffle, repeat, queue and stop-after.
  - `PlaylistExpander`: turns dropped files, folders and containers into ordered, filtered track URLs.
  - `PlaylistLoader`: metadata loading in the background.
  - `PlaylistEntry` extensions.
- macOS already uses `PlaybackOrder`, `PlaylistExpander` and `PlaylistStore` from
  `Playlist/PlaylistController.m` and `PlaylistLoader.m`. Other rules still live in the AppKit
  `Playlist/PlaylistController.m`, and the two must be kept in step. `iOS/README.md` describes what macOS
  uses and what remains: delegating the rest of `PlaylistController`'s rules to `PlaylistModel`.
- `Utils/SQLiteStore` is the older SQLite playlist store, kept only to import old databases.

### macOS app

- `Application/AppController.m` registers the user defaults, wires up the windows and owns application
  lifecycle.
- `Application/PlaybackController.m` drives `AudioPlayer`: transport, volume, fades, Now Playing, and
  scrobbling hand-off.
- `PlaybackEventController` handles notifications.
- AppleScript lives in `ScriptAdditions` and `Cog.sdef`.
- **Window/**:
  - The main window, with toolbar controls (`PositionSlider`, `VolumeSlider`, `WaveformController`,
    `SpeedButton`) and playlist tabs.
  - The mini windows.
  - Docking panes: `PaneManager`/`PanePlacement` dock panes on any edge of the playlist, in layers and rows.
    The equalizer stays a separate window.
- **Preferences/**: SwiftUI panes in `Preferences/Panes/*View.swift`, hosted by `PreferencesWindow` and
  `PreferencesModel`, plus older Objective-C pane code in `Preferences/Preferences/`.
- Other feature folders: `InfoInspector/`, `SignalInspector/`, `FileTree/`, `Spotlight/`, `Scrobbler/` (Last.fm
  and ListenBrainz), `LyricsWindow/` (tags, `.lrc` files or LRCLIB, synced where timed), `Visualization/`,
  `Equalizer/`, `TagEditor/`.
- `CogQuickLook/` is the Finder preview extension. It plays files through the same plugins.
- **Sandbox**:
  - The app is sandboxed. Every file access outside the granted folders goes through `Utils/SandboxBroker`,
    which holds security-scoped bookmarks stored as `SandboxToken` entities.
  - Plugins reach it through the headers, and `beginFolderAccess:`/`endFolderAccess:` bracket each access.
  - Code that opens user files must use these, or it works in Xcode and fails in the shipped app.
  - Entitlements: `Cog.entitlements` (App Store) and `Cog-Direct.entitlements`. The Quick Look extension has
    its own pair.
- **Remote Control / MCP**:
  - `RemoteControl/` bootstraps and adapts the app to `CogRemoteControlTarget`.
  - `CogRemoteControl/` is the MCP server and tool handlers (`RemoteControlToolHandlers.swift`). It listens on a
    Unix socket (`ipc.sock`) in the app-group container.
  - `CogMCP/main.swift` builds `cog-mcp`, a dependency-free stdio↔socket byte pump that MCP clients launch.
  - New tools go in the tool handlers. `cog-mcp` stays dumb.
- Logging: `DLog` (debug builds only) and `ALog` in `Utils/Logging.h`. Sentry crash reporting needs the user's
  consent (`sentryConsented`).

### iOS app (`iOS/`)

- `iOS/App/` is the SwiftUI app: playlist, Now Playing and settings.
  - Its `Player` plays the part `PlaybackController` plays on macOS.
  - Music picked in Files is copied into `Documents/Music`, and folders are played where they are.
- `iOS/README.md` documents the layout and the differences from macOS:
  - No exclusive output or DoP.
  - 2SF runs without its JIT.
  - MIDI's Apple synth is MIDISynth.

## Generated files: edit the generator, not the output

- `iOS/MobileCog.xcodeproj` is generated by `iOS/generate_plugins_project.py`.
  - To add or change a plugin, edit `PLUGINS` there, re-run the script, and don't edit the project in Xcode.
  - Files added under `iOS/App/` are picked up automatically.
- `Info.plist` (gitignored) is generated from `Info.plist.template` by `Scripts/genversion.sh` at build time.
  - Document types are declared in the template.
  - `iOS/generate_document_types.py` derives `iOS/CogApp-Info.plist` from them.
  - `Scripts/generate_quicklook_types.py` derives the Quick Look extension's types.
  - Re-run both scripts after changing document types.
  - The template's document types are themselves produced by hand, at runtime: change the `#if 0` blocks
    around `xmlEscapeString` and the end of `printPluginInfo` in `Audio/PluginController.mm` to `#if 1`,
    build and run the macOS app, and it writes the registered plugins' types to `Cog_Info.plist` in
    `NSTemporaryDirectory()`, which is the sandbox container's `tmp` folder
    (`~/Library/Containers/org.cogx.cog/Data/tmp/`). Merge that into the template and restore the `#if 0`.
    A build-time tool that collects the plugins' types without running the app would be better.
- Acknowledgements: edit `Acknowledgements/components.json` (or add a license under `Acknowledgements/Licenses/`),
  then run `Scripts/build-acknowledgements.py`. It writes `Acknowledgements/Acknowledgements.json` (macOS) and
  `iOS/App/Acknowledgements.json`.
- Versions are commit counts:
  - `Scripts/genversion.sh` counts from the `k54` tag. `iOS/genversion.sh` counts from 700560d4f (version 1).
  - A shallow clone builds version 0. Xcode Cloud unshallows in `ci_pre_xcodebuild.sh`.
- Porting a project from `Frameworks/` to iOS:
  - `Scripts/make-multiplatform.py` adds iOS to its targets.
  - `Scripts/link-ios-libraries.py` links the iOS builds of the `ThirdParty/` libraries it uses.
  - A new iOS `ThirdParty/` library needs a `build_<name>` function in `Scripts/build-ios-libraries.sh`.

## Adding a format plugin

1. Create `Plugins/<Name>/<Name>.xcodeproj` producing a bundle. Copy an existing plugin of similar shape:
   `Flac`, a single decoder with a cue sheet, or `GME`/`vgmstream`, containers plus decoders over a
   `Frameworks/` library.
2. Implement the `Audio/Plugin.h` protocols you need. Set a sensible `priority`, and give `interrupt` a real
   implementation if `readAudio` can block.
3. Add the project to `Cog.xcodeproj` as a dependency, and copy the bundle into PlugIns.
4. Add it to `PLUGINS` in `iOS/generate_plugins_project.py` and regenerate.
5. Declare any new extensions in `Info.plist.template`, then re-run the document-type and Quick Look generators.
6. Add its license to the acknowledgements.

## Conventions

- **Strings:**
  - Strings live in String Catalogs: `Localizable.xcstrings`, `InfoPlist.xcstrings`, `Direct.xcstrings`
    (direct edition only), `iOS/App/Localizable.xcstrings`, and `CogQuickLook/Localizable.xcstrings`.
  - Each has English and Spanish.
  - iOS keeps the keys that shared Objective-C code requests with `NSLocalizedString` by hand, because the Swift
    compiler doesn't extract them. Its wording follows the macOS app's.
- **Formatting:**
  - Objective-C, C and C++ follow `.clang-format`: LLVM base, tabs for indentation, width 4.
  - Swift uses tabs too.
- **Commits:**
  - Subjects take the form `Area: Imperative sentence`, for example `Playlist: Rate only by a click on the
    stars`, `Engine: …`, `Window: …`, `SID: …`, `Quick Look: …`.
- **Spelling:** use American English (center, color) in code, comments, UI strings and commit messages.
- **Licenses:** code under `Frameworks/`, `Plugins/` and `ThirdParty/` keeps its own license. Some of it has been
  modified to build in Cog, so check for local patches before updating it from upstream.
