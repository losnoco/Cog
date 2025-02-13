//
//  DSPRubberbandNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/10/25.
//

#import <Foundation/Foundation.h>

#import <Accelerate/Accelerate.h>

#import "DSPRubberbandNode.h"

#import <rubberband/rubberband-c.h>

static void * kDSPRubberbandNodeContext = &kDSPRubberbandNodeContext;

@implementation DSPRubberbandNode {
	BOOL enableRubberband;

	RubberBandState ts;
	RubberBandOptions tslastoptions, tsnewoptions;
	size_t blockSize, toDrop, samplesBuffered, tschannels;
	BOOL tsapplynewoptions;
	BOOL tsrestartengine;
	double tempo, pitch;
	double lastTempo, lastPitch;
	double stretchIn, stretchOut;

	BOOL stopping, paused;
	BOOL processEntered;

	BOOL observersapplied;

	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;

	uint32_t lastInputChannelConfig, inputChannelConfig;

	float *rsPtrs[32];
	float rsInBuffer[4096 * 32];
	float rsOutBuffer[65536 * 32];
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableRubberband = ![[defaults stringForKey:@"rubberbandEngine"] isEqualToString:@"disabled"];

		pitch = [defaults doubleForKey:@"pitch"];
		tempo = [defaults doubleForKey:@"tempo"];

		lastPitch = pitch;
		lastTempo = tempo;

		[self addObservers];
	}
	return self;
}

- (void)dealloc {
	[self cleanUp];
	[self removeObservers];
}

- (void)addObservers {
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.pitch" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.tempo" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandEngine" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandTransients" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandDetector" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandPhase" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandWindow" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandSmoothing" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandFormant" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandPitch" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.rubberbandChannels" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPRubberbandNodeContext];

	observersapplied = YES;
}

- (void)removeObservers {
	if(observersapplied) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.pitch" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.tempo" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandEngine" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandTransients" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandDetector" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandPhase" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandWindow" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandSmoothing" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandFormant" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandPitch" context:kDSPRubberbandNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.rubberbandChannels" context:kDSPRubberbandNodeContext];
		observersapplied = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kDSPRubberbandNodeContext) {
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
		enableRubberband = ![[defaults stringForKey:@"rubberbandEngine"] isEqualToString:@"disabled"];
		if(enableRubberband && ts) {
			RubberBandOptions options = [self getRubberbandOptions];
			RubberBandOptions changed = options ^ tslastoptions;
			if(changed) {
				BOOL engineR3 = !!(options & RubberBandOptionEngineFiner);
				// Options which require a restart of the engine
				const RubberBandOptions mustRestart = RubberBandOptionEngineFaster | RubberBandOptionEngineFiner | RubberBandOptionWindowStandard | RubberBandOptionWindowShort | RubberBandOptionWindowLong | RubberBandOptionSmoothingOff | RubberBandOptionSmoothingOn | (engineR3 ? RubberBandOptionPitchHighSpeed | RubberBandOptionPitchHighQuality | RubberBandOptionPitchHighConsistency : 0) | RubberBandOptionChannelsApart | RubberBandOptionChannelsTogether;
				if(changed & mustRestart) {
					tsrestartengine = YES;
				} else {
					tsnewoptions = options;
					tsapplynewoptions = YES;
				}
			}
		}
	}
}

