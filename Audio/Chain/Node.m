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

#import <pthread.h>

#import <mach/mach_time.h>

@implementation Node

- (id)initWithController:(id)c previous:(id)p {
	self = [super init];
	if(self) {
		buffer = [[ChunkList alloc] initWithMaximumDuration:10.0];
		writeSemaphore = [[Semaphore alloc] init];
		readSemaphore = [[Semaphore alloc] init];

		accessLock = [[NSLock alloc] init];

		initialBufferFilled = NO;

		controller = c;
		endOfStream = NO;
		shouldContinue = YES;

		nodeChannelConfig = 0;
		nodeLossless = NO;

		durationPrebuffer = 2.0;

		inWrite = NO;
		inPeek = NO;
		inRead = NO;
		inMerge = NO;

		[self setPreviousNode:p];
	}

	return self;
}

- (void)dealloc {
	[self cleanUp];
}

- (void)cleanUp {
	[self setShouldContinue:NO];
	while(inWrite || inPeek || inRead || inMerge) {
		[writeSemaphore signal];
		if(previousNode) {
			[[previousNode readSemaphore] signal];
		}
		usleep(500);
	}
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
	inWrite = YES;
	if(!shouldContinue || [self paused]) {
		inWrite = NO;
		return;
	}

	[accessLock lock];

	AudioChunk *chunk = [[AudioChunk alloc] init];
	[chunk setFormat:nodeFormat];
	if(nodeChannelConfig) {
		[chunk setChannelConfig:nodeChannelConfig];
	}
	[chunk setLossless:nodeLossless];
	[chunk assignSamples:ptr frameCount:amount / nodeFormat.mBytesPerPacket];

	double durationList = [buffer listDuration];
	double durationLeft = [buffer maxDuration] - durationList;

	if(shouldContinue == YES && durationList >= durationPrebuffer) {
		if(initialBufferFilled == NO) {
			initialBufferFilled = YES;
			if([controller respondsToSelector:@selector(initialBufferFilled:)])
				[controller performSelector:@selector(initialBufferFilled:) withObject:self];
		}
	}

	while(shouldContinue == YES && ![self paused] && durationLeft < 0.0) {
		if(durationLeft < 0.0 || shouldReset) {
			[accessLock unlock];
			[writeSemaphore timedWait:2000];
			[accessLock lock];
		}

		durationLeft = [buffer maxDuration] - [buffer listDuration];
	}

	BOOL doSignal = NO;
	if([chunk frameCount]) {
		[buffer addChunk:chunk];
		doSignal = YES;
	}

	[accessLock unlock];

	if(doSignal) {
		[readSemaphore signal];
	}

	inWrite = NO;
}

- (void)writeChunk:(AudioChunk *)chunk {
	inWrite = YES;
	if(!shouldContinue || [self paused]) {
		inWrite = NO;
		return;
	}

	[accessLock lock];

	double durationList = [buffer listDuration];
	double durationLeft = [buffer maxDuration] - durationList;

	if(shouldContinue == YES && durationList >= durationPrebuffer) {
		if(initialBufferFilled == NO) {
			initialBufferFilled = YES;
			if([controller respondsToSelector:@selector(initialBufferFilled:)])
				[controller performSelector:@selector(initialBufferFilled:) withObject:self];
		}
	}

	while(shouldContinue == YES && ![self paused] && durationLeft < 0.0) {
		if(previousNode && [previousNode shouldContinue] == NO) {
			shouldContinue = NO;
			break;
		}

		if(durationLeft < 0.0 || shouldReset) {
			[accessLock unlock];
			[writeSemaphore timedWait:2000];
			[accessLock lock];
		}

		durationLeft = [buffer maxDuration] - [buffer listDuration];
	}

	BOOL doSignal = NO;
	if([chunk frameCount]) {
		[buffer addChunk:chunk];
		doSignal = YES;
	}

	[accessLock unlock];

	if(doSignal) {
		[readSemaphore signal];
	}

	inWrite = NO;
}

// Should be overwriten by subclass.
- (void)process {
}

- (void)threadEntry:(id)arg {
	@autoreleasepool {
		[self process];
	}
}

- (BOOL)peekFormat:(nonnull AudioStreamBasicDescription *)format channelConfig:(nonnull uint32_t *)config {
	inPeek = YES;
	if(!shouldContinue || [self paused]) {
		inPeek = NO;
		return NO;
	}

	[accessLock lock];

	while(shouldContinue && ![self paused] &&
		  [[previousNode buffer] isEmpty] && [previousNode endOfStream] == NO) {
		[accessLock unlock];
		[writeSemaphore signal];
		[[previousNode readSemaphore] timedWait:2000];
		[accessLock lock];
	}

	if(!shouldContinue || [self paused]) {
		[accessLock unlock];
		inPeek = NO;
		return NO;
	}

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		inPeek = NO;
		return NO;
	}

	BOOL ret = [[previousNode buffer] peekFormat:format channelConfig:config];

	[accessLock unlock];

	inPeek = NO;

	return ret;
}

- (BOOL)peekTimestamp:(double *_Nonnull)timestamp timeRatio:(double *_Nonnull)timeRatio {
	inPeek = YES;
	if(!shouldContinue || [self paused]) {
		inPeek = NO;
		return NO;
	}

	[accessLock lock];

	while(shouldContinue && ![self paused] &&
		  [[previousNode buffer] isEmpty] && [previousNode endOfStream] == NO) {
		[accessLock unlock];
		[writeSemaphore signal];
		[[previousNode readSemaphore] timedWait:2000];
		[accessLock lock];
	}

	if(!shouldContinue || [self paused]) {
		[accessLock unlock];
		inPeek = NO;
		return NO;
	}

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		inPeek = NO;
		return NO;
	}

	BOOL ret = [[previousNode buffer] peekTimestamp:timestamp timeRatio:timeRatio];

	[accessLock unlock];

	inPeek = NO;

	return ret;
}

