//
//  HeadphoneFilter.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 1/24/22.
//

#import "HeadphoneFilter.h"
#import "AudioChunk.h"
#import "AudioDecoder.h"
#import "AudioSource.h"

#import <stdlib.h>

#import "r8bstate.h"

#import "lpc.h"
#import "util.h"

#import "pffft_double.h"

@implementation HeadphoneFilter

enum {
	speaker_is_back_center = -1,
	speaker_not_present = -2,
};

static const uint32_t max_speaker_index = 10;

static const int8_t speakers_to_hesuvi_7[11][2] = {
	// front left
	{ 0, 1 },
	// front right
	{ 1, 0 },
	// front center
	{ 6, 6 },
	// lfe
	{ 6, 6 },
	// back left
	{ 4, 5 },
	// back right
	{ 5, 4 },
	// front center left
	{ speaker_not_present, speaker_not_present },
	// front center right
	{ speaker_not_present, speaker_not_present },
	// back center
	{ speaker_is_back_center, speaker_is_back_center },
	// side left
	{ 2, 3 },
	// side right
	{ 3, 2 }
};

static const int8_t speakers_to_hesuvi_14[11][2] = {
	// front left
	{ 0, 1 },
	// front right
	{ 8, 7 },
	// front center
	{ 6, 13 },
	// lfe
	{ 6, 13 },
	// back left
	{ 4, 5 },
	// back right
	{ 12, 11 },
	// front center left
	{ speaker_not_present, speaker_not_present },
	// front center right
	{ speaker_not_present, speaker_not_present },
	// back center
	{ speaker_is_back_center, speaker_is_back_center },
	// side left
	{ 2, 3 },
	// side right
	{ 10, 9 }
};

