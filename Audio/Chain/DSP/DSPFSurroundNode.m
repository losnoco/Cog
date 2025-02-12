//
//  DSPFSurroundNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/11/25.
//

#import <Foundation/Foundation.h>

#import <Accelerate/Accelerate.h>

#import "DSPFSurroundNode.h"

#import "FSurroundFilter.h"

#define OCTAVES 5

static void * kDSPFSurroundNodeContext = &kDSPFSurroundNodeContext;

@implementation DSPFSurroundNode {
	BOOL enableFSurround;
	BOOL FSurroundDelayRemoved;
	BOOL resetStreamFormat;
	FSurroundFilter *fsurround;
	
	BOOL stopping, paused;
	BOOL processEntered;
	
	BOOL observersapplied;
	
	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;
	
	uint32_t lastInputChannelConfig, inputChannelConfig;
	uint32_t outputChannelConfig;

	float inBuffer[4096 * 2];
	float outBuffer[8192 * 6];
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableFSurround = [defaults boolForKey:@"enableFSurround"];

		[self addObservers];
	}
	return self;
}

- (void)dealloc {
	[self cleanUp];
	[self removeObservers];
}

- (void)addObservers {
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableFSurround" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPFSurroundNodeContext];

	observersapplied = YES;
}

- (void)removeObservers {
	if(observersapplied) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.enableFSurround" context:kDSPFSurroundNodeContext];
		observersapplied = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kDSPFSurroundNodeContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.enableFSurround"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableFSurround = [defaults boolForKey:@"enableFSurround"];
	}
}

- (BOOL)fullInit {
	if(enableFSurround && inputFormat.mChannelsPerFrame == 2) {
		fsurround = [[FSurroundFilter alloc] initWithSampleRate:inputFormat.mSampleRate];
		if(!fsurround) {
			return NO;
		}
		outputFormat = inputFormat;
		outputFormat.mChannelsPerFrame = [fsurround channelCount];
		outputFormat.mBytesPerFrame = sizeof(float) * outputFormat.mChannelsPerFrame;
		outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame * outputFormat.mFramesPerPacket;
		outputChannelConfig = [fsurround channelConfig];

		FSurroundDelayRemoved = NO;
		resetStreamFormat = YES;
	} else {
		fsurround = nil;
	}

	return YES;
}

- (void)fullShutdown {
	fsurround = nil;
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
		usleep(1000);
	}
	[self fullShutdown];
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

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk) {
				if([self endOfStream] == YES) {
					break;
				}
				if(paused) {
					continue;
				}
			} else {
				[self writeChunk:chunk];
				chunk = nil;
			}
			if(!enableFSurround && fsurround) {
				[self fullShutdown];
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

	double streamTimestamp;
	double streamTimeRatio;
	if(![self peekTimestamp:&streamTimestamp timeRatio:&streamTimeRatio]) {
		processEntered = NO;
		return nil;
	}

	if((enableFSurround && !fsurround) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(![self setup]) {
			processEntered = NO;
			return nil;
		}
	}

	if(!fsurround) {
		processEntered = NO;
		return [self readChunk:4096];
	}

	size_t totalRequestedSamples = resetStreamFormat ? 2048 : 4096;

	size_t totalFrameCount = 0;
	AudioChunk *chunk;

	float *samplePtr = resetStreamFormat ? &inBuffer[2048 * 2] : &inBuffer[0];

	BOOL isHDCD = NO;

	while(!stopping && totalFrameCount < totalRequestedSamples) {
		AudioStreamBasicDescription newInputFormat;
		uint32_t newChannelConfig;
		if(![self peekFormat:&newInputFormat channelConfig:&newChannelConfig] ||
		   memcmp(&newInputFormat, &inputFormat, sizeof(newInputFormat)) != 0 ||
		   newChannelConfig != inputChannelConfig) {
			break;
		}

		chunk = [self readChunkAsFloat32:totalRequestedSamples - totalFrameCount];
		if(!chunk) {
			break;
		}

		if([chunk isHDCD]) {
			isHDCD = YES;
		}

		size_t frameCount = [chunk frameCount];
		NSData *sampleData = [chunk removeSamples:frameCount];

		cblas_scopy((int)frameCount * 2, [sampleData bytes], 1, &samplePtr[totalFrameCount * 2], 1);

		totalFrameCount += frameCount;
	}

	if(!totalFrameCount) {
		processEntered = NO;
		return nil;
	}

	if(resetStreamFormat) {
		bzero(&inBuffer[0], 2048 * 2 * sizeof(float));
		totalFrameCount += 2048;
		resetStreamFormat = NO;
	}
	
	size_t countToProcess = totalFrameCount;
	size_t samplesRendered;
	if(countToProcess < 4096) {
		bzero(&inBuffer[countToProcess * 2], (4096 - countToProcess) * 2 * sizeof(float));
		countToProcess = 4096;
	}

	[fsurround process:&inBuffer[0] output:&outBuffer[4096 * 6] count:(int)countToProcess];
	samplePtr = &outBuffer[4096 * 6];
	samplesRendered = totalFrameCount;

	if(totalFrameCount < 4096) {
		bzero(&outBuffer[4096 * 6], 4096 * 2 * sizeof(float));
		[fsurround process:&outBuffer[4096 * 6] output:&outBuffer[4096 * 6] count:4096];
		samplesRendered += 2048;
	}

	if(!FSurroundDelayRemoved) {
		FSurroundDelayRemoved = YES;
		if(samplesRendered > 2048) {
			samplePtr += 2048 * 6;
			samplesRendered -= 2048;
		}
	}

	AudioChunk *outputChunk = nil;
	if(samplesRendered) {
		outputChunk = [[AudioChunk alloc] init];
		[outputChunk setFormat:outputFormat];
		if(outputChannelConfig) {
			[outputChunk setChannelConfig:outputChannelConfig];
		}
		if(isHDCD) [outputChunk setHDCD];
		[outputChunk setStreamTimestamp:streamTimestamp];
		[outputChunk setStreamTimeRatio:streamTimeRatio];
		[outputChunk assignSamples:samplePtr frameCount:samplesRendered];
	}

	processEntered = NO;
	return outputChunk;
}

@end