- (AudioChunk *)readChunk:(size_t)maxFrames {
	inRead = YES;
	if(!shouldContinue || [self paused]) {
		inRead = NO;
		return [[AudioChunk alloc] init];
	}

	[accessLock lock];

	while(shouldContinue && ![self paused] &&
		  [[previousNode buffer] isEmpty] && [previousNode endOfStream] == NO) {
		[accessLock unlock];
		[writeSemaphore signal];
		[[previousNode readSemaphore] timedWait:2000];
		[accessLock lock];
		if([previousNode shouldReset] == YES) {
			break;
		}
	}

	if(!shouldContinue || [self paused]) {
		[accessLock unlock];
		inRead = NO;
		return [[AudioChunk alloc] init];
	}

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		inRead = NO;
		return [[AudioChunk alloc] init];
	}

	if([previousNode shouldReset] == YES) {
		@autoreleasepool {
			[buffer reset];
		}

		shouldReset = YES;
		[previousNode setShouldReset:NO];

		[[previousNode writeSemaphore] signal];
	}

	AudioChunk *ret;

	@autoreleasepool {
		ret = [[previousNode buffer] removeSamples:maxFrames];
	}

	[accessLock unlock];

	if([ret frameCount]) {
		[[previousNode writeSemaphore] signal];
	}

	inRead = NO;

	return ret;
}

- (AudioChunk *)readChunkAsFloat32:(size_t)maxFrames {
	inRead = YES;
	if(!shouldContinue || [self paused]) {
		inRead = NO;
		return [[AudioChunk alloc] init];
	}

	[accessLock lock];

	while(shouldContinue && ![self paused] &&
		  [[previousNode buffer] isEmpty] && [previousNode endOfStream] == NO) {
		[accessLock unlock];
		[writeSemaphore signal];
		[[previousNode readSemaphore] timedWait:2000];
		[accessLock lock];
		if([previousNode shouldReset] == YES) {
			break;
		}
	}

	if(!shouldContinue || [self paused]) {
		[accessLock unlock];
		inRead = NO;
		return [[AudioChunk alloc] init];
	}

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		inRead = NO;
		return [[AudioChunk alloc] init];
	}

	if([previousNode shouldReset] == YES) {
		@autoreleasepool {
			[buffer reset];
		}

		shouldReset = YES;
		[previousNode setShouldReset:NO];

		[[previousNode writeSemaphore] signal];
	}

	AudioChunk *ret;

	@autoreleasepool {
		ret = [[previousNode buffer] removeSamplesAsFloat32:maxFrames];
	}

	[accessLock unlock];

	if([ret frameCount]) {
		[[previousNode writeSemaphore] signal];
	}

	inRead = NO;

	return ret;
}

- (AudioChunk *)readAndMergeChunks:(size_t)maxFrames {
	inMerge = YES;
	if(!shouldContinue || [self paused]) {
		inMerge = NO;
		return [[AudioChunk alloc] init];
	}

	[accessLock lock];

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		inMerge = NO;
		return [[AudioChunk alloc] init];
	}

	AudioChunk *ret;

	@autoreleasepool {
		ret = [[previousNode buffer] removeAndMergeSamples:maxFrames callBlock:^BOOL{
			if([previousNode shouldReset] == YES) {
				@autoreleasepool {
					[buffer reset];
				}

				shouldReset = YES;
				[previousNode setShouldReset:NO];
			}

			[accessLock unlock];
			[[previousNode writeSemaphore] signal];
			[[previousNode readSemaphore] timedWait:2000];
			[accessLock lock];

			return !shouldContinue || [self paused] || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES);
		}];
	}

	[accessLock unlock];

	if([ret frameCount]) {
		[[previousNode writeSemaphore] signal];
	}

	inMerge = NO;

	return ret;
}

- (AudioChunk *)readAndMergeChunksAsFloat32:(size_t)maxFrames {
	inMerge = YES;
	if(!shouldContinue || [self paused]) {
		inMerge = NO;
		return [[AudioChunk alloc] init];
	}

	[accessLock lock];

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		inMerge = NO;
		return [[AudioChunk alloc] init];
	}

	AudioChunk *ret;

	@autoreleasepool {
		ret = [[previousNode buffer] removeAndMergeSamplesAsFloat32:maxFrames callBlock:^BOOL{
			if([previousNode shouldReset] == YES) {
				@autoreleasepool {
					[buffer reset];
				}

				shouldReset = YES;
				[previousNode setShouldReset:NO];
			}

			[accessLock unlock];
			[[previousNode writeSemaphore] signal];
			[[previousNode readSemaphore] timedWait:2000];
			[accessLock lock];

			return !shouldContinue || [self paused] || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES);
		}];
	}

	[accessLock unlock];

	if([ret frameCount]) {
		[[previousNode writeSemaphore] signal];
	}

	inMerge = NO;

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

// Implementations should override
- (BOOL)paused {
	return NO;
}

- (Semaphore *)writeSemaphore {
	return writeSemaphore;
}

- (Semaphore *)readSemaphore {
	return readSemaphore;
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
