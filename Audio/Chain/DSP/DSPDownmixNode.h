//
//  DSPDownmixNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/13/25.
//

#ifndef DSPDownmixNode_h
#define DSPDownmixNode_h

#import <AudioToolbox/AudioToolbox.h>

#import <CogAudio/DSPNode.h>

@interface DSPDownmixNode : DSPNode {
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (BOOL)setup;
- (void)cleanUp;

- (void)resetBuffer;

- (BOOL)paused;

- (void)process;
- (AudioChunk * _Nullable)convert;

- (void)setOutputFormat:(AudioStreamBasicDescription)format withChannelConfig:(uint32_t)config;

@end

#endif /* DSPDownmixNode_h */
