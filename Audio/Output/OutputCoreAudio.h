//
//  OutputCoreAudio.h
//  Cog
//
//  Created by Christopher Snowhill on 7/25/23.
//  Copyright 2023-2024 Christopher Snowhill. All rights reserved.
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

#import <simd/simd.h>

#import <CogAudio/ChunkList.h>

#import <CogAudio/Node.h>

#import <CogAudio/DSPFaderNode.h>
#import <CogAudio/DSPDownmixNode.h>
#import <CogAudio/DSPHRTFNode.h>

#import <CogAudio/SimpleBuffer.h>

//#define OUTPUT_LOG

@class OutputNode;

@class AudioChunk;

@interface OutputCoreAudio : Node {
	OutputNode *outputController;

	NSLock *outputLock;

	double streamTimestamp;

	BOOL stopInvoked;
	BOOL stopCompleted;
	BOOL running;
	BOOL stopping;
	BOOL stopped;
	BOOL started;
	BOOL paused;
	BOOL restarted;
	BOOL commandStop;
	BOOL resetting;

	BOOL cutOffInput;
	BOOL fading, faded, fadingstop;
	float fadeLevel;
	float fadeStep;
	float fadeTarget;

	BOOL prebufferReached;
	BOOL prebufferSignaled;

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

	uint32_t deviceChannelConfig;
	uint32_t realStreamChannelConfig;
	uint32_t streamChannelConfig;

	AUAudioUnit *_au;

	size_t _bufferSize;

	BOOL resetStreamFormat;
	
	BOOL shouldPlayOutBuffer;

	BOOL DSPsLaunched;
	DSPHRTFNode *hrtfNode;
	DSPDownmixNode *downmixNode;
	DSPFaderNode *faderNode;

	SimpleBuffer *bufferNode;

#ifdef OUTPUT_LOG
	NSFileHandle *_logFile;
#endif
}

- (id)initWithController:(OutputNode *)c;

- (BOOL)setup;
- (OSStatus)setOutputDeviceByID:(int)deviceID;
- (BOOL)setOutputDeviceWithDeviceDict:(NSDictionary *)deviceDict;
- (void)start;
- (void)pause;
- (void)resume;
- (void)stop;

- (void)fadeOut;
- (void)fadeOutBackground;
- (void)fadeIn;
- (void)faderFadeIn;

- (double)latency;

- (double)volume;
- (void)setVolume:(double)v;

- (void)setShouldPlayOutBuffer:(BOOL)enabled;

- (void)sustainHDCD;

- (AudioStreamBasicDescription)deviceFormat;
- (uint32_t)deviceChannelConfig;

- (DSPDownmixNode *)downmix;
- (DSPFaderNode *)fader;

@end
