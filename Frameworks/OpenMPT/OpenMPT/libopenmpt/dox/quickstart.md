
Quick Start {#quickstart}
===========


### Autotools-based

 1. Grab a `libopenmpt-VERSION.autotools.tar.gz` tarball.
 2. Get dependencies:
     -  **gcc >= 7.1** or **clang >= 5**
     -  **pkg-config >= 0.24**
     -  **zlib**
     -  **libogg**, **libvorbis**, **libvorbisfile**
     -  **libmpg123 >= 1.14.0**
     -  **doxygen >= 1.8**
     -  **libpulse**, **libpulse-simple** (required only by openmpt123)
     -  **portaudio-v19** (required only by openmpt123)
     -  **libFLAC** (required only by openmpt123)
     -  **libsndfile** (required only by openmpt123)
 3. *Optional*:
     -  **libSDL2 >= 2.0.4** (required only by openmpt123)
 4. Run:
    
        ./configure
        make
        make check
        sudo make install

### Windows

 1. Get dependencies:
     -  **Microsoft Visual Studio >= 2017**
 2. *Optionally* get dependencies:
     -  **Winamp SDK**
     -  **XMPlay SDK**
 3. Checkout `https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/` .
 4. Open `build\vs2017\openmpt123.sln` or `build\vs2017\libopenmpt.sln` or `build\vs2017\xmp-openmpt.sln` or `build\vs2017\in_openmpt.sln` in *Microsoft Visual Studio 2017*.
 5. Select appropriate configuration and build. Binaries are generated in `bin\`
 6. Drag a module onto `openmpt123.exe` or copy the player plugin DLLs (`in_openmpt.dll` or `xmp-openmpt.dll`) into the respective player directory.

### Unix-like

 1. Get dependencies:
     -  **GNU make**
     -  **gcc >= 7.1** or **clang >= 5**
     -  **pkg-config**
     -  **zlib**
     -  **libogg**, **libvorbis**, **libvorbisfile**
     -  **libmpg123 >= 1.14.0**
     -  **libpulse**, **libpulse-simple** (required only by openmpt123)
     -  **portaudio-v19** (required only by openmpt123)
     -  **libFLAC** (required only by openmpt123)
     -  **libsndfile** (required only by openmpt123)
 2. *Optional*:
     -  **libSDL2 >= 2.0.4** (required only by openmpt123)
     -  **doxygen >= 1.8**
     -  **help2man**
 3. Run:
    
        svn checkout https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/ openmpt-trunk
        cd openmpt-trunk
        make clean
        make
        make doc
        make check
        sudo make install         # installs into /usr/local by default
        sudo make install-doc     # installs into /usr/local by default
        sudo ldconfig             # required on some systems (i.e. Linux)
        openmpt123 $SOMEMODULE

