# CogAudio engine rewrite

A replacement for the `Audio/Chain/` engine, written mostly in Swift, with far
fewer threads, and no pipeline teardown at track boundaries. It borrows the
proven shape of XPCog's engine (`../xpcog/core/src/audio/AudioEngine.cpp`) and
keeps Cog's plugin contract and `AudioPlayer` API unchanged.

## Why

- **Seam artifacts.** Repeat-one on a 48 kHz Ogg with the device at 384 kHz
  clicks at the loop point. `ConverterNode` keeps frame counts exact, but its
  per-track resampling leaves an error just before each join that grows with
  the upsampling ratio (about 50× larger at 384k than at 44.1k; see stage 1).
  The audible click itself turned out to be the time-stretch node flushing
  and rebuilding its stretcher at every track boundary (stage 1). More
  generally, Cog's handoff is fragile:
  each track owns a `BufferChain`, and a track change drains the whole DSP
  pipeline to empty, swaps chains (`selectNextBuffer` / `reconnectInputAndReplumb`)
  and refills it 512 frames at a time through ~10 node threads, while the
  track-change notification runs off the same path.
- **Threads.** Today: InputNode + ConverterNode per chain (doubled while the
  next chain prerolls), five `OutputNode` DSP threads (Rubber Band, Signalsmith,
  FreeSurround, EQ, visualization), the `OutputCoreAudio` thread, and HRTF,
  downmix, fader and `SimpleBuffer` threads — 13–15 threads, each with a
  `ChunkList`, two semaphores and a recursive lock, exchanging 512-frame chunks.
- **Real-time safety.** The render block takes an `NSLock`, enters an
  `@autoreleasepool` and messages Objective-C objects on the I/O thread.

## Decisions

| Topic | Decision |
| --- | --- |
| Threads | Two workers — **feeder** and **DSP** — plus the CoreAudio render callback. |
| Language | Swift for the engine; plugins stay Objective-C (`CogDecoder` returns `AudioChunk`). |
| Atomics / RT path | A small C core (C11 `stdatomic`): SPSC float ring, render inner loop, unfair locks. Called from Swift. No package dependency. |
| Deployment target | macOS 12. Nothing here needs 13. Dropping Intel is an `ARCHS` change, independent of this work. |
| Rollout | New engine beside `Chain/`, selected by a hidden default. A/B the same files, flip the default at parity, then delete `Chain/`. |

## Target architecture

```
decoder (ObjC plugin, AudioChunk)
  └─ feeder thread ─────────────────────────────────────────────┐
       float convert · HDCD · DSD decimate · channel fit ·       │
       soxr (persistent) · ReplayGain · seam markers             │
                                                                 ▼
                                             deep ring (seconds, SPSC)
                                                                 │
  ┌─ DSP thread ─────────────────────────────────────────────────┘
  │    time-stretch · FreeSurround · EQ · HRTF · downmix · fader
  │    (synchronous in-place transforms, one pass per block)
  ▼
shallow ring (seconds, SPSC)
  └─ render callback (C): read ring · volume · transport fade · DoP pack
```

### Principles

1. **Nothing is torn down at a track boundary.** The next decoder is opened by
   the feeder when the current one hits EOF; the DSP stages and output never
   see a boundary other than a seam marker.
2. **The resampler persists.** One soxr instance for the playback session. At a
   seam where input rate and channel count are unchanged (always true for
   repeat-one), keep feeding it — no drain, no LPC. Drain + LPC pre/post
   extrapolation only on a real format change, at play start and at the final
   end of stream. Equal input and output rates bypass soxr (bit-exact).
3. **Notifications are positional.** A seam is `(frame index, userInfo)` in a
   queue. The render side advances a played-frames counter; the main thread
   (or a timer) publishes `didBeginStream`, play counts and scrobbles when the
   counter passes a seam. Nothing on the audio path waits on the main thread.
4. **Size in seconds, not samples.** Ring capacities are computed from the
   device rate and channel count so 384 kHz gets the same time cushion as
   44.1 kHz. (XPCog sizes in samples; at 384 kHz its shallow ring is ~21 ms.)
5. **The render callback is real-time safe.** No locks, no allocation, no ARC,
   no Objective-C messaging, no autorelease pools. Underruns zero-fill (or
   DoP-silence-fill) and are counted.
6. **Flushes are acknowledged.** Seek and stop use a flush epoch: the producer
   bumps it, the consumer drops its read position to the write position and
   acknowledges; position bookkeeping restarts only after acknowledgement.

### Carried over from Cog (not in XPCog)

