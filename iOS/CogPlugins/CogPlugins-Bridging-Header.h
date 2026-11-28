//
//  CogPlugins-Bridging-Header.h
//  CogPlugins
//
//  The C APIs the Swift plugin sources read through, in place of each
//  macOS plugin bundle's own bridging header.
//

#import <Foundation/Foundation.h>

#import <File_Extractor/fex.h>

#import <FLAC/all.h>

#import <opusfile.h>

// config.h things
#define __MACOSX__
#define HAVE_CONFIG_H

#import <vorbis/codec.h>
#import <vorbis/vorbisfile.h>

#undef __MACOSX__
#undef HAVE_CONFIG_H

#import "picture.h"

#define MINIMP3_FLOAT_OUTPUT 1
#define MINIMP3_NO_STDIO 1

#import "minimp3_ex.h"

#import <id3tag.h>

static const size_t MP3DecoderBufferSize = MINIMP3_BUF_SIZE;
static const size_t MP3DecoderMaxSamplesPerFrame = MINIMP3_MAX_SAMPLES_PER_FRAME;

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

#include <strings.h>

static const int64_t FFMPEGNoPtsValue = AV_NOPTS_VALUE;
static const AVRational FFMPEGTimeBaseQ = AV_TIME_BASE_Q;
static const int FFMPEGErrorEOF = AVERROR_EOF;
static const int FFMPEGErrorEAGAIN = AVERROR(EAGAIN);
static const int FFMPEGErrorMaxStringSize = AV_ERROR_MAX_STRING_SIZE;
static const int FFMPEGSeekSize = AVSEEK_SIZE;

#import "../../Utils/RedundantPlaylistDataStore.h"
#import "../../Utils/NSDictionary+Merge.h"

// The cuesheet grabbag in Plugins/Flac/cuesheet.m.
void grabbag__cuesheet_emit(NSString **out, const FLAC__StreamMetadata *cuesheet, const char *file_reference);

static const size_t FlacBlockBufferSize = ((FLAC__MAX_BLOCK_SIZE + 512) * FLAC__MAX_CHANNELS * (FLAC__MAX_BITS_PER_SAMPLE / 8));