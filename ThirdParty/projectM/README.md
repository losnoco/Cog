libprojectM 4.1.8, the visualizer behind the macOS projectM pane. It needs OpenGL 3.3 Core or OpenGL ES 3.0.

Retrieved from:

https://github.com/projectM-visualizer/projectm/releases/download/v4.1.8/libprojectM-4.1.8.tar.gz

With the following sha256 hash:

f72135e8b8e5fd807f908537255b41ac7ba4830309265e830cb756535487239e

Cog uses projectM's OpenGL ES build, drawing through ANGLE on Metal (see `../angle/README.md`), so it uses no
deprecated OpenGL. projectM only allows its ES build on Linux and only includes desktop GL headers on Apple platforms,
so `patches/apple-gles.patch` lets Darwin through and points `OpenGL::GLES3` at ANGLE. Its ES path needs nothing
beyond ES 3.0 (`#version 300 es` shaders; the one ES 3.2 call is desktop-only).

Built once per architecture (`<arch>` is `arm64`, then `x86_64`), from the patched source:

```
patch -p1 < patches/apple-gles.patch     # in libprojectM-4.1.8
cmake -S libprojectM-4.1.8 -B build-<arch> -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=<arch> -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DBUILD_SHARED_LIBS=ON \
  -DENABLE_GLES=ON -DENABLE_PLAYLIST=ON -DENABLE_SYSTEM_PROJECTM_EVAL=OFF -DENABLE_SYSTEM_GLM=OFF \
  -DENABLE_SDL_UI=OFF -DBUILD_TESTING=OFF "-DCMAKE_C_FLAGS=-DUSE_GLES" "-DCMAKE_CXX_FLAGS=-DUSE_GLES" \
  -DOPENGL_gles3_LIBRARY=<Cog>/ThirdParty/angle/lib/libGLESv2.dylib \
  -DOPENGL_GLES3_INCLUDE_DIR=<Cog>/ThirdParty/angle/include \
  -DCMAKE_INSTALL_PREFIX=$PWD/install-<arch>
cmake --build build-<arch> && cmake --install build-<arch>
```

Then joined:

```
lipo -create install-arm64/lib/libprojectM-4.4.1.8.dylib install-x86_64/lib/libprojectM-4.4.1.8.dylib \
  -output lib/libprojectM-4.4.dylib
lipo -create install-arm64/lib/libprojectM-4-playlist.4.1.8.dylib install-x86_64/lib/libprojectM-4-playlist.4.1.8.dylib \
  -output lib/libprojectM-4-playlist.4.dylib
```

The headers in `include/projectM-4` are the installed ones, the same for both architectures. The dylibs live in
`libraries.tar.xz`; their install names are `@rpath/libprojectM-4.4.dylib` and
`@rpath/libprojectM-4-playlist.4.dylib`, and both link `@rpath/libGLESv2.dylib`.

libprojectM is licensed under the GNU LGPL 2.1.
