//
//  VisualizationController.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Foundation/Foundation.h>

#import <CogAudio/MIDIVisualizationController.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationController : NSObject {
}

+ (VisualizationController *)sharedController;

+ (MIDIVisualizationController *)sharedMIDIController;

- (void)postLatency:(double)latency;
- (double)getLatency;

// monotonically increasing with playback speed, resets to 0 on track change
- (void)postFullLatency:(double)timestamp;
- (double)getFullLatency;

- (UInt64)samplesPosted;

- (void)postSampleRate:(double)sampleRate;
- (void)postVisPCM:(const float *)inPCM amount:(int)amount;
- (double)readSampleRate;
- (void)copyVisPCM:(float *_Nullable)outPCM visFFT:(float *_Nullable)outFFT latencyOffset:(double)latency;

- (void)reset;

@end

NS_ASSUME_NONNULL_END
