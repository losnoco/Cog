Build with CMake, using the following options:

```
cmake -B build-x86 -DCMAKE_LIBRARY_PATH=/usr/local/lib -DCMAKE_OSX_ARCHITECTURES="x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON
cmake -B build-arm -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON
```

For the debug overlay, BUILD_TYPE was replaced with `Debug`.

And some minor tweaks with `install_name_tool -change` to fix up the
resulting libFLAC.14.dylib so that it imports libogg.0.dylib using an
@rpath path instead of an absolute path to Homebrew's install.

Then sign the result ad hoc, after the `install_name_tool` changes and the
`lipo` merge of the two architectures, both of which leave it unsigned:

```
codesign -s - -f libFLAC.14.dylib
```

Apple Silicon refuses to load unsigned code, and a build made without
code signing (as local builds are) copies the library in as it is, so an
unsigned libFLAC keeps the FLAC, Vorbis and Opus plugins from loading:
their files then silently fall back to FFmpeg or Apple's decoders.

Version 1.5.0 was used from the official source code download:

https://downloads.xiph.org/releases/flac/flac-1.5.0.tar.xz

x86_64 and arm64 were built separately, to allow for intrinsic functions
to be used for x86_64.
