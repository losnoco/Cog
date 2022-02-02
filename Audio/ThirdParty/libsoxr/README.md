These were compiled with default settings from:

https://github.com/nanake/libsoxr

Using CMake:

```
mkdir build
cd build
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12"
make -j8
```