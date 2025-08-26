//
//  DSPFaderNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 8/15/25.
//

#ifndef DSPFaderNode_h
#define DSPFaderNode_h

#import <AudioToolbox/AudioToolbox.h>

#import <CogAudio/DSPNode.h>

#import "FadedBuffer.h"

@interface DSPFaderNode : DSPNode

@property double timestamp;

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (BOOL)setup;
- (void)cleanUp;

- (void)resetBuffer;

- (BOOL)paused;

- (void)process;
- (AudioChunk * _Nullable)convert;

- (void)fadeIn;

- (float)fadeLevel;

- (void)appendFadeOut:(FadedBuffer *_Nonnull)buffer;

- (BOOL)fading;

@end

#endif /* DSPFaderNode_h */
