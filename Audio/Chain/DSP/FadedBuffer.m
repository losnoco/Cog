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

static uint8_t doPMarkerForSample(float sample) {
	int32_t packed = (int32_t)llrint((double)sample * 2147483648.0);
	return (uint8_t)(((uint32_t)packed) >> 24);
}

BOOL audioBufferIsDoP(const float *samples, size_t channels, size_t count, uint8_t *nextMarker) {
	if(!samples || !channels || !count) return NO;

	// A DoP frame has the same 0x05/0xFA marker in every channel, and the
	// marker alternates on every frame. Validate the complete buffer: callers use
	// the returned phase to join buffers, so accepting a damaged tail can make a
	// DAC lose DoP lock.
	uint8_t previousMarker = 0;
	for(size_t frame = 0; frame < count; ++frame) {
		const uint8_t marker = doPMarkerForSample(samples[frame * channels]);
		if(marker != 0x05 && marker != 0xFA) return NO;
		if(frame && marker == previousMarker) return NO;
		for(size_t channel = 1; channel < channels; ++channel) {
			if(doPMarkerForSample(samples[frame * channels + channel]) != marker) return NO;
		}
		previousMarker = marker;
	}

	if(nextMarker) {
		*nextMarker = (previousMarker == 0x05) ? 0xFA : 0x05;
	}
	return YES;
}

void fillDoPSilence(float *samples, size_t channels, size_t count, uint8_t *nextMarker) {
	uint8_t marker = (*nextMarker == 0xFA) ? 0xFA : 0x05;
	for(size_t frame = 0; frame < count; ++frame) {
		const uint32_t packed = ((uint32_t)marker << 24) | (0x69U << 16) | (0x69U << 8);
		int32_t signedPacked;
		memcpy(&signedPacked, &packed, sizeof(signedPacked));
		const float silence = (float)((double)signedPacked / 2147483648.0);
		for(size_t channel = 0; channel < channels; ++channel) {
			samples[frame * channels + channel] = silence;
		}
		marker = (marker == 0x05) ? 0xFA : 0x05;
	}
	*nextMarker = marker;
}

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

		writeSemaphore = [Semaphore new];
		readSemaphore = [Semaphore new];

		accessLock = [NSLock new];

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
			if(audioBufferIsDoP((const float *)[sampleData bytes], channels, samplesToMix, NULL)) {
				// DoP is a bitstream disguised as PCM. Mixing or fading it corrupts
				// both its marker bytes and its DSD payload, so use a hard cut.
				return true;
			}
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
