These were compiled with default settings from:

https://github.com/kode54/libsoxr

Original upstream it was forked from:

https://github.com/nanake/libsoxr

Using CMake:

```
mkdir build-x86
mkdir build-arm
cd build-x86
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.12" -DBUILD_SHARED_LIBS=OFF -DWITH_OPENMP=OFF
make -j8
cd ../build-arm
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DBUILD_SHARED_LIBS=OFF -DWITH_OPENMP=OFF
make -j8
cd ..
lipo -create -output libsoxr.a build-x86/src/libsoxr.a build-arm/src/libsoxr.a
```
