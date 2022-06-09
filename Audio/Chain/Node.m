//
//  Node.m
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "Node.h"

#import "BufferChain.h"
#import "Logging.h"

#import "OutputCoreAudio.h"

#import <mach/mach_time.h>

// This workgroup attribute isn't currently used. Set it to NULL.
static os_workgroup_attr_t _Nullable attr = nil;

// One nanosecond in seconds.
static const double kOneNanosecond = 1.0e9;

// The I/O interval time in seconds.
static const double kIOIntervalTime = 0.020;

// The clock identifier that specifies interval timestamps.
static const os_clockid_t clockId = OS_CLOCK_MACH_ABSOLUTE_TIME;

@implementation Node

- (id)initWithController:(id)c previous:(id)p {
	self = [super init];
	if(self) {
		buffer = [[ChunkList alloc] initWithMaximumDuration:3.0];
		semaphore = [[Semaphore alloc] init];

		accessLock = [[NSRecursiveLock alloc] init];

		initialBufferFilled = NO;

		controller = c;
		endOfStream = NO;
		shouldContinue = YES;

		nodeChannelConfig = 0;
		nodeLossless = NO;

		if(@available(macOS 11, *)) {
			workgroup = AudioWorkIntervalCreate("Node Work Interval", clockId, attr);

			// Get the mach time info.
			struct mach_timebase_info timeBaseInfo;
			mach_timebase_info(&timeBaseInfo);

			// The frequency of the clock is: (timeBaseInfo.denom / timeBaseInfo.numer) * kOneNanosecond
			const double nanoSecFrequency = (double)(timeBaseInfo.denom) / (double)(timeBaseInfo.numer);
			const double frequency = nanoSecFrequency * kOneNanosecond;

			// Convert the interval time in seconds to mach time length.
			intervalMachLength = (int64_t)(kIOIntervalTime * frequency);
		}

		[self setPreviousNode:p];
	}

	return self;
}

- (AudioStreamBasicDescription)nodeFormat {
	return nodeFormat;
}

- (uint32_t)nodeChannelConfig {
	return nodeChannelConfig;
}

- (BOOL)nodeLossless {
	return nodeLossless;
}

- (void)writeData:(const void *)ptr amount:(size_t)amount {
	[accessLock lock];

	AudioChunk *chunk = [[AudioChunk alloc] init];
	[chunk setFormat:nodeFormat];
	if(nodeChannelConfig) {
		[chunk setChannelConfig:nodeChannelConfig];
	}
	[chunk setLossless:nodeLossless];
	[chunk assignSamples:ptr frameCount:amount / nodeFormat.mBytesPerPacket];

	const double chunkDuration = [chunk duration];
	double durationLeft = [buffer maxDuration] - [buffer listDuration];

	while(shouldContinue == YES && chunkDuration > durationLeft) {
		if(durationLeft < chunkDuration) {
			if(initialBufferFilled == NO) {
				initialBufferFilled = YES;
				if([controller respondsToSelector:@selector(initialBufferFilled:)])
					[controller performSelector:@selector(initialBufferFilled:) withObject:self];
			}
		}

		if(durationLeft < chunkDuration || shouldReset) {
			[accessLock unlock];
			[semaphore wait];
			[accessLock lock];
		}

		durationLeft = [buffer maxDuration] - [buffer listDuration];
	}

	[buffer addChunk:chunk];

	[accessLock unlock];
}

// Should be overwriten by subclass.
- (void)process {
}

- (void)threadEntry:(id)arg {
	@autoreleasepool {
		[self followWorkgroup];
		[self process];
		[self leaveWorkgroup];
	}
}

- (void)followWorkgroup {
	if(@available(macOS 11, *)) {
		if(!wg) {
			wg = workgroup;
			if(wg) {
				int result = os_workgroup_join(wg, &wgToken);
				if(result == 0) return;
				if(result == EALREADY) {
					DLog(@"Thread already in workgroup");
				} else {
					DLog(@"Cannot join workgroup, error %d", result);
				}
			}
		}
	}
}

- (void)leaveWorkgroup {
	if(@available(macOS 11, *)) {
		if(wg) {
			os_workgroup_leave(wg, &wgToken);
			wg = nil;
		}
	}
}

- (void)startWorkslice {
	if(@available(macOS 11, *)) {
		if(wg) {
			const uint64_t currentTime = mach_absolute_time();
			const uint64_t deadline = currentTime + intervalMachLength;
			int result = os_workgroup_interval_start(wg, currentTime, deadline, nil);
			if(result != 0) {
				DLog(@"Deadline error = %d", result);
			}
		}
	}
}

- (void)endWorkslice {
	if(@available(macOS 11, *)) {
		if(wg) {
			int result = os_workgroup_interval_finish(wg, nil);
			if(result != 0) {
				DLog(@"Deadline end error = %d", result);
			}
		}
	}
}

- (BOOL)peekFormat:(nonnull AudioStreamBasicDescription *)format channelConfig:(nonnull uint32_t *)config {
	[accessLock lock];

	BOOL ret = [[previousNode buffer] peekFormat:format channelConfig:config];

	[accessLock unlock];

	return ret;
}

- (AudioChunk *)readChunk:(size_t)maxFrames {
	[accessLock lock];

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		return [[AudioChunk alloc] init];
	}

	if([previousNode shouldReset] == YES) {
		@autoreleasepool {
			[buffer reset];
		}

		shouldReset = YES;
		[previousNode setShouldReset:NO];

		[[previousNode semaphore] signal];
	}

	AudioChunk *ret;

	@autoreleasepool {
		ret = [[previousNode buffer] removeSamples:maxFrames];
	}

	[accessLock unlock];

	if([ret frameCount]) {
		[[previousNode semaphore] signal];
	}

	return ret;
}

- (void)launchThread {
	[NSThread detachNewThreadSelector:@selector(threadEntry:) toTarget:self withObject:nil];
}

- (void)setPreviousNode:(id)p {
	previousNode = p;
}

- (id)previousNode {
	return previousNode;
}

- (BOOL)shouldContinue {
	return shouldContinue;
}

- (void)setShouldContinue:(BOOL)s {
	shouldContinue = s;
}

- (ChunkList *)buffer {
	return buffer;
}

- (void)resetBuffer {
	shouldReset = YES; // Will reset on next write.
	if(previousNode == nil) {
		@autoreleasepool {
			[accessLock lock];
			[buffer reset];
			[accessLock unlock];
		}
	}
}

- (Semaphore *)semaphore {
	return semaphore;
}

- (BOOL)endOfStream {
	return endOfStream;
}

- (void)setEndOfStream:(BOOL)e {
	endOfStream = e;
}

- (void)setShouldReset:(BOOL)s {
	shouldReset = s;
}
- (BOOL)shouldReset {
	return shouldReset;
}

// Buffering nodes should implement this
- (double)secondsBuffered {
	return 0.0;
}

@end
