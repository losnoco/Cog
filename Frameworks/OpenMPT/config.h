#ifndef _LIBOPENMPT_CONFIG_H_
#define _LIBOPENMPT_CONFIG_H_

#if 1
#define MPT_WITH_MPG123 1
#define MPT_WITH_OGG 1
#define MPT_WITH_VORBIS 1
#define MPT_WITH_VORBISFILE 1
#else
#define MPT_WITH_MINIMP3 1
#define MPT_WITH_STBVORBIS 1
#endif

#define MPT_WITH_ZLIB 1

#define MPT_BUILD_XCODE 1
#define ENABLE_ASM 1

#endif

