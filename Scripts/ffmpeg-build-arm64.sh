#!/bin/sh

set -e

export CPATH=/opt/homebrew/include
export LIBRARY_PATH=/opt/homebrew/lib
export PATH=/opt/homebrew/bin:$PATH

# This is the commands used to build the ffmpeg libs provided here
./configure\
    --enable-cross-compile\
    --arch=arm64\
    --enable-neon\
    --extra-cflags="-arch arm64 -fPIC -isysroot $(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=11.0"\
    --extra-ldflags="-arch arm64 -mmacosx-version-min=11.0"\
    --disable-static --enable-shared\
    --prefix="$1/arm64"\
    --incdir="$1/include"\
    --datadir="$1/share"\
    --pkgconfigdir="$1/pkgconfig"\
    --enable-pic --enable-gpl --disable-doc --disable-ffplay\
    --disable-ffprobe --disable-avdevice --disable-ffmpeg\
    --disable-postproc --disable-avfilter\
    --disable-swscale --disable-network --disable-swscale-alpha --disable-vdpau\
    --disable-dxva2 --disable-everything --enable-hwaccels\
    --enable-swresample\
    --enable-parser=ac3,mpegaudio,xma,vorbis,opus\
    --enable-demuxer=ac3,asf,xwma,mov,oma,ogg,tak,dsf,wav,w64,aac,dts,dtshd,eac3,mp3,bink,flac,msf,xmv,caf,ape,smacker,pcm_s8,spdif,mpc,mpc8,rm,matroska\
    --enable-decoder=ac3,wmapro,wmav1,wmav2,wmavoice,wmalossless,xma1,xma2,dca,tak,dsd_lsbf,dsd_lsbf_planar,dsd_mbf,dsd_msbf_planar,aac_at,atrac3,atrac3p,mp3float,bink,binkaudio_dct,binkaudio_rdft,flac,pcm_s16be,pcm_s16be_planar,pcm_s16le,pcm_s16le_planar,vorbis,ape,adpcm_ima_qt,smackaud,opus,pcm_s8,pcm_s8_planar,mpc7,mpc8,alac,adpcm_ima_dk3,adpcm_ima_dk4,cook\
    --disable-parser=mpeg4video,h263\
    --disable-decoder=mpeg2video,h263,h264,mpeg1video,mpeg2video,mpeg4,hevc,vp9\
    --disable-version3

make -j$(sysctl -n hw.logicalcpu) install

make clean
