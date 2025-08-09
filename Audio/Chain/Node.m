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

#ifdef LOG_CHAINS
#import "NSFileHandle+CreateFile.h"

static NSLock * _Node_lock = nil;
static uint64_t _Node_serial;
#endif

@implementation Node

#ifdef LOG_CHAINS
+ (void)initialize {
	@synchronized (_Node_lock) {
		if(!_Node_lock) {
			_Node_lock = [[NSLock alloc] init];
			_Node_serial = 0;
		}
	}
}

- (void)initLogFiles {
	[_Node_lock lock];
	logFileOut = [NSFileHandle fileHandleForWritingAtPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"%@_output_%08lld.raw", [self className], _Node_serial++]] createFile:YES];
	logFileIn = [NSFileHandle fileHandleForWritingAtPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"%@_input_%08lld.raw", [self className], _Node_serial++]] createFile:YES];
	[_Node_lock unlock];
}
#endif

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

#ifdef LOG_CHAINS
		[self initLogFiles];
#endif
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

#ifdef LOG_CHAINS
	if(logFileOut) {
		[logFileOut writeData:[NSData dataWithBytes:ptr length:amount]];
	}
#endif

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
#ifdef LOG_CHAINS
		if(logFileOut) {
			AudioChunk *chunkCopy = [chunk copy];
			size_t frameCount = [chunkCopy frameCount];
			NSData *chunkData = [chunkCopy removeSamples:frameCount];
			[logFileOut writeData:chunkData];
		}
#endif
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

#ifdef LOG_CHAINS
	if(logFileIn) {
		AudioChunk *chunkCopy = [ret copy];
		size_t frameCount = [chunkCopy frameCount];
		NSData *chunkData = [chunkCopy removeSamples:frameCount];
		[logFileIn writeData:chunkData];
	}
#endif

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

#ifdef LOG_CHAINS
	if(logFileIn) {
		AudioChunk *chunkCopy = [ret copy];
		size_t frameCount = [chunkCopy frameCount];
		NSData *chunkData = [chunkCopy removeSamples:frameCount];
		[logFileIn writeData:chunkData];
	}
#endif

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

#ifdef LOG_CHAINS
		if(logFileIn) {
			AudioChunk *chunkCopy = [ret copy];
			size_t frameCount = [chunkCopy frameCount];
			NSData *chunkData = [chunkCopy removeSamples:frameCount];
			[logFileIn writeData:chunkData];
		}
#endif
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

#ifdef LOG_CHAINS
		if(logFileIn) {
			AudioChunk *chunkCopy = [ret copy];
			size_t frameCount = [chunkCopy frameCount];
			NSData *chunkData = [chunkCopy removeSamples:frameCount];
			[logFileIn writeData:chunkData];
		}
#endif
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

- (void)lockedResetBuffer {
	@autoreleasepool {
		[buffer reset];
	}
}

- (void)unlockedResetBuffer {
	@autoreleasepool {
		[accessLock lock];
		[buffer reset];
		[accessLock unlock];
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

// Reset everything in the chain
- (void)resetBackwards {
	[accessLock lock];
	if(buffer) {
		[self lockedResetBuffer];
		[writeSemaphore signal];
		[readSemaphore signal];
	}
	Node *node = previousNode;
	while(node) {
		[node unlockedResetBuffer];
		[node setShouldReset:YES];
		[[node writeSemaphore] signal];
		[[node readSemaphore] signal];
		node = [node previousNode];
	}
	[accessLock unlock];
}

@end
