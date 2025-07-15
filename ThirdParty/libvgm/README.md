This is verbatim from the following repository:

https://github.com/ValleyBell/libvgm.git

Built on an M4 Mac mini, using CMake from Homebrew, with the following
options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_LIBAUDIO=NO -DBUILD_PLAYER=NO -DBUILD_VGM2WAV=NO
```

And the debug overlays were made with the above, except for:
```
-DCMAKE_BUILD_TYPE=Debug
```

As of this edit, commit: 7cad78367fa35c3f7b3ae16a296d31063cd3a7e4
