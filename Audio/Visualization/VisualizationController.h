//
//  VisualizationController.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationController : NSObject {
	double sampleRate;
	double latency;
	float *visAudio;
	int visAudioCursor, visAudioSize;
	float *visAudioTemp;
	uint64_t visSamplesPosted;
}

+ (VisualizationController *)sharedController;

- (void)postLatency:(double)latency;

- (UInt64)samplesPosted;

- (void)postSampleRate:(double)sampleRate;
- (void)postVisPCM:(const float *)inPCM amount:(int)amount;
- (double)readSampleRate;
- (void)copyVisPCM:(float *)outPCM visFFT:(float *)outFFT latencyOffset:(double)latency;

@end

NS_ASSUME_NONNULL_END
