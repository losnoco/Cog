
Changelog {#changelog}
=========

For fully detailed change log, please see the source repository directly. This
is just a high-level summary.

### libopenmpt 0.6.4 (2022-06-12)

 *  [**Bug**] openmpt123: Linking failed when using Autotools build system
    targeting MinGW.
 *  [**Bug**] tests: Linking failed when using Autotools build system targeting
    MinGW.
 *  [**Bug**] examples: Linking failed when using Autotools build system
    targeting MinGW.

 *  [**Change**] Support for mingw-std-threads when building for MinGW targets
    is now deprecated because this is not supported for GCC 11 or later (see
    <https://github.com/meganz/mingw-std-threads/issues/79>).

 *  Ping-pong sample loops sometimes stopped playing at the end of the loop.

### libopenmpt 0.6.3 (2022-04-24)

 *  Pitch / Pan Separation and Random Variation instrument properties were not
    resetting properly when seeking, potentially causing instruments to be
    played e.g. at a vastly different pan position compared to playing the
    module continuously.
 *  MED: Stereo samples were not imported correctly.

 *  zlib: Update to v1.2.12 (2022-03-27).

### libopenmpt 0.6.2 (2022-03-13)

 *  [**Sec**] Possible out-of-bounds write in malformed IT / XM / MPTM files
    using the internal LFO plugin. (r17076)
 *  [**Sec**] Possible out-of-bounds read when using Amiga BLEP interpolation
    with extremely high-pitched notes. (r17078, r17079)

 *  ISO-8859-1-related charsets from Amiga OS and RISC OS are now handled more
    accurately, thus avoiding some unwanted control characters.
 *  MO3: Pattern indices 254 / 255 were not treated as playable patterns even if
    the original file was a MOD / XM.
 *  Correctly apply ST3-style effect memory when seeking in S3M files.
 *  Command S (S3M / IT style) effect memory was not applied when seeking.
 *  Initial channel mute status was not reported correctly in `get_channel_mute_status`
    since libopenmpt 0.6.0.

 *  FLAC: Update to v1.3.4 (2022-02-21).
 *  pugixml: Update to v1.12.1 (2022-02-16).

### libopenmpt 0.6.1 (2022-01-30)

 *  [**Bug**] Linking libmpg123 no longer fails on OpenBSD.
 *  [**Bug**] Possible hang with malformed DMF, DSM, MED, MUS, OKT and SymMOD
    files containing 65536 or more patterns when destroying the module.
 *  [**Bug**] Avoid NaNs and infinite values with custom tunings and in the
    I3DL2Reverb plugin.

 *  MIDI macros are now evaluated when seeking.
 *  The letter "z" is now evaluated in fixed MIDI macros (Z80...ZFF) the same
    way as in Impulse Tracker.
 *  MOD: Loosened VBlank timing heuristics so that "frame of mind" by Dascon
    plays correctly.
 *  MOD: Validate the contents of "hidden" patterns beyond the end of the order
    list when the file size matches the expected size when only taken "official"
    patterns into account. This fixes Shofixti Ditty.mod from Star Control 2
    while keeping other (partly broken) modules working.
 *  MED: Command 20 (reverse sample) is now only applied when it's next to a
    note.
 *  S3M: Introducing the "Send OPL key-off when triggering notes" compatibility
    setting broke retrigger for OPL notes again (they retriggered rather than
    not retriggering).
 *  S3M: Retriggering a note no longer resets its pitch after a portamento.
 *  S3M: Partially implement retrigger behaviour for stopped notes in
    SoundBlaster mode: Like in IT, it is not possible to retrigger a sample that
    has already stopped playing.
 *  DIGI: Improve compatibility with E3x reverse sample command.
 *  DSym: Tempos < 32 were treated as tempo slides.
 *  SymMOD: Key-off command was not implemented properly.

### libopenmpt 0.6.0 (2021-12-23)

 *  [**New**] `MUS` files from Psycho Pinball and Micro Machines 2 are now
    supported.
 *  [**New**] `SymMOD` files created with Symphonie / Symphonie Pro are now
    supported.
 *  [**New**] `FMT` files created with Davey W Taylor's FM Tracker are now
    supported.
 *  [**New**] `DSYM` files created with Digital Symphony are now supported.
 *  [**New**] `STX` files (transitional format between Scream Tracker 2 and 3)
    are now supported.
 *  [**New**] TakeTracker MODs with `TDZ1` to `TDZ3` magic bytes are now
    supported.
 *  [**New**] openmpt123: openmpt123 will now expand file wildcards passed on
    the command line in Windows when built with MSVC.
 *  [**New**] libopenmpt_ext: New interface `interactive2` adding
    `openmpt::ext::interactive2::note_off()`,
    `openmpt::ext::interactive2::note_fade()`,
    `openmpt::ext::interactive2::set_channel_panning()`,
    `openmpt::ext::interactive2::get_channel_panning()`,
    `openmpt::ext::interactive2::set_note_finetune()`, and
    `openmpt::ext::interactive2::get_note_finetune()` (C++) and
    `openmpt_module_ext_interface_interactive2.note_off()`,
    `openmpt_module_ext_interface_interactive2.note_fade()`,
    `openmpt_module_ext_interface_interactive2.set_channel_panning()`,
    `openmpt_module_ext_interface_interactive2.get_channel_panning()`,
    `openmpt_module_ext_interface_interactive2.set_note_finetune()`, and
    `openmpt_module_ext_interface_interactive2.get_note_finetune()` (C). 
 *  [**New**] `Makefile` `CONFIG=emscripten` now supports
    `EMSCRIPTEN_TARGET=audioworkletprocessor` which builds an ES6 module in
    a single file with reduced dependencies suitable to be used in an
    AudioWorkletProcessor.
 *  [**New**] `Makefile` `CONFIG=emscripten` now supports `EMSCRIPTEN_PORTS=1`
    which uses dependencies (zlib, mp123, ogg, and vorbis) from Emscripten Ports
    instead of using miniz, minimp3, and stb_vorbis locally or building zlib,
    mp123, ogg, and vorbis locally.
 *  [**New**] `Makefile` `CONFIG=emscripten` and `CONFIG=djgpp` can now build
    zlib, mpg123, and vorbis locally instead of only supporting miniz, minimp3,
    and stb_vorbis via `ALLOW_LGPL=1`.

 *  [**Change**] `Makefile` `CONFIG=emscripten` now supports
    `EMSCRIPTEN_TARGET=all` which provides WebAssembly as well as fallback to
    JavaScript in a single build.
 *  [**Change**] openmpt123: DOS builds now use the Mercury fork of
    `liballegro 4.2` for improved hardware compatibility.
 *  [**Change**] libopenmpt no longer generates internal interpolation tables on
    library load time, but instead only on first module load time.

 *  [**Regression**] `Makefile` `CONFIG=emscripten` does not support
    `EMSCRIPTEN_TARGET=asmjs` or `EMSCRIPTEN_TARGET=asmjs128m` any more because
    support has been removed from current Emscripten versions.
 *  [**Regression**] Support for GCC 7 has been removed.
 *  [**Regression**] Support for Clang 5, 6 has been removed.
 *  [**Regression**] Support for Emscripten versions older than 1.39.7 has been
    removed.
 *  [**Regression**] Building with Android NDK older than NDK r19c is not
    supported any more.

 *  libopenmpt can now detect infinite pattern loops and treats them as the song
    end. This means that setting a repeat count other than -1 now always
    guarantees that playback will eventually end. The song loop counter is
    decremented each time it ends up at the start of the infinite loop, so the
    song does not restart from the beginning even if the repeat count is not 0.
 *  `openmpt::module::set_position_seconds()` accuracy has been improved for
    modules with pattern loops.
 *  Samples played at the wrong volume when rendering modules in mono.
 *  IT: Portamentos in files with Linear Slides disabled are now more accurate.
 *  IT: Pitch/Pan Separation was affected by note-off commands, and wasn't reset
    by panning commands like in Impulse Tracker.
 *  IT: Even after libopenmpt 0.5.14 the filter reset logic was still not 100%
    identical to Impulse Tracker: A note triggered on tick 0 of a row with a
    Pattern Delay effect still caused the filter to be reset on repetitions of
    that row even though the note wasn't retriggered.
 *  IT: Added read-only support for BeRoTracker commands 1 and 2 (equivalent to
    XM commands K and L).
 *  XM: BeRoTracker saves smooth MIDI macros in a different way from OpenMPT.
    This command is now imported correctly.
 *  XM: Emulate FT2 Tone Portamento quirk that inverts portamento direction
    after the target was reached (if target note was higher than previous note).
 *  S3M files saved with Impulse Tracker and latest Schism Tracker now also
    compute sample playback speed in Hertz.
 *  Depending on whether an S3M file was last saved in Scream Tracker with the
    Sound Blaster or Gravis Ultrasound drivers loaded, different compatibility
    flags are now applied. For files saved with the GUS, the sample volume
    factor is now also ignored (fixes volume levels in S3Ms made on the GUS, in
    particular if they use both samples and OPL instruments).
 *  S3M: Enforce the lower frequency bound.
 *  MOD: Loosened VBlank timing heuristics so that the original copy of
    Guitar Slinger from Dizzy Tunes II plays correctly.
 *  FAR: Correct portamento depth is now used.
 *  DMF / IMF: Improved accuracy of finetune commands.
 *  MDL: Implemented finetune command.
 *  OKT: Various accuracy improvements such as: Sharing volume between mixed
    channels, volume commands on mixed channels are permanent (not reset with
    new notes), mixed channels do not support default sample volume, 7-bit
    samples are actually supposed to be played as-is (not amplified to full
    8-bit range), reject speed command parameters >= 20.

 *  zlib: v1.2.11 (2017-01-15).
 *  mpg123: v1.29.3 (2021-12-11).
 *  ogg: v1.3.5 (2021-06-04).
 *  vorbis: v1.3.7 (2020-07-04).
 *  miniz: v2.2.0 (2021-06-27).
 *  minimp3: commit 50d2aaf360a53653b718fead8e258d654c3a7e41 (2021-11-27).
 *  stb_vorbis: v1.22 commit 5a0bb8b1c1b1ca3f4e2485f4114c1c8ea021b781
    (2021-07-12).
 *  FLAC: v1.3.3 (2019-08-04).
 *  PortAudio: v19.7.0 (2021-04-06).

### libopenmpt 0.5.0 (2020-05-24)

 *  [**New**] OggMod compressed FastTracker 2 XM (OXM) modules are now
    supported.
 *  [**New**] The emulated Amiga type when Amiga resampler emulation is enabled
    can now be selected via ctl `render.resampler.emulate_amiga_type`. Possible
    values are: `"auto"`, `"a500"`, `"a1200"`, and an experimental option
    `"unfiltered"`.
 *  [**New**] libopenmpt: New API `openmpt::module::get_current_estimated_bpm()`
    (C++), and `openmpt_module_get_current_estimated_bpm()` (C) which provides
    accurate beats per minute information for module formats with time signature
    and an educated guess based on speed and tempo for others.
 *  [**New**] libopenmpt: New type-aware ctl APIs that do not require memory
    allocations and are thus realtime-safe:
    `openmpt::module::ctl_get_boolean()`, `openmpt::module::ctl_get_integer()`,
    `openmpt::module::ctl_get_floatingpoint()`,
    `openmpt::module::ctl_get_text()`, `openmpt::module::ctl_set_boolean()`,
    `openmpt::module::ctl_set_integer()`,
    `openmpt::module::ctl_set_floatingpoint()` (C++), and
    `openmpt_module_ctl_get_boolean()`, `openmpt_module_ctl_get_integer()`,
    `openmpt_module_ctl_get_floatingpoint()`, `openmpt_module_ctl_get_text()`,
    `openmpt_module_ctl_set_boolean()`, `openmpt_module_ctl_set_integer()`,
    `openmpt_module_ctl_set_floatingpoint()` (C).
 *  [**New**] libopenmpt C++ New API `openmpt::is_extension_supported2()` which
    takes a `std::string_view` parameter instead of `std::string`.
 *  [**New**] libopenmpt C++: New API
    `openmpt::module::module(std::vector<std::byte> data)`,
    `openmpt::module::module(const std::byte * data, std::size_t size)`,
    `openmpt::module::module(const std::byte * beg, const std::byte * end)`.
 *  [**New**] libopenmpt C++: New API
    `openmpt::probe_file_header(flags, const std::byte * data, std::size_t size, filesize)`,
    `openmpt::probe_file_header(flags, const std::byte * data, std::size_t size)`.
 *  [**New**] libopenmpt_ext C++: New API
    `openmpt::module_ext::module_ext(std::vector<std::byte> data)`,
    `openmpt::module_ext::module_ext(const std::byte * data, std::size_t size)`,
    `openmpt::module_ext::module_ext(std::vector<std::uint8_t> data)`,
    `openmpt::module_ext::module_ext(const std::uint8_t * data, std::size_t size)`.

 *  [**Change**] std::istream based file I/O has been speed up.
 *  [**Change**] Dependency on iconv on Linux has been removed.

 *  [**Regression**] libmodplug: The libmodplug emulation layer has been removed
    from the libopenmpt tree. Please use the separate `libopenmpt-modplug`
    package instead.
 *  [**Regression**] foo_openmpt: foo_openmpt is discontinued. Please use
    Kode54's fork foo_openmpt54:
    <https://www.foobar2000.org/components/view/foo_openmpt54>.
 *  [**Regression**] Support for building with C++11 or C++14 has been removed.
    C++17 is now required to build libopenmpt.
 *  [**Regression**] Support for client code using C++11 or C++ 14 has been
    removed. C++17 is now required to build libopenmpt client applications.
 *  [**Regression**] Support for Visual Studio 2015 has been removed.
 *  [**Regression**] Support for GCC 4.8, 4.9, 5, 6 has been removed.
 *  [**Regression**] Support for Clang 3.6, 3.7, 3.8, 3.9, 4 has been removed.
 *  [**Regression**] Support for Emscripten versions older than 1.39.1 has been
    removed.
 *  [**Regression**] Building with Android NDK older than NDK r18b is not
    supported any more.
 *  [**Regression**] openmpt123: Support for SDL1 (but not SDL2) output has been
    removed.
 *  [**Regression**] openmpt123: Support for SDL2 older than 2.0.4 has been
    removed.
 *  [**Regression**] Windows XP and Windows Vista are no longer supported.
 *  [**Regression**] It is no longer possible to optionally use iconv for
    character set conversions.

 *  [**Bug**] openmpt123: openmpt123 now honors the current locale and outputs
    text appropriately.
 *  [**Bug**] openmpt123: Piping text output to other than console window
    targets on Windows has been fixed.

 *  Greatly improved MED import. Synthesized instruments are still not supported
    but support was added for: Multisampled instruments, delta samples, more
    pattern commands, Hold and Decay, multiple songs and many other small
    changes.
 *  Improved OPL channel allocation when more than 18 notes are active, so that
    channels that have completely faded out are prioritized over channels that
    have already been released but have not faded out yet.
 *  Interactively triggering an OPL instrument could cause the first pattern
    channel to no longer be played back correctly.
 *  Fix some inaccuracies in OPL emulator.
 *  Fix overflow of OPL amplification happening at a synth volume level of 510.
 *  End-of-sample pop reduction of surround channels was applied to front
    channels instead, causing a pop on the front channels instead of removing it
    on the back channels.
 *  IT: Disable retrigger with short notes quirk for modules saved with
    Chibi Tracker, as it does not implement that quirk.
 *  IT: Instrument and sample panning should not override channel panning for
    following notes.
 *  IT: SBx is now prioritized over Bxx commands that are to the left of it.
 *  IT: Duplicate Check Type "Sample" should only be applied if the instruments
    match, too.
 *  IT: Duplicate Check Type "Note" should compare pattern notes, but it was
    comparing the new pattern note against the old translated note.
 *  IT: Various fixes for envelope resetting.
 *  IT / S3M: When combining SBx and EEx effects, don't skip the first row of
    the loop like in FastTracker 2.
 *  S3M: Empty pattern commands now affect effect memory as well.
 *  S3M: Offset beyond loop end wraps around to loop start like in
    Scream Tracker 3 + GUS (previously it just keep playing from the loop start,
    which is neither what GUS nor Sound Blaster drivers do).
 *  S3M: Notes cannot be retriggered after they have been cut.
 *  S3M: Fix portamento after note cut (fixes antediluvian_song.s3m).
 *  S3M / MOD: Previous note offset is no longer used for retriggered notes if
    there was no instrument number next to the Qxy effect.
 *  MOD: Sample swapping now also works if the sample that is being swapped from
    does not loop. Swapping to a non-looped sample now stops playback once the
    swapped-from sample reaches its (loop) end.
 *  MOD: Fix early song ending due to ProTracker pattern jump quirk
    (EEx + Dxx on same row) if infinite looping is disabled.
    Fixes Haunted Tracks.mod by Triace.
 *  MOD: Previous note offset is no longer used for retriggered notes if there
    was no instrument number next to the E9x effect.
 *  MOD: Vibrato type "ramp down" was upside down.
 *  XM: If a file contains patterns longer than 1024 rows, they are now clamped
    to 1024 rows instead of 64 rows.
 *  XM: Do not reset note-off status on portamento if there is no instrument
    number.

 *  mpg123: v1.26rc3.
 *  ogg: v1.3.4.
 *  vorbis: v1.3.6.
 *  zlib: v1.2.11.
 *  minimp3: commit 55da78cbeea5fb6757f8df672567714e1e8ca3e9 (2020-03-04).
 *  stb_vorbis: v1.19 commit 37b9b20fdec06c75a0493e0bb59e2d0f288bfb51
    (2020-02-05).
 *  miniz: v2.1.0.
 *  FLAC: v1.3.3.
 *  PortAudio: commit 799a6834a58592eadc5712cba73b35956dc51579 (2020-03-26).

### libopenmpt 0.4.0 (2018-12-23)

 *  [**New**] libopenmpt now includes emulation of the OPL chip and thus plays
    OPL instruments in S3M, C67 and MPTM files. OPL chip emulation volume can be
    changed with the new ctl `render.opl.volume_factor`.
 *  [**New**] libopenmpt now supports CDFM / Composer 670 module files.
 *  [**New**] Autotools `configure` and plain `Makefile` now honor the variable
    `CXXSTDLIB_PCLIBSPRIVATE` which serves the sole purpose of listing the
    standard library (or libraries) required for static linking. The contents of
    this variable will be put in `libopenmpt.pc` `Libs.private` and used for
    nothing else. See \ref libopenmpt_c_staticlinking .
 *  [**New**] foo_openmpt: foo_openmpt now also works on Windows XP.
 *  [**New**] libopenmpt Emscripten builds now ship with MP3 support by
    default, based on minimp3 by Lion (github.com/lieff).
 *  [**New**] libopenmpt: New ctl `play.at_end` can be used to change what
    happens when the song end is reached:
     *  "fadeout": Fades the module out for a short while. Subsequent reads
        after the fadeout will return 0 rendered frames. This is the default and
        identical to the behaviour in previous libopenmpt versions. 
     *  "continue": Returns 0 rendered frames when the song end is reached.
        Subsequent reads will continue playing from the song start or loop
        start. This can be used for custom loop logic, such as loop
        auto-detection and longer fadeouts.
     *  "stop": Returns 0 rendered frames when the song end is reached.
        Subsequent reads will return 0 rendered frames.
 *  [**New**] Add new metadata fields `"originaltype"` and `"originaltype_long"`
    which allow more clearly reflecting what is going on with converted formats
    like MO3 and GDM.
 *  [**New**] `Makefile` `CONFIG=emscripten` now can generate WebAssembly via
    the additional option `EMSCRIPTEN_TARGET=wasm`.
 *  [**New**] Compiling for DOS is now experimentally supported via DJGPP GCC
    7.2 or later.

 *  [**Change**] minimp3: Instead of the LGPL-2.1-licensed minimp3 by KeyJ,
    libopenmpt now uses the CC0-1.0-licensed minimp3 by Lion (github.com/lieff)
    as a fallback if libmpg123 is unavailable. The `USE_MINIMP3` `Makefile`
    option is gone and minimp3 will be used automatically in the `Makefile`
    build system if libmpg123 is not available.
 *  [**Change**] openmpt123: openmpt123 now rejects `--output-type` in `--ui`
    and `--batch` modes and also rejects `--output` in `--render` mode. These
    combinations of options really made no sense and were rather confusing.
 *  [**Change**] Android NDK build system now uses libc++ (`c++_shared`) instead
    of GNU libstdc++ (`gnustl_shared`), as recommended by Android NDK r16b.
 *  [**Change**] xmp-openmpt: `openmpt-mpg123.dll` is no longer optional and
    must be placed into the same directory as `xmp-openmpt.dll`.
 *  [**Change**] in_openmpt: `openmpt-mpg123.dll` is no longer optional and must
    be placed either into the directory of the player itself or into the same
    directory as `in_openmpt.dll`. This is dependent on how the player loads its
    plugins. For WinAMP 5, `openmpt-mpg123.dll` needs to be in the directory
    which contains `winamp.exe`. `in_openmpt.dll` needs to be in the `Plugins`
    directory.
 *  [**Change**] foo_openmpt: foo_openmpt is now packaged as a fb2k-component
    package for easier installation.
 *  [**Change**] When building libopenmpt with MinGW-w64, it is now recommended
    to use the posix thread model (as opposed to the win32 threading model),
    because the former does support std::mutex while the latter does not. When
    building with win32 threading model with the Autotools build system, it is
    recommended to provide the `mingw-std-threads` package. Building libopenmpt
    with MinGW-w64 without any `std::thread`/`std::mutex` support is deprecated
    and support for such configurations will be removed in libopenmpt 0.5.
 *  [**Change**] `Makefile` `CONFIG=emscripten` now has 4 `EMSCRIPTEN_TARGET=`
    settings: `wasm` generates WebAssembly, `asmjs128m` generates asm.js with a
    fixed size 128MB heap, `asmjs` generates asm.js with a fixed default size
    heap (as of Emscripten 1.38.11, this amounts to 16MB), `js` generates
    JavaScript with dynamic heap growth and with compatibility for older VMs.
 *  [**Change**] libmodplug: Update public headers to libmodplug 0.8.8.5. This
    adds support for kind-of automatic MODPLUG_EXPORT decoration on Windows.

 *  [**Regression**] Support for Clang 3.4, 3.5 has been removed.
 *  [**Regression**] Building with Android NDK older than NDK r16b is not
    supported any more.
 *  [**Regression**] Support for Emscripten versions older than 1.38.5 has been
    removed.
 *  [**Regression**] Support for libmpg123 older than 1.14.0 has been removed.
 *  [**Regression**] Using MediaFoundation to decode MP3 samples is no longer
    supported. Use libmpg123 or minimp3 instead.
 *  [**Regression**] libmodplug: Support for emulating libmodplug 0.8.7 API/ABI
    has been removed.

 *  [**Bug**] xmp-openmpt: Sample rate and number of output channels were not
    applied correctly when using per-file settings.
 *  [**Bug**] Internal mixer state was not initialized properly when initially
    rendering in 44100kHz stereo format.
 *  [**Bug**] openmpt123: Prevent libsdl2 and libsdl from being enabled at the
    same time because they conflict with each other.
 *  [**Bug**] libmodplug: Setting `SNDMIX_NORESAMPLING` in the C++ API always
    resulted in linear interpolation instead of nearest neighbour

 *  IT: In Compatible Gxx mode, allow sample changes next to a tone portamento
    effect if a previous sample has already stopped playing.
 *  IT: Fix broken volume envelopes with negative values as found in breakdwn.it
    by Elysis.
 *  MOD: Slides and delayed notes are executed on every repetition of a row with
    row delay (fixes "ode to protracker").
 *  XM: If the sustain point of the panning envelope is reached before key-off,
    it is never released.
 *  XM: Do not default recall volume / panning for delayed instrument-less notes
 *  XM :E60 loop bug was not considered in song length calucation.
 *  S3M: Notes without instrument number use previous note's sample offset.
 *  Tighten M15 and MOD file rejection heuristics.
 *  J2B: Ignore frequency limits from file header. Fixes Medivo.j2b, broken
    since libopenmpt-0.2.6401-beta17.
 *  STM: More accurate tempo calculation.
 *  STM: Better support for early format revisions (no such files have been
    found in the wild, though).
 *  STM: Last character of sample name was missing.
 *  SFX: Work around bad conversions of the "Operation Stealth" soundtrack by
    turning pattern breaks into note stops.
 *  IMF: Filter cutoff was upside down and the cutoff range was too small.
 *  ParamEq plugin center frequency was not limited correctly.
 *  Keep track of active SFx macro during seeking.
 *  The "note cut" duplicate note action did not volume-ramp the previously
    playing sample.
 *  A song starting with non-existing patterns could not be played.
 *  DSM: Support restart position and 16-bit samples.
 *  DTM: Import global volume.
 *  MOD: Support notes in octave 2, like in FastTracker 2 (fixes DOPE.MOD).
 *  Do not apply Amiga playback heuristics to MOD files that have clearly been
    written with a PC tracker.
 *  MPTM: More logical release node behaviour.
 *  Subsong search is now less thorough. It could previously find many subsongs
    that are technically correct (unplayed rows at the beginning of patterns
    that have been jumped over due to pattern breaks), but so far no real-world
    module that would require such a thorough subsong detection was found. The
    old mechanism caused way more false positives than intended with real-world
    modules, though.
 *  Restrict the unpacked size of compressed DMF, IT, MDL and MO3 samples to
    avoid huge allocations with malformed small files.

### libopenmpt 0.3 (2017-09-27)

 *  [**New**] New error handling functionality in the C API, which in particular
    allows distinguishing potentially transient out-of-memory errors from parse
    errors during module loading. 
 *  [**New**] New API `openmpt::module::get_selected_subsong()` (C++) and
    `openmpt_module_get_selected_subsong()` (C).
 *  [**New**] Faster file header probing API `openmpt::probe_file_header()` and
    `openmpt::probe_file_header_get_recommended_size` (C++), and
    `openmpt_probe_file_header()`,
    `openmpt_probe_file_header_without_filesize()`,
    `openmpt_probe_file_header_from_stream()` and
    `openmpt_probe_file_header_get_recommended_size()` (C).
 *  [**New**] New API `openmpt::could_open_probability()` (C++) and
    `openmpt_could_open_probability()` (C). This fixes a spelling error in the
    old 0.2 API.
 *  [**New**] openmpt123: openmpt123 can now open M3U, M3U8, M3UEXT, M3U8EXT and
    PLSv2 playlists via the `--playlist` option.
 *  [**New**] openmpt123: openmpt123 now supports very fast file header probing
    via the `--probe` option.
 *  [**New**] Libopenmpt now supports building for Windows 10 Universal (Windows
    Store 8.2) APIs with MSVC, and also for the older Windows Runtime APIs with
    MinGW-w64.
 *  [**New**] New API header `libopenmpt_ext.h` which implements the libopenmpt
    extension APIs also for the C interface.
 *  [**New**] The Reverb effect (S99 in S3M/IT/MPTM, and X99 in XM) is now
    implemented in libopenmpt.
 *  [**New**] For Amiga modules, a new resampler based on the Amiga's sound
    characteristics has been added. It can be activated by passing the
    `render.resampler.emulate_amiga` ctl with a value of `1`. Non-Amiga modules
    are not affected by this, and setting the ctl overrides the resampler choice
    specified by `OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH` or
    `openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH`. Support for the MOD
    command E0x (Set LED Filter) is also available when the Amiga resampler is
    enabled. 

 *  [**Change**] libopenmpt versioning changed and follows the more conventional
    major.minor.patch as well as the recommendations of the
    [SemVer](http://semver.org/) scheme now. In addition to the SemVer
    requirements, pre-1.0.0 versions will also honor API and ABI stability in
    libopenmpt (i.e. libopenmpt ignores SemVer Clause 4). 
 *  [**Change**] The output directories of the MSVC build system were changed to
    `bin/vs2015-shared/x86-64-win7/` (and similar) layout which allows building
    in the same tree with different compiler versions without overwriting other
    outputs.
 *  [**Change**] The emscripten build now exports libopenmpt as 'libopenmpt'
    instead of the default 'Module'.
 *  [**Change**] Android: The build system changed. The various Android.mk files
    have been merged into a single one which can be controlled using command
    line options.
 *  [**Change**] The `Makefile` build system now passes `std=c++11` to the
    compiler by default. Older compilers may still work if you pass
    `STDCXX=c++0x` to the `make` invocation.
 *  [**Change**] The `Makefile` option `ANCIENT=1` is gone.
 *  [**Change**] The optional dependencies on `libltdl` or `libdl` are gone.
    They are no longer needed for any functionality.

 *  [**Regression**] Compiling client code using the C++ API now requires a
    compiler running in C++11 mode.
 *  [**Regression**] Support for GCC 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7 has been
    removed.
 *  [**Regression**] Support for Clang 3.0, 3.1, 3.2, 3.3 has been removed.
 *  [**Regression**] Support for Emscripten versions older than 1.31.0 has been
    removed.
 *  [**Regression**] Support for Android NDK versions older than 11 has been
    removed.
 *  [**Regression**] Visual Studio 2008, 2010, 2012, 2013 support has been
    removed.
 *  [**Regression**] Dynamic run-time loading of libmpg123 is no longer
    supported. Libmpg123 must be linked at link-time now.
 *  [**Regression**] xmp-openmpt: xmp-openmpt now requires XMPlay 3.8 or later
    and compiling xmp-openmpt requires an appropriate XMPlay SDK with
    `XMPIN_FACE` >= `4`.
 *  [**Regression**] Support for libmpg123 older than 1.13.0 has been removed.
 *  [**Regression**] Un4seen unmo3 support has been removed.

 *  [**Bug**] C++ API: `openmpt::exception` did not define copy and move
    constructors or copy and move assignment operators in libopenmpt 0.2. The
    compiler-generated ones were not adequate though. libopenmpt 0.3 adds the
    appropriate special member functions. This adds the respective symbol names
    to the exported ABI, which, depending on the compiler, might or might not
    have been there in libopenmpt 0.2. The possibly resulting possible ODR
    violation only affects cases that did crash in the libopenmpt 0.2 API anyway
    due to memory double-free, and does not cause any further problems in
    practice for all known platforms and compilers.
 *  [**Bug**] The C API could crash instead of failing gracefully in
    out-of-memory situations.
 *  [**Bug**] The test suite could fail on MacOSX or FreeBSD in non-fatal ways
    when no locale was active.
 *  [**Bug**] `libopenmpt_stream_callbacks_fd.h` and
    `libopenmpt_stream_callbacks_file.h` were missing in Windows development
    packages.
 *  [**Bug**] libopenmpt on Windows did not properly guard against current
    working directory DLL injection attacks.
 *  [**Bug**] localtime() was used to determine the version of Schism Tracker
    used to save IT and S3M files. This function is not guaranteed to be
    thread-safe by the standard and is now no longer used.
 *  [**Bug**] Possible crashes with malformed IT, ITP, AMS, MDL, MED, MPTM, PSM
    and Startrekker files.
 *  [**Bug**] Possible hangs with malformed DBM, MPTM and PSM files.
 *  [**Bug**] Possible hangs with malformed files containing cyclic plugin
    routings.
 *  [**Bug**] Excessive loading times with malformed ITP / truncated AMS files.
 *  [**Bug**] Plugins did not work correctly when changing the sample rate
    between two render calls.
 *  [**Bug**] Possible NULL-pointer dereference read during obscure
    out-of-memory situations while handling exceptions in the C API.
 *  [**Bug**] libmodplug: `libmodplug.pc` was wrong.
 *  [**Bug**] Cross-compiling libopenmpt with autotools for Windows now properly
    sets `-municode` and `-mconsole` as well as all required Windows system
    libraries.
 *  [**Bug**] foo_openmpt: Interpolation filter and volume ramping settings were
    confused in previous versions. This version resets both to the defaults.
 *  [**Bug**] libmodplug: The CSoundFile::Read function in the emulated
    libmodplug C++ API returned the wrong value, causing qmmp (and possibly
    other software) to crash.

 *  Support for SoundTracker Pro II (STP) and Digital Tracker (DTM) modules.
 *  Increased accuracy of the sample position and sample rate to drift less when
    playing very long samples.
 *  Various playback improvements for IT and XM files.
 *  Channel frequency could wrap around after some excessive portamento / down
    in some formats since libopenmpt 0.2-beta17.
 *  Playback improvements for S3M files made with Impulse Tracker and
    Schism Tracker.
 *  ParamEq plugin emulation didn't do anything at full gain (+15dB).
 *  All standard DMO effects are now also emulated on non-Windows and non-MSVC
    systems.
 *  Added `libopenmpt_stream_callbacks_buffer.h` which adds
    `openmpt_stream_callbacks` support for in-memory buffers, possibly even only
    using a truncated prefix view into a bigger file which is useful for
    probing.
 *  Avoid enabling some ProTracker-specific quirks for MOD files most likely
    created with ScreamTracker 3.
 *  Tremolo effect only had half the intended strength in MOD files.
 *  Pattern loops ending on the last row a pattern were not executed correctly
    in S3M files.
 *  Work-around for reading MIDI macros and plugin settings in some malformed IT
    files written by old UNMO3 versions.
 *  Improve tracker detection in IT format.
 *  Playback fixes for 8-channel MED files
 *  Do not set note volume to 0 on out-of-range offset in XM files.
 *  Better import of some slide commands in SFX files.
 *  Sample 15 in "Crew Generation" by Necros requires short loops at the
    beginning of the sample to not be ignored. Since we need to ignore them in
    some (non-ProTracker) modules, we heuristically disable the old loop
    sanitization behaviour based on the module channel count.
 *  Both normal and percentage offset in PLM files were handled as percentage
    offset.
 *  MT2 files with instruments that had both sample and plugin assignments were
    not read correctly.
 *  Some valid FAR files were rejected erroneously.
 *  Support for VBlank timing flag and comment field in PT36 files.
 *  Improved accuracy of vibrato command in DIGI / DBM files.
 *  STM: Add support for "WUZAMOD!" magic bytes and allow some slightly
    malformed STM files to load which were previously rejected.
 *  Detect whether "hidden" patterns in the order list of SoundTracker modules
    should be taken into account or not.
 *  Tighten heuristics for rejecting invalid 669, M15, MOD and ICE files and
    loosen them in other places to allow some valid MOD files to load.
 *  Improvements to seeking: Channel panning was not always updated from
    instruments / samples when seeking, and out-of-range global volume was not
    applied correctly in some formats.
 *  seek.sync_samples=1 did not apply PTM reverse offset effect and the volume
    slide part of combined volume slide + vibrato commands.
 *  If the order list was longer than 256 items and there was a pattern break
    effect without a position jump on the last pattern of the sequence, it did
    not jump to the correct restart order.
 *  `Makefile` has now explicit support for FreeBSD with no special option or
    configuration required.
 *  openmpt123: Improved section layout in man page.
 *  libmodplug: Added all missing C++ API symbols that are accessible via the
    public libmodplug header file.
 *  Autotools build system now has options `--disable-openmpt123`,
    `--disable-tests` and `--disable-examples` which may be desireable when
    cross-compiling.
 *  Windows binary packages now ship with libmpg123 included.

### libopenmpt 0.2-beta20 (2016-08-07)

 *  [**Bug**] PSM loader was broken on big-endian platforms since forever.
 *  [**Bug**] `load.skip_samples` ctl did not work for PSM16 modules.

 *  There is a new `subsong` ctl, which can return the currently selected
    subsong.
 *  More accurate ProTracker arpeggio wrap-around emulation.
 *  More accurate sample tuning in PSM16 files.
 *  Samples in DSM files were sometimes detuned and some pattern commands were
    not imported correctly.
 *  More accurate import of MDL 7-bit panning command.
 *  Only import pattern commands supported by the UltraTracker version that was
    used to save ULT files. Add support for command 5-C (end loop).
 *  DMF sample loop lengths were off by one.
 *  Unis 669 pan slide effect was too deep.
 *  Several valid (but slightly corrupted possibly due to disk failures or data
    transfer errors) SoundTracker files were no longer loading since libopenmpt
    0.2-beta18.

### libopenmpt 0.2-beta19 (2016-07-23)

 *  [**Change**] libopenmpt now uses C++14 `[[deprecated]]` attribute instead
    of compiler-specific solutions when appropriate.
 *  [**Change**] libopenmpt C++ header now uses C++11 `noexcept` instead of
    C++98 `throw()` exception specification when supported. `throw()` is
    deprecated since C++11. This does not change API or ABI as they are
    equivalent. Use `LIBOPENMPT_ASSUME_CPLUSPLUS_NOEXCEPT` to override the
    default.
 *  [**Change**] The preprocessor macro `LIBOPENMPT_ANCIENT_COMPILER_STDINT` is
    gone. Please use `LIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT instead`.
    Additionally, the typedefs moved from illegal namespace ::std into somewhat
    less dangerous namespace ::openmpt::std. You can test
    `#ifdef LIBOPENMPT_QUIRK_NO_CSTDINT` client-side to check whether
    `libopenmpt.hpp` used the non-standard types. (Note: Of all supported
    compilers, this change only affects the 3 compilers with only limited
    support: MSVC 2008, GCC 4.1, GCC 4.2.)

 *  [**Bug**] xmp-openmpt: Crash when viewing sample texts.

 *  The public libopenmpt C++ header has auto-detection logic for the used C++
    standard now. In case your client code compiler misreports the standard
    version or you want to override it for other reasons,
    `#define LIBOPENMPT_ASSUME_CPLUSPLUS` to the value of the standard version
    you desire to be used. There is also a macro for each individual aspect,
    like `LIBOPENMPT_ASSUME_CPLUSPLUS_CSTDINT`,
    `LIBOPENMPT_ASSUME_CPLUSPLUS_DEPRECATED`,
    `LIBOPENMPT_ASSUME_CPLUSPLUS_NOEXCEPT` which take precedence over the
    general macro.
 *  Portamento with sample swap behaviour was wrong for ProTracker MODs.
 *  Rewritten loader and various playback fixes for MDL files.
 *  libopenmpt 0.2-beta18 broke import of many pattern commands in DBM, DMF and
    ULT files.
 *  ADPCM samples in MOD files were broken since libopenmpt 0.2-beta17.

### libopenmpt 0.2-beta18 (2016-07-11)

 *  [**Change**] openmpt123: Add PulseAudio output support. Autotools and
    `Makefile` build systems now depend on `libpulse` and `libpulse-simple` by
    default. Disable with `--without-pulseaudio` or `NO_PULSEAUDIO=1`
    respectively. When enabled, PulseAudio will be the default output driver,
 *  [**Change**] xmp-openmpt: Settings are now stored in xmplay.ini like with
    every other plugin.

 *  [**Regression**] openmpt123: Support for FLAC < 1.3.0 has been removed. FLAC
    before 1.3.0 is broken beyond repair as it provides `assert.h` in the
    include path.

 *  [**Bug**] Generated pkg-config file libopenmpt.pc by both `Makefile` and
    Autotools build systems was totally broken.
 *  [**Bug**] libopenmpt no longer uses the non-thread-safe global std::rand()
    function.
 *  [**Bug**] Sample loops in GDM modules did not work when using Emscripten.
 *  [**Bug**] XM and MO3 loaders could crash due to unaligned memory accesses.
 *  [**Bug**] Fixed incorrect handling of custom MPTM tunings on big endian
    platforms.
 *  [**Bug**] Fixed various problems found with clang 3.8 static analyzer,
    address sanitizer and undefined behaviour sanitizer.
 *  [**Bug**] File header probing functionality was broken for most formats.
 *  [**Bug**] With non-seekable streams, the entire file was almost always
    cached even if it was not of any supported module type.

 *  Seeking in allsubsongs-mode now works correctly.
 *  openmpt123: Added subsong support.
 *  Various playback fixes for 669, IT, MT2 and MTM files.
 *  Some MOD files with more than 128 patterns (e.g. NIETNU.MOD) were not loaded
    correctly.
 *  A new example `libopenmpt_example_c_probe` has been added which demonstrates
    the usage and flexibility of openmpt_could_open_propability() in the C API
    under various constraints.

### libopenmpt 0.2-beta17 (2016-05-21)

 *  [**Change**] The Makefile and Autotools build systems now require to
    explicitly specify `NO_LTDL=1` or `--without-ltdl` respectively if no
    support for dynamic loading of third party libraries via libtool libltdl is
    desired.
 *  [**Change**] In the Makefile build system option `USE_MO3` and the Autotools
    build system option `--enable-mo3` are gone. Dynamic loading of un4seen
    unmo3 is now always enabled when dynamic loading is possible and built-in
    MO3 support is not possible because either a MP3 or a Vorbis decoder is
    missing.
 *  [**Change**] The MSVC build system changed. The `libopenmptDLL` project is
    gone. Use the new `ReleaseShared` configuration of the `libopenmpt` project
    instead. libopenmpt now links against zlib by default. A separate project
    with smaller footprint linking against miniz is still available as
    `libopenmpt-small`.
 *  [**Change**] The constants used to query library information from
    `openmpt_get_string()` and `openmpt::string::get()` (i.e. OPENMPT_STRING_FOO
    and openmpt::string::FOO) have been deprecated because having syntactic
    constants for theses keys makes extending the API in a backwards and
    forwards compatible way harder than it should be. Please just use the string
    literals directly.
 *  [**Change**] Deprecated API identifiers will now cause deprecation warnings
    with MSVC, GCC and clang. `#define LIBOPENMPT_NO_DEPRECATE` to disable the
    warnings.
 *  [**Change**] openmpt123: `--[no-]shuffle` option has been renamed to
    `--[no-]randomize`. A new `--[no-]shuffle` option has been added which
    shuffles randomly through the playlist as opposed to randomizing the
    playlist upfront.
 *  [**Change**] Support for Un4seen unmo3 has generally been deprecated in
    favour of the new internal mo3 decoder. Un4seen unmo3 support will be
    removed on 2018-01-01.

 *  [**Bug**] Memory consumption during loading has been reduced by about 1/3 in
    case a seekable input stream is provided (either via C API callback open
    functions or via C++ API iostream constructors).
 *  [**Bug**] Some samples in AMS modules were detuned when using Emscripten.
 *  [**Bug**] Possible crash with excessive portamento down in some formats.
 *  [**Bug**] Possible crashes with malformed AMF, AMS, DBM, IT, MDL, MED, MPTM,
    MT2, PSM and MMCMP-, XPK- and PP20-compressed files.
 *  [**Bug**] `openmpt::module::format_pattern_row_channel` with `width == 0`
    was returning an empty string instead of an string with unconstrained
    length.

 *  Support for ProTracker 3.6 IFF-style modules and SoundFX / MultiMedia Sound
    (SFX / MMS) modules.
 *  libopenmpt now has support for DMO plugins on Windows when built with MSVC.
    Additionally, the DMO Compression, Distortion, Echo, Gargle, ParamEQ and
    WavesReverb DSPs are emulated on on all other platforms.
 *  libopenmpt now supports the DigiBooster Echo DSP.
 *  To avoid any of the aforementioned plugins to be used, the load.skip_plugins
    ctl can be passed when loading a module.
 *  libopenmpt got native MO3 support with MP3 decoding either via libmpg123 or
    MediaFoundation (on Windows 7 and up) and Vorbis decoding via libogg,
    libvorbis, libvorbisfile or stb_vorbis.
 *  libopenmpt MSVC builds with Visual Studio 2010 or later on Windows 7 or
    later now use an internal MO3 decoder with libogg, libvorbis, libvorbisfile,
    and libmpg123 or minimp3 or MediaFoundation suppport by default. Visual
    Studio 2008 builds still use unmo3.dll by default but also support the
    built-in decoder in which case libmpg123 is required.
 *  libopenmpt with Makefile or Autotools build system can now also use
    glibc/libdl instead of libtool/libltdl for dynamic loading of third-party
    libraries. Options `NO_DL=1` and `--without-dl` have been added
    respectively.
 *  The `Makefile` build system got 4 new options NO_MPG123, NO_OGG, NO_VORBIS,
    NO_VORBISFILE. The default is to use the new dependencies automatically.
 *  The `Autotools` build system got 4 new options --without-mpg123,
    --without-ogg, --without-vorbis, --without-vorbisfile. The default is to use
    the new dependencies automatically.
 *  Makefile and Android builds got support for using minimp3 instead of
    libmpg123. For Android, use `Android-minimp3-stbvorbis.mk`, for Makefile use
    `USE_MINIMP3=1`. You have to download
    [minimp3](http://keyj.emphy.de/minimp3/) yourself and put its contents into
    `include/minimp3/`.
 *  `"source_url"`, `"source_date"` and `"build_compiler"` keys have been added
    to `openmpt_string_get()` and `openmpt::string::get()`.
 *  openmpt123: Add new `--[no-]restart]` option which restarts the playlist
    when finished.
 *  Improved Ultimate SoundTracker version detection heuristics.
 *  Playing a sample at a sample rate close to the mix rate could lead to small
    clicks when using vibrato.
 *  More fine-grained internal legacy module compatibility settings to correctly
    play back modules made with older versions of OpenMPT and a few other
    trackers.
 *  The tail of compressed MDL samples was slightly off.
 *  Some probably hex-edited XM files (e.g. cybernostra weekend.xm) were not
    loaded correctly.
 *  Countless other playback fixes for MOD, XM, S3M, IT and MT2 files.

### libopenmpt 0.2-beta16 (2015-11-22)

 *  [**Change**] The Autotools build system does strict checking of all
    dependencies now. Instead of best effort auto-magic detection of all
    potentially optional dependencies, the default set of dependencies is now
    enforced unless each individual dependency gets explicitely disabled via
    `--without-foo` or `--disable-foo` `./configure` switches. Run
    `./configure --help` for the full list of options.

 *  [**Bug**] Some MOD files were erroneously detected as 669 files.
 *  [**Bug**] Some malformed AMF files could result in very long loading times.
 *  [**Bug**] Fixed crashes in IMF and MT2 loaders.
 *  [**Bug**] MTM files generated by UNMO3 were not loaded properly.
 
 *  Improved MTM playback.
 *  `make CONFIG=haiku` for Haiku has been added.
 *  Language bindings for FreeBASIC have been added (see
    `libopenmpt/bindings/`).

### libopenmpt 0.2-beta15 (2015-10-31)

 *  [**Change**] openmpt123: SDL2 is now supported and preferred to SDL1 if
    available with the `Makefile` build system.

 *  [**Bug**] Emscripten support for older emscripten versions broke in -beta14.
    These are now supported again when using `make CONFIG=emscripten-old`.
 *  [**Bug**] Fixed crashes in MED loader.

 *  Playback improvements and loader fixes for MOD, MT2 and MED.

### libopenmpt 0.2-beta14 (2015-09-13)

 *  [**Change**] The C++ API example now uses the PortAudio C++ bindings
    instead of the C API.
 *  [**Change**] Default compiler options for Emscripten have been changed to
    more closely match the Emscripten recommendations.

 *  [**Bug**] Client code compilation with C89 compilers was broken in beta13.
 *  [**Bug**] Test suite failed on certain Emscripten/node.js combinations.
 *  [**Bug**] Fixed various crashes or hangs in DMF, OKT, PLM, IT and MPTM
    loaders.

 *  Implemented error handling in the libopenmpt API examples.
 *  Various playback improvements and fixes for OKT, IT and MOD.

### libopenmpt 0.2-beta13 (2015-08-16)

 *  [**Change**] The MSVC build system has been redone. Solutions are now
    located in `build/vsVERSION/`.

 *  [**Bug**] get_current_channel_vu_left and get_current_channel_vu_right only
    return the volume of the front left and right channels now.
    get_current_channel_vu_rear_left and get_current_channel_vu_rear_right
    do now actually work and return non-zero values.
 *  [**Bug**] Fix crashes and hangs in MED and MDL loaders and with some
    truncated compressed IT samples.
 *  [**Bug**] Fix crash when playing extremely high-pitched samples.

 *  Completed C and C++ documentation
 *  Added new key for openmpt::module::get_metadata, "message_raw", which
    returns an empty string if there is no song message rather than a list of
    instrument names.
 *  in_openmpt: Support for compiling with VS2008.
 *  xmp-openmpt: Support for compiling with VS2008.
 *  in_openmpt: Add a more readable file information window.

### libopenmpt 0.2-beta12 (2015-04-19)

 *  Playback fix when row delay effect is used together with offset command.
 *  A couple of fixes for the seek.sync_samples=1 case.
 *  IT compatibility fix for IT note delay.
 *  ProTracker MOD playback compatibility improvement.

### libopenmpt 0.2-beta11 (2015-04-18)

 *  [**Change**] openmpt_stream_seek_func() now gets called with
    OPENMPT_STREAM_SEEK_SET, OPENMPT_STREAM_SEEK_CUR and
    OPENMPT_STREAM_SEEK_END whence parameter instead of SEEK_SET, SEEK_CUR and
    SEEK_END. These are defined to 0, 1 and 2 respectively which corresponds to
    the definition in all common C libraries. If your C library uses different
    constants, this theoretically breaks binary compatibility. The old
    libopenmpt code, however, never actually called the seek function, thus,
    there will be no problem in practice.
 *  [**Change**] openmpt123: When both SDL1.2 and PortAudio are available,
    SDL is now the preferred backend because SDL is more widespread and better
    tested on all kinds of different platforms, and in general, SDL is just
    more reliable.

 *  [**Bug**] libopenmpt now also compiles with GCC 4.3.

 *  libopenmpt now supports PLM (Disorder Tracker 2) files.
 *  Various playback improvements and fixes for IT, S3M, XM, MOD, PTM and 669
    files.

### libopenmpt 0.2-beta10 (2015-02-17)

 *  [**Change**] Makefile configuration filenames changed from
    `build/make/Makefile.config.*` to `build/make/config-*.mk`.
 *  [**Change**] libopenmpt for Android now supports unmo3 from un4seen. See
    `build/android_ndk/README.AndroidNDK.txt` for details.

 *  [**Bug**] Fix out-of-bounds read in mixer code for ProTracker-compatible
    MOD files which was introduced back in r4223 / beta6.

 *  Vibrato effect was too weak in beta8 and beta9 in IT linear slide mode.
 *  Very small fine portamento was wrong in beta8 and beta9 in IT linear slide
    mode.
 *  Tiny IT playback compatibility improvements.
 *  STM playback improvements.

### libopenmpt 0.2-beta9 (2014-12-21)

 *  [**Bug**] libopenmpt_ext.hpp was missing from the Windows binary zip files.

### libopenmpt 0.2-beta8 (2014-12-21)

 *  [**Change**] foo_openmpt: Settings are now accessible via foobar2000
    advanced settings.
 *  [**Change**] Autotools based build now supports libunmo3. Specify
    --enable-unmo3.
 *  [**Change**] Support for dynamic loading of libunmo3 on MacOS X.
 *  [**Change**] libopenmpt now uses libltld (from libtool) for dynamic loading
    of libunmo3 on all non-Windows platforms.
 *  [**Change**] Support for older compilers:
     *  GCC 4.1.x to 4.3.x (use `make ANCIENT=1`)
     *  Microsoft Visual Studio 2008 (with latest Service Pack)
        (see `build/vs2008`)
 *  [**Change**] libopenmpt_ext.hpp is now distributed by default. The API is
    still considered experimental and not guaranteed to stay API or ABI
    compatible.
 *  [**Change**] xmp-openmpt / in_openmpt: No more libopenmpt_settings.dll.
    The settings dialog now uses a statically linked copy of MFC.

 *  [**Bug**] The -autotools tarballs were not working at all.

 *  Vastly improved MT2 loader.
 *  Improved S3M playback compatibility.
 *  Added openmpt::ext::interactive, an extension which adds a whole bunch of
    new functionality to change playback in some way or another.
 *  Added possibility to sync sample playback when using
    openmpt::module::set_position_* by setting the ctl value
    seek.sync_samples=1  
 *  Support for "hidden" subsongs has been added.
    They are accessible through the same interface as ordinary subsongs, i.e.
    use openmpt::module::select_subsong to switch between any kind of subsongs.
 *  All subsongs can now be played consecutively by passing -1 as the subsong
    index in openmpt::module::select_subsong.
 *  Added documentation for a couple of more functions.

### libopenmpt 0.2-beta7 (2014-09-07)

 *  [**Change**] libopenmpt now has an GNU Autotools based build system (in
    addition to all previously supported ways of building libopenmpt).
    Autotools support is packaged separately as tarballs ending in
    `-autotools.tar.gz`.

 *  [**Bug**] The distributed windows .zip file did not include pugixml.

 *  [**Regression**] openmpt123: Support for writing WavPack (.wv) files has
    been removed.
    
    Reasoning:
     1. WavPack support was incomplete and did not include support for writing
        WavPack metadata at all.
     2. openmpt123 already supports libSndFile which can be used to write
        uncompressed lossless WAV files which can then be encoded to whatever
        format the user desires with other tools.

### libopenmpt 0.2-beta6 (2014-09-06)

 *  [**Change**] openmpt123: SDL is now also used by default if availble, in
    addition to PortAudio.
 *  [**Change**] Support for emscripten is no longer experimental.
 *  [**Change**] libopenmpt itself can now also be compiled with VS2008.

 *  [**Bug**] Fix all known crashes on platforms that do not support unaligned
    memory access.
 *  [**Bug**] openmpt123: Effect column was always missing in pattern display.

### libopenmpt 0.2-beta5 (2014-06-15)

 *  [**Change**] Add unmo3 support for non-Windows builds.
 *  [**Change**] Namespace all internal functions in order to allow statically
    linking against libopenmpt without risking duplicate symbols.
 *  [**Change**] Iconv is now completely optional and only used on Linux
    systems by default.
 *  [**Change**] Added libopenmpt_example_c_stdout.c, an example without
    requiring PortAudio.
 *  [**Change**] Add experimental support for building libopenmpt with
    emscripten.

 *  [**Bug**] Fix ping-pong loop behaviour which broke in 0.2-beta3.
 *  [**Bug**] Fix crashes when accessing invalid patterns through libopenmpt
    API.
 *  [**Bug**] Makefile: Support building with missing optional dependencies
    without them being stated explicitely.
 *  [**Bug**] openmpt123: Crash when quitting while playback is stopped.
 *  [**Bug**] openmpt123: Crash when writing output to a file in interactive UI
    mode.
 *  [**Bug**] openmpt123: Wrong FLAC output filename in --render mode.

 *  Various smaller playback accuracy improvements.

### libopenmpt 0.2-beta4 (2014-02-25)

 *  [**Bug**] Makefile: Dependency tracking for the test suite did not work.

### libopenmpt 0.2-beta3 (2014-02-21)

 *  [**Change**] The test suite is now built by default with Makefile based
    builds. Use `TEST=0` to skip building the tests. `make check` runs the test
    suite.

 *  [**Bug**] Crash in MOD and XM loaders on architectures not supporting
    unaligned memory access.
 *  [**Bug**] MMCMP, PP20 and XPK unpackers should now work on non-x86 hardware
    and implement proper bounds checking.
 *  [**Bug**] openmpt_module_get_num_samples() returned the wrong value.
 *  [**Bug**] in_openmpt: DSP plugins did not work properly.
 *  [**Bug**] in_openmpt/xmp-openmpt: Setting name for stereo separation was
    misspelled. This version will revert your stereo separation settings to
    default.
 *  [**Bug**] Crash when loading some corrupted modules with stereo samples.

 *  Support building on Android NDK.
 *  Avoid clicks in sample loops when using interpolation.
 *  IT filters are now done in integer instead of floating point. This improves
    performance, especially on architectures with no or a slow FPU.
 *  MOD pattern break handling fixes.
 *  Various XM playback improvements.
 *  Improved and switchable dithering when using 16bit integer API.

### libopenmpt 0.2-beta2 (2014-01-12)

 *  [**Bug**] MT2 loader crash.
 *  [**Bug**] Saving settings in in_openmpt and xmp-openmpt did not work.
 *  [**Bug**] Load libopenmpt_settings.dll also from below Plugins directory in
    Winamp.

 *  DBM playback improvements.

### libopenmpt 0.2-beta1 (2013-12-31)

 *  First release.

