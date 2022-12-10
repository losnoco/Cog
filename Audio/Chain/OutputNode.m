//
//  OutputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "OutputNode.h"
#import "AudioPlayer.h"
#import "BufferChain.h"
#import "OutputAVFoundation.h"

#import "Logging.h"

@implementation OutputNode

- (void)setup {
	amountPlayed = 0.0;
	amountPlayedInterval = 0.0;

	paused = YES;
	started = NO;
	intervalReported = NO;

	output = [[OutputAVFoundation alloc] initWithController:self];

	[output setup];
}

- (void)seek:(double)time {
	//	[output pause];
	[self resetBuffer];

	amountPlayed = time;
}

- (void)process {
	paused = NO;
	[output start];
}

- (void)pause {
	paused = YES;
	[output pause];
}

- (void)resume {
	paused = NO;
	[output resume];
}

- (void)incrementAmountPlayed:(double)seconds {
	amountPlayed += seconds;
	amountPlayedInterval += seconds;
	if(!intervalReported && amountPlayedInterval >= 60.0) {
		intervalReported = YES;
		[controller reportPlayCount];
	}
}

- (void)resetAmountPlayed {
	amountPlayed = 0;
}

- (void)resetAmountPlayedInterval {
	amountPlayedInterval = 0;
	intervalReported = NO;
}

- (BOOL)selectNextBuffer {
	return [controller selectNextBuffer];
}

- (void)endOfInputPlayed {
	if(!intervalReported) {
		intervalReported = YES;
		[controller reportPlayCount];
	}
	[controller endOfInputPlayed];
	[self resetAmountPlayedInterval];
}

- (BOOL)chainQueueHasTracks {
	return [controller chainQueueHasTracks];
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

- (AudioChunk *)readChunk:(size_t)amount {
	@autoreleasepool {
		[self setPreviousNode:[[controller bufferChain] finalNode]];

		AudioChunk *ret = [super readChunk:amount];

		/*	if (n == 0) {
		        DLog(@"Output Buffer dry!");
		    }
		*/
		return ret;
	}
}

- (BOOL)peekFormat:(nonnull AudioStreamBasicDescription *)format channelConfig:(nonnull uint32_t *)config {
	@autoreleasepool {
		[self setPreviousNode:[[controller bufferChain] finalNode]];

		return [super peekFormat:format channelConfig:config];
	}
}

- (double)amountPlayed {
	return amountPlayed;
}

- (double)amountPlayedInterval {
	return amountPlayedInterval;
}

- (AudioStreamBasicDescription)format {
	return format;
}

- (uint32_t)config {
	return config;
}

- (void)setFormat:(AudioStreamBasicDescription *)f channelConfig:(uint32_t)channelConfig {
	format = *f;
	config = channelConfig;
	// Calculate a ratio and add to double(seconds) instead, as format may change
	// double oldSampleRatio = sampleRatio;
	BufferChain *bufferChain = [controller bufferChain];
	if(bufferChain) {
		ConverterNode *converter = [bufferChain converter];
		if(converter) {
			// This clears the resampler buffer, but not the input buffer
			// We also have to jump the play position ahead accounting for
			// the data we are flushing
			amountPlayed += [[converter buffer] listDuration];

			AudioStreamBasicDescription inf = [bufferChain inputFormat];
			uint32_t config = [bufferChain inputConfig];

			format.mChannelsPerFrame = inf.mChannelsPerFrame;
			format.mBytesPerFrame = ((inf.mBitsPerChannel + 7) / 8) * format.mChannelsPerFrame;
			format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
			channelConfig = config;

			[converter inputFormatDidChange:[bufferChain inputFormat] inputConfig:[bufferChain inputConfig]];
		}
	}
}

- (void)close {
	[output stop];
	output = nil;
}

- (void)setVolume:(double)v {
	[output setVolume:v];
}

- (void)setShouldContinue:(BOOL)s {
	[super setShouldContinue:s];

	//	if (s == NO)
	//		[output stop];
}

- (void)setShouldPlayOutBuffer:(BOOL)s {
	[output setShouldPlayOutBuffer:s];
}

- (BOOL)isPaused {
	return paused;
}

- (void)beginEqualizer:(AudioUnit)eq {
	[controller beginEqualizer:eq];
}

- (void)refreshEqualizer:(AudioUnit)eq {
	[controller refreshEqualizer:eq];
}

- (void)endEqualizer:(AudioUnit)eq {
	[controller endEqualizer:eq];
}

- (void)sustainHDCD {
	[output sustainHDCD];
}

- (void)restartPlaybackAtCurrentPosition {
	[controller restartPlaybackAtCurrentPosition];
}

- (double)latency {
	return [output latency];
}

@end
