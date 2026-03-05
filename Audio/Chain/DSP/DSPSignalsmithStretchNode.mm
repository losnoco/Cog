//
//  DSPSignalsmithStretchNode.m
//  CogAudio
//
//  Created by Christopher Snowhill on 3/4/26.
//

#import <Foundation/Foundation.h>

#import <Accelerate/Accelerate.h>

#import "DSPSignalsmithStretchNode.h"

#import "Logging.h"

#define SIGNALSMITH_USE_ACCELERATE 1
#import <signalsmith-stretch/signalsmith-stretch.h>

static void * kDSPSignalsmithStretchNodeContext = &kDSPSignalsmithStretchNodeContext;

using Stretch = signalsmith::stretch::SignalsmithStretch<float>;

@implementation DSPSignalsmithStretchNode {
	BOOL enableStretch;

	Stretch *ts;
	size_t tschannels;
	double tssamplerate;
	ssize_t samplesBuffered;
	BOOL tsapplynewoptions;
	double tempo, pitch;
	double lastTempo, lastPitch;
	double countIn;
	uint64_t countOut;

	double streamTimestamp;
	double streamTimeRatio;
	BOOL isHDCD;
	BOOL isResetForward;

	BOOL stopping, paused;
	NSRecursiveLock *mutex;

	BOOL flushed;

	BOOL observersapplied;

	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;

	uint32_t lastInputChannelConfig, inputChannelConfig;

	float rsInBuffer[32][4096];
	float rsOutBuffer[32][65536];
	float rsDeinterleaveBuffer[32 * 65536];
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableStretch = [[defaults stringForKey:@"rubberbandEngine"] isEqualToString:@"signalsmith"];

		pitch = [defaults doubleForKey:@"pitch"];
		tempo = [defaults doubleForKey:@"tempo"];

		lastPitch = pitch;
		lastTempo = tempo;

		mutex = [NSRecursiveLock new];

		[self addObservers];
	}
	return self;
}

- (void)dealloc {
	DLog(@"Signalsmith Stretch dealloc");
	[self setShouldContinue:NO];
	[self cleanUp];
	[self removeObservers];
	[super cleanUp];
}

- (void)addObservers {
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.pitch" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPSignalsmithStretchNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.tempo" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPSignalsmithStretchNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandEngine" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPSignalsmithStretchNodeContext];

	observersapplied = YES;
}

