//
//  DSPHRTFNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/11/25.
//

#ifndef DSPHRTFNode_h
#define DSPHRTFNode_h

#import <simd/types.h>

#import <CogAudio/DSPNode.h>

@interface DSPHRTFNode : DSPNode {
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (BOOL)setup;
- (void)cleanUp;

- (void)resetBuffer;

- (BOOL)paused;

- (void)process;
- (AudioChunk * _Nullable)convert;

- (void)reportMotion:(simd_float4x4)matrix;
- (void)resetReferencePosition:(NSNotification *_Nullable)notification;

@end

#endif /* DSPHRTFNode_h */