- (RubberBandOptions)getRubberbandOptions {
	RubberBandOptions options = RubberBandOptionProcessRealTime;

	NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
	NSString *value = [defaults stringForKey:@"rubberbandEngine"];
	BOOL engineR3 = NO;
	if([value isEqualToString:@"faster"]) {
		options |= RubberBandOptionEngineFaster;
	} else if([value isEqualToString:@"finer"]) {
		options |= RubberBandOptionEngineFiner;
		engineR3 = YES;
	}

	if(!engineR3) {
		value = [defaults stringForKey:@"rubberbandTransients"];
		if([value isEqualToString:@"crisp"]) {
			options |= RubberBandOptionTransientsCrisp;
		} else if([value isEqualToString:@"mixed"]) {
			options |= RubberBandOptionTransientsMixed;
		} else if([value isEqualToString:@"smooth"]) {
			options |= RubberBandOptionTransientsSmooth;
		}

		value = [defaults stringForKey:@"rubberbandDetector"];
		if([value isEqualToString:@"compound"]) {
			options |= RubberBandOptionDetectorCompound;
		} else if([value isEqualToString:@"percussive"]) {
			options |= RubberBandOptionDetectorPercussive;
		} else if([value isEqualToString:@"soft"]) {
			options |= RubberBandOptionDetectorSoft;
		}

		value = [defaults stringForKey:@"rubberbandPhase"];
		if([value isEqualToString:@"laminar"]) {
			options |= RubberBandOptionPhaseLaminar;
		} else if([value isEqualToString:@"independent"]) {
			options |= RubberBandOptionPhaseIndependent;
		}
	}

	value = [defaults stringForKey:@"rubberbandWindow"];
	if([value isEqualToString:@"standard"]) {
		options |= RubberBandOptionWindowStandard;
	} else if([value isEqualToString:@"short"]) {
		options |= RubberBandOptionWindowShort;
	} else if([value isEqualToString:@"long"]) {
		if(engineR3) {
			options |= RubberBandOptionWindowStandard;
		} else {
			options |= RubberBandOptionWindowLong;
		}
	}

	if(!engineR3) {
		value = [defaults stringForKey:@"rubberbandSmoothing"];
		if([value isEqualToString:@"off"]) {
			options |= RubberBandOptionSmoothingOff;
		} else if([value isEqualToString:@"on"]) {
			options |= RubberBandOptionSmoothingOn;
		}
	}

	value = [defaults stringForKey:@"rubberbandFormant"];
	if([value isEqualToString:@"shifted"]) {
		options |= RubberBandOptionFormantShifted;
	} else if([value isEqualToString:@"preserved"]) {
		options |= RubberBandOptionFormantPreserved;
	}

	value = [defaults stringForKey:@"rubberbandPitch"];
	if([value isEqualToString:@"highspeed"]) {
		options |= RubberBandOptionPitchHighSpeed;
	} else if([value isEqualToString:@"highquality"]) {
		options |= RubberBandOptionPitchHighQuality;
	} else if([value isEqualToString:@"highconsistency"]) {
		options |= RubberBandOptionPitchHighConsistency;
	}

	value = [defaults stringForKey:@"rubberbandChannels"];
	if([value isEqualToString:@"apart"]) {
		options |= RubberBandOptionChannelsApart;
	} else if([value isEqualToString:@"together"]) {
		options |= RubberBandOptionChannelsTogether;
	}

	return options;
}

- (BOOL)fullInit {
	RubberBandOptions options = [self getRubberbandOptions];
	tslastoptions = options;
	tschannels = inputFormat.mChannelsPerFrame;
	ts = rubberband_new(inputFormat.mSampleRate, inputFormat.mChannelsPerFrame, options, 1.0 / tempo, pitch);
	if(!ts)
		return NO;

	blockSize = rubberband_get_process_size_limit(ts);
	toDrop = rubberband_get_start_delay(ts);
	samplesBuffered = 0;
	if(blockSize > 4096)
		blockSize = 4096;
	rubberband_set_max_process_size(ts, (unsigned int)blockSize);

	for(size_t i = 0; i < 32; ++i) {
		rsPtrs[i] = &rsInBuffer[4096 * i];
	}

	size_t toPad = rubberband_get_preferred_start_pad(ts);
	if(toPad > 0) {
		for(size_t i = 0; i < inputFormat.mChannelsPerFrame; ++i) {
			memset(rsPtrs[i], 0, 4096 * sizeof(float));
		}
		while(toPad > 0) {
			size_t p = toPad;
			if(p > blockSize) p = blockSize;
			rubberband_process(ts, (const float * const *)rsPtrs, (int)p, false);
			toPad -= p;
		}
	}

	tsapplynewoptions = NO;
	tsrestartengine = NO;

	stretchIn = 0.0;
	stretchOut = 0.0;

	return YES;
}

- (void)partialInit {
	if(stopping || paused || !ts) return;

	processEntered = YES;

	RubberBandOptions changed = tslastoptions ^ tsnewoptions;

	if(changed) {
		tslastoptions = tsnewoptions;

		BOOL engineR3 = !!(tsnewoptions & RubberBandOptionEngineFiner);
		const RubberBandOptions transientsmask = RubberBandOptionTransientsCrisp | RubberBandOptionTransientsMixed | RubberBandOptionTransientsSmooth;
		const RubberBandOptions detectormask = RubberBandOptionDetectorCompound | RubberBandOptionDetectorPercussive | RubberBandOptionDetectorSoft;
		const RubberBandOptions phasemask = RubberBandOptionPhaseLaminar | RubberBandOptionPhaseIndependent;
		const RubberBandOptions formantmask = RubberBandOptionFormantShifted | RubberBandOptionFormantPreserved;
		const RubberBandOptions pitchmask = RubberBandOptionPitchHighSpeed | RubberBandOptionPitchHighQuality | RubberBandOptionPitchHighConsistency;
		if(changed & transientsmask)
			rubberband_set_transients_option(ts, tsnewoptions & transientsmask);
		if(!engineR3) {
			if(changed & detectormask)
				rubberband_set_detector_option(ts, tsnewoptions & detectormask);
			if(changed & phasemask)
				rubberband_set_phase_option(ts, tsnewoptions & phasemask);
		}
		if(changed & formantmask)
			rubberband_set_formant_option(ts, tsnewoptions & formantmask);
		if(!engineR3 && (changed & pitchmask))
			rubberband_set_pitch_option(ts, tsnewoptions & pitchmask);
	}

	if(fabs(pitch - lastPitch) > 1e-5 ||
	   fabs(tempo - lastTempo) > 1e-5) {
		lastPitch = pitch;
		lastTempo = tempo;
		rubberband_set_pitch_scale(ts, pitch);
		rubberband_set_time_ratio(ts, 1.0 / tempo);
	}

	tsapplynewoptions = NO;

	processEntered = NO;
}

