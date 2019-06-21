How to add support for new module formats
=========================================

This document describes the basics of writing a new module loader and related
work that has to be done. We will not discuss in detail how to write the loader,
have a look at existing loaders to get an idea how they work in general.

General hints
-------------
* We strive for quality over quantity. The goal is not to support as many module
  formats as possible, but to support them as well as possible. 
* Write defensive code. Guard against out-of-bound values, division by zero and
  similar stuff. libopenmpt is constantly fuzz-tested to catch any crashes, but
  of course we want our code to be reliable from the start.
* Every format should have its own `MODTYPE` flag, unless it can be reasonably
  represented as a subset of another format (like Ice Tracker ICE files being
  a subset of ProTracker MOD).
* When reading binary structs from the file, use our data types with defined
  endianness, which can be found in `common/Endianness.h`:
  * Big-Endian: (u)int8/16/32/64be, float32be, float64be
  * Little-Endian: (u)int8/16/32/64le, float32le, float64le

  Entire structs containing integers with defined endianness can be read in one
  go if they are tagged with `MPT_BINARY_STRUCT` (see existing loaders for an
  example).
* `m_nChannels` **MUST NOT** be changed after a pattern has been created, as
  existing patterns will be interpreted incorrectly. For module formats that
  support per-pattern channel amounts, the maximum number of channels must be
  determined beforehand.
* Strings can be safely handled using:
  * `FileReader::ReadString` and friends for reading them directly from a file
  * `mpt::String::Read` for reading them from a struct or char array,
  * `mpt::String::Copy` for copying between char arrays or `std::string`.

  "Read" functions take care of string padding (zero / space padding), so those
  should be used when extracting strings from files. "Copy" should only be used
  on strings that have previously been read using the "Read" functions.
  If the target is a char array rather than a `std::string`, these will take
  care of properly null-terminating the target char array, and prevent reading
  past the end of a (supposedly null-terminated) source char array.
* Do not use non-const static variables in your loader. Loaders need to be
  thread-safe for libopenmpt.
* `FileReader` instances may be used to treat a portion of another file as its
  own independent file (through `FileReader::ReadChunk`). This can be useful
  with "embedded files" such as WAV or Ogg samples. Container formats are
  another good example for this usage.
* Samples *either* use middle-C frequency *or* finetune + transpose. For the few
  weird formats that use both, it may make sense to translate everything into
  middle-C frequency.
* Add the new `MODTYPE` to `CSoundFile::UseFinetuneAndTranspose` if applicable,
  and see if any effect handlers in `soundlib/Snd_fx.cpp` need to know the new
  `MODTYPE`.
* Do not rely on hard-coded magic numbers. For example, when comparing if an
  index is valid for a given array, do not hard-code the array size but rather
  use `mpt::size` or, for ensuring that char arrays are null-terminated,
  `mpt::String::SetNullTerminator`. Similarly, do not assume any specific
  quantities for OpenMPT's constants like MAX_SAMPLES, MAX_PATTERN_ROWS, etc.
  These may change at any time.
* Pay attention to off-by-one errors when comparing against MAX_SAMPLES and
  MAX_INSTRUMENTS, since sample and instrument numbers are 1-based. 
* Placement of the loader function in `CSoundFile::Create` depends on various
  factors. In general, module formats that have very bad magic numbers (and thus
  might cause other formats to get mis-interpreted) should be placed at the
  bottom of the list. Two notable examples are 669 files, where the first two
  bytes of the file are "if" (which may e.g. cause a song title starting with
  "if ..." in various other formats to be interpreted as a 669 module), and of
  course Ultimate SoundTracker modules, which have no magic bytes at all.
* Avoid use of functions tagged with MPT_DEPRECATED.

Probing
-------
libopenmpt provides fast probing functions that can be used by library users
to quickly check if a file is most likely playable with libopenmpt, even if only
a fraction of the file is available (e.g. when streaming from the internet).

In order to satisfy these requirements, probing functions should do as little
work as possible (e.g. only parse the header of the file), but as much as
required to tell with some certainty that the file is really of a certain mod
format. However, probing functions should not rely on having access to more than
the first `CSoundFile::ProbeRecommendedSize` bytes of the file.

* Probing functions **must not** allocate any memory on the heap.
* Probing functions **must not** return ProbeFailure or ProbeWantMoreData for
  any file that would normally be accepted by the loader. In particular, this
  means that any header checks must not be any more aggressive than they would
  be in the real loader (hence it is a good idea to not copy-paste this code but
  rather put it in a separate function), and the minimum additional size passed
  to `CSoundFile::ProbeAdditionalSize` must not be higher than the biggest size
  that would cause a hard failure (i.e. returning `false`) in the module loader.
* Probing functions **may** return ProbeSuccess for files that would be rejected
  by a loader after a more thorough inspection. For example, probing functions
  do not need to verify that all required chunks of an IFF-like file format are
  actually present, if the header makes it obvious enough that the file is
  highly likely to be a module.

Adding loader to the build systems and various other locations
--------------------------------------------------------------
Apart from writing the module loader itself, there are a couple of other places
that need to be updated:
* Add loader file to `build/android_ndk/Android.mk`.
* Add loader file to `build/autotools/Makefile.am`.
* Run `build/regenerate_vs_projects.sh` / `build/regenerate_vs_projects.cmd`
  (depending on your platform)
* Add file extension to `installer/filetypes.iss` (in four places).
* Add file extension to `CTrackApp::OpenModulesDialog` in `mptrack/Mptrack.cpp`.
* Add format information to `soundlib/Tables.cpp`.
