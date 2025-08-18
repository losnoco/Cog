//
//  SimpleBuffer.h
//  CogAudio
//
//  Created by Christopher Snowhill on 8/18/25.
//

#import "DSPNode.h"

@interface SimpleBuffer : DSPNode

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (BOOL)setup;
- (void)cleanUp;

- (void)resetBuffer;

- (BOOL)paused;

- (void)process;
- (AudioChunk * _Nullable)convert;

@end
