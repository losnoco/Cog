
README
======


OpenMPT and libopenmpt
======================

This repository contains OpenMPT, a free Windows/Wine-based
[tracker](https://en.wikipedia.org/wiki/Music_tracker) and libopenmpt,
a library to render tracker music (MOD, XM, S3M, IT MPTM and dozens of other
legacy formats) to a PCM audio stream. libopenmpt is directly based on OpenMPT,
offering the same playback quality and format support, and development of the
two happens in parallel.


License
-------

The OpenMPT/libopenmpt project is distributed under the *BSD-3-Clause* License.
See [LICENSE](LICENSE) for the full license text.

Files below the `include/` (external projects) and `contrib/` (related assets
not directly considered integral part of the project) folders may be subject to
other licenses. See the respective subfolders for license information. These
folders are not distributed in all source packages, and in particular they are
not distributed in the Autotools packages.


How to compile
--------------


### OpenMPT

 -  Supported Visual Studio versions:

     -  Visual Studio 2017 and 2019 Community/Professional/Enterprise

        To compile the project, open `build/vsVERSIONwin7/OpenMPT.sln` (VERSION
        being 2017 or 2019) and hit the compile button. Other target systems can
        be found in the `vs2017*` and `vs2019*` sibling folders.

 -  OpenMPT requires the compile host system to be 64bit x86-64.

 -  The Windows 8.1 SDK is required to build OpenMPT with Visual Studio 2017
    (this is included with Visual Studio, however may need to be selected
    explicitly during setup).

 -  Microsoft Foundation Classes (MFC) are required to build OpenMPT.


### libopenmpt and openmpt123

For detailed requirements, see `libopenmpt/dox/quickstart.md`.

 -  Autotools

    Grab a `libopenmpt-VERSION-autotools.tar.gz` tarball.

        ./configure
        make
        make check
        sudo make install

    Cross-compilation is generally supported (although only tested for
    targetting MinGW-w64).

    Note that some MinGW-w64 distributions come with the `win32` threading model
    enabled by default instead of the `posix` threading model. The `win32`
    threading model lacks proper support for C++11 `<thread>` and `<mutex>` as
    well as thread-safe magic statics. It is recommended to use the `posix`
    threading model for libopenmpt for this reason. On Debian, the appropriate
    configure command is
    `./configure --host=x86_64-w64-mingw32 CC=x86_64-w64-mingw32-gcc-posix CXX=x86_64-w64-mingw32-g++-posix`
    for 64bit, or
    `./configure --host=i686-w64-mingw32 CC=i686-w64-mingw32-gcc-posix CXX=i686-w64-mingw32-g++-posix`
    for 32bit. Other MinGW-w64 distributions may differ.

 -  Visual Studio:

     -  You will find solutions for Visual Studio 2017 and 2019 in the
        corresponding `build/vsVERSIONwin7/` folder.
        Projects that target Windows 10 Desktop (including ARM and ARM64) are
        available in `build/vsVERSIONwin10/` (Visual Studio 2017 requires SDK
        version 10.0.16299).
        Minimal projects that target Windows 10 UWP are available in
        `build/vsVERSIONuwp/`.
        Most projects are supported with any of the mentioned Visual Studio
        verions, with the following exceptions:

         -  in_openmpt: Requires Visual Studio with MFC.

         -  xmp-openmpt: Requires Visual Studio with MFC.

     -  libopenmpt requires the compile host system to be 64bit x86-64 when
        building with Visual Studio.

     -  You will need the Winamp 5 SDK and the XMPlay SDK if you want to
        compile the plugins for these 2 players. They can be downloaded
        automatically on Windows 7 or later by just running the
        `build/download_externals.cmd` script.

        If you do not want to or cannot use this script, you may follow these
        manual steps instead:

         -  Winamp 5 SDK:

            To build libopenmpt as a winamp input plugin, copy the contents of
            `WA5.55_SDK.exe` to include/winamp/.

            Please visit
            [winamp.com](http://wiki.winamp.com/wiki/Plug-in_Developer) to
            download the SDK.
            You can disable in_openmpt in the solution configuration.

         -  XMPlay SDK:

            To build libopenmpt with XMPlay input plugin support, copy the
            contents of xmp-sdk.zip into include/xmplay/.

            Please visit [un4seen.com](https://www.un4seen.com/xmplay.html) to
            download the SDK.
            You can disable xmp-openmpt in the solution configuration.

 -  Makefile

    The makefile supports different build environments and targets via the
    `CONFIG=` parameter directly to the make invocation.
    Use `make CONFIG=$newconfig clean` when switching between different configs
    because the makefile cleans only intermediates and target that are active
    for the current config and no configuration state is kept around across
    invocations.

     -  mingw-w64:

        The required compiler version is at least GCC 7.

            make CONFIG=mingw64-win32    # for win32

            make CONFIG=mingw64-win64    # for win64

     -  gcc or clang (on Unix-like systems, including Mac OS X with MacPorts,
        and Haiku (32-bit Hybrid and 64-bit)):

        The minimum required compiler versions are:

         -  gcc 7

         -  clang 5

        The Makefile requires pkg-config for native builds.
        For sound output in openmpt123, PortAudio or SDL is required.
        openmpt123 can optionally use libflac and libsndfile to render PCM
        files to disk.

        When using gcc, run:

            make CONFIG=gcc

        When using clang, it is recommended to do:

            make CONFIG=clang

        Otherwise, simply run

            make

        which will try to guess the compiler based on your operating system.

     -  emscripten (on Unix-like systems):

        Run:

            # generates WebAssembly with JavaScript fallback
            make CONFIG=emscripten EMSCRIPTEN_TARGET=all

        or

            # generates WebAssembly
            make CONFIG=emscripten EMSCRIPTEN_TARGET=wasm

        or

            # generates JavaScript with compatibility for older VMs
            make CONFIG=emscripten EMSCRIPTEN_TARGET=js

        Running the test suite on the command line is also supported by using
        node.js. Depending on how your distribution calls the `node.js` binary,
        you might have to edit `build/make/config-emscripten.mk`.

     -  DJGPP / DOS

        Cross-compilation from Linux systems is supported with DJGPP GCC 7.2 or
        later via

            make CONFIG=djgpp

        `openmpt123` can use liballegro 4.2 for sound output on DJGPP/DOS.
        liballegro can either be installed system-wide in the DJGPP environment
        or downloaded into the `libopenmpt` source tree.

            make CONFIG=djgpp USE_ALLEGRO42=1    # use installed liballegro

        or

            ./build/download_externals.sh    # download liballegro binaries
            make CONFIG=djgpp USE_ALLEGRO42=1 BUNDLED_ALLEGRO42=1

     -  American Fuzzy Lop:

        To compile libopenmpt with fuzzing instrumentation for afl-fuzz, run:
        
            make CONFIG=afl
        
        For more detailed instructions, read `contrib/fuzzing/readme.md`.

     -  other compilers:

        To compile libopenmpt with other C++17 compliant compilers, run:
        
            make CONFIG=generic
        
    
    The `Makefile` supports some customizations. You might want to read the top
    which should get you some possible make settings, like e.g.
    `make DYNLINK=0` or similar. Cross compiling or different compiler would
    best be implemented via new `config-*.mk` files.

    The `Makefile` also supports building doxygen documentation by using

        make doc

    Binaries and documentation can be installed systen-wide with

        make PREFIX=/yourprefix install
        make PREFIX=/yourprefix install-doc

    Some systems (i.e. Linux) require running

        sudo ldconfig

    in order for the system linker to be able to pick up newly installed
    libraries.

    `PREFIX` defaults to `/usr/local`. A `DESTDIR=` parameter is also
    supported.

 -  Android NDK

    See `build/android_ndk/README.AndroidNDK.txt`.



Contributing to OpenMPT/libopenmpt
----------------------------------


See [contributing](doc/contributing.md).

