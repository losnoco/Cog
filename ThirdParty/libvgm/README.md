This is verbatim from the following repository:

https://github.com/ValleyBell/libvgm.git

Built on an M1 Mac mini, using CMake from Homebrew, with the following
options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
    -DBUILD_LIBAUDIO=NO -DBUILD_PLAYER=NO -DBUILD_VGM2WAV=NO
```

As of this edit, commit: 82ba45d3906a0b54b6de2555468dd9e9598f617d