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

3. **Feeder + converter in Swift.** Decoder open/advance, float conversion,
   HDCD, DSD, channel fit, persistent soxr with seam carry-over, ReplayGain,
   seam markers. Output to the deep ring.

4. **Output.** AUHAL render through the C core; device format, device change
   and default-device handling ported from `OutputCoreAudio`. With stages 2–4
   the new engine plays audio with no DSP — flip the hidden default and A/B.
5. **DSP thread.** Port stages as in-place transforms, one at a time: fader
   (transport + seek crossfade), downmix, EQ (re-home the EQ window's
   coupling), FreeSurround, HRTF, Rubber Band, Signalsmith. Visualization tap.
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
