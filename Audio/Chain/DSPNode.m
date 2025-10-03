//
//  DSPNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/10/25.
//

#import <Foundation/Foundation.h>

#import "DSPNode.h"

@implementation DSPNode {
	BOOL threadTerminated;
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super init];
	if(self) {
		buffer = [[ChunkList alloc] initWithMaximumDuration:latency];

		writeSemaphore = [Semaphore new];
		readSemaphore = [Semaphore new];

		accessLock = [NSLock new];

		initialBufferFilled = NO;

		controller = c;
		endOfStream = NO;
		shouldContinue = YES;

		nodeChannelConfig = 0;
		nodeLossless = NO;

		durationPrebuffer = latency * 0.25;

		inWrite = NO;
		inPeek = NO;
		inRead = NO;
		inMerge = NO;

		[self setPreviousNode:p];

#ifdef LOG_CHAINS
		[self initLogFiles];
#endif
	}

	return self;
}

// DSP threads buffer for low latency, and therefore should have high priority
- (void)threadEntry:(id _Nullable)arg {
	@autoreleasepool {
		NSThread *currentThread = [NSThread currentThread];
		[currentThread setThreadPriority:0.75];
		[currentThread setQualityOfService:NSQualityOfServiceUserInitiated];
		threadTerminated = NO;
		[self process];
		threadTerminated = YES;
	}
}

- (void)setShouldContinue:(BOOL)s {
	BOOL currentShouldContinue = shouldContinue;
	shouldContinue = s;
	if(!currentShouldContinue && s && threadTerminated) {
		[self launchThread];
	}
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

@end
