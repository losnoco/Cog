//
//  DSPDownmixNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/13/25.
//

#import <Foundation/Foundation.h>
#import <math.h>

#import "Downmix.h"

#import "Logging.h"
#import "FadedBuffer.h"

#import "DSPDownmixNode.h"

@implementation DSPDownmixNode {
	DownmixProcessor *downmix;

	BOOL stopping, paused;
	BOOL formatSet;

	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;

	uint32_t lastInputChannelConfig, inputChannelConfig;
	uint32_t outputChannelConfig;

	float outBuffer[4096 * 32];
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	return [super initWithController:c previous:p latency:latency];
}

- (void)dealloc {
	DLog(@"Downmix dealloc");
	[self setShouldContinue:NO];
	[self cleanUp];
	[super cleanUp];
}

- (BOOL)fullInit {
	[mutex lock];
	mutexLocked = [NSThread currentThread];
	if(formatSet) {
		downmix = [[DownmixProcessor alloc] initWithInputFormat:inputFormat inputConfig:inputChannelConfig andOutputFormat:outputFormat outputConfig:outputChannelConfig];
		if(!downmix) {
			mutexLocked = nil;
			[mutex unlock];
			return NO;
		}
	}
	mutexLocked = nil;
	[mutex unlock];
	return YES;
}

- (void)fullShutdown {
	[mutex lock];
	mutexLocked = [NSThread currentThread];
	downmix = nil;
	mutexLocked = nil;
	[mutex unlock];
}

- (BOOL)setup {
	if(stopping)
		return NO;
	[self fullShutdown];
	return [self fullInit];
}

- (void)cleanUp {
	stopping = YES;
	[self fullShutdown];
	formatSet = NO;
}

- (void)resetBuffer {
	paused = YES;
	[mutex lock];
	mutexLocked = [NSThread currentThread];
	[buffer reset];
	paused = NO;
	mutexLocked = nil;
	[mutex unlock];
}

- (void)setOutputFormat:(AudioStreamBasicDescription)format withChannelConfig:(uint32_t)config {
	if(memcmp(&outputFormat, &format, sizeof(outputFormat)) != 0 ||
	   outputChannelConfig != config) {
		paused = YES;
		[mutex lock];
		mutexLocked = [NSThread currentThread];
		[buffer reset];
		[self fullShutdown];
		outputFormat = format;
		outputChannelConfig = config;
		formatSet = YES;
		paused = NO;
		mutexLocked = nil;
		[mutex unlock];
	}
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

	[mutex lock];
	mutexLocked = [NSThread currentThread];

	if(stopping || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) || [self shouldContinue] == NO) {
		mutexLocked = nil;
		[mutex unlock];
		return nil;
	}

	if(![self peekFormat:&inputFormat channelConfig:&inputChannelConfig]) {
		mutexLocked = nil;
		[mutex unlock];
		return nil;
	}

	if(!inputFormat.mSampleRate ||
	   !inputFormat.mBitsPerChannel ||
	   !inputFormat.mChannelsPerFrame ||
	   !inputFormat.mBytesPerFrame ||
	   !inputFormat.mFramesPerPacket ||
	   !inputFormat.mBytesPerPacket) {
		mutexLocked = nil;
		[mutex unlock];
		return nil;
	}

	if((formatSet && !downmix) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(formatSet && ![self setup]) {
			mutexLocked = nil;
			[mutex unlock];
			return nil;
		}
	}

	if(!downmix) {
		mutexLocked = nil;
		[mutex unlock];
		return [self readChunk:4096];
	}

	AudioChunk *chunk = [self readChunkAsFloat32:4096];
	if(!chunk || ![chunk frameCount]) {
		mutexLocked = nil;
		[mutex unlock];
		return nil;
	}

	double streamTimestamp = [chunk streamTimestamp];

	size_t frameCount = [chunk frameCount];
	NSData *sampleData = [chunk removeSamples:frameCount];
	const float *inSamples = (const float *)[sampleData bytes];
	uint8_t nextDoPMarker = 0x05;
	if(fabs(inputFormat.mSampleRate - outputFormat.mSampleRate) < 1.0 &&
	   audioBufferIsDoP(inSamples, inputFormat.mChannelsPerFrame, frameCount, &nextDoPMarker)) {
		AudioChunk *outputChunk = [AudioChunk new];
		[outputChunk setFormat:outputFormat];
		if(outputChannelConfig) {
			[outputChunk setChannelConfig:outputChannelConfig];
		}
		if([chunk isHDCD]) [outputChunk setHDCD];
		if(chunk.resetForward) outputChunk.resetForward = YES;
		[outputChunk setStreamTimestamp:streamTimestamp];
		[outputChunk setStreamTimeRatio:[chunk streamTimeRatio]];
		if(inputFormat.mChannelsPerFrame == outputFormat.mChannelsPerFrame &&
		   inputFormat.mBytesPerPacket == outputFormat.mBytesPerPacket) {
			[outputChunk assignData:sampleData];
		} else {
			const size_t inputChannels = inputFormat.mChannelsPerFrame;
			const size_t outputChannels = outputFormat.mChannelsPerFrame;
			const size_t channelsToCopy = MIN(inputChannels, outputChannels);
			const uint8_t firstMarker = (frameCount % 2) ? ((nextDoPMarker == 0x05) ? 0xFA : 0x05) : nextDoPMarker;
			uint8_t marker = firstMarker;
			fillDoPSilence(&outBuffer[0], outputChannels, frameCount, &marker);
			for(size_t frame = 0; frame < frameCount; ++frame) {
				memcpy(&outBuffer[frame * outputChannels], &inSamples[frame * inputChannels], channelsToCopy * sizeof(float));
			}
			[outputChunk assignSamples:&outBuffer[0] frameCount:frameCount];
		}
		mutexLocked = nil;
		[mutex unlock];
		return outputChunk;
	}

	[downmix process:inSamples frameCount:frameCount output:&outBuffer[0]];

	AudioChunk *outputChunk = [AudioChunk new];
	[outputChunk setFormat:outputFormat];
	if(outputChannelConfig) {
		[outputChunk setChannelConfig:outputChannelConfig];
	}
	if([chunk isHDCD]) [outputChunk setHDCD];
	if(chunk.resetForward) outputChunk.resetForward = YES;
	[outputChunk setStreamTimestamp:streamTimestamp];
	[outputChunk setStreamTimeRatio:[chunk streamTimeRatio]];
	[outputChunk assignSamples:&outBuffer[0] frameCount:frameCount];

	mutexLocked = nil;
	[mutex unlock];
	return outputChunk;
}

@end
