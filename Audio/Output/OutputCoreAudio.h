//
//  OutputCoreAudio.h
//  Cog
//
//  Created by Christopher Snowhill on 7/25/23.
//  Copyright 2023 Christopher Snowhill. All rights reserved.
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

#import <simd/simd.h>

#import "HeadphoneFilter.h"

//#define OUTPUT_LOG
#ifdef OUTPUT_LOG
#import <stdio.h>
#endif

@class OutputNode;

@class FSurroundFilter;

@class AudioChunk;

@interface OutputCoreAudio : NSObject {
	OutputNode *outputController;

	dispatch_semaphore_t writeSemaphore;
	dispatch_semaphore_t readSemaphore;

	NSLock *outputLock;

	double secondsLatency;
	double visPushed;

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

	AVAudioFormat *_deviceFormat;

	AudioDeviceID outputDeviceID;
	AudioStreamBasicDescription deviceFormat;
	AudioStreamBasicDescription realStreamFormat; // stream format pre-hrtf
	AudioStreamBasicDescription streamFormat; // stream format last seen in render callback

	AudioStreamBasicDescription visFormat; // Mono format for vis

	uint32_t deviceChannelConfig;
	uint32_t realStreamChannelConfig;
	uint32_t streamChannelConfig;

	AUAudioUnit *_au;

	AudioTimeStamp timeStamp;

	size_t _bufferSize;

	AudioUnit _eq;

	DownmixProcessor *downmixer;
	DownmixProcessor *downmixerForVis;

	VisualizationController *visController;

	BOOL enableHrtf;
	HeadphoneFilter *hrtf;
	
	BOOL enableFSurround;
	BOOL FSurroundDelayRemoved;
	int inputBufferLastTime;
	FSurroundFilter *fsurround;
	
	int inputRemain;
	
	AudioChunk *chunkRemain;
	
	int visResamplerRemain;

	BOOL resetStreamFormat;
	
	BOOL shouldPlayOutBuffer;

	float *samplePtr;
	float tempBuffer[512 * 32];
	float rsInBuffer[8192 * 32];
	float rsTempBuffer[4096 * 32];
	float inputBuffer[4096 * 32]; // 4096 samples times maximum supported channel count
	float fsurroundBuffer[8192 * 6];
	float hrtfBuffer[4096 * 2];
	float eqBuffer[4096 * 32];
	float downmixBuffer[4096 * 8];

	float visAudio[4096];
	float visResamplerInput[8192];
	float visTemp[8192];

	BOOL referenceMatrixSet;
	BOOL rotationMatrixUpdated;
	simd_float4x4 rotationMatrix;
	simd_float4x4 referenceMatrix;

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

- (void)reportMotion:(simd_float4x4)matrix;

@end
