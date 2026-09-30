//
//  DSPEqualizerNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/11/25.
//

#ifndef DSPEqualizerNode_h
#define DSPEqualizerNode_h

#import <CogAudio/DSPNode.h>

/// The 31-band graphic equalizer as the EQ window drives it, whichever
/// audio engine provides it.
@protocol CogEqualizer <NSObject>
- (void)setBandGain:(float)gainDB forIndex:(int)i;
- (void)setAllBands:(float *_Nonnull)gainsDB;
- (void)setPreamp:(float)preampDB;
@end

@interface DSPEqualizerNode : DSPNode <CogEqualizer>

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
