Built with the Arch Linux defaults, sort of:

```
cmake -B build.x86 -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" -DBUILD_SHARED_LIBS=OFF
cmake -B build.arm -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DBUILD_SHARED_LIBS=OFF

cd build.x86
make -j8
cd ..

cd build.arm
make -j8
cd ..

mkdir out.release
lipo -create -output out.release/libid3tag.a build.x86/libid3tag.a build.arm/libid3tag.a
```

Version 0.16.2 was used.
