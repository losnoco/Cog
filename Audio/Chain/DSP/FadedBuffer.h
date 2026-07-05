//
//  FadedBuffer.h
//  CogAudio
//
//  Created by Christopher Snowhill on 8/17/25.
//

#import <Cocoa/Cocoa.h>

#import "ChunkList.h"

#import "Node.h"

NS_ASSUME_NONNULL_BEGIN

extern float fadeTimeMS;

extern BOOL fadeAudio(const float *inSamples, float *outSamples, size_t channels, size_t count, float *fadeLevel, float fadeStep, float fadeTarget);
extern BOOL audioBufferIsDoP(const float *samples, size_t channels, size_t count, uint8_t * _Nullable nextMarker);
extern void fillDoPSilence(float *samples, size_t channels, size_t count, uint8_t *nextMarker);

@interface FadedBuffer : Node

- (id)initWithBuffer:(ChunkList *)buffer withDSPs:(NSArray *)DSPs fadeStart:(float)fadeStart fadeTarget:(float)fadeTarget sampleRate:(double)sampleRate;
- (BOOL)mix:(float *)outputBuffer sampleCount:(size_t)samples channelCount:(size_t)channels;

@end

NS_ASSUME_NONNULL_END
