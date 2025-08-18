//
//  FadedBuffer.m
//  CogAudio
//
//  Created by Christopher Snowhill on 8/17/25.
//

#import "FadedBuffer.h"

#import <Accelerate/Accelerate.h>

#import "OutputCoreAudio.h"

float fadeTimeMS = 200.0f;

BOOL fadeAudio(const float *inSamples, float *outSamples, size_t channels, size_t count, float *fadeLevel, float fadeStep, float fadeTarget) {
	float _fadeLevel = *fadeLevel;
	BOOL towardZero = fadeStep < 0.0;
	BOOL stopping = NO;
	size_t maxCount = (size_t)floor(fabs(fadeTarget - _fadeLevel) / fabs(fadeStep));
	if(maxCount) {
		size_t countToDo = MIN(count, maxCount);
		for(size_t i = 0; i < channels; ++i) {
			_fadeLevel = *fadeLevel;
			vDSP_vrampmuladd(&inSamples[i], channels, &_fadeLevel, &fadeStep, &outSamples[i], channels, countToDo);
		}
	}
	if(maxCount <= count) {
		if(!towardZero && maxCount < count) {
			vDSP_vadd(&inSamples[maxCount * channels], 1, &outSamples[maxCount * channels], 1, &outSamples[maxCount * channels], 1, (count - maxCount) * channels);
		}
		stopping = YES;
	}
	*fadeLevel = _fadeLevel;
	return stopping;
}

@implementation FadedBuffer {
	float fadeLevel;
	float fadeStep;
	float fadeTarget;

	ChunkList *lastBuffer;

	NSArray *DSPs;
}

- (id)initWithBuffer:(ChunkList *)buffer withDSPs:(NSArray *)DSPs fadeStart:(float)fadeStart fadeTarget:(float)fadeTarget sampleRate:(double)sampleRate {
	self = [super init];
	if(self) {
		self->buffer = buffer;
		self->DSPs = DSPs;

		writeSemaphore = [[Semaphore alloc] init];
		readSemaphore = [[Semaphore alloc] init];

		accessLock = [[NSLock alloc] init];

		initialBufferFilled = NO;

		controller = self;
		endOfStream = NO;
		shouldContinue = YES;

		nodeChannelConfig = 0;
		nodeLossless = NO;

		durationPrebuffer = 0;

		inWrite = NO;
		inPeek = NO;
		inRead = NO;
		inMerge = NO;

		[self setPreviousNode:nil];

#ifdef LOG_CHAINS
		[self initLogFiles];
#endif

		fadeLevel = fadeStart;
		self->fadeTarget = fadeTarget;
		lastBuffer = buffer;
		const double maxFadeDurationMS = 1000.0 * [buffer listDuration];
		const double fadeDuration = MIN(fadeTimeMS, maxFadeDurationMS);
		fadeStep = ((fadeTarget - fadeLevel) / sampleRate) * (1000.0f / fadeDuration);

		Node *node = DSPs[0];
		[node setPreviousNode:self];
	}
	return self;
}

- (void)dealloc {
	for(Node *node in DSPs) {
		[node setShouldContinue:NO];
	}
}

- (BOOL)mix:(float *)outputBuffer sampleCount:(size_t)samples channelCount:(size_t)channels {
	if(lastBuffer) {
		size_t dspCount = [DSPs count];
		Node *node = DSPs[dspCount - 1];
		AudioChunk * chunk = [[node buffer] removeAndMergeSamples:samples callBlock:^BOOL{
			if(![buffer isEmpty] && fadeStep) return false;
			else return true;
		}];
		if(chunk && [chunk frameCount]) {
			// Will always be input request size or less
			size_t samplesToMix = [chunk frameCount];
			NSData *sampleData = [chunk removeSamples:samplesToMix];
			BOOL stopping = fadeAudio((const float *)[sampleData bytes], outputBuffer, channels, samplesToMix, &fadeLevel, fadeStep, fadeTarget);
			if(stopping) {
				fadeStep = 0;
				fadeLevel = fadeTarget;
			}
			return stopping;
		}
	}
	// No buffer or no chunk, stream ended
	return true;
}

@end
