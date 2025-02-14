//
//  DSPDownmixNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/13/25.
//

#import <Foundation/Foundation.h>

#import "Downmix.h"

#import "Logging.h"

#import "DSPDownmixNode.h"

@implementation DSPDownmixNode {
	DownmixProcessor *downmix;

	BOOL stopping, paused;
	BOOL processEntered;
	BOOL formatSet;

	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;

	uint32_t lastInputChannelConfig, inputChannelConfig;
	uint32_t outputChannelConfig;

	float outBuffer[4096 * 32];
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	return self;
}

- (void)dealloc {
	DLog(@"Downmix dealloc");
	[self cleanUp];
	[super cleanUp];
}

- (BOOL)fullInit {
	if(formatSet) {
		downmix = [[DownmixProcessor alloc] initWithInputFormat:inputFormat inputConfig:inputChannelConfig andOutputFormat:outputFormat outputConfig:outputChannelConfig];
		if(!downmix) {
			return NO;
		}
	}
	return YES;
}

- (void)fullShutdown {
	downmix = nil;
}

- (BOOL)setup {
	if(stopping)
		return NO;
	[self fullShutdown];
	return [self fullInit];
}

- (void)cleanUp {
	stopping = YES;
	while(processEntered) {
		usleep(500);
	}
	[self fullShutdown];
	formatSet = NO;
}

- (void)resetBuffer {
	paused = YES;
	while(processEntered) {
		usleep(500);
	}
	[super resetBuffer];
	[self fullShutdown];
	paused = NO;
}

- (void)setOutputFormat:(AudioStreamBasicDescription)format withChannelConfig:(uint32_t)config {
	outputFormat = format;
	outputChannelConfig = config;
	formatSet = YES;
}

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk || ![chunk frameCount]) {
				if([self endOfStream] == YES) {
					break;
				}
				if(paused) {
					continue;
				}
				usleep(500);
			} else {
				[self writeChunk:chunk];
				chunk = nil;
			}
		}
	}
}

- (AudioChunk *)convert {
	if(stopping)
		return nil;

	processEntered = YES;

	if(stopping || [self endOfStream] == YES || [self shouldContinue] == NO) {
		processEntered = NO;
		return nil;
	}

	if(![self peekFormat:&inputFormat channelConfig:&inputChannelConfig]) {
		processEntered = NO;
		return nil;
	}

	if(!inputFormat.mSampleRate ||
	   !inputFormat.mBitsPerChannel ||
	   !inputFormat.mChannelsPerFrame ||
	   !inputFormat.mBytesPerFrame ||
	   !inputFormat.mFramesPerPacket ||
	   !inputFormat.mBytesPerPacket) {
		processEntered = NO;
		return nil;
	}

	if((formatSet && !downmix) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(formatSet && ![self setup]) {
			processEntered = NO;
			return nil;
		}
	}

	if(!downmix) {
		processEntered = NO;
		return [self readChunk:4096];
	}

	AudioChunk *chunk = [self readChunkAsFloat32:4096];
	if(!chunk || ![chunk frameCount]) {
		processEntered = NO;
		return nil;
	}

	double streamTimestamp = [chunk streamTimestamp];

	size_t frameCount = [chunk frameCount];
	NSData *sampleData = [chunk removeSamples:frameCount];

	[downmix process:[sampleData bytes] frameCount:frameCount output:&outBuffer[0]];

	AudioChunk *outputChunk = [[AudioChunk alloc] init];
	[outputChunk setFormat:outputFormat];
	if(outputChannelConfig) {
		[outputChunk setChannelConfig:outputChannelConfig];
	}
	if([chunk isHDCD]) [outputChunk setHDCD];
	[outputChunk setStreamTimestamp:streamTimestamp];
	[outputChunk setStreamTimeRatio:[chunk streamTimeRatio]];
	[outputChunk assignSamples:&outBuffer[0] frameCount:frameCount];

	processEntered = NO;
	return outputChunk;
}

@end
