Build with CMake, using the following options:

```
env PATH=/usr/local/bin:$PATH cmake -B build-x86 -DCMAKE_LIBRARY_PATH=/usr/local/lib \
    -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON -DOP_DISABLE_HTTP=ON -DOP_DISABLE_DOCS=ON
cmake build-arm -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=ON -DOP_DISABLE_HTTP=ON \
    -DOP_DISABLE_DOCS=ON
```

The debug overlay was built the same, except with BUILD_TYPE set to `Debug`.

The CMakeCache.txt also needed hacking to replace the libopus with our own build for compatible version
numbers in the import.

Finally, `install_name_tool -change` was needed to change the import path for libogg. The version
numbers were already compatible.

Version v0.12-51-g24d6e75 was used from the following repository:

https://github.com/xiph/opusfile.git
