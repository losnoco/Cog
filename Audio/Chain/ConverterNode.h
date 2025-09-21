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

#import <CogAudio/soxr.h>

#import <CogAudio/Node.h>

@interface ConverterNode : Node {
	NSDictionary *rgInfo;

	soxr_t soxr;

	void *inputBuffer;
	size_t inputBufferSize;
	size_t inpSize, inpOffset;

	double streamTimestamp, streamTimeRatio;

	BOOL stopping;
	BOOL paused;

	BOOL skipResampler;

	unsigned int PRIME_LEN_;
	unsigned int N_samples_to_add_;
	unsigned int N_samples_to_drop_;

	BOOL is_preextrapolated_;
	int is_postextrapolated_;

	int latencyEaten;
	int latencyEatenPost;

	double sampleRatio;

	BOOL observersAdded;

	BOOL resetProcessed;

	float volumeScale;

	void *floatBuffer;
	size_t floatBufferSize;

	void *extrapolateBuffer;
	size_t extrapolateBufferSize;

	BOOL rememberedLossless;

	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription floatFormat;
	AudioStreamBasicDescription outputFormat;

	uint32_t inputChannelConfig;

	BOOL streamFormatChanged;
	AudioStreamBasicDescription newInputFormat;
	uint32_t newInputChannelConfig;
}

@property AudioStreamBasicDescription inputFormat;

- (id)initWithController:(id)c previous:(id)p;

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat withInputConfig:(uint32_t)inputConfig outputFormat:(AudioStreamBasicDescription)outputFormat isLossless:(BOOL)lossless;
- (void)cleanUp;

- (BOOL)paused;

- (void)process;
- (AudioChunk *)convert;

- (void)setRGInfo:(NSDictionary *)rgi;

- (void)setOutputFormat:(AudioStreamBasicDescription)outputFormat;

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format inputConfig:(uint32_t)inputConfig;

- (void)refreshVolumeScaling;

@end
