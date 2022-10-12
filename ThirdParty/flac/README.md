Build with CMake, using the following options:

```
cmake build-x86 -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" -DBUILD_SHARED_LIBS=ON
cmake build-arm -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DBUILD_SHARED_LIBS=ON
```


And some minor tweaks with `install_name_tool -id` to make sure that the
resulting libFLAC.12.dylib referred to itself with @rpath and not full
paths of the build directory, and imported libogg.0.dylib with an @rpath.

Version 1.4.1 was used from the official source code download:

https://downloads.xiph.org/releases/flac/flac-1.4.1.tar.xz

x86_64 and arm64 were built separately, to allow for intrinsic functions
to be used for x86_64.
