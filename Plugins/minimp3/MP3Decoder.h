//
//  MP3Decoder.h
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#define MINIMP3_FLOAT_OUTPUT 1
#define MINIMP3_NO_STDIO 1

#import "ThirdParty/minimp3_ex.h"

#import "Plugin.h"

#define INPUT_BUFFER_SIZE 16 * 1024

@interface MP3Decoder : NSObject <CogDecoder> {
	BOOL seekable;
	unsigned char _decoder_buffer[INPUT_BUFFER_SIZE];
	size_t _decoder_buffer_filled;

	mp3dec_ex_t _decoder_ex;
	mp3dec_io_t _decoder_io;

	mp3dec_frame_info_t _decoder_info;

	size_t samples_filled;
	mp3d_sample_t _decoder_buffer_output[MINIMP3_MAX_SAMPLES_PER_FRAME];

	double seconds;
	
	id<CogSource> _source;

	uint32_t _startPadding, _endPadding;
	BOOL _foundiTunSMPB;

	long _fileSize;
	long _framesDecoded;
	BOOL _foundID3v2;

	BOOL inputEOF;

	int channels;
	float sampleRate;
	int bitrate;
	long totalFrames;
	int layer;

	int metadataUpdateInterval;
	int metadataUpdateCount;

	NSString *genre;
	NSString *album;
	NSString *artist;
	NSString *title;
}

@end
