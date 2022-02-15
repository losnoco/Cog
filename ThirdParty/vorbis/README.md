Build with CMake, using the following options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" -DBUILD_SHARED_LIBS=ON
```

And some minor tweaks with `install_name_tool -id` to make sure that the
resulting libvorbis.0.dylib and libvorbisfile.3.dylib referred to themselves
with @rpath and not full paths of the build directory.

Version v1.3.7-10-g84c02369 was used from the following repository:

https://github.com/xiph/vorbis.git
