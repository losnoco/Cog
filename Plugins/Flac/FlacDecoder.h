//
//  FlacFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "FLAC/all.h"

#define SAMPLES_PER_WRITE 512
#define FLAC__MAX_SUPPORTED_CHANNELS 2
#define SAMPLE_BUFFER_SIZE ((FLAC__MAX_BLOCK_SIZE + SAMPLES_PER_WRITE) * FLAC__MAX_SUPPORTED_CHANNELS * (24/8))

#import "Plugin.h"

@interface FlacDecoder : NSObject <CogDecoder>
{
	FLAC__FileDecoder *decoder;
	void *buffer;
	int bufferAmount;
	
	int bitsPerSample;
	int channels;
	float frequency;
	double length;
}

- (FLAC__FileDecoder *)decoder;
- (char *)buffer;
- (int)bufferAmount;
- (void)setBufferAmount:(int)amount;

@end
