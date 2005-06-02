//
//  FlacFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "flac/all.h"
#import "SoundFile.h"

#define SAMPLES_PER_WRITE 512
#define FLAC__MAX_SUPPORTED_CHANNELS 2
#define SAMPLE_BUFFER_SIZE ((FLAC__MAX_BLOCK_SIZE + SAMPLES_PER_WRITE) * FLAC__MAX_SUPPORTED_CHANNELS * (24/8))

@interface FlacFile : SoundFile {
	FLAC__FileDecoder *decoder;
	char buffer[SAMPLE_BUFFER_SIZE];
	int bufferAmount;
}

- (FLAC__FileDecoder *)decoder;
- (char *)buffer;
- (int)bufferAmount;
- (void)setBufferAmount:(int)amount;

@end
