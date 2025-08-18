//
//  FadedBuffer.h
//  CogAudio
//
//  Created by Christopher Snowhill on 8/17/25.
//

#import <Cocoa/Cocoa.h>

#import "ChunkList.h"

#import "Node.h"

extern float fadeTimeMS;

extern BOOL fadeAudio(const float *inSamples, float *outSamples, size_t channels, size_t count, float *fadeLevel, float fadeStep, float fadeTarget);

@interface FadedBuffer : Node

- (id)initWithBuffer:(ChunkList *)buffer withDSPs:(NSArray *)DSPs fadeStart:(float)fadeStart fadeTarget:(float)fadeTarget sampleRate:(double)sampleRate;
- (BOOL)mix:(float *)outputBuffer sampleCount:(size_t)samples channelCount:(size_t)channels;

@end
