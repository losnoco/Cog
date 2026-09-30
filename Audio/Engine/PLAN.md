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

   *Not behind the switch at this stage* (all since ported, in stages 5
   and 6): DSP, visualization, `resetNextStreams`, the seek crossfade, live
   `volumeScaling` changes, and device changes in place.
5. **DSP thread.** Port stages as in-place transforms, one at a time: fader
   (transport + seek crossfade), downmix, EQ (re-home the EQ window's
   coupling), FreeSurround, HRTF, Rubber Band, Signalsmith. Visualization tap.
   *Done:* chain order is time-stretch, FreeSurround, EQ, visualization tap,
   HRTF, then the channel fit to the device.
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
   - `VisualizationTap`: a pass-through stage after the EQ (where the node
     sat) that folds to mono with `DownmixProcessor`, resamples to 44.1 kHz
     (soxr, quick quality) and posts to `VisualizationController`; the
     monitor posts the latency from the shallow ring's write position to
     what is heard, and the full latency with the deep ring added.

6. **Parity and switch.** Seek, pause, stop, fades, DoP, HDCD sustain, cue
   `setTrack:` reuse, error handling, play-count/scrobble timing, Remote
   Control and MCP paths. Flip the default.
   *Done:* the new engine is the default; `enableNewAudioEngine` set to NO
   selects the chain engine until stage 7 removes it.
   - Metadata changes (`pushInfo`): the feeder watches the decoder's
     `metadata` through KVO, as InputNode did, and puts the merged
     properties and metadata on the timeline at the first frame decoded
     after the change; the pump turns it into a presentation event, and the
     monitor pushes it to the app when that audio is heard. InputNode
     pushed it as soon as the decoder changed, up to the whole buffer early
     (about ten seconds in this engine's deep ring), so a stream title
     changed before the song did.
   - Remote Control and MCP: nothing to port. The adapter works through
     `PlaybackController`, `AppController.clickSeek` and the playlist
     controller, which reach whichever engine `AudioPlayer` uses; playlist
     edits arrive as `resetNextStreams`. The MCP helper only relays to the
     same listener over its socket.
   - Errors: as InputNode did on starting each track, the engine flags a
     track whose file could not be read (the feeder opened the silence
     standing in for it) and clears the flag on one that opened normally.
     Tracks no decoder takes are flagged and skipped; a refused seek
     restarts the track rather than failing.
   - Cue sheets: when the next track is in the same file (URLs equal but
     for the fragment) and the decoder takes `setTrack:`, the feeder keeps
     it, as AudioPlayer arranged: the next track carries straight on from
     the last one's end, with no reopening or seeking.
   - Device changes: a new device (by setting, or a new system default
     while following it) is switched under the running pipeline first; if
     it renders the same rate and channel count, playback carries on
     without a rebuild. Otherwise, or on a format change, or for a DoP
     pipeline, the app is asked to restart at the current position.
   - Suspend on pause: once the pause fade ends, the renderer is held
     (silence, or DoP silence, without reading the ring) and the device
     keeps running, so resuming does not restart it and a DoP DAC stays
     locked. With `suspendOutputOnPause` on (the default) the device stops
     ten seconds in, as OutputCoreAudio's idle timer did; changing the
     setting while paused starts or stops that clock.
   - HDCD: decoding comes with ChunkList, which only looks for it in
     lossless audio, so the feeder marks each chunk lossless or not from
     the decoder's `encoding`, as InputNode did (most decoders already
     do). There is no indicator to port: the UI's was removed, and the old
     engine's sustain calls end in a commented-out delegate method.
   - `resetNextStreams`: the feeder remembers each join (the deep-ring
     frame where a track began after another, and the end of stream). On a
     playlist edit it asks the pump, through the timeline, to abandon from
     the earliest join the pump has not yet reached. The pump accepts only
     if it has not read past that frame (nor taken the entries at it), and
     then skips the deep ring from that frame to what the feeder had
     written; the feeder drops those timeline entries, restarts the
     converter at the join and asks the delegate again for the track after
     the one before it. A join already reached is left alone and the next
     one tried, so a track that has begun playing is never cut.
   - Seek crossfade: the renderer does it, in C, as it honours the shallow
     ring's flush. It keeps up to 200 ms of the discarded frames (what was
     about to be heard) and fades them out on an equal-power curve while
     the new audio fades in on the matching one from its first frame, so a
     slow seek fades out into silence rather than stopping. A seek during a
     crossfade folds the one under way into the new tail, so nothing steps.
     With fades off, or while paused, a seek cuts, as it will for DoP. The
     old engine used linear ramps and ran the fading audio through its own
     copy of the HRTF, downmix and fader; the tail here has already been
     through the chain.
   - Manual track changes crossfade the same way: `play:` while playing on
     the same device moves the running feeder to the new track as a seek
     would, instead of building a new pipeline. Paused, prebuffering,
     starting paused, or after a device change, it rebuilds as before.
   - DoP: `play:` opens the first track's decoder (as BufferChain did) to
     learn whether it wants a DoP carrier: a sixteenth of the rate for DSD,
     or the rate itself for integer PCM of 24 bits or more at 176.4 kHz or
     more (possibly DoP already), with channels matching the device. If
     the device takes the rate, it is switched to it (for everything using
     it, as OutputCoreAudio did) and a carrier pipeline is built: the
     feeder packs DSD as DoP, the pump passes DoP blocks through with no
     ReplayGain, DSP or channel fit, and the renderer renders 24-bit
     integer, high-aligned. The renderer checks each slice: DoP passes
     bit-exact (the transport can only pass it or replace it with DoP
     silence), keeps its marker phase (dropping a frame rather than
     repeating a marker), is cut rather than crossfaded, and any shortfall
     while it plays is DoP silence, so the DAC stays locked. A next track
     wanting another carrier ends the stream before it; the engine then
     rebuilds for it and announces it when heard (not gapless, as the
     device rate changes). PCM after DoP stays in the carrier pipeline,
     resampled to its rate, as the old engine left the device rate alone.
     Unlike the old engine, a track whose carrier the device cannot take
     plays as PCM rather than failing.
     DoP is opt-in (`enableDoP`, off by default: a DAC that does not decode
     it plays noise, and there is no telling which kind is connected), and
     needs a specific output device, not the system default. As Pine Player
     does, the engine takes that device exclusively while playing DoP: hog
     mode, mixing off, and the stream's physical format set to integer at
     the carrier rate, with the unit's input matching it word for word.
     Without that the system mixer's float stage sits in the path and
     macOS DACs do not lock (seen on a FiiO KA11). macOS will not hand
     over a device that is the system's default output or sound-effects
     output, so, as Pine Player does, those defaults move to another output
     (built-in speakers first) while it is held and come back on release,
     unless changed meanwhile. It is released when the pipeline is torn
     down. No exclusive access means PCM.
