libopenmpt fuzz suite
=====================

In this directory, you can find the necessary tools for fuzzing libopenmpt with
the American Fuzzy Lop fuzzer (afl-fuzz).

Contents:

* `all_formats.dict`: A dictionary containing magic bytes from all supported
  module formats to make the life of the fuzzer a bit easier.
* `fuzz-master.sh`: Script to launch the main fuzzing process. If you want to
  use just one fuzzer instance, run this one.
* `fuzz-slave.sh`: Script to launch the secondary fuzzing process. It is
  recommended to run at least two fuzzer instances, as the deterministic and
  random fuzz mode have been found to complement each other really well.
* `fuzz-settings.sh`: Set up your preferences and afl settings here before the
  first run.
* `fuzz.c`: A tiny C program that is used by the fuzzer to test libopenmpt.
* `get-afl.sh`: A simple script to obtain the latest version of the fuzzer.

Prerequisites
=============
* afl from http://lcamtuf.coredump.cx/afl/ - the makefile expects this to be
  installed in `contrib/fuzzing/afl`, as it is automatically done by the
  `get-afl.sh` install script.
* Clang with LLVM dev headers (llvm-config needs to be installed).
  afl also works with gcc, but our makefile has been set up to make use of afl's
  faster LLVM mode.

How to use
==========
* Run `get-afl.sh`, or manually extract afl to `contrib/fuzzing/afl`, use `make`
  to build afl-fuzz, `cd llvm_mode`, `make` to build afl-clang-fast.
  If building with either option fails because `llvm-config` cannot be found,
  try prepending `LLVM_CONFIG=/usr/bin/llvm-config-3.8` or similar, and read the
  afl manual.
* Build libopenmpt with the `build.sh` script in this directory.
* Set up `fuzz-settings.sh` to your taste. Most importantly, you will have to
  specify the input directory for first use.
  The default setup mounts a tmpfs folder for all temporary files. You may
  change this behaviour if you do not have root privileges.
* Run `fuzz-master.sh` for the first (deterministic) instance of afl-fuzz.
* For a "slave" instance to run on another core, run `fuzz-slave.sh`.
* If you want to make use of even more cores, make a copy of `fuzz-slave.sh`
  and adjust "infile02" / "fuzzer02" to "infile03" / "fuzzer03" (they need to be
  unique)