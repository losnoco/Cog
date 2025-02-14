//
//  DSPNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/10/25.
//

#import <Foundation/Foundation.h>

#import "DSPNode.h"

@implementation DSPNode

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super init];
	if(self) {
		buffer = [[ChunkList alloc] initWithMaximumDuration:latency];

		writeSemaphore = [[Semaphore alloc] init];
		readSemaphore = [[Semaphore alloc] init];

		accessLock = [[NSLock alloc] init];

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
	}

	return self;
}

// DSP threads buffer for low latency, and therefore should have high priority
- (void)threadEntry:(id _Nullable)arg {
	@autoreleasepool {
		NSThread *currentThread = [NSThread currentThread];
		[currentThread setThreadPriority:0.75];
		[currentThread setQualityOfService:NSQualityOfServiceUserInitiated];
		[self process];
	}
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

@end
