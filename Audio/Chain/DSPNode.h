//
//  DSPNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/10/25.
//

#ifndef DSPNode_h
#define DSPNode_h

#import <CogAudio/Node.h>

@interface DSPNode : Node {
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (void)threadEntry:(id _Nullable)arg;

- (void)setShouldContinue:(BOOL)s;

- (double)secondsBuffered;

@end

#endif /* DSPNode_h */
