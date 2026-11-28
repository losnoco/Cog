//
//  FFMPEG-Bridging-Header.h
//  FFMPEG
//
//  The C APIs the Swift decoder reads through, plus the FFmpeg macros that
//  do not import into Swift.
//

#import <Foundation/Foundation.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
#include <libavutil/mem.h>
#include <libavutil/rational.h>

#include <string.h>
#include <strings.h>

#import "../../Utils/NSDictionary+Merge.h"

static const int64_t FFMPEGNoPtsValue = AV_NOPTS_VALUE;
static const AVRational FFMPEGTimeBaseQ = AV_TIME_BASE_Q;
static const int FFMPEGErrorEOF = AVERROR_EOF;
static const int FFMPEGErrorEAGAIN = AVERROR(EAGAIN);
static const int FFMPEGErrorMaxStringSize = AV_ERROR_MAX_STRING_SIZE;
static const int FFMPEGSeekSize = AVSEEK_SIZE;
