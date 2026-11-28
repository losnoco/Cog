//
//  ChunkList.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#import <CoreAudio/CoreAudioTypes.h>
#import <Foundation/Foundation.h>

#import <CogAudio/AudioChunk.h>


NS_ASSUME_NONNULL_BEGIN

@interface ChunkList : NSObject {
	NSMutableArray<AudioChunk *> *chunkList;

	BOOL inAdder;
	BOOL inRemover;
	BOOL inConverter;
	BOOL stopping;
	
	// For format converter
	void *inputBuffer;
	size_t inputBufferSize;

	void **dsd2pcm;
	size_t dsd2pcmCount;
	int dsd2pcmLatency;
	
	BOOL observersRegistered;
	BOOL halveDSDVolume;
	BOOL outputDSDAsDoP;
	BOOL dsdDoPHasPendingFrame;
	uint8_t dsdDoPPendingFrame[32];
	uint8_t dsdDoPMarker;
	
	BOOL enableHDCD;
	void *hdcd_decoder;

	BOOL formatRead;
	
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription floatFormat;

	uint32_t inputChannelConfig;
	BOOL inputLossless;

	uint8_t *tempData;
	size_t tempDataSize;
}

- (id)init;

- (void)reset;

- (BOOL)isEmpty;

- (void)addChunk:(AudioChunk *)chunk;

- (AudioChunk *)removeSamplesAsFloat32:(size_t)maxFrameCount;

- (void)setOutputDSDAsDoP:(BOOL)enabled;

@end

NS_ASSUME_NONNULL_END
