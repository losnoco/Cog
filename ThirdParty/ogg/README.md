Build with CMake, using the following options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" -DBUILD_SHARED_LIBS=ON
```

And some minor tweaks with `install_name_tool -id` to make sure that the
resulting libogg.0.dylib referred to itself with @rpath.

Version 1.3.5 was used.
