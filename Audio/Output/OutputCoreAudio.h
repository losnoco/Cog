//
//  OutputCoreAudio.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
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

#import "Semaphore.h"

//#define OUTPUT_LOG
#ifdef OUTPUT_LOG
#import <stdio.h>
#endif

@class OutputNode;

@interface OutputCoreAudio : NSObject {
	OutputNode *outputController;

	Semaphore *writeSemaphore;
	Semaphore *readSemaphore;

	BOOL stopInvoked;
	BOOL running;
	BOOL stopping;
	BOOL stopped;
	BOOL started;
	BOOL paused;
	BOOL stopNext;
	BOOL restarted;

	BOOL eqEnabled;

	BOOL streamFormatStarted;

	atomic_long bytesRendered;
	atomic_long bytesHdcdSustained;

	BOOL listenerapplied;
	BOOL observersapplied;

	float volume;
	float eqPreamp;

	AVAudioFormat *_deviceFormat;

	AudioDeviceID outputDeviceID;
	AudioStreamBasicDescription deviceFormat; // info about the default device
	AudioStreamBasicDescription streamFormat; // stream format last seen in render callback

	AudioStreamBasicDescription visFormat; // Mono format for vis

	uint32_t deviceChannelConfig;
	uint32_t streamChannelConfig;

	AUAudioUnit *_au;
	size_t _bufferSize;

	AudioUnit _eq;

	DownmixProcessor *downmixer;
	DownmixProcessor *downmixerForVis;

	VisualizationController *visController;

	os_workgroup_t wg;
	os_workgroup_join_token_s wgToken;

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
