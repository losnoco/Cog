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

//#define OUTPUT_LOG
#ifdef OUTPUT_LOG
#import <stdio.h>
#endif

@class OutputNode;

@interface OutputAVFoundation : NSObject {
	OutputNode *outputController;

	BOOL stopInvoked;
	BOOL running;
	BOOL stopping;
	BOOL stopped;
	BOOL started;
	BOOL paused;
	BOOL restarted;

	BOOL eqEnabled;
	BOOL eqInitialized;

	BOOL dontRemix;

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
	AudioStreamBasicDescription streamFormat; // stream format last seen in render callback

	AudioStreamBasicDescription visFormat; // Mono format for vis

	uint32_t deviceChannelConfig;
	uint32_t streamChannelConfig;

	AVSampleBufferAudioRenderer *audioRenderer;
	AVSampleBufferRenderSynchronizer *renderSynchronizer;

	CMAudioFormatDescriptionRef audioFormatDescription;

	AudioChannelLayoutTag streamTag;

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

	float inputBuffer[2048 * 32]; // 2048 samples times maximum supported channel count
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

- (void)setVolume:(double)v;

- (void)setEqualizerEnabled:(BOOL)enabled;

- (void)sustainHDCD;

@end