//
//  WavPackFile.h
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "Plugin.h"
#import <Cocoa/Cocoa.h>

#define ChunkHeader WavPackChunkHeader

#import <wavpack.h>

@interface WavPackReader : NSObject {
	id<CogSource> source;
}

- (id)initWithSource:(id<CogSource>)s;

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

@end

@interface WavPackDecoder : NSObject <CogDecoder> {
	WavpackContext *wpc;
	WavpackStreamReader reader;

	WavPackReader *wv;
	WavPackReader *wvc;

	int32_t *inputBuffer;
	size_t inputBufferSize;

	uint8_t *outputBuffer;
	size_t outputBufferSize;

	BOOL isDSD;
	BOOL isLossy;

	int bitsPerSample;
	int channels;
	uint32_t channelConfig;
	BOOL floatingPoint;
	int bitrate;
	float frequency;
	long totalFrames;
	long frame;
}

@end
