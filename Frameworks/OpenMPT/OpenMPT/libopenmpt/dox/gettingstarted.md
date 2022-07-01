
Getting Started {#gettingstarted}
===============


How to compile
--------------


### libopenmpt and openmpt123

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

     -  You will find solutions for Visual Studio in the matching
        `build/vsVERSIONwinWINDOWSVERSION/` folder.
        Minimal projects that target Windows 10 UWP are available in
        `build/vsVERSIONuwp/`.
        Most projects are supported with any of the mentioned Visual Studio
        verions, with the following exceptions:

         -  in_openmpt: Requires Visual Studio with MFC.

         -  xmp-openmpt: Requires Visual Studio with MFC.

     -  libopenmpt requires the compile host system to be amd64 or ARM64 when
        building with Visual Studio.

     -  In order to build libopenmpt for Windows XP, the Visual Studio 2017 XP 
        targetting toolset as well as the Windows 8.1 SDK need to be installed.
        The SDK is optionally included with Visual Studio 2017, but must be
        separately installed with later Visual Studio versions.

        The Windows 8.1 SDK is available from
        <https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/>
        or directly from
        <https://download.microsoft.com/download/B/0/C/B0C80BA3-8AD6-4958-810B-6882485230B5/standalonesdk/sdksetup.exe>
        .

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

     -  native build:

        Simply run

            make

        which will try to guess the compiler based on your operating system.

     -  gcc or clang (on Unix-like systems, including Mac OS X with MacPorts,
        and Haiku (32-bit Hybrid and 64-bit)):

        The Makefile requires pkg-config for native builds.
        For sound output in openmpt123, PortAudio or SDL is required.
        openmpt123 can optionally use libflac and libsndfile to render PCM
        files to disk.

        When you want to use gcc, run:

            make CONFIG=gcc

        When you want to use clang, it is recommended to do:

            make CONFIG=clang

     -  mingw-w64:

            make CONFIG=mingw64-win32    # for win32

            make CONFIG=mingw64-win64    # for win64

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

        Cross-compilation from Linux systems is supported with DJGPP GCC via

            make CONFIG=djgpp

        `openmpt123` can use liballegro 4.2 for sound output on DJGPP/DOS.
        liballegro can either be installed system-wide in the DJGPP environment
        or downloaded into the `libopenmpt` source tree.

            make CONFIG=djgpp USE_ALLEGRO42=1    # use installed liballegro

        or

            ./build/download_externals.sh    # download liballegro source
            make CONFIG=djgpp USE_ALLEGRO42=1 BUNDLED_ALLEGRO42=1

     -  American Fuzzy Lop:

        To compile libopenmpt with fuzzing instrumentation for afl-fuzz, run:
        
            make CONFIG=afl
        
        For more detailed instructions, read `contrib/fuzzing/readme.md`.

     -  other compilers:

        To compile libopenmpt with other compliant compilers, run:
        
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
