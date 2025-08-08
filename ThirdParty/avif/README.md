These were built from libavif, libdav1d, and libyuv, from the following repositories:

https://code.videolan.org/videolan/dav1d.git - at revision: 1.5.1-0-g42b2b24
https://chromium.googlesource.com/libyuv/libyuv - at revision: 4db2af62d
https://github.com/AOMediaCodec/libavif - at revision: v1.3.0-46-g1540d752

dav1d was built to two separate targets, using the meson crossfiles included in the root ThirdParty directory:

```
meson setup --cross-file=/path/to/meson-x86_64.txt --default-library=static --buildtype release -Denable_tools=false -Denable_tests=false build.x64 .
ninja -C build.x64
```

```
meson setup --cross-file=/path/to/meson-arm64.txt --default-library=static --buildtype release -Denable_tools=false -Denable_tests=false build.x64 .
ninja -C build.arm
```

Then:

```
ninja -C build.arm install

lipo -create -output libdav1d.a build.x64/src/libdav1d.a build.arm/src/libdav1d.a
cp libdav1d.a /usr/local/lib
cp /usr/local/lib/pkgconfig/dav1d.pc /opt/homebrew/lib/pkgconfig
```

libyuv was build to two separate targets, using CMake options and arch direction to control the build flow:

```
arch -x86_64 /usr/local/bin/cmake -G Ninja -B build.x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" -DBUILD_SHARED_LIBS=OFF
ninja -C build.x64
```

```
cmake -G Ninja -B build.arm -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DBUILD_SHARED_LIBS=OFF
ninja -C build.arm
```

Then:

```
ninja -C build.arm install

lipo -create -output libyuv.a build.x64/libyuv.a build.arm/libyuv.a
cp libyuv.a /usr/local/lib
rm /usr/local/lib/libyuv.dylib
```

libavif was built to two separate targets, with the following options:

```
env PATH=/usr/local/bin:$PATH arch -x86_64 /usr/local/bin/cmake -B build.x64 -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" -DAVIF_CODEC_DAV1D=SYSYEM -DAVIF_LIBYUV=SYSTEM -DBUILD_SHARED_LIBS=OFF
arch -x86_64 make -j14
```

```
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DAVIF_CODEC_DAV1D=SYSTEM -DAVIF_LIBYUV=SYSTEM -DBUILD_SHARED_LIBS=OFF
make -j14
```

And then they were merged using lipo.

The AVIF decoder contained in the source directory was based on code from
the following repository:

https://github.com/dreampiggy/AVIFQuickLook


It was updated to work with the newer libavif.
