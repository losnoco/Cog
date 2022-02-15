Build with CMake, using the following options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" -DBUILD_SHARED_LIBS=ON
```

And some minor tweaks with `install_name_tool -id` to make sure that the
resulting libopusfile.0.dylib referred to itself with @rpath and not full
paths of the build directory.

Version v0.12-38-gcf218fb was used from the following repository:

https://github.com/xiph/opusfile.git
