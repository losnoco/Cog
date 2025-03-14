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
	[self setShouldContinue:NO];
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
	[buffer reset];
	paused = NO;
}

- (void)setOutputFormat:(AudioStreamBasicDescription)format withChannelConfig:(uint32_t)config {
	if(memcmp(&outputFormat, &format, sizeof(outputFormat)) != 0 ||
	   outputChannelConfig != config) {
		paused = YES;
		while(processEntered) {
			usleep(500);
		}
		[buffer reset];
		[self fullShutdown];
		paused = NO;
	}
	outputFormat = format;
	outputChannelConfig = config;
	formatSet = YES;
}

- (BOOL)paused {
	return paused;
}

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused || endOfStream) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk || ![chunk frameCount]) {
				if([previousNode endOfStream] == YES) {
					usleep(500);
					endOfStream = YES;
					continue;
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

	if(stopping || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) || [self shouldContinue] == NO) {
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
