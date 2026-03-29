//
//  DSPEqualizerNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/11/25.
//

#ifndef DSPEqualizerNode_h
#define DSPEqualizerNode_h

#import <CogAudio/DSPNode.h>

@interface DSPEqualizerNode : DSPNode

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (BOOL)setup;
- (void)cleanUp;

- (void)resetBuffer;

- (BOOL)paused;

- (void)process;
- (AudioChunk * _Nullable)convert;

- (void)setBandGain:(float)gainDB forIndex:(int)i;
- (void)setAllBands:(float *_Nonnull)gainsDB;
- (void)setPreamp:(float)preampDB;

@end

#endif /* DSPEqualizerNode_h */
