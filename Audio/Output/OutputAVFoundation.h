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

#import <CogAudio/CogAudio-Swift.h>

#import "HeadphoneFilter.h"

//#define OUTPUT_LOG
#ifdef OUTPUT_LOG
#import <stdio.h>
#endif

@class OutputNode;

@class FSurroundFilter;

@interface OutputAVFoundation : NSObject {
	OutputNode *outputController;

	BOOL rsDone;
	void *rsstate, *rsold;
	
	double lastClippedSampleRate;

	void *rsvis;
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
	BOOL streamFormatChanged;

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
	
	BOOL enableFSurround;
	BOOL FSurroundDelayRemoved;
	int inputBufferLastTime;
	FSurroundFilter *fsurround;

	BOOL resetStreamFormat;
	
	BOOL shouldPlayOutBuffer;

	float *samplePtr;
	float tempBuffer[512 * 32];
	float rsTempBuffer[4096 * 32];
	float inputBuffer[4096 * 32]; // 4096 samples times maximum supported channel count
	float fsurroundBuffer[8192 * 6];
	float hrtfBuffer[4096 * 2];
	float eqBuffer[4096 * 32];

	float visAudio[4096];
	float visTemp[8192];

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

- (void)setShouldPlayOutBuffer:(BOOL)enabled;

- (void)sustainHDCD;

@end
