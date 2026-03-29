//
//  DSPEqualizerNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/11/25.
//

#import <Foundation/Foundation.h>

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import <Accelerate/Accelerate.h>

#import "DSPEqualizerNode.h"

#import "OutputNode.h"

#import "Logging.h"

#import "AudioPlayer.h"

extern void scale_by_volume(float *buffer, size_t count, float volume);

static void * kDSPEqualizerNodeContext = &kDSPEqualizerNodeContext;

static const double equalizer_q = 1.4;

static const float apple_equalizer_bands[31] = { 20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1200, 1600, 2000, 2500, 3100, 4000, 5000, 6300, 8000, 10000, 12000, 16000, 20000 };

typedef struct {
	double b0, b1, b2;
	double a1, a2;
} biquadcoefficients;

@implementation DSPEqualizerNode {
	BOOL enableEqualizer;
	BOOL equalizerInitialized;
	BOOL equalizerBegin;

	double equalizerPreamp;

	__weak AudioPlayer *audioPlayer;

	float eqBandGains[31];

	int eqSectionCount;
	biquadcoefficients *coefs;
	vDSP_biquadm_Setup eqSetup;

	BOOL stopping, paused;
	NSRecursiveLock *mutex;

	BOOL observersapplied;

	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;

	uint32_t lastInputChannelConfig, inputChannelConfig;
	uint32_t outputChannelConfig;

	float outBuffer[4096 * 32];
}

static inline void setupOneBand(double frequency, float gainDB, double q, double sampleRate, biquadcoefficients *coefs) {
	if(frequency <= 0.0 || frequency >= sampleRate / 2.0) {
		coefs->b0 = 1.0;
		coefs->b1 = 0.0;
		coefs->b2 = 0.0;
		coefs->a1 = 0.0;
		coefs->a2 = 0.0;
		return;
	}

	double A = pow(10.0, (double)(gainDB) / 40.0);
	double omega = 2.0 * M_PI * frequency / sampleRate;
	double sinW = sin(omega);
	double cosW = cos(omega);
	double alpha = sinW / (2.0 * q);

	double b0 = 1.0 + alpha * A;
	double b1 = -2.0 * cosW;
	double b2 = 1.0 - alpha * A;
	double a0 = 1.0 + alpha / A;
	double a1 = -2.0 * cosW;
	double a2 = 1.0 - alpha / A;

	coefs->b0 = b0 / a0;
	coefs->b1 = b1 / a0;
	coefs->b2 = b2 / a0;
	coefs->a1 = a1 / a0;
	coefs->a2 = a2 / a0;
}

- (void)setupCoefficients {
	if(!equalizerBegin) [mutex lock];

	eqSectionCount = 0;

	int channels = inputFormat.mChannelsPerFrame;

	if(!coefs) {
		coefs = (biquadcoefficients *) calloc(31 * channels, sizeof(*coefs));
		if(!coefs) {
			if(!equalizerBegin) [mutex unlock];
			return;
		}
	}

	for(int i = 0; i < 31; ++i) {
		setupOneBand(apple_equalizer_bands[i], eqBandGains[i], equalizer_q, inputFormat.mSampleRate, &coefs[i * channels]);
		for(int j = 1; j < channels; ++j) {
			memcpy(&coefs[i * channels + j], &coefs[i * channels], sizeof(*coefs));
		}
	}

	eqSectionCount = 31;

	if(!eqSetup) {
		eqSetup = vDSP_biquadm_CreateSetup((const double *)coefs, 31, channels);
		if(!eqSetup) {
			if(!equalizerBegin) [mutex unlock];
			return;
		}
	} else {
		vDSP_biquadm_ResetState(eqSetup);
		vDSP_biquadm_SetCoefficientsDouble(eqSetup, (const double *)coefs, 0, 0, 31, channels);
	}

	if(!equalizerBegin) [mutex unlock];
}

- (void)setBandGain:(float)gainDB forIndex:(int)i {
	if(!equalizerBegin) [mutex lock];
	if(coefs) {
		int channels = inputFormat.mChannelsPerFrame;
		eqBandGains[i] = gainDB;
		setupOneBand(apple_equalizer_bands[i], gainDB, equalizer_q, inputFormat.mSampleRate, &coefs[i * channels]);
		for(int j = 1; j < channels; ++j) {
			memcpy(&coefs[i * channels + j], &coefs[i * channels], sizeof(*coefs));
		}
		if(eqSetup) {
			vDSP_biquadm_ResetState(eqSetup);
			vDSP_biquadm_SetCoefficientsDouble(eqSetup, (const double *)(&coefs[i * channels]), i, 0, 1, channels);
		}
	}
	if(!equalizerBegin) [mutex unlock];
}

- (void)setAllBands:(float *_Nonnull)gainsDB {
	if(!equalizerBegin) [mutex lock];
	if(coefs) {
		memcpy(eqBandGains, gainsDB, sizeof(eqBandGains));
		[self setupCoefficients];
	}
	if(!equalizerBegin) [mutex unlock];
}

