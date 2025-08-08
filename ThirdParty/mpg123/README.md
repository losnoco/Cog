Built from the latest version indicated by Homebrew, 1.33.2, using the
following commands:

```
for arch in x86_64 arm64; do
  arch -"$arch" env CFLAGS="-g -Os -mmacosx-version-min=10.13 -arch $arch" CXXFLAGS="-g -Os -mmacosx-version-min=10.13 -arch $arch" LDFLAGS="-mmacosx-version-min=10.13 -arch $arch" ./configure
  make -j14
  cp src/libmpg123/.libs/libmpg123.0.dylib ./libmpg123.0_$arch.dylib
  make clean
done

lipo -create -output libmpg123.0.dylib libmpg123.0_x86_64.dylib libmpg123.0_arm64.dylib

install_name_tool -id @rpath/libmpg123.0.dylib libmpg123.0.dylib
```

The debug version was made substituting -O0 for the -Os.