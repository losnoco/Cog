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
#import "OutputCoreAudio.h"

#import "Logging.h"

@implementation OutputNode

- (void)setup {
	amountPlayed = 0.0;
	amountPlayedInterval = 0.0;

	paused = YES;
	started = NO;
	intervalReported = NO;

	output = [[OutputCoreAudio alloc] initWithController:self];

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

- (void)setAmountPlayed:(double)seconds {
	double delta = seconds - amountPlayed;
	if(delta > 0.0 && delta < 5.0) {
		[self incrementAmountPlayed:delta];
	} else if(delta) {
		amountPlayed = seconds;
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
		Node *finalNode = [[controller bufferChain] finalNode];
		[self setPreviousNode:finalNode];

		if(finalNode) {
			AudioChunk *ret = [super readChunk:amount];

			/*	if (n == 0) {
			 DLog(@"Output Buffer dry!");
			 }
			 */
			return ret;
		} else {
			return [[AudioChunk alloc] init];
		}
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

- (AudioStreamBasicDescription)deviceFormat {
	return [output deviceFormat];
}

- (uint32_t)deviceChannelConfig {
	return [output deviceChannelConfig];
}

- (void)setFormat:(AudioStreamBasicDescription *)f channelConfig:(uint32_t)channelConfig {
	if(!shouldContinue) return;

	format = *f;
	config = channelConfig;
	// Calculate a ratio and add to double(seconds) instead, as format may change
	// double oldSampleRatio = sampleRatio;
	AudioPlayer *audioPlayer = controller;
	BufferChain *bufferChain = [audioPlayer bufferChain];
	if(bufferChain) {
		ConverterNode *converter = [bufferChain converter];
		DSPDownmixNode *downmix = [bufferChain downmix];
		AudioStreamBasicDescription outputFormat;
		uint32_t outputChannelConfig;
		BOOL formatChanged = NO;
		if(converter) {
			AudioStreamBasicDescription converterFormat = [converter nodeFormat];
			if(memcmp(&converterFormat, &format, sizeof(converterFormat)) != 0) {
				formatChanged = YES;
			}
		}
		if(downmix && output && !formatChanged) {
			outputFormat = [output deviceFormat];
			outputChannelConfig = [output deviceChannelConfig];
			AudioStreamBasicDescription currentOutputFormat = [downmix nodeFormat];
			uint32_t currentOutputChannelConfig = [downmix nodeChannelConfig];
			if(memcmp(&currentOutputFormat, &outputFormat, sizeof(currentOutputFormat)) != 0 ||
			   currentOutputChannelConfig != outputChannelConfig) {
				formatChanged = YES;
			}
		}
		if(formatChanged) {
			InputNode *inputNode = [bufferChain inputNode];
			if(converter) {
				[converter setOutputFormat:format];
			}
			if(downmix && output) {
				[downmix setOutputFormat:[output deviceFormat] withChannelConfig:[output deviceChannelConfig]];
			}
			if(inputNode) {
				AudioStreamBasicDescription inputFormat = [inputNode nodeFormat];
				[inputNode seek:(long)(amountPlayed * inputFormat.mSampleRate)];
			}
		}
	}
}

- (void)close {
	[output stop];
	output = nil;
}

- (double)volume {
	return [output volume];
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

- (void)sustainHDCD {
	[output sustainHDCD];
}

- (void)restartPlaybackAtCurrentPosition {
	[controller restartPlaybackAtCurrentPosition];
}

- (double)latency {
	return [output latency];
}

- (double)getTotalLatency {
	return [[controller bufferChain] secondsBuffered] + [output latency];
}

- (double)getPostVisLatency {
	return [[controller bufferChain] getPostVisLatency] + [output latency];
}

@end
