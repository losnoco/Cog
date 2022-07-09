//
//  OutputAVFoundation.h
//  Cog
//
//  Created by Christopher Snowhill on 6/23/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

#import <AssertMacros.h>
#import <Cocoa/Cocoa.h>

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreAudio/AudioHardware.h>
#import <CoreAudio/CoreAudioTypes.h>

#ifdef __cplusplus
#import <atomic>
using std::atomic_long;
#else
#import <stdatomic.h>
#endif

#import "Downmix.h"

#import "VisualizationController.h"

#import "HeadphoneFilter.h"

//#define OUTPUT_LOG
#ifdef OUTPUT_LOG
#import <stdio.h>
#endif

@class OutputNode;

@interface OutputAVFoundation : NSObject {
	OutputNode *outputController;

	BOOL r8bDone;
	void *r8bstate, *r8bold;

	void *r8bvis;
	double lastVisRate;

	BOOL stopInvoked;
	BOOL stopCompleted;
	BOOL running;
	BOOL stopping;
	BOOL stopped;
	BOOL started;
	BOOL paused;
	BOOL restarted;
	BOOL commandStop;

	BOOL eqEnabled;
	BOOL eqInitialized;

	BOOL streamFormatStarted;

	double secondsHdcdSustained;

	BOOL defaultdevicelistenerapplied;
	BOOL currentdevicelistenerapplied;
	BOOL devicealivelistenerapplied;
	BOOL observersapplied;
	BOOL outputdevicechanged;

	float volume;
	float eqPreamp;

	AudioDeviceID outputDeviceID;
	AudioStreamBasicDescription realStreamFormat; // stream format pre-hrtf
	AudioStreamBasicDescription streamFormat; // stream format last seen in render callback
	AudioStreamBasicDescription realNewFormat; // in case of resampler flush
	AudioStreamBasicDescription newFormat; // in case of resampler flush

	AudioStreamBasicDescription visFormat; // Mono format for vis

	uint32_t realStreamChannelConfig;
	uint32_t streamChannelConfig;
	uint32_t realNewChannelConfig;
	uint32_t newChannelConfig;

	AVSampleBufferAudioRenderer *audioRenderer;
	AVSampleBufferRenderSynchronizer *renderSynchronizer;

	CMAudioFormatDescriptionRef audioFormatDescription;

	id currentPtsObserver;
	NSLock *currentPtsLock;
	CMTime currentPts, lastPts;
	double secondsLatency;

	CMTime outputPts, trackPts, lastCheckpointPts;
	AudioTimeStamp timeStamp;

	size_t _bufferSize;

	AudioUnit _eq;

	DownmixProcessor *downmixerForVis;

	VisualizationController *visController;

	BOOL enableHrtf;
	HeadphoneFilter *hrtf;

	float inputBuffer[2048 * 32]; // 2048 samples times maximum supported channel count
	float hrtfBuffer[2048 * 2];
	float eqBuffer[2048 * 32];

#ifdef OUTPUT_LOG
	FILE *_logFile;
#endif
}

- (id)initWithController:(OutputNode *)c;

- (BOOL)setup;
- (OSStatus)setOutputDeviceByID:(AudioDeviceID)deviceID;
- (BOOL)setOutputDeviceWithDeviceDict:(NSDictionary *)deviceDict;
- (void)start;
- (void)pause;
- (void)resume;
- (void)stop;

- (double)latency;

- (void)setVolume:(double)v;

- (void)setEqualizerEnabled:(BOOL)enabled;

- (void)sustainHDCD;

@end