- (void)fullShutdown {
	if(ts) {
		rubberband_delete(ts);
		ts = NULL;
	}
}

- (BOOL)setup {
	if(stopping)
		return NO;
	[self fullShutdown];
	return [self fullInit];
}

- (void)cleanUp {
	stopping = YES;
	while(processEntered) {
		usleep(1000);
	}
	[self fullShutdown];
}

- (void)resetBuffer {
	paused = YES;
	while(processEntered) {
		usleep(500);
	}
	[super resetBuffer];
	[self fullShutdown];
	paused = NO;
}

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk) {
				if([self endOfStream] == YES) {
					break;
				}
				if(paused) {
					continue;
				}
			} else {
				[self writeChunk:chunk];
				chunk = nil;
			}
			if(!enableRubberband && ts) {
				[self fullShutdown];
			} else if(tsrestartengine) {
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

	processEntered = YES;

	if(stopping || [self endOfStream] == YES || [self shouldContinue] == NO) {
		processEntered = NO;
		return nil;
	}

	if(![self peekFormat:&inputFormat channelConfig:&inputChannelConfig]) {
		processEntered = NO;
		return nil;
	}

	if((enableRubberband && !ts) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(![self setup]) {
			processEntered = NO;
			return nil;
		}
	}

	if(!ts) {
		processEntered = NO;
		return [self readChunk:4096];
	}

	size_t samplesToProcess = rubberband_get_samples_required(ts);
	if(samplesToProcess > blockSize)
		samplesToProcess = blockSize;

	AudioChunk *chunk = [self readAndMergeChunksAsFloat32:samplesToProcess];
	if(![chunk duration]) {
		processEntered = NO;
		return nil;
	}

	double streamTimestamp = [chunk streamTimestamp];

	int channels = (int)(inputFormat.mChannelsPerFrame);
	size_t frameCount = [chunk frameCount];
	NSData *sampleData = [chunk removeSamples:frameCount];

	for (size_t i = 0; i < channels; ++i) {
		cblas_scopy((int)frameCount, ((const float *)[sampleData bytes]) + i, channels, rsPtrs[i], 1);
	}

	stretchIn += [chunk duration] / tempo;

	bool endOfStream = [[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES;

	int len = (int)frameCount;

	rubberband_process(ts, (const float * const *)rsPtrs, len, endOfStream);

	size_t samplesAvailable;
	if(!stopping && (samplesAvailable = rubberband_available(ts)) > 0) {
		while(!stopping && samplesAvailable > 0) {
			if(toDrop > 0) {
				size_t blockDrop = toDrop;
				if(blockDrop > samplesAvailable) blockDrop = samplesAvailable;
				if(blockDrop > blockSize) blockDrop = blockSize;
				rubberband_retrieve(ts, (float * const *)rsPtrs, (int)blockDrop);
				toDrop -= blockDrop;
				samplesAvailable -= blockDrop;
				continue;
			}
			size_t maxAvailable = 65536 - samplesBuffered;
			size_t samplesOut = samplesAvailable;
			if(samplesOut > maxAvailable) {
				samplesOut = maxAvailable;
				if(!samplesOut) {
					break;
				}
			}
			if(samplesOut > blockSize) samplesOut = blockSize;
			rubberband_retrieve(ts, (float * const *)rsPtrs, (int)samplesOut);
			for(size_t i = 0; i < channels; ++i) {
				cblas_scopy((int)samplesOut, rsPtrs[i], 1, &rsOutBuffer[samplesBuffered * channels + i], channels);
			}
			samplesBuffered += samplesOut;
			samplesAvailable -= samplesOut;
		}
	}

	if(endOfStream) {
		self->endOfStream = YES;
		if(samplesBuffered) {
			double outputDuration = (double)(samplesBuffered) / inputFormat.mSampleRate;
			if(outputDuration + stretchOut > stretchIn) {
				// Seems that Rubber Band over-flushes when it hits end of stream
				samplesBuffered = (size_t)((stretchIn - stretchOut) * inputFormat.mSampleRate);
			}
		}
	}

	AudioChunk *outputChunk = nil;
	if(samplesBuffered) {
		outputChunk = [[AudioChunk alloc] init];
		[outputChunk setFormat:inputFormat];
		if(inputChannelConfig) {
			[outputChunk setChannelConfig:inputChannelConfig];
		}
		if([chunk isHDCD]) [outputChunk setHDCD];
		[outputChunk setStreamTimestamp:streamTimestamp];
		[outputChunk setStreamTimeRatio:[chunk streamTimeRatio] * tempo];
		[outputChunk assignSamples:rsOutBuffer frameCount:samplesBuffered];
		samplesBuffered = 0;
		stretchOut += [outputChunk duration];
	}

	processEntered = NO;
	return outputChunk;
}

@end
