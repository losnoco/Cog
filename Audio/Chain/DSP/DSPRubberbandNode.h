//
//  DSPRubberbandNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/10/25.
//

#ifndef DSPRubberbandNode_h
#define DSPRubberbandNode_h

#import <CogAudio/DSPNode.h>

@interface DSPRubberbandNode : DSPNode {
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (BOOL)setup;
- (void)cleanUp;

- (void)resetBuffer;

- (BOOL)paused;

- (void)process;
- (AudioChunk * _Nullable)convert;

- (double)secondsBuffered;

@end

#endif /* DSPRubberbandNode_h */
