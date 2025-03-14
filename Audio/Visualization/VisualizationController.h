//
//  VisualizationController.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationController : NSObject {
}

+ (VisualizationController *)sharedController;

- (void)postLatency:(double)latency;

- (UInt64)samplesPosted;

- (void)postSampleRate:(double)sampleRate;
- (void)postVisPCM:(const float *)inPCM amount:(int)amount;
- (double)readSampleRate;
- (void)copyVisPCM:(float *_Nullable)outPCM visFFT:(float *_Nullable)outFFT latencyOffset:(double)latency;

- (void)reset;

@end

NS_ASSUME_NONNULL_END
