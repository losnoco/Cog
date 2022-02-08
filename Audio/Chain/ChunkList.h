//
//  ChunkList.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

#import "AudioChunk.h"

#import "Semaphore.h"

NS_ASSUME_NONNULL_BEGIN

@interface ChunkList : NSObject {
	NSMutableArray<AudioChunk *> *chunkList;
	double listDuration;
	double maxDuration;

	BOOL inAdder;
	BOOL inRemover;
	BOOL inPeeker;
	BOOL stopping;
}

@property(readonly) double listDuration;
@property(readonly) double maxDuration;

- (id)initWithMaximumDuration:(double)duration;

- (void)reset;

- (BOOL)isEmpty;
- (BOOL)isFull;

- (void)addChunk:(AudioChunk *)chunk;
- (AudioChunk *)removeSamples:(size_t)maxFrameCount;

- (BOOL)peekFormat:(nonnull AudioStreamBasicDescription *)format channelConfig:(nonnull uint32_t *)config;

@end

NS_ASSUME_NONNULL_END
