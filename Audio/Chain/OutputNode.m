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

#import "DSPRubberbandNode.h"
#import "DSPFSurroundNode.h"
#import "DSPHRTFNode.h"
#import "DSPEqualizerNode.h"
#import "VisualizationNode.h"
#import "DSPDownmixNode.h"

#import "Logging.h"

@implementation OutputNode {
	BOOL DSPsLaunched;

	Node *previousInput;

	DSPRubberbandNode *rubberbandNode;
	DSPFSurroundNode *fsurroundNode;
	DSPHRTFNode *hrtfNode;
	DSPEqualizerNode *equalizerNode;
	DSPDownmixNode *downmixNode;
	VisualizationNode *visualizationNode;
}

- (void)setup {
	[self setupWithInterval:NO];
}

- (void)setupWithInterval:(BOOL)resumeInterval {
	if(!resumeInterval) {
		amountPlayed = 0.0;
		amountPlayedInterval = 0.0;
		intervalReported = NO;
	}

	paused = YES;
	started = NO;

	output = [[OutputCoreAudio alloc] initWithController:self];

	[output setup];

	if(!DSPsLaunched) {
		rubberbandNode = [[DSPRubberbandNode alloc] initWithController:self previous:nil latency:0.1];
		if(!rubberbandNode) return;
		fsurroundNode = [[DSPFSurroundNode alloc] initWithController:self previous:rubberbandNode latency:0.03];
		if(!fsurroundNode) return;
		equalizerNode = [[DSPEqualizerNode alloc] initWithController:self previous:fsurroundNode latency:0.03];
		if(!equalizerNode) return;
		hrtfNode = [[DSPHRTFNode alloc] initWithController:self previous:equalizerNode latency:0.03];
		if(!hrtfNode) return;
		downmixNode = [[DSPDownmixNode alloc] initWithController:self previous:hrtfNode latency:0.03];
		if(!downmixNode) return;

		// Approximately double the chunk size for Vis at 44100Hz
		visualizationNode = [[VisualizationNode alloc] initWithController:self previous:downmixNode latency:8192.0 / 44100.0];
		if(!visualizationNode) return;

		[self setPreviousNode:visualizationNode];

		DSPsLaunched = YES;

		[self launchDSPs];

		previousInput = nil;
	}
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
	BOOL ret = [controller selectNextBuffer];
	if(!ret) {
		Node *finalNode = [[controller bufferChain] finalNode];
		[rubberbandNode setPreviousNode:finalNode];
		[self reconnectInput];
	}
	return ret;
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

- (NSArray *)DSPs {
	if(DSPsLaunched) {
		return @[rubberbandNode, fsurroundNode, equalizerNode, hrtfNode, downmixNode, visualizationNode];
	} else {
		return @[];
	}
}

- (void)reconnectInput {
	NSArray *DSPs = [self DSPs];

	for (Node *node in DSPs) {
		[node setEndOfStream:NO];
	}
}

- (void)launchDSPs {
	NSArray *DSPs = [self DSPs];

	for (Node *node in DSPs) {
		[node launchThread];
	}
}

- (AudioChunk *)readChunk:(size_t)amount {
	@autoreleasepool {
		Node *finalNode = [[controller bufferChain] finalNode];
		[rubberbandNode setPreviousNode:finalNode];

		if(finalNode) {
			AudioChunk *ret = [super readChunk:amount];

			if((!ret || ![ret frameCount]) && [previousNode endOfStream]) {
				endOfStream = YES;
			}

			return ret;
		} else {
			return [[AudioChunk alloc] init];
		}
	}
}

- (BOOL)peekFormat:(nonnull AudioStreamBasicDescription *)format channelConfig:(nonnull uint32_t *)config {
	@autoreleasepool {
		Node *finalNode = [[controller bufferChain] finalNode];
		[rubberbandNode setPreviousNode:finalNode];

		BOOL ret = [super peekFormat:format channelConfig:config];
		if(!ret && [previousNode endOfStream]) {
			endOfStream = YES;
		}
		return ret;
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
		AudioStreamBasicDescription outputFormat;
		uint32_t outputChannelConfig;
		BOOL formatChanged = NO;
		if(converter) {
			AudioStreamBasicDescription converterFormat = [converter nodeFormat];
			if(memcmp(&converterFormat, &format, sizeof(converterFormat)) != 0) {
				formatChanged = YES;
			}
		}
		if(downmixNode && output && !formatChanged) {
			outputFormat = [output deviceFormat];
			outputChannelConfig = [output deviceChannelConfig];
			AudioStreamBasicDescription currentOutputFormat = [downmixNode nodeFormat];
			uint32_t currentOutputChannelConfig = [downmixNode nodeChannelConfig];
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
			if(downmixNode && output) {
				[downmixNode setOutputFormat:[output deviceFormat] withChannelConfig:[output deviceChannelConfig]];
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
	if(DSPsLaunched) {
		NSArray *DSPs = [self DSPs];
		for(Node *node in DSPs) {
			[node setShouldContinue:NO];
		}
		previousNode = nil;
		visualizationNode = nil;
		downmixNode = nil;
		hrtfNode = nil;
		fsurroundNode = nil;
		rubberbandNode = nil;
		previousInput = nil;
		DSPsLaunched = NO;
	}
}

- (double)volume {
	return [output volume];
}

- (void)setVolume:(double)v {
	[output setVolume:v];
}

- (void)setShouldContinue:(BOOL)s {
	[super setShouldContinue:s];

	NSArray *DSPs = [self DSPs];
	for(Node *node in DSPs) {
		[node setShouldContinue:s];
	}
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
	double latency = 0.0;
	NSArray *DSPs = [self DSPs];
	for(Node *node in DSPs) {
		latency += [node secondsBuffered];
	}
	return [output latency] + latency;
}

- (double)getVisLatency {
	return [output latency] + [visualizationNode secondsBuffered];
}

- (double)getTotalLatency {
	return [[controller bufferChain] secondsBuffered] + [self latency];
}

- (id)controller {
	return controller;
}

- (id)downmix {
	return downmixNode;
}

@end
