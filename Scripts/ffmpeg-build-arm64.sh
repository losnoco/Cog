#!/bin/sh

set -e

export CPATH=/opt/homebrew/include
export LIBRARY_PATH=/opt/homebrew/lib
export PATH=/opt/homebrew/bin:$PATH

PCM_CODECS=pcm_alaw,pcm_bluray,pcm_dvd,pcm_f16le,pcm_f24le,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_lxf,pcm_mulaw,pcm_s8,pcm_s8_planar,pcm_s16be,pcm_s16be_planar,pcm_s16le,pcm_s16le_planar,pcm_s24be,pcm_s24daud,pcm_s24le,pcm_s24le_planar,pcm_s32be,pcm_s32le,pcm_s32le_planar,pcm_s64be,pcm_s64le,pcm_sga,pcm_u8pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_vidc
ADPCM_CODECS=adpcm_4xm,adpcm_adx,adpcm_afx,adpcm_agm,adpcm_aica,adpcm_argo,adpcm_ct,adpcm_dtk,adpcm_ea,adpcm_ea_maxis_xa,adpcm_ea_r1,adpcm_ea_r2,adpcm_ea_r3,adpcm_ea_xa,adpcm_g722,adpcm_g726,adpcm_g726le,adpcm_ima_amv,adpcm_ima_alp,adpcm_ima_apc,adpcm_ima_apm,adpcm_ima_cunning,adpcm_ima_dat4,adppcm_ima_dk3,adpcm_ima_dk4,adpcm_ima_ea_eacs,adpcm_ima_ea_sead,adpcm_ima_iss,adpcm_ima_moflex,adpcm_ima_mtf,adpcm_ima_oki,adpcm_ima_qt,adpcm_ima_rad,adpcm_ima_ssi,adpcm_ima_smjpeg,adpcm_ima_wav,adpcm_ima_ws,adpcm_ms,adpcm_mtaf,adpcm_psx,adpcm_sbpro_2,adpcm_sbpro_3,adpcm_sbpro_4,adpcm_swf,adpcm_thp,adpcm_thp_le,adpcm_vima,adpcm_xa,adpcm_yamaha,adpcm_zork

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
    --enable-nonfree --enable-libfdk-aac\
    --enable-pic --enable-gpl --disable-doc --disable-ffplay\
    --disable-ffprobe --disable-avdevice --disable-ffmpeg\
    --disable-avfilter\
    --disable-swscale --enable-network --disable-swscale-alpha --disable-vdpau\
    --disable-dxva2 --disable-everything --enable-hwaccels\
    --enable-swresample\
    --enable-protocol=tcp,tls,http,https,icecast\
    --enable-parser=ac3,mpegaudio,xma,vorbis,opus\
    --enable-demuxer=hls,mpegts,mpegtsraw,ac3,asf,xwma,mov,oma,ogg,tak,dsf,wav,w64,aac,dts,dtshd,eac3,mp3,bink,flac,msf,xmv,caf,ape,smacker,spdif,mpc,mpc8,rm,matroska,tta,dff,wsd,iff,aiff,truehd,$PCM_CODECS,$ADPCM_CODECS\
    --enable-decoder=ac3,ac3_t,eac3,wmapro,wmav1,wmav2,wmavoice,wmalossless,xma1,xma2,dca,tak,dsd_lsbf,dsd_lsbf_planar,dsd_mbf,dsd_msbf_planar,aac,libfdk_aac,atrac3,atrac3p,mp3float,mp2float,mp1float,bink,binkaudio_dct,binkaudio_rdft,flac,vorbis,ape,smackaud,opus,mpc7,mpc8,alac,cook,tta,truehd,$PCM_CODECS,$ADPCM_CODECS\
    --disable-parser=mpeg4video,h263\
    --disable-decoder=mpeg2video,h263,h264,mpeg1video,mpeg2video,mpeg4,hevc,vp9\
    --disable-version3\
    --disable-xlib

make -j$(sysctl -n hw.logicalcpu) install

make clean
