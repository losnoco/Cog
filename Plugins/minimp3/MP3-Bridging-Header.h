//
//  MP3-Bridging-Header.h
//  minimp3
//
//  The C APIs the Swift decoder reads through.
//

#import <Foundation/Foundation.h>

#define MINIMP3_FLOAT_OUTPUT 1
#define MINIMP3_NO_STDIO 1

#import "ThirdParty/minimp3_ex.h"

#import <id3tag.h>

static const size_t MP3DecoderBufferSize = MINIMP3_BUF_SIZE;
static const size_t MP3DecoderMaxSamplesPerFrame = MINIMP3_MAX_SAMPLES_PER_FRAME;
