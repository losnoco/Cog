//
//  DSPSignalsmithStretchNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 3/4/26.
//

#ifndef DSPSignalsmithStretchNode_h
#define DSPSignalsmithStretchNode_h

#import <CogAudio/DSPNode.h>

@interface DSPSignalsmithStretchNode : DSPNode {
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

#endif /* DSPSignalsmithStretchNode_h */
