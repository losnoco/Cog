# This is the commands used to build the ffmpeg libs provided here
./configure --extra-cflags="-fPIC -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk -mmacosx-version-min=10.6"\
    --enable-static --disable-shared --prefix=$HOME/Source/Repos/cog/ThirdParty/ffmpeg\
    --enable-pic --enable-gpl --disable-doc --disable-ffplay\
    --disable-ffprobe --disable-ffserver --disable-avdevice --disable-ffmpeg\
    --disable-postproc --disable-swresample --disable-avfilter\
    --disable-swscale --enable-network --disable-swscale-alpha --disable-vdpau\
    --disable-dxva2 --enable-hwaccels\
    --disable-encoders --disable-muxers --disable-indevs --disable-outdevs\
    --disable-devices --disable-filters --disable-parsers --enable-parser=ac3\
    --enable-demuxer=ac3 --disable-bsfs --disable-bzlib --disable-protocols\
    --disable-decoders --disable-decoder=libopus --disable-libopus\
    --enable-decoder=wmapro --enable-decoder=wmav1 --enable-decoder=wmav2\
    --enable-decoder=wmavoice --enable-decoder=ac3\
    --disable-decoder=amrnb\
    --disable-demuxers --enable-demuxer=asf\
    --enable-demuxer=mov --enable-demuxer=oma --enable-demuxer=ac3\
    --disable-demuxer=amr --enable-demuxer=ogg\
    --enable-demuxer=tak --enable-decoder=tak\
    --enable-decoder=dsd_lsbf --enable-decoder=dsd_lsbf_planar\
    --enable-decoder=dsd_msbf --enable-decoder=dsd_msbf_planar\
    --enable-demuxer=dsf\
    --enable-demuxer=aac --enable-decoder=aac\
    --enable-decoder=atrac3 --enable-decoder=atrac3p\
	--enable-libdcadec --enable-demuxer=dts --enable-demuxer=dtshd\
	--enable-demuxer=mp3 --enable-decoder=mp3float\
    --disable-libopencore-amrnb --disable-libopencore-amrwb\
    --disable-version3 --disable-mmx\
    --disable-yasm

make -j8
