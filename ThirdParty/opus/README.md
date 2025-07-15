Build with CMake, using the following options:

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON
```

The debug overlay was built the same, except with BUILD_TYPE set to `Debug`.

Version v1.5.2-153-g7aa5be98 was used from the following repository:

https://github.com/xiph/opus.git
