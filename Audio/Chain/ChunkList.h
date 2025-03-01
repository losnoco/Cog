//
//  ChunkList.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

#import <CogAudio/AudioChunk.h>

#import <CogAudio/CogSemaphore.h>

NS_ASSUME_NONNULL_BEGIN

#define DSD_DECIMATE 1

@interface ChunkList : NSObject {
	NSMutableArray<AudioChunk *> *chunkList;
	double listDuration;
	double listDurationRatioed;
	double maxDuration;

	BOOL inAdder;
	BOOL inRemover;
	BOOL inPeeker;
	BOOL inMerger;
	BOOL inConverter;
	BOOL stopping;
	
	// For format converter
	void *inputBuffer;
	size_t inputBufferSize;

#if DSD_DECIMATE
	void **dsd2pcm;
	size_t dsd2pcmCount;
	int dsd2pcmLatency;
#endif
	
	BOOL halveDSDVolume;
	
	void *hdcd_decoder;

	BOOL formatRead;
	
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription floatFormat;

	uint32_t inputChannelConfig;
	BOOL inputLossless;

	uint8_t *tempData;
	size_t tempDataSize;
}

@property(readonly) double listDuration;
@property(readonly) double listDurationRatioed;
@property(readonly) double maxDuration;

- (id)initWithMaximumDuration:(double)duration;

- (void)reset;

- (BOOL)isEmpty;
- (BOOL)isFull;

- (void)addChunk:(AudioChunk *)chunk;
- (AudioChunk *)removeSamples:(size_t)maxFrameCount;

- (AudioChunk *)removeSamplesAsFloat32:(size_t)maxFrameCount;

- (BOOL)peekFormat:(nonnull AudioStreamBasicDescription *)format channelConfig:(nonnull uint32_t *)config;

- (BOOL)peekTimestamp:(nonnull double *)timestamp timeRatio:(nonnull double *)timeRatio;

// Helpers
- (AudioChunk *)removeAndMergeSamples:(size_t)maxFrameCount callBlock:(BOOL(NS_NOESCAPE ^ _Nonnull)(void))block;
- (AudioChunk *)removeAndMergeSamplesAsFloat32:(size_t)maxFrameCount callBlock:(BOOL(NS_NOESCAPE ^ _Nonnull)(void))block;

@end

NS_ASSUME_NONNULL_END