- (void)removeObservers {
	if(observersapplied) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.pitch" context:kDSPSignalsmithStretchNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.tempo" context:kDSPSignalsmithStretchNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandEngine" context:kDSPSignalsmithStretchNodeContext];
		observersapplied = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kDSPSignalsmithStretchNodeContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.pitch"] ||
	   [keyPath isEqualToString:@"values.tempo"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		pitch = [defaults doubleForKey:@"pitch"];
		tempo = [defaults doubleForKey:@"tempo"];
		tsapplynewoptions = YES;
	} else if([[keyPath substringToIndex:17] isEqualToString:@"values.rubberband"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableStretch = [[defaults stringForKey:@"rubberbandEngine"] isEqualToString:@"signalsmith"];
	}
}

- (BOOL)fullInit {
	[mutex lock];

	tschannels = inputFormat.mChannelsPerFrame;
	tssamplerate = inputFormat.mSampleRate;

	ts = new Stretch;
	if(!ts) {
		[mutex unlock];
		return NO;
	}

	ts->presetDefault((unsigned int)tschannels, tssamplerate);

	ts->setTransposeFactor(pitch, 8000.0 / tssamplerate);

	samplesBuffered = 0;

	tsapplynewoptions = NO;
	flushed = NO;

	countIn = 0.0;
	countOut = 0;

	[mutex unlock];
	return YES;
}

- (void)partialInit {
	if(stopping || paused || !ts) return;

	[mutex lock];

	if(fabs(pitch - lastPitch) > 1e-5 ||
	   fabs(tempo - lastTempo) > 1e-5) {
		lastPitch = pitch;
		lastTempo = tempo;
		ts->setTransposeFactor(pitch, 8000.0 / tssamplerate);
	}

	tsapplynewoptions = NO;

	[mutex unlock];
}

- (void)fullShutdown {
	[mutex lock];
	if(ts) {
		delete ts;
		ts = NULL;
	}
	[mutex unlock];
}

- (BOOL)setup {
	if(stopping)
		return NO;
	[self fullShutdown];
	return [self fullInit];
}

- (void)cleanUp {
	stopping = YES;
	[self fullShutdown];
}

- (void)resetBuffer {
	paused = YES;
	[mutex lock];
	shouldReset = YES;
	[buffer reset];
	[self fullShutdown];
	paused = NO;
	[mutex unlock];
}

- (BOOL)paused {
	return paused;
}

- (void)setPreviousNode:(id)p {
	if(previousNode != p) {
		paused = YES;
		[mutex lock];
		previousNode = p;
		paused = NO;
		[mutex unlock];
	}
}

- (void)setEndOfStream:(BOOL)e {
	if(endOfStream && !e) {
		paused = YES;
		[self fullShutdown];
	}
	[super setEndOfStream:e];
	flushed = e;
	paused = NO;
}

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused || endOfStream) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk || ![chunk frameCount]) {
				if(!ts) {
					flushed = previousNode && [[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES;
				}
				if(flushed) {
					usleep(500);
					endOfStream = YES;
					continue;
				}
				if(paused) {
					continue;
				}
				usleep(500);
			} else {
				[self writeChunk:chunk];
				chunk = nil;
			}
			if(!enableStretch && ts) {
				[self fullShutdown];
			} else if(tsapplynewoptions) {
				[self partialInit];
			}
		}
	}
}

