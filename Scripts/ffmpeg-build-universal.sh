#!/bin/sh
# Run from ffmpeg repo.

info() {
    GREEN="\033[1;32m"
    RESET="\033[0m"
    echo "$GREEN==>$RESET $@"
}

info "Installing build dependencies of FFmpeg with Homebrew"
brew install --only-dependencies ffmpeg
brew install nasm pkgconf

info "Installing build dependencies for cross build with Homebrew"
arch -x86_64 /usr/local/bin/brew install --only-dependencies ffmpeg

ARCHS="arm64 x86_64"
LIBS="libavcodec libavformat libavutil libswresample"
BASEDIR=$(dirname "$0")
COG_FFMPEG_PREFIX="$BASEDIR/../ThirdParty/ffmpeg"

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:"$BASEDIR/../ThirdParty/fdk-aac/lib/pkgconfig"

for arch in $ARCHS; do
    info "Building FFmpeg for $arch"
    $BASEDIR/ffmpeg-build-$arch.sh $COG_FFMPEG_PREFIX
    rm -rf $COG_FFMPEG_PREFIX/pkgconfig
    rm -rf $COG_FFMPEG_PREFIX/share
    # Workaround Xcode linking error
    git clean -ffdx > /dev/null 2>&1
done

all_out_libs=""
for lib in $LIBS; do
    out_lib=$(find -E "$COG_FFMPEG_PREFIX/arm64/lib" -type l -regex ".*/$lib\.[0-9]+\.dylib")
    out_lib=$(echo "$out_lib" | awk -F "/" '{ print $NF }')
    all_out_libs="$all_out_libs $out_lib"
done

echo "all_out_libs: $all_out_libs"

mkdir -p $COG_FFMPEG_PREFIX/lib
for lib in $LIBS; do
    out_lib=$(find -E "$COG_FFMPEG_PREFIX/arm64/lib" -type l -regex ".*/$lib\.[0-9]+\.dylib")
    out_lib=$(echo "$out_lib" | awk -F "/" '{ print $NF }')
    all_libs=""
    all_symlinks=""
    for arch in $ARCHS; do
        libs=$(find -E "$COG_FFMPEG_PREFIX/$arch/lib" -type f -regex ".*/$lib\..*\.dylib")
        symlinks=$(find -E "$COG_FFMPEG_PREFIX/$arch/lib" -type l -regex ".*/$lib\..*dylib")
        all_libs="$all_libs $libs"
        all_symlinks="$all_symlinks $symlinks"
        # clean up ids and import paths
        install_name_tool -id "@rpath/$out_lib" $libs
        for temp_out_lib in $all_out_libs; do
            install_name_tool -change "$COG_FFMPEG_PREFIX/$arch/lib/$temp_out_lib" "@rpath/$temp_out_lib" $libs
        done
    done
    info "Making $lib universal"
    lipo -create -output $COG_FFMPEG_PREFIX/lib/$out_lib $all_libs
    rm $all_libs
    rm $all_symlinks
done

info "Done!"
