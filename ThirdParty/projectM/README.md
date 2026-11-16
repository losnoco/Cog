libprojectM 4.1.8, the visualizer behind the macOS projectM pane. It needs OpenGL 3.3 Core or OpenGL ES 3.0.

Retrieved from:

https://github.com/projectM-visualizer/projectm/releases/download/v4.1.8/libprojectM-4.1.8.tar.gz

With the following sha256 hash:

f72135e8b8e5fd807f908537255b41ac7ba4830309265e830cb756535487239e

The two dylibs are universal, but their halves are different builds:

- **arm64** is projectM's OpenGL ES build, drawing through ANGLE on Metal (see `../angle/README.md`), so Apple
  Silicon uses no deprecated OpenGL. projectM only allows its ES build on Linux and only includes desktop GL headers
  on Apple platforms, so `patches/apple-gles.patch` lets Darwin through and points `OpenGL::GLES3` at ANGLE. Its ES
  path needs nothing beyond ES 3.0 (`#version 300 es` shaders; the one ES 3.2 call is desktop-only).
- **x86_64** is the unmodified desktop OpenGL build. The ANGLE package has no Intel slice, so Intel Macs keep OpenGL.

arm64, from the patched source:

```
patch -p1 < patches/apple-gles.patch     # in libprojectM-4.1.8
cmake -S libprojectM-4.1.8 -B build-arm64-gles -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DBUILD_SHARED_LIBS=ON \
  -DENABLE_GLES=ON -DENABLE_PLAYLIST=ON -DENABLE_SYSTEM_PROJECTM_EVAL=OFF -DENABLE_SYSTEM_GLM=OFF \
  -DENABLE_SDL_UI=OFF -DBUILD_TESTING=OFF "-DCMAKE_C_FLAGS=-DUSE_GLES" "-DCMAKE_CXX_FLAGS=-DUSE_GLES" \
  -DOPENGL_gles3_LIBRARY=<ANGLE>/libGLESv2.dylib -DOPENGL_GLES3_INCLUDE_DIR=<Cog>/ThirdParty/angle/include \
  -DCMAKE_INSTALL_PREFIX=$PWD/install-arm64-gles
cmake --build build-arm64-gles && cmake --install build-arm64-gles
# Both libraries name ANGLE by the package's framework path; Cog ships plain dylibs.
install_name_tool -change @rpath/libGLESv2.framework/libGLESv2 @rpath/libGLESv2.dylib <each arm64 dylib>
```

x86_64, from the unmodified source:

```
cmake -S libprojectM-4.1.8 -B build-x86_64 -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DBUILD_SHARED_LIBS=ON \
  -DENABLE_PLAYLIST=ON -DENABLE_SYSTEM_PROJECTM_EVAL=OFF -DENABLE_SYSTEM_GLM=OFF -DENABLE_SDL_UI=OFF \
  -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=$PWD/install-x86_64
cmake --build build-x86_64 && cmake --install build-x86_64
```

Then joined:

```
lipo -create <arm64>/libprojectM-4.4.1.8.dylib install-x86_64/lib/libprojectM-4.4.1.8.dylib \
  -output lib/libprojectM-4.4.dylib
lipo -create <arm64>/libprojectM-4-playlist.4.1.8.dylib install-x86_64/lib/libprojectM-4-playlist.4.1.8.dylib \
  -output lib/libprojectM-4-playlist.4.dylib
```

The arm64 half needs macOS 13, as ANGLE does, so Cog weak-links both libraries: on Apple Silicon with macOS 12 they
don't load and the pane isn't offered. The headers in `include/projectM-4` are the installed ones, the same for every
build. The dylibs live in `libraries.tar.xz`; their install names are `@rpath/libprojectM-4.4.dylib` and
`@rpath/libprojectM-4-playlist.4.dylib`.

libprojectM is licensed under the GNU LGPL 2.1.
