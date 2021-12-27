
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
This is preliminary documentation.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 0. The minimum required Android NDK version is r18b.
 1. Copy the whole libopenmpt source tree below your jni directory.
 2. Copy build/android_ndk/* into the root of libopenmpt, i.e. also into the
    jni directory and adjust as needed.
 3. If you want to support MO3 decoding, you have to either make libmpg123,
    libogg, libvorbis and libvorbisfile available (recommended) OR build
    libopenmpt with minimp3 and stb_vorbis support (not recommended).
    Pass the appropriate options to ndk-build:
        MPT_WITH_MINIMP3=1    : Build minimp3 into libopenmpt
        MPT_WITH_MPG123=1     : Link against libmpg123 compiled externally
        MPT_WITH_OGG=1        : Link against libogg compiled externally
        MPT_WITH_STBVORBIS=1  : Build stb_vorbis into libopenmpt
        MPT_WITH_VORBIS=1     : Link against libvorbis compiled externally
        MPT_WITH_VORBISFILE=1 : Link against libvorbisfile compiled externally
 4. Use ndk-build as usual.

