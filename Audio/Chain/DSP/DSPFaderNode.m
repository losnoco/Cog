//
//  DSPFaderNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 8/15/25.
//

#import <Foundation/Foundation.h>

#import <CogAudio/OutputCoreAudio.h>

#import "Logging.h"

#import "DSPFaderNode.h"

@implementation DSPFaderNode {
	NSLock *fadersLock;
	NSMutableArray<FadedBuffer *> *faders;

	BOOL stopping, paused;
	BOOL formatSet;
	BOOL waitForResetEvent;
	NSRecursiveLock *mutex;

	double timestamp;

	AudioStreamBasicDescription outputFormat;
	uint32_t outputChannelConfig;

	float fadeLevel, fadeStep;

	float inBuffer[512 * 32];
	float outBuffer[512 * 32];
}

@synthesize timestamp;

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		mutex = [[NSRecursiveLock alloc] init];
		fadersLock = [[NSLock alloc] init];
		faders = [[NSMutableArray alloc] init];
		fadeLevel = 1.0;
	}
	return self;
}

- (void)dealloc {
	DLog(@"Downmix dealloc");
	[self setShouldContinue:NO];
	[self cleanUp];
	[super cleanUp];
}

- (void)cleanUp {
	stopping = YES;
	[fadersLock lock];
	[faders removeAllObjects];
	[fadersLock unlock];
	formatSet = NO;
}

- (BOOL)setup {
	return YES;
}

- (void)resetBuffer {
	paused = YES;
	[mutex lock];
	[buffer reset];
	[fadersLock lock];
	[faders removeAllObjects];
	[fadersLock unlock];
	paused = NO;
	waitForResetEvent = NO;
	[mutex unlock];
}

- (void)setOutputFormat:(AudioStreamBasicDescription)format withChannelConfig:(uint32_t)config {
	if(memcmp(&outputFormat, &format, sizeof(outputFormat)) != 0 ||
	   outputChannelConfig != config) {
		if(fadeStep && !formatSet) {
			fadeStep *= 1.0 / format.mSampleRate;
		}
        outputFormat = format;
        outputChannelConfig = config;
        formatSet = YES;
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

	if(stopping || [self shouldContinue] == NO) {
		[mutex unlock];
		return nil;
	}

	AudioStreamBasicDescription format;
	uint32_t channelConfig;
	if(!formatSet && [self peekFormat:&format channelConfig:&channelConfig]) {
		[self setOutputFormat:format withChannelConfig:channelConfig];
	}
		
	BOOL inputRead = YES;
	AudioChunk *chunk = [self readChunkAsFloat32:512];
	size_t frameCount = chunk ? [chunk frameCount] : 0;
	if(waitForResetEvent && frameCount && !chunk.resetForward) {
		frameCount = 0;
	}
	[fadersLock lock];
	size_t count = [faders count];
	[fadersLock unlock];
	if(!frameCount && count && formatSet) {
		chunk = [[AudioChunk alloc] init];
		[chunk setFormat:outputFormat];
		[chunk setChannelConfig:outputChannelConfig];
		bzero(inBuffer, 512 * outputFormat.mBytesPerPacket);
		frameCount = 512;
		inputRead = NO;
	}

	if(!frameCount) {
		return nil;
	}

	if(chunk.resetForward) {
		waitForResetEvent = NO;
		chunk.resetForward = NO;
	}

	if(inputRead) {
		timestamp = chunk.streamTimestamp;
	}

	BOOL fadingOut = NO;
	if(frameCount && (fadeStep || count)) {
		if(inputRead) {
			NSData *sampleData = [chunk removeSamples:frameCount];
			memcpy(inBuffer, [sampleData bytes], frameCount * outputFormat.mBytesPerPacket);
		} else {
			// [chunk removeSamples:frameCount];
			// Only happens above, and since the samples aren't assigned, they don't need to be removed
		}
		float *nextBuffer = inBuffer;
		if(inputRead && fadeStep) {
			bzero(outBuffer, frameCount * outputFormat.mBytesPerPacket);
			BOOL stopping = fadeAudio(inBuffer, outBuffer, outputFormat.mChannelsPerFrame, frameCount, &fadeLevel, fadeStep, 1.0);
			if(stopping) {
				fadeStep = 0;
				fadeLevel = 1.0;
			}
			nextBuffer = outBuffer;
		}
		[fadersLock lock];
		NSArray<FadedBuffer *> *fadersCopy = [faders copy];
		[fadersLock unlock];
		for(FadedBuffer *buffer in fadersCopy) {
			BOOL stopping = [buffer mix:nextBuffer sampleCount:frameCount channelCount:outputFormat.mChannelsPerFrame];
			fadingOut = YES;
			if(stopping) {
				[fadersLock lock];
				[faders removeObject:buffer];
				[fadersLock unlock];
			}
		}
		[chunk assignSamples:nextBuffer frameCount:frameCount];
	}

	if(!inputRead) {
		chunk.streamTimestamp = timestamp;
		chunk.streamTimeRatio = 1.0;
	}
	timestamp += chunk.duration;

	[mutex unlock];
	return (fadingOut || inputRead) ? chunk : nil;
}

- (void)fadeIn {
	fadeLevel = 0.0;
	if(formatSet) {
		fadeStep = (1.0f / outputFormat.mSampleRate) * (1000.0f / fadeTimeMS);
	} else {
		fadeStep = 1000.0f / fadeTimeMS;
	}
	waitForResetEvent = YES;
}

- (float)fadeLevel {
	return fadeLevel;
}

- (void)appendFadeOut:(FadedBuffer *)buffer {
	[fadersLock lock];
	[faders addObject:buffer];
	[fadersLock unlock];
}

@end
