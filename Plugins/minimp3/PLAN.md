# MP3 Chapters in minimp3 — Plan and Record

Chapter splitting for the minimp3 input: `MP3Decoder` plays a chaptered
MPEG audio file's `#N` spans itself, instead of refusing the fragment and
leaving it to FFmpeg. Ported from XPCog's implementation and investigation
(`../xpcog/docs/MINIMP3-CHAPTERS.md`, `../xpcog/docs/MINIMP3-CHAPTER-PROBE.md`,
`codecs/minimp3/Mp3Decoder.cpp`, `codecs/common/Id3v2.cpp`), adapted to Cog.

**Status:** Phase 1 landed (commit `ec49043af`, "Tag Editor: Share the
chapter reader with the ID3v2 tag reader"). Phases 2 and 3 are implemented and
verified: `MP3Decoder` plays `#N` spans, `CueSheetDecoder`'s embedded branch
opens the inner decoder fragment-less, `MP3SeekAccuracyTests` proves
sample-exact seeking on real LAME encodes, and `MP3ChapterTests` covers the
spans, titles, album, refusals, `setTrack:`, the cue guard, gapless chapter 0
and `iTunSMPB` composition. Both apps build and all 57 plugin tests pass.
Phase 4 (docs and manual E2E) remains. This file is the plan and the record;
keep it current as each phase lands, the way `Audio/Engine/PLAN.md` is kept.

## Goal and scope

A chaptered MP3 (an audiobook, say) expands into one playlist row per
chapter and each row plays its span. Today `FFMPEGContainer` lists the rows
and `FFMPEGDecoder` plays them; `MP3Decoder` refuses every fragment
(`Plugins/minimp3/MP3Decoder.m`, the `if([[[source url] fragment] length])
return NO;` at the top of `open:`). This work makes `MP3Decoder` accept a
numeric `#N` fragment on a seekable source and play the span in minimp3-ex's
gapless-adjusted sample space.

In scope: the MPEG audio family minimp3 already claims — `mp3`, `m2a`,
`mpa`. **Do not widen the extension list** (XPCog also claims `mp2`; Cog's
minimp3 does not, and adding it is an unrelated behavior change).

Out of scope: a new container (FFmpeg keeps listing the rows — see below),
chapter writing (TagLib owns writes, already shipped), HLS/stream chapters
(no seeking), M4A/Matroska chapters (FFmpeg's, unchanged).

## Why minimp3, not FFmpeg

Two reasons, both verified:

1. **Accuracy.** XPCog's probe found minimp3-ex seeking is sample-exact on
   CBR and VBR, with and without Xing, mono and stereo (0 samples positional
   error at every target), and the ex layer applies LAME's gapless delay and
   padding natively — ex sample 0 is the first *real* audio sample, so
   chapter 0 at ms 0 is gapless-correct by construction. FFmpeg's chapter
   playback seeks at packet granularity and reports the raw (untrimmed)
   length. The one genuine upside over FFmpeg is sample-accurate,
   gapless-correct chapter 0.
2. **Free-format MP3.** minimp3 decodes free-bitrate frames (it sizes them
   by scanning to the next sync word — `mp3d_find_frame` /
   `free_format_bytes` in `ThirdParty/minimp3.h` and `minimp3_ex.h`);
   FFmpeg's native MP3 decoder does not. A chaptered free-format file plays
   poorly or not at all through FFmpeg today, and correctly through minimp3.
   This is the additional reason to gate chapter playback on minimp3 rather
   than leave it to FFmpeg.

**Transferability:** Cog's vendored `minimp3_ex.h` differs from XPCog's only
in `(int)` casts, and `minimp3.h` only in XPCog's MSVC SIMD fixes. The
index/seek arithmetic is byte-identical, so the probe's conclusions apply to
Cog's copy directly.

## Settled decisions

- **No build gate.** XPCog gated acceptance on `XPCOG_WITH_FFMPEG=0` because
  its minimal builds have no FFmpeg. Cog always builds FFmpeg (both
  editions, both platforms), so acceptance is unconditional. The safety net
  is `CogDecoderMulti.open` (`Audio/CogPluginMulti.m`): it tries decoders in
  priority order and rewinds between attempts. minimp3 (priority 2.0) is
  strict — numeric fragment, seekable source, index in range, non-empty span
  — and **anything it refuses falls through to FFmpeg (1.5) exactly as
  today.**
- **No new container.** `FFMPEGContainer` (1.5) already lists `#0..#N-1`
  when `nb_chapters > 1`, and the editor's chapter N already equals row `#N`
  (`testMP3ChaptersRoundTripAndFFmpegListsThemInTheSameOrder` in
  `iOS/CogPluginsTests/TagWriterTests.swift`). XPCog's 0.4 container exists
  only for FFmpeg-less builds. Keep expansion FFmpeg's; change only
  playback. `CueSheetContainer` (16.0) still wins `mp3` for embedded cue
  sheets.
- **Parser: reuse TagLib, do not port a second one.** XPCog wrote a sibling
  C++ parser because its minimp3 codec cannot link TagLib. Cog's plugins use
  runtime `NSClassFromString` lookups across bundles (the established
  pattern: `FFMPEGDecoder` → `TagLibID3v2Reader`), so minimp3 reads chapters
  through TagLib's own reader. One reader means the editor, the playlist's
  rows and the decoder agree on the chapters and their order **by
  construction**. This is phase 1, landed.
- **Order rule (TagLib's, which the shared reader implements):** the
  top-level *ordered* CTOC's child order when one resolves, else every
  chapter stable-sorted by start. CHAP/CTOC are four-character frames, so a
  v2.2 tag yields no chapters, correctly.
- **ms→frames rounded to nearest:** `(ms * hz + 500) / 1000`,
  `av_rescale_q`'s convention, so spans agree with FFmpeg's within a frame.
- **Metadata harvest (FFmpeg's rule):** the chapter's embedded TIT2 is the
  row's `title`; the file's TIT2 is its `album`. `TagLibMetadataReader`
  already retitles a chaptered file's title to album, so the merge gives
  chapter rows the same shape FFmpeg's do.

## Current state in Cog

| Concern | Where | Behavior |
| --- | --- | --- |
| Chapter playback | `Plugins/FFMPEG/FFMPEGDecoder.m` (1.5) | Plays `#N` spans; harvests chapter title, files global title under album |
| Fragment refusal | `Plugins/minimp3/MP3Decoder.m` (2.0) | Refuses any fragment (being replaced in phase 3) |
| Expansion | `Plugins/FFMPEG/FFMPEGContainer.m` (1.5) | `nb_chapters > 1` → `#0..#N-1` |
| Chapter read/write | `Plugins/TagLib/TagLibChapters.mm` | Shared reader/writer (phase 1) |
| Bytes entry point | `Plugins/TagLib/TagLibID3v2Reader.mm` | `+chaptersForTag:` (phase 1) |
| Reader retitling | `Plugins/TagLib/TagLibMetadataReader.m` | Chaptered file's title → album |
| Editor integration | `TagEditor/TagSaver.swift`, `TagEditorWindowController.swift` | Chapter rows edit their file; post-save row rebuild |

## Phase 1 — the shared chapter reader (LANDED, `ec49043af`)

- `Plugins/TagLib/TagLibChapters.{h,mm}`: the chapter section extracted from
  `TagLibMetadataWriter.m`, byte-identical code motion. The header declares
  `Chapter`, `id3v2TagOf`, `readID3v2Chapters`, `readChapters`,
  `chaptersFromArray`, `writeID3v2Chapters`, `writeMatroskaChapters`,
  `writeMP4Chapters`, `chapterArray`. The private helpers (`toString`,
  `toNSString`, `readMatroskaChapters`, `readMP4Chapters`, `randomUID`) stay
  in an anonymous namespace in the `.mm`. The writer imports the header and
  dropped its chapter-only includes.
- `TagLibID3v2Reader +chaptersForTag:(NSData *)`: parses a tag block through
  `TagLib::MPEG::File file(&byteVectorStream, false)` (no audio properties)
  → `ID3v2Tag()` → `readID3v2Chapters` → `chapterArray`, returning the same
  `CogChapterStart/End/Title/ID` dictionaries the writer answers. No file
  access, so no SandboxBroker bracket is needed here.
- `Plugins/TagLib/TagLib.xcodeproj` wired; `iOS/MobileCog.xcodeproj`
  regenerated (`python3 iOS/generate_plugins_project.py`).
- Tests: `iOS/CogPluginsTests/TagLibChapterTests.swift` — agreement with the
  writer, the CTOC order rule (ordered wins, unordered/non-top-level fall to
  start sort), v2.2 yields nothing, every title encoding, truncated-frame
  robustness, empty/non-tag data.

**Verification done:** the three Objective-C++ files pass `clang
-fsyntax-only` against the real `tag.framework` headers; the Swift test
parses. **Not yet run:** the iOS simulator service is unavailable in the
authoring sandbox, so the tests have not executed. Whoever finishes must
run them (see Verification).

## Phase 2 — seek-accuracy tests (LANDED)

Promote XPCog's probe method to `iOS/CogPluginsTests` (e.g.
`MP3SeekAccuracyTests.swift`): decode sequentially from sample 0 as a
reference; for each target, `seek` and read a window, and confirm the
decoder landed on the exact sample. Targets: 0, 1, frame boundaries ±1
(1151/1152/1153), one second, two-thirds through, an arbitrary mid-file
sample, near the end, a chapter-style boundary. CBR and VBR, with and
without Xing, mono and stereo. Assert zero positional error; keep a content
tolerance for the bit-reservoir transient (0 on clean LAME fixtures).

This hardens today's decoder regardless of the rest. It needs real
LAME-encoded fixtures — see the fixture note under Verification.

### Phase 2 record

`MP3SeekAccuracyTests.swift` runs the probe on four real LAME 4.0 encodes
(1.2 s of 440 Hz sine at 44100 Hz): CBR stereo, VBR stereo, CBR mono, and a
no-Xing CBR stereo (`lame -t`). Targets 0, 1, 1151/1152/1153, 44100, 26460 (a
chapter-style boundary), two-thirds, mid+123, and near the end. For each, the
post-seek window must align with the sequential reference at offset 0 past a
two-frame reservoir transient (max abs diff < 1e-4); a one-sample error would
shift the sine's phase and fail. All pass: zero positional error.

The fixtures live in `MP3Fixtures.swift` as base64 (the `TagWriterTests` PNG
pattern), generated with `lame`/`ffmpeg` on a normal machine. `cbrStereo`
decodes to 52920 samples (the LAME tag's delay 1105 / padding 119 trim the raw
54144); `noXingStereo` keeps the raw 54144.

## Phase 3 — decoder spans + the cue guard (LANDED)

The core, in `Plugins/minimp3/MP3Decoder.{h,m}`, mirroring XPCog's
`Mp3Decoder::openSeekable`. Integration points in the current `open:`:

- **Fragment parse (top of `open:`, replacing the blanket refusal):** accept
  a decimal index on a seekable source only; refuse non-numeric fragments,
  negative indices, and streams. Parse the whole fragment (not `intValue`,
  which accepts trailing junk). Keep the refusal comment, updated to name
  the numeric acceptance and the FFmpeg fall-through.
- **The tag bytes are already read.** The `iTunSMPB` block reads the whole
  ID3v2 tag into `tagBuffer` (and frees it before `mp3dec_ex_open_cb`). Ride
  the same buffer: call `+[TagLibID3v2Reader chaptersForTag:]` (via
  `NSClassFromString`) on it **before it is freed**, and keep the chapter
  list (or the selected chapter's start/end/title) in an ivar. No second
  read. Also capture the file's TIT2 for the album field — extend the
  existing libid3tag frame loop to read `TIT2` (do not use
  `metadataForTag:`, which retitles a chaptered file's title to album).
- **Chapter setup, after the format-proving frame decode** (hz known) and
  **after the bitrate line** (bitrate keeps the whole file's numbers):
  - Validate `index < chapters.count`, else `return NO` (FFmpeg takes it).
  - `toFrames(ms) = (ms * hz + 500) / 1000`.
  - `chapterStart = min(toFrames(startMs), totalFrames)`;
    `chapterEnd = clamp(toFrames(endMs), chapterStart, totalFrames)`.
  - `chapterEnd <= chapterStart` → `return NO`.
  - Set `chapterTitle`, `fileTitle`; `totalFrames = chapterEnd - chapterStart`;
    `chapterMode = YES`.
  - `mp3dec_ex_seek(&_decoder_ex, chapterStart * channels)`; then
    `samples_filled = 0`, `seconds = 0`, `_framesDecoded = 0`. **The discarded
    format-proof frame must not leak into the stream timestamp.**
- **`seek:`** targets `(chapterStart + frame) * channels` in ex space when in
  chapter mode; `frame` stays chapter-relative.
- **`readAudio`** clamps at `chapterEnd`: stop at the span's end, truncating
  the last chunk with `setFrameCount:` (`AudioChunk` supports truncation;
  `CueSheetDecoder.m` does exactly this). `seconds`/`_framesDecoded` stay
  chapter-relative, like cue tracks.
- **`properties`** reports the span (`totalFrames` is already the ivar).
- **`metadata`** in chapter mode answers `title = chapterTitle`,
  `album = fileTitle`. Guard `updateMetadata` (the HTTP stream path) so it
  cannot clobber the chapter title.
- **Gapless is inherited, not implemented:** ex sample 0 is the first real
  sample (Xing/Info delay applied by the ex layer; `iTunSMPB` by the existing
  override of the same three ex fields). A chapter span measured in that
  space composes with both — test `iTunSMPB` + chapters explicitly.

New ivars in `MP3Decoder.h`: `chapterMode` (BOOL), `chapterStart`/`chapterEnd`
(long), `chapterTitle`/`fileTitle` (NSString *), and the chapter list or the
selected chapter's milliseconds.

### The cue guard (required companion change)

XPCog's cue plugin handles `.cue` files only and opens the inner decoder on
a fragment-less track URL. **Cog supports embedded cue sheets**, and
`CueSheetDecoder.m`'s embedded branch opens the inner decoder
(`skipCue:YES`) on the source **with the fragment still attached**
(`file.mp3#01`). Today minimp3 refuses it and FFmpeg plays the whole file.
Once minimp3 accepts numeric fragments, a file with **both an embedded cue
sheet and CHAP frames** would open chapter 1 as the inner decoder and compute
cue track bounds from the chapter span — wrong.

**Recommended fix (root cause):** in `CueSheetDecoder`'s embedded branch,
open the inner decoder on a fragment-less URL (build it from `[source url]`,
get an `AudioSource` for it, open that). This also fixes the latent FFmpeg
subsong confusion for that combination. Needs an embedded-cue regression
test. **Conservative alternative** (confined to minimp3): refuse fragments
when the parsed tag holds an embedded `cuesheet` frame — preserves today's
behavior for cue+chapter files rather than fixing it. Pick one; the
root-cause fix is preferred.

### Phase 3 tests (`iOS/CogPluginsTests/MP3ChapterTests.swift`)

A chaptered fixture's `#1` opens on its span with its title; the span is
exact (reading to the end yields precisely its frames); chapter 0 starts at
the first real sample (compare against the whole-file sequential reference —
the gapless proof); seeking within a chapter lands inside it; non-numeric
fragments, out-of-range indices and streams refuse (and fall through to
FFmpeg); removing chapters returns the file to whole-file playback;
`iTunSMPB` + chapters compose; the embedded-cue regression.

### Phase 3 record

Implemented in `MP3Decoder.{h,m}`: the numeric-fragment acceptance (the whole
fragment must be decimal digits, so `1x`/`-1` refuse and fall through), the
chapter list read from the tag block via `TagLibID3v2Reader +chaptersForTag:`
and start-sorted to match `FFMPEGContainer`'s row order, the span setup after
the bitrate line, `readAudio` clamped at the span's end, chapter-relative
`seek:`, and `setTrack:` carrying on to the next chapter. `metadata` answers
`title = chapterTitle`, `album = fileTitle`; `updateMetadata` returns early in
chapter mode so a stream cannot clobber them.

The file's TIT2 is read in the existing libid3tag frame loop. **libid3tag
stores a text frame's string as a `STRINGLIST` field (field 1), not a full
string:** `id3_field_getfullstring` and `id3_field_getstring` both return
null there; take `id3_field_getstrings(field, 0)` guarded by
`id3_field_getnstrings(field)`.

The cue guard took the root-cause fix: `CueSheetDecoder`'s embedded branch
opens the inner decoder on a fragment-less URL (built with `NSURLComponents`),
so a `file.mp3#01` cue row is not mistaken for a chapter span.

`minimp3.xcodeproj` gained `../TagLib` on `HEADER_SEARCH_PATHS` for the
`TagLibID3v2Reader.h` import (the FFMPEG plugin reaches it by a project file
reference instead).

Verified: macOS `Cog` and iOS `Cog` app builds succeed; all 57
`CogPluginsTests` pass, including `MP3ChapterTests`' span/title/album,
whole-file playback, the loader properties path, FFmpeg agreement (within a
frame), the numeric/out-of-range/non-numeric refusals with multi fall-through,
seek clamping and replay, `setTrack` contiguous/jump/other-file, and the
embedded-cue-over-chapters regression. On the real LAME fixtures it also
proves gapless chapter 0 (chapter 0's samples equal the whole file's opening
samples) and `iTunSMPB` composition: a distinct-duration `iTunSMPB` overrides
the Xing tag, and merging chapters in (`replace: false`) keeps the `iTunSMPB`
COMM frame while chapter 0 still opens on the first real sample.

## Phase 4 — agreement, docs, E2E (agreement + docs LANDED; E2E pending)

- **Agreement tests:** minimp3's chapter N == `FFMPEGContainer`'s row N ==
  the editor's chapter N, on fixtures written by both TagLib and ffmpeg.
  Same count, same order, each start and every interior span within a frame.
  **Do not assert the last chapter's tail to the frame** — it runs to the end
  of the audio, and minimp3 trims the gapless padding while FFmpeg reports
  the raw length; walk the boundaries cumulatively instead (XPCog's
  agreement tests do exactly this).
- **Docs:** note the feature in `README.md` (MP3 chapters play through
  minimp3, gapless chapter 0, free-format support); `iOS/README.md` if a
  difference emerges. Keep this file current.
- **Manual E2E** on both platforms with a real audiobook: rows expand, each
  plays its span, seeking and the waveform are chapter-relative, the tag
  editor edits the file and offers to rebuild rows.

### Record

Agreement and docs landed; the manual E2E is the one remaining step.

- `MP3ChapterTests.assertChapterAgreement(_:label:)` walks the boundaries
  cumulatively: the running sum of minimp3's spans lands at each editor
  chapter's start (within a frame), interior spans match FFmpeg's and the
  editor's `end - start` (within a frame), titles and row order agree, and
  the spans tile the whole gapless-trimmed file. The last chapter's tail is
  not asserted to the frame. Two cases drive it:
  `testTagLibWrittenChaptersAgreeAcrossReaderContainerAndDecoders` (real
  `cbrStereo` + `writeChapters`) and
  `testFFmpegWrittenChaptersAgreeAcrossReaderContainerAndDecoders`.
- The ffmpeg-written fixture (`MP3Fixtures.ffmpegChapters`, 20296 bytes) is
  `cbr_stereo.mp3` remuxed with an `FFMETADATA1` chapter list
  (`TIMEBASE=1/1000`, chapters 0/300/600/1200 ms, titles First/Second/Third)
  via `ffmpeg -map_metadata 1 -map 0:a -c:a copy -write_id3v2 1
  -id3v2_version 4`. It carries 3 `CHAP` + 1 `CTOC`, keeps the Xing/LAME tag
  (still 52920 samples), and the shared TagLib reader answers its chapters.
- `README.md` gained a "Chaptered files" bullet under *What it plays*. No
  `iOS/README.md` change: MP3 chapters use the same plugins on both
  platforms, with no exclusive-output/DoP difference to note.
- Verified: 59 `CogPluginsTests` pass (was 57, +2 agreement), 190
  `CogAudioTests` pass (9 skipped), and both the macOS `Cog` and iOS `Cog`
  app builds succeed.
- **Remaining:** the manual E2E above, on both platforms with a real
  audiobook. Nothing this phase is committed yet.

## Verification

Per `CLAUDE.md`, after shared-code changes build **both** apps and run the
plugin tests:

```sh
# iOS plugin tests (the real-plugin test home), including the new cases
xcodebuild test -project iOS/MobileCog.xcodeproj -scheme CogPlugins \
  -destination 'platform=iOS Simulator,name=<a booted iPhone>'
# macOS engine tests
xcodebuild test -project Audio/CogAudio.xcodeproj -scheme CogAudio -destination 'platform=macOS'
# both app builds
xcodebuild -project Cog.xcodeproj -scheme Cog -configuration Debug \
  -arch x86_64 -arch arm64 ONLY_ACTIVE_ARCH=NO CODE_SIGNING_ALLOWED=NO build
xcodebuild -project iOS/MobileCog.xcodeproj -scheme Cog -configuration Debug \
  -destination generic/platform=iOS CODE_SIGNING_ALLOWED=NO build
```

**Environment note:** the authoring sandbox has no working simulator service
(`CoreSimulatorService` connection refused), so the iOS tests could not run
there; phase 1 was verified by `clang -fsyntax-only` against the real
`tag.framework` headers and `swiftc -parse` only. Run the tests in a normal
environment before calling any phase done.

**Fixtures:** XPCog's tests shell out to `lame`/`ffmpeg`; iOS tests cannot.
Embed small real LAME encodes as base64 in the test Swift (the pattern
`TagWriterTests` already uses for a PNG): a second of sine, CBR 128k and VBR,
each with the Xing/Info header that carries the gapless numbers the
chapter-0 and seek-accuracy cases lean on. Create them with `lame`/`ffmpeg`
on a normal machine and embed the bytes. `TagWriterTests.writeMP3()`'s
silent header-only frames are fine for the order/encoding tests but carry no
Xing, so they cannot prove gapless chapter 0.

## Risks and notes

- **Last-chapter tail:** minimp3 trims gapless padding, FFmpeg reports raw
  length — the final span differs by up to ~1152 samples. Agreement asserts
  boundaries cumulatively, not the last tail to the frame.
- **Corrupt files:** the reservoir fill breaks early at unreadable side info;
  position stays exact (index arithmetic), content may transient. Tests carry
  a content tolerance; do not assert bit-exactness on non-LAME fixtures.
- **Rounding:** nearest ms→frame matches `av_rescale_q`; assert agreement
  within one frame, not equality.
- **The opening frame:** chapter setup seeks after the format-proving read;
  the discarded frame must not leak into `samples_filled` or the stream
  timestamp.
- **`iTunSMPB` + chapters together:** an iTunes-ripped audiobook exercises
  both overrides of the ex fields; the span is in the same post-padding
  space, so it composes — test it explicitly.
- **Priorities:** minimp3's decoder is 2.0; FFmpeg's decoder and container
  are 1.5; `CueSheetContainer`/`CueSheetDecoder` are 16.0. The fall-through
  and the cue guard both depend on this order.
- **VBRI:** `CVbriHeader.h` is imported but never called; minimp3-ex scans
  instead of using VBRI tables. Chapters still work; not a blocker.