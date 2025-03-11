//
//  VisualizationNode.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/12/25.
//

#ifndef VisualizationNode_h
#define VisualizationNode_h

#import <CogAudio/Node.h>

@interface VisualizationNode : Node {
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency;

- (void)threadEntry:(id _Nullable)arg;

- (BOOL)setup;
- (void)cleanUp;

- (BOOL)paused;

- (void)resetBuffer;

- (void)setShouldContinue:(BOOL)s;

- (void)process;

- (double)secondsBuffered;

@end

#endif /* VisualizationNode_h */
