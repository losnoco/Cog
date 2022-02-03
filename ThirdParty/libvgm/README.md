This is verbatim from the following repository:

https://github.com/ValleyBell/libvgm.git

Built on an M1 Mac mini, using CMake from Homebrew, with the following
options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" \
    -DBUILD_LIBAUDIO=NO -DBUILD_PLAYER=NO -DBUILD_VGM2WAV=NO
```
