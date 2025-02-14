//
//  VisualizationNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/12/25.
//

#ifndef VisualizationNode_h
#define VisualizationNode_h

#import "Node.h"

@interface VisualizationNode : Node {
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (void)threadEntry:(id _Nullable)arg;

- (BOOL)setup;
- (void)cleanUp;

- (BOOL)paused;

- (void)resetBuffer;

- (void)pop;
- (void)replayPreroll;

- (void)process;

- (double)secondsBuffered;

@end

#endif /* VisualizationNode_h */
