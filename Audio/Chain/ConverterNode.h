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

#import "HeadphoneFilter.h"

@interface ConverterNode : Node {
	NSDictionary *rgInfo;

	void *_r8bstate;

	void *inputBuffer;
	size_t inputBufferSize;
	size_t inpSize, inpOffset;

	BOOL stopping;
	BOOL convertEntered;
	BOOL paused;

	BOOL skipResampler;

	unsigned int PRIME_LEN_;
	unsigned int N_samples_to_add_;
	unsigned int N_samples_to_drop_;

	unsigned int is_preextrapolated_;
	unsigned int is_postextrapolated_;

	int latencyEaten;
	int latencyEatenPost;

	double sampleRatio;

	float volumeScale;

	void *floatBuffer;
	size_t floatBufferSize;
	size_t floatSize, floatOffset;

	void *extrapolateBuffer;
	size_t extrapolateBufferSize;

	void **dsd2pcm;
	size_t dsd2pcmCount;
	int dsd2pcmLatency;

	BOOL rememberedLossless;

	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription floatFormat;
	AudioStreamBasicDescription dmFloatFormat; // downmixed/upmixed float format
	AudioStreamBasicDescription outputFormat;

	uint32_t inputChannelConfig;
	uint32_t outputChannelConfig;

	BOOL streamFormatChanged;
	AudioStreamBasicDescription newInputFormat;
	uint32_t newInputChannelConfig;

	AudioChunk *lastChunkIn;

	void *hdcd_decoder;

	HeadphoneFilter *hFilter;
}

@property AudioStreamBasicDescription inputFormat;

- (id)initWithController:(id)c previous:(id)p;

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat withInputConfig:(uint32_t)inputConfig outputFormat:(AudioStreamBasicDescription)outputFormat outputConfig:(uint32_t)outputConfig isLossless:(BOOL)lossless;
- (void)cleanUp;

- (void)process;
- (int)convert:(void *)dest amount:(int)amount;

- (void)setRGInfo:(NSDictionary *)rgi;

- (void)setOutputFormat:(AudioStreamBasicDescription)format outputConfig:(uint32_t)outputConfig;

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format inputConfig:(uint32_t)inputConfig;

- (void)refreshVolumeScaling;

@end
