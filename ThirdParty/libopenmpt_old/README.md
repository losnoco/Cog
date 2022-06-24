Built from the latest archive, version 0.5.18 as of this writing, using the
Makefile variant:

for arch in x86_64 arm64; do
    env CFLAGS="-g -Os -mmacosx-version-min=10.13 -arch $arch" CXXFLAGS="-g -Os -mmacosx-version-min=10.13 -arch $arch" LDFLAGS="-mmacosx-version-min=10.13 -arch $arch" make -j8 SHARED_LIB=0 EXAMPLES=0 OPENMPT123=0 DEBUG=1 OPTIMIZE_SIZE=1 TEST=0
    cp bin/libopenmpt.a libopenmpt_$arch.a
    make clean
done

lipo -create -output libopenmpt.old.a libopenmpt_x86_64.a libopenmpt_arm64.a