- DoP carrier output and marker-phase continuity (`FadedBuffer.m`,
  `OutputCoreAudio.m` DoP paths), integer render format for DoP.
- HRTF (`DSPHRTFNode`, `HeadphoneFilter`).
- Device-change handling: default-device and device-alive listeners,
  nominal-rate and stream-format listeners (`OutputCoreAudio.m`).
- Visualization posting via the existing `VisualizationController.swift`,
  fed from a tap in the DSP thread rather than a node thread.
- `AudioPlayer` public API and delegate protocol (`AudioPlayer.h`), used by
  `PlaybackController.m`; the EQ window's `DSPEqualizerNode` coupling is the
  one app-side dependency to re-home.

### Borrowed from XPCog

- Two-ring topology and synchronous DSP chain (`AudioEngine.hpp:15-33`).
- Consumer-honoured ring flush (`RingBuffer.cpp:74-138`).
- `dspBusy`-style in-flight accounting so end-of-stream drain never loses a
  block (`AudioEngine.cpp:893-901`).
- Equal-power seek crossfade from the exact discarded frames (`Fader.hpp`).
- Stretch map for position under time-stretch instead of per-chunk
  `streamTimestamp`/`streamTimeRatio` (`PORTING.md:417-427`).
- Exact GCD-reduced LPC pad/trim and "a drained soxr is dead" rule
  (`AudioConverter.cpp:100-157, 690-747`).

## Stages

Each stage ships on its own and leaves the old engine working.

1. **Seam test harness.** An offline test target that drives the converter (old
   and new) with a continuous signal split into tracks and a track looped into
   itself, compared against one uninterrupted soxr pass. Rates: 44.1k/48k in,
   44.1k/48k/96k/192k/384k out. Plus an `OUTPUT_LOG`/`LOG_CHAINS` capture of the
   old engine at 384 kHz to confirm where the click is introduced.

   *Done (converter part):* `Audio/CogAudioTests/ConverterSeamTests.swift`
   runs the real `ConverterNode`, one per track, on a loopable noise-like
   signal. Findings:
   - Frame counts are exact at every rate; nothing is dropped or repeated.
   - The join error is confined to about one input sample period before the
     seam, oscillates at the input Nyquist, and grows with the upsampling
     ratio: 2.8e-4 (48k→44.1k), 7.7e-4 (44.1k→48k), 5.0e-3 (48k→96k),
     1.1e-2 (48k→192k), 1.4e-2 (48k→384k), 2.9e-2 (44.1k→384k). The output
     frames between the last input sample of one track and the first of the
     next are interpolated toward the LPC guess, because a per-track
     resampler cannot see the next track. Only a resampler that carries
     state across the seam (principle 2) removes it; the test's strict
     1e-4 check is an expected failure until then.
   - The error is mostly ultrasonic, so it may not be the audible click;
     the full-engine capture at 384 kHz is still needed to see whether the
     handoff adds anything.

   *Done (full-engine capture, 2026-09-29):* `LOG_CHAINS` + `OUTPUT_LOG` dumps
   of SunkenSea.ogg (48 kHz Vorbis) on repeat-one at 384 kHz, with the
   Signalsmith stretch engine selected at tempo 1 / pitch 1. Every node dump
   up to `DSPSignalsmithStretchNode`'s input is bit-identical to the
   converter output across the seam. Signalsmith's output departs from its
   input only at the seam: from 90 ms before it (peak error 0.15, about
   −16 dB, 2 ms before the join) to about 23 ms after. **That is the click.**
   Cause: at the end of each chain the node calls `ts->flush()`
   (`DSPSignalsmithStretchNode.mm:418-427`), which renders the stretcher's
   latency as an ending, and on reconnection `setEndOfStream:NO` deletes
   the stretcher (`:220-222`), so the next track starts cold with
   `outputSeek`. `DSPRubberbandNode` has the same flush-and-shutdown pattern
   (`DSPRubberbandNode.m:383-391, 523`). XPCog's `TimeStretch` runs straight
   through seams, which is why it does not click. Principle 1 covers this:
   stretchers are reset only on play, seek or a device format change.
