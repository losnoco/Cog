# Cog

[![buildable](https://github.com/losnoco/Cog/actions/workflows/debug.yml/badge.svg)](https://github.com/losnoco/Cog/actions/workflows/debug.yml)
[![iOS](https://github.com/losnoco/Cog/actions/workflows/ios.yml/badge.svg)](https://github.com/losnoco/Cog/actions/workflows/ios.yml)
[![Join the chat at https://gitter.im/losnoco/cog](https://badges.gitter.im/losnoco/cog.svg)](https://gitter.im/losnoco/cog?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Cog is a free, open-source audio player for Mac, iPhone and iPad. It plays a
very wide range of formats, from everyday lossy and lossless audio to tracker
modules, video game music and MIDI, and it takes care over how they sound.

Every platform is built from this one repository and shares the same audio
engine and format plugins. The Mac and iOS apps are released separately.

## Get Cog

- **Mac App Store:** [Cog](https://apps.apple.com/us/app/cog-kode54/id1630499622)
- **Direct download:** [cog.losno.co](https://cog.losno.co/). This edition
  updates itself with Sparkle.

Cog runs on macOS 12 or later, and on iOS and iPadOS 18 or later.

## What it plays

Each format is handled by a plugin under [`Plugins/`](Plugins):

- **Common formats:** MP3, AAC, ALAC, FLAC, Ogg Vorbis, Opus, WavPack,
  Musepack, Shorten, and anything else Core Audio or FFmpeg can decode
- **Tracker modules:** libopenmpt, HivelyTracker, Organya and Syntrax
- **Video game music:** Game_Music_Emu, vgmstream, libvgm, sidplay (C64
  SID), AdPlug (AdLib), and the PSF family through HighlyComplete
- **MIDI**, through Cog's built-in synthesizers (SoundFonts, OPL3 FM, and
  Roland SC-55 emulation from your own ROM set), or through Audio Unit instruments on the Mac
- **Playlists and containers:** cue sheets, M3U and PLS, and files inside
  archives
- **Streams** over HTTP and HLS

## Features

**Sound**
- ReplayGain by album or track, with clipping prevention, falling back to
  Sound Check
- An equalizer with presets
- Speed and pitch control with the Rubber Band or Signalsmith Stretch
  engines, or plain varispeed like a record player
- Spatial audio: surround tracks are spatialized on headphones and stereo
  speakers, with head tracking
- On the Mac, an exclusive mode that takes the output device for Cog and
  runs it at each track's sample rate

**Listening**
- A spectrum visualizer
- Synced lyrics from the file's own tags, or looked up on
  [LRCLIB](https://lrclib.net) when the file has none
- Scrobbling to Last.fm and ListenBrainz

**On the Mac**
- A playlist with an Info Inspector, a file tree browser, Spotlight search,
  and a mini window
- Global hot keys, notifications, and AppleScript support
- Remote Control (macOS 13 or later): with the bundled `cog-mcp` tool, MCP
  clients such as AI assistants can control playback and the playlist

**On iPhone and iPad**
- Plays files and folders from the Files app where they are, without
  importing copies
- Now Playing with a scrubbing slider, fast forward and rewind by holding
  Next and Previous, and lock screen controls

## Using the sandboxed Mac app

Cog runs in the App Sandbox, so it can only open folders you have granted it.
Grant them in **Settings → General**. Right-clicking the list there lets you
edit it, and can suggest additions based on the current playlist. The
suggestions cover every path not already granted, so pick only the few
folders that hold most of your music. Your Music, Downloads and Movies
folders are granted already.

## Building from source

### Common setup

1. Clone the repository and its submodules:

   ```sh
   git submodule update --init --recursive
   ```

2. Create `Xcode-config/DEVELOPMENT_TEAM.xcconfig` with your team ID. That
   file is gitignored. Because the `group.org.cogx.cog` App Group belongs to
   the upstream team, also turn off provisioning profiles:

   ```
   DEVELOPMENT_TEAM = YOURTEAMID
   PROVISIONING_PROFILE_SUPPORTED = NO
   ```

   [`Xcode-config/Shared.xcconfig`](Xcode-config/Shared.xcconfig) explains
   both settings and how to find your team ID.

3. Install the hooks, so a team ID never ends up committed in a project file:

   ```sh
   git config core.hooksPath .githooks
   ```

4. Optionally, turn on Last.fm scrobbling by copying
   `Xcode-config/Secrets.template.xcconfig` to `Xcode-config/Secrets.xcconfig`
   and adding your [API key](https://www.last.fm/api/account/create).

### Mac

1. Unpack the prebuilt static and dynamic libraries. Repeat this whenever
   `ThirdParty/libraries.tar.xz` changes:

   ```sh
   cd ThirdParty
   tar xvf libraries.tar.xz
   ```

2. Open `Cog.xcodeproj` and build one of the two schemes:

   | Scheme       | Builds                                                                  |
   | ------------ | ----------------------------------------------------------------------- |
   | `Cog`        | The App Store edition                                                   |
   | `Cog Direct` | The direct edition (Release-Direct): adds Sparkle and the donation menu |

### iPhone and iPad

1. Run `iOS/prepare.sh`. It applies the patches the iOS build needs to some
   submodules, and builds the third-party libraries for iOS, which takes a
   few minutes and needs `brew install cmake ninja`. The "Check if Cog builds
   for iOS" workflow also publishes them as its `ios-libraries` artifact.

2. Open `iOS/MobileCog.xcodeproj` and build the `Cog` scheme.

The iOS project is generated, so don't change it in Xcode.
[`iOS/README.md`](iOS/README.md) explains how it is laid out and how to
regenerate it.

## History

Vincent Spader wrote Cog and released it under the GPL. When its development
stopped, Christopher Snowhill forked it in 2013 and has maintained it since.

## License

Cog is licensed under the GNU General Public License. See [`COPYING`](COPYING).

The libraries under `ThirdParty/`, `Frameworks/` and `Plugins/` remain under
their own licenses and copyrights. Some have been modified to build in Cog.

## Contact

Christopher Snowhill, chris@kode54.net

Share and enjoy.