7. **Delete `Audio/Chain/`** and `OutputCoreAudio.m`.
   *Done:* the chain engine, its nodes, `OutputCoreAudio` and the unused
   `OutputAVFoundation` are gone, and with them `enableNewAudioEngine`.
   What the engine and the plugins still use moved to `Audio/Shared/` (a
   synchronized folder, public headers listed in its exception set):
   `AudioChunk`, `ChunkList` (float conversion, HDCD, DSD decimation and
   DoP packing), `Downmix`, `FSurroundFilter`, `HeadphoneFilter`, and the
   `CogEqualizer` protocol in a header of its own. `AudioPlayer` is a thin
   shell over `PlaybackEngine` with its public interface unchanged, and
   every plugin project's `AudioChunk.h` reference points at the new
   folder. The old per-track converter's seam test went with it.

8. **Exclusive output.** Ported from a fork of the chain engine
   (`feat/bit-perfect-pcm-dop-output`), whose settings it keeps:
   `exclusiveIntegerOutput` and `setDeviceVolumeTo100ForExclusiveOutput`,
   both off by default.
   *Done:*
   - With `exclusiveIntegerOutput` on and a specific output device (not the
     followed system default, one output stream of at most eight channels),
     every PCM track holds the device: hog mode, and the device at the
     track's own rate, or failing that the nearest it offers (a whole
     multiple above, else a whole fraction below: DSD64 made PCM, 352.8 kHz,
     plays at 176.4 kHz on a 192 kHz device), so nothing is resampled that
     need not be.
   - The stream is set to the best format it offers at that rate
     (`DeviceOutput.exclusiveCandidates`): integer the system cannot mix
     first, widest first (32, then 24 in its layouts, then 16), then mixable
     integer (the system converts float to it, exactly for 24 bits), then
     float. Widest rather than the source's depth: every source up to 24 bits
     passes exactly through any of 24 bits or more, processed audio loses
     least, and tracks of other depths at the same rate stay gapless. DoP
     now goes through the same path, needing integer of 24 bits or more.
   - The device's rate is set through the stream's format alone. Setting
     the nominal rate first as well (as the DoP path did), two
     reconfigurations back to back, left an SMSL DAC unable to start I/O in
     three starts out of four: the HAL waits seven seconds ("IO is still
     disabled after waiting") and fails with EAGAIN. The nominal rate is now
     only a fallback, for a device that will not take a rate as part of a
     format. Should a held device still not start, playback restarts shared.
   - A non-mixable stream's virtual format is its physical one, so the
     renderer writes the DAC's own words. AUHAL will not drive such a device
     (setting it as the unit's device fails with -10851 and the unit stays on
     the old one, which the fork met as silent DoP), so a held device is
     rendered into by an IOProc on the device, `cog_renderer_device_io_proc`,
     plain C like the unit callback, with no converter after the renderer.
   - The renderer converts to Int32, Int24 (high- or low-aligned in 32 bits,
     or packed in 3 bytes) or Int16 by scaling by 2^(bits-1), the exact
     inverse of ChunkList's integer-to-float scaling, rounding to nearest and
     clipping before rounding (the fork's converter could round just short
     of full scale past it and wrap). Every 16- and 24-bit sample comes back
     exactly, with or without the HDCD decoder engaged (unity gain unless
     HDCD is found), and DoP words exactly in any 24- or 32-bit layout, so
     DoP needs no special case. The old 24-bit PCM conversion scaled by
     2^31 - 1 and was one step low for positive samples. Callbacks loop over
     the scratch buffer rather than truncating a cycle.
   - Bit-perfect needs nothing more: at unity track gain, volume 100 % and no
     active stage nothing multiplies the samples (transport fades and seek
     crossfades aside, which only shape transitions). Otherwise the output is
     still integer, and the status says what changed it.
   - The feeder admits a track into the stream when it wants the same plan
     (`PlaybackEngine.OutputPlan`: the device's rate, DoP or not, held or
     not) or PCM wanting the held device at the rate it already runs at;
     otherwise the stream ends before it and the engine rebuilds at the new
     rate. The device stays held through that rebuild (and through `play:`
     rebuilds), so another app or the system default cannot take it in
     between; it is given back on stop, at the end of the playlist, or when a
     plan no longer wants it. Pause keeps it, as for DoP.
   - Fallbacks: DoP that cannot be had becomes PCM (held, if wanted), and a
     device that cannot be held (another process holds it, or it will not
     start) plays shared, and is not tried again until playback stops or the
     setting changes. Turning the setting on or off while playing restarts at
     the current position.
   - Releasing puts back each stream's physical format (and with it the
     device's rate), mixing, the system defaults moved off the device, and a
     device volume set to full (if still full). Setting hog mode toggles it,
     whatever value is written, so it is set only on an observed owner: the
     old release would have taken a device whose hold had been lost.
   - A process that dies holding a device loses hog mode, but macOS leaves
     the stream in the format it was given, non-mixable, where no other app
     can play (seen on an SMSL DAC). So what holding changes is recorded in
     the defaults (`exclusiveOutputSession`, devices by UID) while held, and
     `DeviceOutput.recoverAbandonedSession` undoes it at the next launch, or
     before the next hold, if nobody else holds the device by then.
   - `setDeviceVolumeTo100ForExclusiveOutput` turns the device's own volume
     (main control, else each channel's) to full while held, and back.
   - Not carried over: the fork passed integer PCM through untouched in
     integer containers, so 25- to 32-bit integer sources stayed exact. Here
     everything is Float32 between the decoder and the renderer, so those
     are rounded to 24 bits (the status says "Sample precision reduced").
     Carrying them exactly needs a wider sample path than this engine's.
   - Tests: `ExclusiveOutputTests` (conversion through ChunkList, the
     callbacks, format ranking, rate choice, planning, status) offline, and
     `ExclusiveDeviceTests` against a real device, run only when
     `COG_EXCLUSIVE_TEST_DEVICE` names one (pass it to xcodebuild as
     `TEST_RUNNER_COG_EXCLUSIVE_TEST_DEVICE`).

## Open questions

- Where the new engine's Swift lives in the target (a folder in CogAudio vs a
  separate framework) and how the C core is exposed (module map vs bridging
  header).
- Whether FreeSurround belongs in the feeder (XPCog) or the DSP thread (so it
  can be toggled without a flush).
