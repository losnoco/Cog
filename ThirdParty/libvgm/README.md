This is verbatim from the following repository:

https://github.com/ValleyBell/libvgm.git

Built on an M4 Mac mini, using CMake from Homebrew, with the following
options:

```
cmake -B build.x86 -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_LIBAUDIO=NO -DBUILD_PLAYER=NO -DBUILD_VGM2WAV=NO
cmake -B build.arm -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_LIBAUDIO=NO -DBUILD_PLAYER=NO -DBUILD_VGM2WAV=NO
```

And the debug overlays were made with the above, except for:
```
-B build.d.{x86,arm} -DCMAKE_BUILD_TYPE=Debug
```

As of this edit, commit: 867223e7c33d63de115d1ab955f784c44f19040a
