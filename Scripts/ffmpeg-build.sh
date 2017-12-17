# This is the commands used to build the ffmpeg libs provided here
./configure --extra-cflags="-fPIC -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -mmacosx-version-min=10.8" --extra-ldflags="-mmacosx-version-min=10.8"\
    --enable-static --disable-shared --prefix=$HOME/Source/Repos/cog/ThirdParty/ffmpeg\
    --enable-pic --enable-gpl --disable-doc --disable-ffplay\
    --disable-ffprobe --disable-ffserver --disable-avdevice --disable-ffmpeg\
    --disable-postproc --disable-avfilter\
    --disable-swscale --disable-network --disable-swscale-alpha --disable-vdpau\
    --disable-dxva2 --disable-everything --enable-hwaccels\
    --enable-swresample\
    --enable-parser=ac3,mpegaudio,xma,vorbis,opus\
    --enable-demuxer=ac3,asf,xwma,mov,oma,ogg,tak,dsf,wav,aac,dts,dtshd,mp3,bink,flac,msf,xmv,caf,ape,smacker,pcm_s8,spdif,mpc,mpc8,rm\
    --enable-decoder=ac3,wmapro,wmav1,wmav2,wmavoice,wmalossless,xma1,xma2,dca,tak,dsd_lsbf,dsd_lsbf_planar,dsd_mbf,dsd_msbf_planar,aac,atrac3,atrac3p,mp3float,bink,binkaudio_dct,binkaudio_rdft,flac,pcm_s16be,pcm_s16be_planar,pcm_s16le,pcm_s16le_planar,vorbis,ape,adpcm_ima_qt,smackaud,opus,pcm_s8,pcm_s8_planar,mpc7,mpc8,cook\
    --disable-parser=mpeg4video,h263\
    --disable-decoder=mpeg2video,h263,h264,mpeg1video,mpeg2video,mpeg4,hevc,vp9\
    --disable-version3

make -j8