2. **C core.** SPSC ring (power-of-two, whole frames, flush epoch), render loop,
   lock shim. Unit-tested from Swift.

   *Done:* `Audio/Engine/Core/`, compiled into CogAudio through a
   synchronized `Engine` folder (PLAN.md excluded; `CogRing.h` and
   `CogRender.h` public).
   - `CogRing`: SPSC interleaved float frames, power-of-two capacity,
     64-bit frame positions that never wrap, producer and consumer counters
     on separate 128-byte cache lines. Unlike XPCog's ring, a flush request
     records the producer's write position and the consumer jumps to *that*
     position, so the producer can keep writing post-seek audio without
     waiting for the acknowledgement.
   - `CogGain`: a per-frame linear ramp steered from any thread (target and
     length packed into one atomic word), used for volume and transport
     fades; lands exactly on its target and reports when settled.
   - `CogRenderer`: the callback's inner loop — honour flush, read, zero-fill,
     apply transport and volume gains, count rendered and silent frames and
     underrun events. No allocation, locks or Objective-C.
   - The lock shim is unnecessary: Swift uses `os_unfair_lock` through a
     heap-allocated pointer off the real-time path.
   - Tests: `CogRingTests` (including two-thread lossless-transfer and
     flush-only-skips-forward stress) and `CogRenderTests`; clean under
     Thread Sanitizer.
3. **Feeder + converter in Swift.** Decoder open/advance, float conversion,
   HDCD, DSD, channel fit, persistent soxr with seam carry-over, ReplayGain,
   seam markers. Output to the deep ring.

   *Done (first cut):*
   - CogAudio is now a proper framework module: `DEFINES_MODULE`, umbrella
     `Audio/CogAudio.h` listing every public header, no bridging header (the
     old one only served `VisualizationController.swift`, which is not in the
     target — the Objective-C `VisualizationController.m` is what builds), no
     project-level `PRODUCT_MODULE_NAME` (it made the test bundle's module
     `CogAudio` too). `DSPFaderNode.h` forward-declares `FadedBuffer` instead
     of importing a project header; `AudioSource.h` is public.
   - `SWIFT_INSTALL_OBJC_HEADER = NO`: the plugins build in parallel with
     CogAudio without a dependency on it, and a module map naming the
     late-generated `CogAudio-Swift.h` broke them. CogAudio's own Objective-C
     can still `#import "CogAudio-Swift.h"` from derived sources.
   - Private C for the engine's Swift goes through
     `Engine/Internal/module.modulemap` (`CogAudioEngineInternal`, imported
     `@_implementationOnly`); currently lvqcl's `lpc.h`.
   - `Plugin.h` no longer declares `-dealloc` in `CogSource`/`CogDecoder`;
     Swift cannot conform to a protocol that does, and it meant nothing.
   - `StreamConverter`: one soxr run across same-rate, same-channel-count
     tracks; drain with LPC forward extrapolation only on a format change or
     at the end; LPC lead-in waits for a full prime length of input; bypass
     at equal rates; ReplayGain applied to the input; exact seam positions
     (`outputPositionOfNextInput`). Repeat-one seam error at 48k → 384k is
     5.7e-7 against a continuous resample (interior: 6e-7), versus 1.4e-2
     for `ConverterNode`.
   - `Feeder`: one thread; opens decoders through the plugins (or an
     injected opener), converts via `ChunkList.removeSamplesAsFloat32:`
     (PCM, DSD, HDCD), resamples, and writes to a deep `CogRing` of single
     samples (channel count may change between tracks) sized in seconds at
     8 channels. `Timeline` carries `.format`, `.trackStart(track, offset)`
     and `.endOfStream` at output-frame positions, stamped with the ring's
     flush epoch. Seek flushes the ring, resets the converter and restarts
     the frame count in a new epoch; it reopens the track if the feeder had
     already moved on. Unopenable tracks are skipped.
   - Tests: `StreamConverterTests`, `FeederTests` (with a Swift
     `CogDecoder` fake); Thread Sanitizer clean.
   - Still to do in the feeder: HDCD sustain notification, DoP passthrough,
     cue-sheet `setTrack:` reuse, metadata/property change events, and the
     ReplayGain calculation from `rgInfo` (currently `EngineTrack.gain`).
