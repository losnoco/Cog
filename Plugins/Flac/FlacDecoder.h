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
#define SAMPLE_blockBuffer_SIZE ((FLAC__MAX_BLOCK_SIZE + SAMPLES_PER_WRITE) * FLAC__MAX_CHANNELS * (FLAC__MAX_BITS_PER_SAMPLE / 8))

#import "Plugin.h"

#import "RedundantPlaylistDataStore.h"

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
	long frame;
	double seconds;

	long fileSize;

	BOOL hasStreamInfo;
	BOOL streamOpened;
	BOOL abortFlag;
	BOOL hasVorbisComment;

	NSDictionary *metaDict;
	NSDictionary *icyMetaDict;

	NSData *albumArt;

	BOOL cuesheetFound;
	NSString *cuesheet;

	RedundantPlaylistDataStore *dataStore;
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
