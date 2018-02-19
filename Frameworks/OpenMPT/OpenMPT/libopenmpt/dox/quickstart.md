
Quick Start {#quickstart}
===========


### Autotools-based

 1. Grab a `libopenmpt-VERSION.autotools.tar.gz` tarball.
 2. Get dependencies:
     -  **gcc >= 4.8** or **clang >= 3.4**
     -  **pkg-config >= 0.24**
     -  **zlib**
     -  **libogg**, **libvorbis**, **libvorbisfile**
     -  **libmpg123 >= 1.13.0**
     -  **doxygen >= 1.8**
     -  **libpulse**, **libpulse-simple** (required only by openmpt123)
     -  **portaudio-v19** (required only by openmpt123)
     -  **libFLAC** (required only by openmpt123)
     -  **libsndfile** (required only by openmpt123)
 3. *Optional*:
     -  **libSDL >= 1.2.x** (required only by openmpt123)
 4. Run:
    
        ./configure
        make
        make check
        sudo make install

### Windows

 1. Get dependencies:
     -  **Microsoft Visual Studio >= 2015**
 2. *Optionally* get dependencies:
     -  **Winamp SDK**
     -  **XMPlay SDK**
 3. Checkout `https://source.openmpt.org/svn/openmpt/trunk/OpenMPT/` .
 4. Open `build\vs2015\openmpt123.sln` or `build\vs2015\libopenmpt.sln` or `build\vs2015\xmp-openmpt.sln` or `build\vs2015\in_openmpt.sln` or `build\vs2015\foo_openmpt.sln` in *Microsoft Visual Studio 2015*.
 5. Select appropriate configuration and build. Binaries are generated in `bin\`
 6. Drag a module onto `openmpt123.exe` or copy the player plugin DLLs (`in_openmpt.dll`, `xmp-openmpt.dll` or `foo_openmpt.dll`) into the respective player directory.

### Unix-like

 1. Get dependencies:
     -  **GNU make**
     -  **gcc >= 4.8** or **clang >= 3.4**
     -  **pkg-config**
     -  **zlib**
     -  **libogg**, **libvorbis**, **libvorbisfile**
     -  **libmpg123 >= 1.13.0**
     -  **libpulse**, **libpulse-simple** (required only by openmpt123)
     -  **portaudio-v19** (required only by openmpt123)
     -  **libFLAC** (required only by openmpt123)
     -  **libsndfile** (required only by openmpt123)
 2. *Optional*:
     -  **libSDL >= 1.2.x** (required only by openmpt123)
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