4. **Output.** AUHAL render through the C core; device format, device change
   and default-device handling ported from `OutputCoreAudio`. With stages 2–4
   the new engine plays audio with no DSP — flip the hidden default and A/B.

   *Done:*
   - `DeviceOutput`: AUHAL unit rendering interleaved float at the device
     rate (≤ 8 channels, Cog's layouts) from `CogRenderer`; the render block
     captures only the C pointer. Device chosen by ID, then name, else the
     followed system default; block listeners for default-device, alive,
     nominal-rate and stream-format changes. Reports presentation latency
     (device + stream latency, safety offset, one I/O buffer).
   - `Pump`: the DSP thread's skeleton — deep ring to shallow ring,
     frame-accurate against the timeline, channel fit via
     `DownmixProcessor`, track starts and end of stream turned into
     presentation events at absolute shallow-ring positions; a seek flushes
     the shallow ring and drops unheard events.
   - `PlaybackEngine` facade behind `AudioPlayer` when the hidden default
     `enableNewAudioEngine` is set (`defaults write org.cogx.cog
     enableNewAudioEngine -bool YES`). `AudioPlayer` implements
     `PlaybackEngineHost` with its existing delegate messages. A main-thread
     monitor announces tracks as they are heard (read position less device
     latency, or the latency in wall time once the ring runs dry), keeps
     `amountPlayed` with `OutputNode`'s rules, and reports play counts and
     scrobbles. Transport fades use the renderer's gain ramp; the device
     starts once 0.1 s is buffered. ReplayGain comes from `rgInfo` via a
     port of `refreshVolumeScaling`.
   - `Feeder.stop()` pumps the main run loop when called there, as
     `waitUntilCallbacksExit` does, so a feeder blocked asking the main
     thread for the next track cannot deadlock a stop.
   - Tests: `DeviceOutputTests` and `PlaybackEngineTests` run on the real
     default device (silently); `PumpTests` offline.

   *Not yet behind the switch:* DSP (EQ, HRTF, FreeSurround, time-stretch),
   visualization, DoP, HDCD indicator, `resetNextStreams` (a playlist edit
   after the next track has started decoding only applies from the track
   after it), seek crossfade (seeks duck for 5 ms instead), live
   `volumeScaling` changes, suspend-on-pause idle timer, and device changes
   rebuild playback through `restartPlaybackAtCurrentPosition` rather than
   in place.
5. **DSP thread.** Port stages as in-place transforms, one at a time: fader
   (transport + seek crossfade), downmix, EQ (re-home the EQ window's
   coupling), FreeSurround, HRTF, Rubber Band, Signalsmith. Visualization tap.
   *In progress:*
   - The pump runs `DSPStage`s in order between the track gain and the
     channel fit, reconfiguring on a format or active-set change, resetting
     on seek, and draining them (`drain(_:)`) at the end of the stream and
     before a format change.
   - `EqualizerStage`: the 31-band `vDSP_biquadm` EQ, driven by the EQ
     window through the `CogEqualizer` protocol; band changes no longer
     reset the filter state.
   - `FreeSurroundStage`: stereo to 5.1 through a FIFO of full 4096-frame
     blocks, the half-block lag removed and the tail drained, so the
     output is exactly as long as the input (the old node zero-padded short
     chunks mid-stream).
   - `HRTFStage`: `HeadphoneFilter` with the SADIE D02 set, primed from an
     LPC backward extrapolation after each start or seek; any layout in,
     binaural stereo out. Head tracking moves to a Swift `HeadTracker`
     (`CMHeadphoneMotionManager`, macOS 14+, same matrix conventions and
     `CogPlaybackDidResetHeadTracking` reset).
   - `TimeStretchStage`: Rubber Band (R2 `faster`, R3 `finer`, through its C
     API with the node's options mapping and live option changes) or
     Signalsmith (through a small C wrapper, `CogSignalsmith`), chosen by
     `rubberbandEngine`. Active only while `tempo` or `pitch` is off 1, so
     unity playback is untouched (the nodes always ran; entering or leaving
     unity mid-track restarts the stretcher, a brief glitch). Output is
     exactly `round(input / tempo)` frames. Rubber Band's start delay is
     reported in input frames and is now converted before being dropped;
     the node dropped it unconverted, shifting the start at any tempo but 1.
   - Stages report `pendingFrames`, and the pump places track starts and
     the end of stream after them, so announcements line up with the audio
     through FreeSurround's block and a stretcher's latency. The pump emits
     `.rate` presentation events when the chain's time ratio changes, and
     the monitor keeps a piecewise stretch map for `amountPlayed`.
   - Still to port: the visualization tap.

6. **Parity and switch.** Seek, pause, stop, fades, DoP, HDCD sustain, cue
   `setTrack:` reuse, error handling, play-count/scrobble timing, Remote
   Control and MCP paths. Flip the default.
7. **Delete `Audio/Chain/`** and `OutputCoreAudio.m`.

## Open questions

- Where the new engine's Swift lives in the target (a folder in CogAudio vs a
  separate framework) and how the C core is exposed (module map vs bridging
  header).
- Whether FreeSurround belongs in the feeder (XPCog) or the DSP thread (so it
  can be toggled without a flush).
- Hog/exclusive mode and following the track's rate on the device, versus
  always resampling to the device's current rate.
