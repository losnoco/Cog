//
//  DSPFSurroundNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/11/25.
//

#import <Foundation/Foundation.h>

#import <Accelerate/Accelerate.h>

#import "DSPFSurroundNode.h"

#import "Logging.h"

#import "FSurroundFilter.h"

#define OCTAVES 5

static void * kDSPFSurroundNodeContext = &kDSPFSurroundNodeContext;

@implementation DSPFSurroundNode {
	BOOL enableFSurround;
	BOOL FSurroundDelayRemoved;
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
	DLog(@"FreeSurround dealloc");
	[self cleanUp];
	[self removeObservers];
	[super cleanUp];
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
		usleep(500);
	}
	[self fullShutdown];
}

- (void)resetBuffer {
	paused = YES;
	while(processEntered) {
		usleep(500);
	}
	[buffer reset];
	[self fullShutdown];
	paused = NO;
}

- (BOOL)paused {
	return paused;
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
				if([previousNode endOfStream] == YES) {
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
			if(!enableFSurround && fsurround) {
				[self fullShutdown];
			}
		}
	}
	endOfStream = YES;
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

	if((enableFSurround && !fsurround) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(enableFSurround && ![self setup]) {
			processEntered = NO;
			return nil;
		}
	}

	if(!fsurround) {
		processEntered = NO;
		return [self readChunk:4096];
	}

	size_t totalRequestedSamples = 4096;

	size_t totalFrameCount = 0;
	AudioChunk *chunk = [self readAndMergeChunksAsFloat32:totalRequestedSamples];
	if(!chunk || ![chunk frameCount]) {
		processEntered = NO;
		return nil;
	}

	double streamTimestamp = [chunk streamTimestamp];

	float *samplePtr = &inBuffer[0];

	size_t frameCount = [chunk frameCount];
	NSData *sampleData = [chunk removeSamples:frameCount];

	cblas_scopy((int)frameCount * 2, [sampleData bytes], 1, &samplePtr[0], 1);

	totalFrameCount = frameCount;

	size_t countToProcess = totalFrameCount;
	size_t samplesRendered;
	if(countToProcess < 4096) {
		bzero(&inBuffer[countToProcess * 2], (4096 - countToProcess) * 2 * sizeof(float));
		countToProcess = 4096;
	}

	[fsurround process:&inBuffer[0] output:&outBuffer[0] count:(int)countToProcess];
	samplePtr = &outBuffer[0];
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
		if([chunk isHDCD]) [outputChunk setHDCD];
		[outputChunk setStreamTimestamp:streamTimestamp];
		[outputChunk setStreamTimeRatio:[chunk streamTimeRatio]];
		[outputChunk assignSamples:samplePtr frameCount:samplesRendered];
	}

	processEntered = NO;
	return outputChunk;
}

@end
