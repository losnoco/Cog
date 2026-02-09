//
//  OutputNode.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreAudio/AudioHardware.h>

#import <CogAudio/Node.h>
#import <CogAudio/OutputCoreAudio.h>

@interface OutputNode : Node {
	AudioStreamBasicDescription format;
	uint32_t config;

	double amountPlayed;
	double amountPlayedInterval;
	OutputCoreAudio *output;

	BOOL paused;
	BOOL started;
	BOOL intervalReported;
}

- (double)amountPlayed;
- (double)amountPlayedInterval;

- (void)incrementAmountPlayed:(double)seconds;
- (void)setAmountPlayed:(double)seconds;
- (void)resetAmountPlayed;
- (void)resetAmountPlayedInterval;

- (BOOL)selectNextBuffer;
- (void)endOfInputPlayed;

- (BOOL)chainQueueHasTracks;

- (double)secondsBuffered;

- (BOOL)setup;
- (BOOL)setupWithInterval:(BOOL)resumeInterval;
- (void)process;
- (void)close;
- (void)seek:(double)time;

- (void)fadeOut;
- (void)fadeOutBackground;
- (void)fadeIn;
- (void)faderFadeIn;

- (void)timeOut;

- (AudioChunk *)readChunk:(size_t)amount;

- (void)setFormat:(AudioStreamBasicDescription *)f channelConfig:(uint32_t)channelConfig;
- (AudioStreamBasicDescription)format;
- (uint32_t)config;

- (AudioStreamBasicDescription)deviceFormat;
- (uint32_t)deviceChannelConfig;

- (double)volume;
- (void)setVolume:(double)v;

- (void)setShouldContinue:(BOOL)s;

- (void)setShouldPlayOutBuffer:(BOOL)s;

- (void)pause;
- (void)resume;

- (BOOL)isPaused;

- (void)sustainHDCD;

- (void)restartPlaybackAtCurrentPosition;

- (double)latency;
- (double)getVisLatency;
- (double)getTotalLatency;

- (id)controller;

- (id)downmix;

- (void)resetDSPs;

@end