+ (BOOL)validateImpulseFile:(NSURL *)url {
	id<CogSource> source = [AudioSource audioSourceForURL:url];
	if(!source)
		return NO;

	if(![source open:url])
		return NO;

	id<CogDecoder> decoder = [AudioDecoder audioDecoderForSource:source];

	if(decoder == nil) {
		[source close];
		source = nil;
		return NO;
	}

	if(![decoder open:source]) {
		decoder = nil;
		[source close];
		source = nil;
		return NO;
	}

	NSDictionary *properties = [decoder properties];

	[decoder close];
	decoder = nil;
	[source close];
	source = nil;

	int impulseChannels = [[properties objectForKey:@"channels"] intValue];

	if([[properties objectForKey:@"floatingPoint"] boolValue] != YES ||
	   [[properties objectForKey:@"bitsPerSample"] intValue] != 32 ||
	   !([[properties objectForKey:@"endian"] isEqualToString:@"host"] ||
	     [[properties objectForKey:@"endian"] isEqualToString:@"little"]) ||
	   (impulseChannels != 14 && impulseChannels != 7))
		return NO;

	return YES;
}

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(size_t)channels withConfig:(uint32_t)config {
	self = [super init];

	if(self) {
		id<CogSource> source = [AudioSource audioSourceForURL:url];
		if(!source)
			return nil;

		if(![source open:url])
			return nil;

		id<CogDecoder> decoder = [AudioDecoder audioDecoderForSource:source];

		if(decoder == nil) {
			[source close];
			source = nil;
			return nil;
		}

		if(![decoder open:source]) {
			decoder = nil;
			[source close];
			source = nil;
			return nil;
		}

		NSDictionary *properties = [decoder properties];

		double sampleRateOfSource = [[properties objectForKey:@"sampleRate"] doubleValue];

		int sampleCount = [[properties objectForKey:@"totalFrames"] intValue];
		int impulseChannels = [[properties objectForKey:@"channels"] intValue];

		if([[properties objectForKey:@"floatingPoint"] boolValue] != YES ||
		   [[properties objectForKey:@"bitsPerSample"] intValue] != 32 ||
		   !([[properties objectForKey:@"endian"] isEqualToString:@"host"] ||
		     [[properties objectForKey:@"endian"] isEqualToString:@"little"]) ||
		   (impulseChannels != 14 && impulseChannels != 7)) {
			[decoder close];
			decoder = nil;
			[source close];
			source = nil;
			return nil;
		}

		float *impulseBuffer = (float *)pffft_aligned_malloc(sampleCount * sizeof(float) * impulseChannels);
		if(!impulseBuffer) {
			[decoder close];
			decoder = nil;
			[source close];
			source = nil;
			return nil;
		}

		if([decoder readAudio:impulseBuffer frames:sampleCount] != sampleCount) {
			pffft_aligned_free(impulseBuffer);
			[decoder close];
			decoder = nil;
			[source close];
			source = nil;
			return nil;
		}

		[decoder close];
		decoder = nil;
		[source close];
		source = nil;

		if(sampleRateOfSource != sampleRate) {
			double sampleRatio = sampleRate / sampleRateOfSource;
			int resampledCount = (int)ceil((double)sampleCount * sampleRatio);

			r8bstate *_r8bstate = new r8bstate(impulseChannels, 1024, sampleRateOfSource, sampleRate);
			
			unsigned long PRIME_LEN_ = MAX(sampleRateOfSource / 20, 1024u);
			PRIME_LEN_ = MIN(PRIME_LEN_, 16384u);
			PRIME_LEN_ = MAX(PRIME_LEN_, 2 * LPC_ORDER + 1);

			unsigned int N_samples_to_add_ = sampleRateOfSource;
			unsigned int N_samples_to_drop_ = sampleRate;

			samples_len(&N_samples_to_add_, &N_samples_to_drop_, 20, 8192u);

			int resamplerLatencyIn = (int)N_samples_to_add_;
			int resamplerLatencyOut = (int)N_samples_to_drop_;

			float *tempImpulse = (float *)pffft_aligned_malloc((sampleCount + resamplerLatencyIn * 2 + 1024) * sizeof(float) * impulseChannels);
			if(!tempImpulse) {
				pffft_aligned_free(impulseBuffer);
				return nil;
			}

			resampledCount += resamplerLatencyOut * 2 + 1024;

			float *resampledImpulse = (float *)pffft_aligned_malloc(resampledCount * sizeof(float) * impulseChannels);
			if(!resampledImpulse) {
				pffft_aligned_free(impulseBuffer);
				pffft_aligned_free(tempImpulse);
				return nil;
			}

			size_t prime = MIN(sampleCount, PRIME_LEN_);

			void *extrapolate_buffer = NULL;
			size_t extrapolate_buffer_size = 0;

			memcpy(tempImpulse + resamplerLatencyIn * impulseChannels, impulseBuffer, sampleCount * sizeof(float) * impulseChannels);
			lpc_extrapolate_bkwd(tempImpulse + N_samples_to_add_ * impulseChannels, sampleCount, prime, impulseChannels, LPC_ORDER, N_samples_to_add_, &extrapolate_buffer, &extrapolate_buffer_size);
			lpc_extrapolate_fwd(tempImpulse + N_samples_to_add_ * impulseChannels, sampleCount, prime, impulseChannels, LPC_ORDER, N_samples_to_add_, &extrapolate_buffer, &extrapolate_buffer_size);
			free(extrapolate_buffer);

			size_t inputDone = 0;
			size_t outputDone = 0;

			outputDone = _r8bstate->resample(tempImpulse, sampleCount + N_samples_to_add_ * 2, &inputDone, resampledImpulse, resampledCount);

			if (outputDone < resampledCount) {
				outputDone += _r8bstate->flush(resampledImpulse + outputDone * impulseChannels, resampledCount - outputDone);
			}
			
			delete _r8bstate;

			outputDone -= N_samples_to_drop_ * 2;

			memmove(resampledImpulse, resampledImpulse + N_samples_to_drop_ * impulseChannels, outputDone * sizeof(float) * impulseChannels);

			pffft_aligned_free(tempImpulse);
			pffft_aligned_free(impulseBuffer);
			impulseBuffer = resampledImpulse;
			sampleCount = (int)outputDone;

			// Normalize resampled impulse by sample ratio
			float fSampleRatio = (float)sampleRatio;
			vDSP_vsdiv(impulseBuffer, 1, &fSampleRatio, impulseBuffer, 1, sampleCount * impulseChannels);
		}

		channelCount = channels;

		bufferSize = 512;
		fftSize = sampleCount + bufferSize;

		fftSize = (size_t)pffftd_next_power_of_two((int)fftSize);

		float *deinterleavedImpulseBuffer = (float *)pffft_aligned_malloc(fftSize * sizeof(float) * impulseChannels);
		if(!deinterleavedImpulseBuffer) {
			pffft_aligned_free(impulseBuffer);
			return nil;
		}

		for(size_t i = 0; i < impulseChannels; ++i) {
			cblas_scopy(sampleCount, impulseBuffer + i, impulseChannels, deinterleavedImpulseBuffer + i * fftSize, 1);
			vDSP_vclr(deinterleavedImpulseBuffer + i * fftSize + sampleCount, 1, fftSize - sampleCount);
		}

		pffft_aligned_free(impulseBuffer);

		paddedBufferSize = fftSize;

		fftSetup = pffft_new_setup((int)fftSize, PFFFT_REAL);
		if(!fftSetup) {
			pffft_aligned_free(deinterleavedImpulseBuffer);
			return nil;
		}

		workBuffer = (float *)pffft_aligned_malloc(sizeof(float) * fftSize);
		if(!workBuffer) {
			pffft_aligned_free(deinterleavedImpulseBuffer);
			return nil;
		}

		paddedSignal = (float *)pffft_aligned_malloc(sizeof(float) * paddedBufferSize);
		if(!paddedSignal) {
			pffft_aligned_free(deinterleavedImpulseBuffer);
			return nil;
		}

		impulse_responses = (float **)calloc(sizeof(float *), channels * 2);
		if(!impulse_responses) {
			pffft_aligned_free(deinterleavedImpulseBuffer);
			return nil;
		}

		for(size_t i = 0; i < channels; ++i) {
			impulse_responses[i * 2 + 0] = (float *)pffft_aligned_malloc(sizeof(float) * fftSize * 2);
			impulse_responses[i * 2 + 1] = (float *)pffft_aligned_malloc(sizeof(float) * fftSize * 2);

			if(!impulse_responses[i * 2 + 0] || !impulse_responses[i * 2 + 1]) {
				pffft_aligned_free(deinterleavedImpulseBuffer);
				return nil;
			}

			uint32_t channelFlag = [AudioChunk extractChannelFlag:(uint32_t)i fromConfig:config];
			uint32_t channelIndex = [AudioChunk findChannelIndex:channelFlag];

			int leftInChannel = speaker_not_present;
			int rightInChannel = speaker_not_present;

			if(impulseChannels == 7) {
				if(channelIndex <= max_speaker_index) {
					leftInChannel = speakers_to_hesuvi_7[channelIndex][0];
					rightInChannel = speakers_to_hesuvi_7[channelIndex][1];
				}
			} else {
				if(channelIndex <= max_speaker_index) {
					leftInChannel = speakers_to_hesuvi_14[channelIndex][0];
					rightInChannel = speakers_to_hesuvi_14[channelIndex][1];
				}
			}

			if(leftInChannel == speaker_is_back_center || rightInChannel == speaker_is_back_center) {
				if(impulseChannels == 7) {
					cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + 4 * fftSize, 1, impulse_responses[i * 2 + 0], 1);
					vDSP_vadd(impulse_responses[i * 2 + 0], 1, deinterleavedImpulseBuffer + 5 * fftSize, 1, impulse_responses[i * 2 + 0], 1, fftSize);
					cblas_scopy((int)fftSize, impulse_responses[i * 2 + 0], 1, impulse_responses[i * 2 + 1], 1);
				} else {
					cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + 4 * fftSize, 1, impulse_responses[i * 2 + 0], 1);
					vDSP_vadd(impulse_responses[i * 2 + 0], 1, deinterleavedImpulseBuffer + 12 * fftSize, 1, impulse_responses[i * 2 + 0], 1, fftSize);

					cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + 5 * fftSize, 1, impulse_responses[i * 2 + 1], 1);
					vDSP_vadd(impulse_responses[i * 2 + 1], 1, deinterleavedImpulseBuffer + 11 * fftSize, 1, impulse_responses[i * 2 + 1], 1, fftSize);
				}
			} else if(leftInChannel == speaker_not_present || rightInChannel == speaker_not_present) {
				vDSP_vclr(impulse_responses[i * 2 + 0], 1, fftSize);
				vDSP_vclr(impulse_responses[i * 2 + 1], 1, fftSize);
			} else {
				cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + leftInChannel * fftSize, 1, impulse_responses[i * 2 + 0], 1);
				cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + rightInChannel * fftSize, 1, impulse_responses[i * 2 + 1], 1);
			}

			pffft_transform(fftSetup, impulse_responses[i * 2 + 0], impulse_responses[i * 2 + 0], workBuffer, PFFFT_FORWARD);
			pffft_transform(fftSetup, impulse_responses[i * 2 + 1], impulse_responses[i * 2 + 1], workBuffer, PFFFT_FORWARD);
		}

		pffft_aligned_free(deinterleavedImpulseBuffer);

		left_result = (float *)pffft_aligned_malloc(sizeof(float) * fftSize);
		right_result = (float *)pffft_aligned_malloc(sizeof(float) * fftSize);
		if(!left_result || !right_result)
			return nil;

		prevInputs = (float **)calloc(sizeof(float *), channels);
		if(!prevInputs) {
			return nil;
		}
		for(size_t i = 0; i < channels; ++i) {
			prevInputs[i] = (float *)pffft_aligned_malloc(sizeof(float) * fftSize);
			if(!prevInputs[i]) {
				return nil;
			}
			vDSP_vclr(prevInputs[i], 1, fftSize);
		}
	}

	return self;
}