- (void)setPreamp:(float)preampDB {
	if(!equalizerBegin) [mutex lock];
	equalizerPreamp = pow(10.0, preampDB / 20.0);
	if(!equalizerBegin) [mutex unlock];
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableEqualizer = [defaults boolForKey:@"GraphicEQenable"];

		float preamp = [defaults floatForKey:@"eqPreamp"];
		equalizerPreamp = pow(10.0, preamp / 20.0);

		OutputNode *outputNode = c;
		audioPlayer = [outputNode controller];

		mutex = [NSRecursiveLock new];

		[self addObservers];
	}
	return self;
}

- (void)dealloc {
	DLog(@"Equalizer dealloc");
	[self setShouldContinue:NO];
	[self cleanUp];
	[self removeObservers];
	[super cleanUp];
}

- (void)addObservers {
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.GraphicEQenable" options:0 context:kDSPEqualizerNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.eqPreamp" options:0 context:kDSPEqualizerNodeContext];
	
	observersapplied = YES;
}

- (void)removeObservers {
	if(observersapplied) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.GraphicEQenable" context:kDSPEqualizerNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.eqPreamp" context:kDSPEqualizerNodeContext];
		observersapplied = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kDSPEqualizerNodeContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.GraphicEQenable"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableEqualizer = [defaults boolForKey:@"GraphicEQenable"];
	} else if([keyPath isEqualToString:@"values.eqPreamp"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		float preamp = [defaults floatForKey:@"eqPreamp"];
		equalizerPreamp = pow(10.0, preamp / 20.0);
	}
}

- (AudioPlayer *)audioPlayer {
	return audioPlayer;
}

- (BOOL)fullInit {
	[mutex lock];
	if(enableEqualizer) {
		[self setupCoefficients];

		if(!coefs || !eqSetup) {
			[mutex unlock];
			return NO;
		}

		equalizerInitialized = YES;

		equalizerBegin = YES;
		[[self audioPlayer] beginEqualizer:(__bridge void *)self];
		equalizerBegin = NO;
	}

	[mutex unlock];

	return YES;
}

- (void)fullShutdown {
	[mutex lock];
	if(coefs) {
		if(equalizerInitialized) {
			[[self audioPlayer] endEqualizer:(__bridge void *)self];
			if(eqSetup) {
				vDSP_biquadm_DestroySetup(eqSetup);
				eqSetup = NULL;
			}
			free(coefs); coefs = NULL;
			equalizerInitialized = NO;
		}
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
	[buffer reset];
	[self fullShutdown];
	paused = NO;
	[mutex unlock];
}

- (BOOL)paused {
	return paused;
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
				if([previousNode endOfStream] == YES) {
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
			if(!enableEqualizer && equalizerInitialized) {
				[self fullShutdown];
			}
		}
	}
}

- (AudioChunk *)convert {
	if(stopping)
		return nil;

	[mutex lock];

	if(stopping || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) || [self shouldContinue] == NO) {
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

	if((enableEqualizer && !equalizerInitialized) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(enableEqualizer && ![self setup]) {
			[mutex unlock];
			return nil;
		}
	}

	if(!equalizerInitialized) {
		[mutex unlock];
		return [self readChunk:4096];
	}

	AudioChunk *chunk = [self readChunkAsFloat32:4096];
	if(!chunk || ![chunk frameCount]) {
		[mutex unlock];
		return nil;
	}

	double streamTimestamp = [chunk streamTimestamp];

	int channels = inputFormat.mChannelsPerFrame;

	size_t frameCount = [chunk frameCount];

	AudioChunk *outputChunk = nil;
	if(frameCount) {
		NSData *sampleData = [chunk removeSamples:frameCount];
		
		const float *inBuffer = (const float *)[sampleData bytes];

		memcpy(outBuffer, inBuffer, frameCount * channels * sizeof(float));

		scale_by_volume(&outBuffer[0], frameCount * channels, equalizerPreamp);

		float * buffers[channels];
		for(int i = 0; i < channels; ++i) {
			buffers[i] = &outBuffer[i];
		}
		vDSP_biquadm(eqSetup, (const float **)buffers, channels, buffers, channels, (vDSP_Length)frameCount);

		outputChunk = [AudioChunk new];
		[outputChunk setFormat:inputFormat];
		if(outputChannelConfig) {
			[outputChunk setChannelConfig:inputChannelConfig];
		}
		if([chunk isHDCD]) [outputChunk setHDCD];
		if(chunk.resetForward) outputChunk.resetForward = YES;
		[outputChunk setStreamTimestamp:streamTimestamp];
		[outputChunk setStreamTimeRatio:[chunk streamTimeRatio]];
		[outputChunk assignSamples:&outBuffer[0] frameCount:frameCount];
	}

	[mutex unlock];
	return outputChunk;
}

@end
