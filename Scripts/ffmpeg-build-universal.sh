#!/bin/sh
# Run from ffmpeg repo.

info() {
    GREEN="\033[1;32m"
    RESET="\033[0m"
    echo "$GREEN==>$RESET $@"
}

info "Installing build dependencies of FFmpeg with Homebrew"
brew install --only-dependencies ffmpeg
brew install yasm

info "Installing build dependencies for cross build with Homebrew"
arch -x86_64 /usr/local/bin/brew install --only-dependencies ffmpeg

ARCHS="arm64 x86_64"
LIBS="libavcodec libavformat libavutil libswresample"
BASEDIR=$(dirname "$0")
COG_FFMPEG_PREFIX="$BASEDIR/../ThirdParty/ffmpeg"

for arch in $ARCHS; do
    info "Building FFmpeg for $arch"
    $BASEDIR/ffmpeg-build-$arch.sh $COG_FFMPEG_PREFIX
    rm -rf $COG_FFMPEG_PREFIX/pkgconfig
    rm -rf $COG_FFMPEG_PREFIX/share
    # Workaround Xcode linking error
    git clean -ffdx > /dev/null 2>&1
done

mkdir -p $COG_FFMPEG_PREFIX/lib
for lib in $LIBS; do
    all_libs=""
    for arch in $ARCHS; do
        all_libs="$all_libs $COG_FFMPEG_PREFIX/$arch/lib/$lib.a"
    done
    info "Making $lib universal"
    lipo -create -output $COG_FFMPEG_PREFIX/lib/$lib.a $all_libs
    rm $all_libs
done

info "Done!"
