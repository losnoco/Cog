//
//  ChunkList.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#import "ChunkList.h"

@implementation ChunkList

@synthesize listDuration;
@synthesize maxDuration;

- (id)initWithMaximumDuration:(double)duration {
	self = [super init];

	if(self) {
		chunkList = [[NSMutableArray alloc] init];
		listDuration = 0.0;
		maxDuration = duration;

		inAdder = NO;
		inRemover = NO;
		stopping = NO;
	}

	return self;
}

- (void)dealloc {
	stopping = YES;
	while(inAdder || inRemover) {
		usleep(500);
	}
}

- (void)reset {
	@synchronized(chunkList) {
		[chunkList removeAllObjects];
		listDuration = 0.0;
	}
}

- (BOOL)isEmpty {
	@synchronized(chunkList) {
		return [chunkList count] == 0;
	}
}

- (BOOL)isFull {
	return (maxDuration - listDuration) < 0.01;
}

- (void)addChunk:(AudioChunk *)chunk {
	if(stopping) return;

	inAdder = YES;

	const double chunkDuration = [chunk duration];

	@synchronized(chunkList) {
		[chunkList addObject:chunk];
		listDuration += chunkDuration;
	}

	inAdder = NO;
}

- (AudioChunk *)removeSamples:(size_t)maxFrameCount {
	if(stopping) {
		return [[AudioChunk alloc] init];
	}

	@synchronized(chunkList) {
		inRemover = YES;
		if(![chunkList count]) {
			inRemover = NO;
			return [[AudioChunk alloc] init];
		}
		AudioChunk *chunk = [chunkList objectAtIndex:0];
		if([chunk frameCount] <= maxFrameCount) {
			[chunkList removeObjectAtIndex:0];
			listDuration -= [chunk duration];
			inRemover = NO;
			return chunk;
		}
		NSData *removedData = [chunk removeSamples:maxFrameCount];
		AudioChunk *ret = [[AudioChunk alloc] init];
		[ret setFormat:[chunk format]];
		[ret setChannelConfig:[chunk channelConfig]];
		[ret assignSamples:[removedData bytes] frameCount:maxFrameCount];
		listDuration -= [ret duration];
		inRemover = NO;
		return ret;
	}
}

@end
