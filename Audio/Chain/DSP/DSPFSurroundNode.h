//
//  DSPFSurroundNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/11/25.
//

#ifndef DSPFSurroundNode_h
#define DSPFSurroundNode_h

#import "DSPNode.h"

@interface DSPFSurroundNode : DSPNode {
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (BOOL)setup;
- (void)cleanUp;

- (void)resetBuffer;

- (BOOL)paused;

- (void)process;
- (AudioChunk * _Nullable)convert;

@end

#endif /* DSPFSurroundNode_h */
