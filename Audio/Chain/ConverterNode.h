//
//  ConverterNode.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreAudio/AudioHardware.h>

#import "Node.h"

#define DSD_DECIMATE 1

@interface ConverterNode : Node {
	NSDictionary *rgInfo;

	void *inputBuffer;
	size_t inputBufferSize;
	size_t inpSize, inpOffset;

	BOOL stopping;
	BOOL convertEntered;
	BOOL paused;

	float volumeScale;

	void *floatBuffer;
	size_t floatBufferSize;
	size_t floatSize, floatOffset;

#if DSD_DECIMATE
	void **dsd2pcm;
	size_t dsd2pcmCount;
	int dsd2pcmLatency;
#endif

	BOOL rememberedLossless;

	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription floatFormat;
	AudioStreamBasicDescription dmFloatFormat; // downmixed/upmixed float format

	uint32_t inputChannelConfig;

	BOOL streamFormatChanged;
	AudioStreamBasicDescription newInputFormat;
	uint32_t newInputChannelConfig;

	AudioChunk *lastChunkIn;

	void *hdcd_decoder;
}

@property AudioStreamBasicDescription inputFormat;

- (id)initWithController:(id)c previous:(id)p;

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat withInputConfig:(uint32_t)inputConfig isLossless:(BOOL)lossless;
- (void)cleanUp;

- (void)process;
- (int)convert:(void *)dest amount:(int)amount;

- (void)setRGInfo:(NSDictionary *)rgi;

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format inputConfig:(uint32_t)inputConfig;

- (void)refreshVolumeScaling;

@end
