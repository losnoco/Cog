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

#import "Node.h"
#import "OutputCoreAudio.h"

@interface OutputNode : Node {
	AudioStreamBasicDescription format;
	uint32_t config;

	double amountPlayed;
	OutputCoreAudio *output;

	BOOL paused;
	BOOL started;
}

- (void)beginEqualizer:(AudioUnit)eq;
- (void)refreshEqualizer:(AudioUnit)eq;
- (void)endEqualizer:(AudioUnit)eq;

- (double)amountPlayed;

- (void)incrementAmountPlayed:(double)seconds;
- (void)resetAmountPlayed;

- (void)endOfInputPlayed;

- (BOOL)chainQueueHasTracks;

- (double)secondsBuffered;

- (void)setup;
- (void)process;
- (void)close;
- (void)seek:(double)time;

- (AudioChunk *)readChunk:(size_t)amount;

- (void)setFormat:(AudioStreamBasicDescription *)f channelConfig:(uint32_t)channelConfig;
- (AudioStreamBasicDescription)format;
- (uint32_t)config;

- (void)setVolume:(double)v;

- (void)setShouldContinue:(BOOL)s;

- (void)pause;
- (void)resume;

- (BOOL)isPaused;

- (void)sustainHDCD;

- (void)restartPlaybackAtCurrentPosition;

@end
