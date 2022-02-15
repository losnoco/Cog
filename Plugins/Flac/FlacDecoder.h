//
//  FlacFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FLAC/all.h"
#import <Cocoa/Cocoa.h>

#define SAMPLES_PER_WRITE 512
#define FLAC__MAX_SUPPORTED_CHANNELS 8
#define SAMPLE_blockBuffer_SIZE ((FLAC__MAX_BLOCK_SIZE + SAMPLES_PER_WRITE) * FLAC__MAX_SUPPORTED_CHANNELS * (24 / 8))

#import "Plugin.h"

@interface FlacDecoder : NSObject <CogDecoder> {
	FLAC__StreamDecoder *decoder;
	void *blockBuffer;
	int blockBufferFrames;

	id<CogSource> source;

	BOOL endOfStream;

	int bitsPerSample;
	int channels;
	uint32_t channelConfig;
	float frequency;
	long totalFrames;

	long fileSize;

	BOOL hasStreamInfo;
	BOOL streamOpened;
	BOOL abortFlag;

	NSString *artist;
	NSString *albumartist;
	NSString *album;
	NSString *title;
	NSString *genre;
	NSNumber *year;
	NSNumber *track;
	NSNumber *disc;
	float replayGainAlbumGain;
	float replayGainAlbumPeak;
	float replayGainTrackGain;
	float replayGainTrackPeak;

	NSData *albumArt;

	BOOL cuesheetFound;
	NSString *cuesheet;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

- (void)setEndOfStream:(BOOL)eos;
- (BOOL)endOfStream;

- (void)setSize:(long)size;

- (FLAC__StreamDecoder *)decoder;
- (char *)blockBuffer;
- (int)blockBufferFrames;
- (void)setBlockBufferFrames:(int)frames;

@end
