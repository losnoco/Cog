Build with CMake, using the following options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON
```

And for the debug overlay, replacing the BUILD_TYPE with `Debug`.

Version 1.3.6 was used.