- (AudioChunk *)convert {
	if(stopping)
		return nil;

	[mutex lock];

	if(stopping || flushed || !previousNode || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) || [self shouldContinue] == NO) {
		[mutex unlock];
		return nil;
	}

	if(![self peekFormat:&inputFormat channelConfig:&inputChannelConfig]) {
		[mutex unlock];
		return nil;
	}

	if(!inputFormat.mSampleRate ||
	   !inputFormat.mBitsPerChannel ||
	   !inputFormat.mChannelsPerFrame ||
	   !inputFormat.mBytesPerFrame ||
	   !inputFormat.mFramesPerPacket ||
	   !inputFormat.mBytesPerPacket) {
		[mutex unlock];
		return nil;
	}

	if((enableStretch && !ts) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(enableStretch && ![self setup]) {
			[mutex unlock];
			return nil;
		}
	}

	if(!ts) {
		[mutex unlock];
		return [self readChunk:4096];
	}

	ssize_t samplesToProcess = 4096;
	ssize_t samplesToOutput = 0;

	int channels = (int)(inputFormat.mChannelsPerFrame);

	if(!flushed && samplesToProcess > 0) {
		if(!countOut) {
			ssize_t seekLength = ts->outputSeekLength(tempo);
			if(seekLength > 65536)
				seekLength = 65536;
			AudioChunk *chunk = [self readAndMergeChunksAsFloat32:seekLength];
			if(!chunk || ![chunk frameCount]) {
				[mutex unlock];
				return nil;
			}

			streamTimestamp = [chunk streamTimestamp];
			streamTimeRatio = [chunk streamTimeRatio];
			isHDCD = isHDCD || [chunk isHDCD];
			isResetForward = isResetForward || chunk.resetForward;

			size_t frameCount = [chunk frameCount];
			countIn += ((double)frameCount) / tempo;

			NSData *sampleData = [chunk removeSamples:frameCount];

			for (size_t i = 0; i < channels; ++i) {
				cblas_scopy((int)frameCount, ((const float *)[sampleData bytes]) + i, channels, rsOutBuffer[i], 1);
			}

			ts->outputSeek(rsOutBuffer, (int)frameCount);
		}

		AudioChunk *chunk = [self readAndMergeChunksAsFloat32:samplesToProcess];
		if(!chunk || ![chunk frameCount]) {
			[mutex unlock];
			return nil;
		}

		if(countOut) {
			streamTimestamp = [chunk streamTimestamp];
			streamTimeRatio = [chunk streamTimeRatio];
		}
		isHDCD = isHDCD || [chunk isHDCD];
		isResetForward = isResetForward || chunk.resetForward;

		size_t frameCount = [chunk frameCount];
		countIn += ((double)frameCount) / tempo;

		NSData *sampleData = [chunk removeSamples:frameCount];

		for (size_t i = 0; i < channels; ++i) {
			cblas_scopy((int)frameCount, ((const float *)[sampleData bytes]) + i, channels, rsInBuffer[i], 1);
		}

		flushed = [[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES;

		samplesToOutput = floor(frameCount / tempo + 0.5);
		if(samplesToOutput > 65536) {
			samplesToProcess = floor(65536.0 * tempo + 0.5);
			if(!samplesToProcess)
				return nil;
			frameCount = (size_t)std::min((ssize_t)frameCount, samplesToProcess);
		}

		ts->process(rsInBuffer, (int)frameCount, rsOutBuffer, (int)samplesToOutput);

		for (size_t i = 0; i < channels; ++i) {
			cblas_scopy((int)samplesToOutput, rsOutBuffer[i], 1, &rsDeinterleaveBuffer[i], channels);
		}

		if(flushed) {
			ssize_t toFlush = ts->outputLatency() + (ssize_t)floor(ts->inputLatency() / tempo + 0.5);
			if(toFlush + samplesToOutput > 65536)
				toFlush = 65536 - samplesToOutput;
			ts->flush(rsOutBuffer, (int)toFlush);
			for (size_t i = 0; i < channels; ++i) {
				cblas_scopy((int)toFlush, rsOutBuffer[i], 1, &rsDeinterleaveBuffer[i + channels * samplesToOutput], channels);
			}
			samplesToOutput += toFlush;
		}

		samplesBuffered = samplesToOutput;
	}

	if(flushed) {
		if(samplesBuffered > 0) {
			ssize_t ideal = (ssize_t)floor(countIn + 0.5);
			if(countOut + samplesBuffered > ideal) {
				samplesBuffered = ideal - countOut;
			}
		}
	}

	AudioChunk *outputChunk = nil;
	if(samplesBuffered > 0) {
		outputChunk = [AudioChunk new];
		[outputChunk setFormat:inputFormat];
		if(inputChannelConfig) {
			[outputChunk setChannelConfig:inputChannelConfig];
		}
		if(isHDCD) {
			[outputChunk setHDCD];
			isHDCD = NO;
		}
		if(isResetForward) {
			outputChunk.resetForward = YES;
			isResetForward = NO;
		}
		[outputChunk setStreamTimestamp:streamTimestamp];
		[outputChunk setStreamTimeRatio:streamTimeRatio * tempo];
		[outputChunk assignSamples:&rsDeinterleaveBuffer[0] frameCount:samplesBuffered];
		countOut += samplesBuffered;
		samplesBuffered = 0;
		double chunkDuration = [outputChunk duration];
		streamTimestamp += chunkDuration * [outputChunk streamTimeRatio];
	}

	[mutex unlock];
	return outputChunk;
}

- (double)secondsBuffered {
	double rbBuffered = 0.0;
	if(ts) {
		rbBuffered = countIn - (double)(countOut);
		if(rbBuffered < 0) {
			rbBuffered = 0.0;
		} else {
			rbBuffered /= inputFormat.mSampleRate;
		}
	}
	return [buffer listDuration] + rbBuffered;
}

@end