- (void)dealloc {
	if(fftSetup) pffft_destroy_setup(fftSetup);

	pffft_aligned_free(workBuffer);

	pffft_aligned_free(paddedSignal);

	if(impulse_responses) {
		for(size_t i = 0; i < channelCount * 2; ++i) {
			pffft_aligned_free(impulse_responses[i]);
		}
		free(impulse_responses);
	}

	if(prevInputs) {
		for(size_t i = 0; i < channelCount; ++i) {
			pffft_aligned_free(prevInputs[i]);
		}
		free(prevInputs);
	}

	pffft_aligned_free(left_result);
	pffft_aligned_free(right_result);
}

- (void)process:(const float *)inBuffer sampleCount:(size_t)count toBuffer:(float *)outBuffer {
	const float scale = 1.0 / ((float)fftSize);

	while(count > 0) {
		const size_t countToDo = (count > bufferSize) ? bufferSize : count;
		const size_t outOffset = fftSize - countToDo;

		vDSP_vclr(left_result, 1, fftSize);
		vDSP_vclr(right_result, 1, fftSize);

		for(size_t i = 0; i < channelCount; ++i) {
			cblas_scopy((int)outOffset, prevInputs[i] + countToDo, 1, paddedSignal, 1);
			cblas_scopy((int)countToDo, inBuffer + i, (int)channelCount, paddedSignal + outOffset, 1);
			cblas_scopy((int)fftSize, paddedSignal, 1, prevInputs[i], 1);

			pffft_transform(fftSetup, paddedSignal, paddedSignal, workBuffer, PFFFT_FORWARD);

			pffft_zconvolve_accumulate(fftSetup, paddedSignal, impulse_responses[i * 2 + 0], left_result, 1.0);
			pffft_zconvolve_accumulate(fftSetup, paddedSignal, impulse_responses[i * 2 + 1], right_result, 1.0);
		}

		pffft_transform(fftSetup, left_result, left_result, workBuffer, PFFFT_BACKWARD);
		pffft_transform(fftSetup, right_result, right_result, workBuffer, PFFFT_BACKWARD);

		vDSP_vsmul(left_result + outOffset, 1, &scale, left_result + outOffset, 1, countToDo);
		vDSP_vsmul(right_result + outOffset, 1, &scale, right_result + outOffset, 1, countToDo);

		cblas_scopy((int)countToDo, left_result + outOffset, 1, outBuffer + 0, 2);
		cblas_scopy((int)countToDo, right_result + outOffset, 1, outBuffer + 1, 2);

		inBuffer += countToDo * channelCount;
		outBuffer += countToDo * 2;

		count -= countToDo;
	}
}

- (void)reset {
	for(size_t i = 0; i < channelCount; ++i) {
		vDSP_vclr(prevInputs[i], 1, fftSize);
	}
}

@end
