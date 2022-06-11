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

// Apparently _mm_malloc is Intel-only on newer macOS targets, so use supported posix_memalign
static void *_memalign_malloc(size_t size, size_t align) {
	void *ret = NULL;
	if(posix_memalign(&ret, align, size) != 0) {
		return NULL;
	}
	return ret;
}

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

		double sampleRateOfSource = [[properties objectForKey:@"sampleRate"] floatValue];

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

		float *impulseBuffer = (float *)_memalign_malloc(sampleCount * sizeof(float) * impulseChannels, 16);
		if(!impulseBuffer) {
			[decoder close];
			decoder = nil;
			[source close];
			source = nil;
			return nil;
		}

		if([decoder readAudio:impulseBuffer frames:sampleCount] != sampleCount) {
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

			float *tempImpulse = (float *)_memalign_malloc((sampleCount + resamplerLatencyIn * 2 + 1024) * sizeof(float) * impulseChannels, 16);
			if(!tempImpulse) {
				free(impulseBuffer);
				return nil;
			}

			resampledCount += resamplerLatencyOut * 2 + 1024;

			float *resampledImpulse = (float *)_memalign_malloc(resampledCount * sizeof(float) * impulseChannels, 16);
			if(!resampledImpulse) {
				free(tempImpulse);
				free(impulseBuffer);
				return nil;
			}

			size_t prime = MIN(sampleCount, PRIME_LEN_);

			void *extrapolate_buffer = NULL;
			size_t extrapolate_buffer_size = 0;

			memcpy(tempImpulse + resamplerLatencyIn * impulseChannels, impulseBuffer, sampleCount * sizeof(float) * impulseChannels);
			free(impulseBuffer);
			lpc_extrapolate_bkwd(tempImpulse + N_samples_to_add_ * impulseChannels, sampleCount, prime, impulseChannels, LPC_ORDER, N_samples_to_add_, &extrapolate_buffer, &extrapolate_buffer_size);
			lpc_extrapolate_fwd(tempImpulse + N_samples_to_add_ * impulseChannels, sampleCount, prime, impulseChannels, LPC_ORDER, N_samples_to_add_, &extrapolate_buffer, &extrapolate_buffer_size);
			free(extrapolate_buffer);

			size_t inputDone = 0;
			size_t outputDone = 0;

			outputDone = _r8bstate->resample(tempImpulse, sampleCount + N_samples_to_add_ * 2, &inputDone, resampledImpulse, resampledCount);

			free(tempImpulse);

			if (outputDone < resampledCount) {
				outputDone += _r8bstate->flush(resampledImpulse + outputDone * impulseChannels, resampledCount - outputDone);
			}
			
			delete _r8bstate;

			outputDone -= N_samples_to_drop_ * 2;

			memmove(resampledImpulse, resampledImpulse + N_samples_to_drop_ * impulseChannels, outputDone * sizeof(float) * impulseChannels);

			impulseBuffer = resampledImpulse;
			sampleCount = (int)outputDone;

			// Normalize resampled impulse by sample ratio
			float fSampleRatio = (float)sampleRatio;
			vDSP_vsdiv(impulseBuffer, 1, &fSampleRatio, impulseBuffer, 1, sampleCount * impulseChannels);
		}

		channelCount = channels;

		bufferSize = 512;
		fftSize = sampleCount + bufferSize;

		int pow = 1;
		while(fftSize > 2) {
			pow++;
			fftSize /= 2;
		}
		fftSize = 2 << pow;

		float *deinterleavedImpulseBuffer = (float *)_memalign_malloc(fftSize * sizeof(float) * impulseChannels, 16);
		if(!deinterleavedImpulseBuffer) {
			free(impulseBuffer);
			return nil;
		}

		for(size_t i = 0; i < impulseChannels; ++i) {
			cblas_scopy(sampleCount, impulseBuffer + i, impulseChannels, deinterleavedImpulseBuffer + i * fftSize, 1);
			vDSP_vclr(deinterleavedImpulseBuffer + i * fftSize + sampleCount, 1, fftSize - sampleCount);
		}

		free(impulseBuffer);

		paddedBufferSize = fftSize;
		fftSizeOver2 = (fftSize + 1) / 2;

		dftSetupF = vDSP_DFT_zrop_CreateSetup(nil, fftSize, vDSP_DFT_FORWARD);
		dftSetupB = vDSP_DFT_zrop_CreateSetup(nil, fftSize, vDSP_DFT_INVERSE);
		if(!dftSetupF || !dftSetupB) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		paddedSignal = (float *)_memalign_malloc(sizeof(float) * paddedBufferSize, 16);
		if(!paddedSignal) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		signal_fft.realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		signal_fft.imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		if(!signal_fft.realp || !signal_fft.imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_per_channel[0].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		input_filtered_signal_per_channel[0].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		if(!input_filtered_signal_per_channel[0].realp ||
		   !input_filtered_signal_per_channel[0].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_per_channel[1].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		input_filtered_signal_per_channel[1].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		if(!input_filtered_signal_per_channel[1].realp ||
		   !input_filtered_signal_per_channel[1].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_totals[0].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		input_filtered_signal_totals[0].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		if(!input_filtered_signal_totals[0].realp ||
		   !input_filtered_signal_totals[0].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_totals[1].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		input_filtered_signal_totals[1].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
		if(!input_filtered_signal_totals[1].realp ||
		   !input_filtered_signal_totals[1].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		impulse_responses = (DSPSplitComplex *)calloc(sizeof(DSPSplitComplex), channels * 2);
		if(!impulse_responses) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		for(size_t i = 0; i < channels; ++i) {
			impulse_responses[i * 2 + 0].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
			impulse_responses[i * 2 + 0].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
			impulse_responses[i * 2 + 1].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);
			impulse_responses[i * 2 + 1].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2, 16);

			if(!impulse_responses[i * 2 + 0].realp || !impulse_responses[i * 2 + 0].imagp ||
			   !impulse_responses[i * 2 + 1].realp || !impulse_responses[i * 2 + 1].imagp) {
				free(deinterleavedImpulseBuffer);
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
				float *temp;
				if(impulseChannels == 7) {
					temp = (float *)malloc(sizeof(float) * fftSize);
					if(!temp) {
						free(deinterleavedImpulseBuffer);
						return nil;
					}

					cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + 4 * fftSize, 1, temp, 1);
					vDSP_vadd(temp, 1, deinterleavedImpulseBuffer + 5 * fftSize, 1, temp, 1, fftSize);

					vDSP_ctoz((DSPComplex *)temp, 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
					vDSP_ctoz((DSPComplex *)temp, 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);
				} else {
					temp = (float *)malloc(sizeof(float) * fftSize * 2);
					if(!temp) {
						free(deinterleavedImpulseBuffer);
						return nil;
					}

					cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + 4 * fftSize, 1, temp, 1);
					vDSP_vadd(temp, 1, deinterleavedImpulseBuffer + 12 * fftSize, 1, temp, 1, fftSize);

					cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + 5 * fftSize, 1, temp + fftSize, 1);
					vDSP_vadd(temp + fftSize, 1, deinterleavedImpulseBuffer + 11 * fftSize, 1, temp + fftSize, 1, fftSize);

					vDSP_ctoz((DSPComplex *)temp, 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
					vDSP_ctoz((DSPComplex *)(temp + fftSize), 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);
				}

				free(temp);
			} else if(leftInChannel == speaker_not_present || rightInChannel == speaker_not_present) {
				vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + impulseChannels * fftSize), 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
				vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + impulseChannels * fftSize), 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);
			} else {
				vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + leftInChannel * fftSize), 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
				vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + rightInChannel * fftSize), 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);
			}

			vDSP_DFT_Execute(dftSetupF, impulse_responses[i * 2 + 0].realp, impulse_responses[i * 2 + 0].imagp, impulse_responses[i * 2 + 0].realp, impulse_responses[i * 2 + 0].imagp);
			vDSP_DFT_Execute(dftSetupF, impulse_responses[i * 2 + 1].realp, impulse_responses[i * 2 + 1].imagp, impulse_responses[i * 2 + 1].realp, impulse_responses[i * 2 + 1].imagp);
		}

		free(deinterleavedImpulseBuffer);

		left_result = (float *)_memalign_malloc(sizeof(float) * fftSize, 16);
		right_result = (float *)_memalign_malloc(sizeof(float) * fftSize, 16);
		if(!left_result || !right_result)
			return nil;

		prevInputs = (float **)calloc(channels, sizeof(float *));
		if(!prevInputs)
			return nil;
		for(size_t i = 0; i < channels; ++i) {
			prevInputs[i] = (float *)_memalign_malloc(sizeof(float) * fftSize, 16);
			if(!prevInputs[i])
				return nil;
			vDSP_vclr(prevInputs[i], 1, fftSize);
		}
	}

	return self;
}

- (void)dealloc {
	if(dftSetupF) vDSP_DFT_DestroySetup(dftSetupF);
	if(dftSetupB) vDSP_DFT_DestroySetup(dftSetupB);

	free(paddedSignal);

	free(signal_fft.realp);
	free(signal_fft.imagp);

	free(input_filtered_signal_per_channel[0].realp);
	free(input_filtered_signal_per_channel[0].imagp);
	free(input_filtered_signal_per_channel[1].realp);
	free(input_filtered_signal_per_channel[1].imagp);

	free(input_filtered_signal_totals[0].realp);
	free(input_filtered_signal_totals[0].imagp);
	free(input_filtered_signal_totals[1].realp);
	free(input_filtered_signal_totals[1].imagp);

	if(impulse_responses) {
		for(size_t i = 0; i < channelCount * 2; ++i) {
			free(impulse_responses[i].realp);
			free(impulse_responses[i].imagp);
		}
		free(impulse_responses);
	}

	free(left_result);
	free(right_result);

	if(prevInputs) {
		for(size_t i = 0; i < channelCount; ++i) {
			free(prevInputs[i]);
		}
		free(prevInputs);
	}
}

- (void)process:(const float *)inBuffer sampleCount:(size_t)count toBuffer:(float *)outBuffer {
	const float scale = 1.0 / (4.0 * (float)fftSize);

	while(count > 0) {
		const size_t countToDo = (count > bufferSize) ? bufferSize : count;
		const size_t prevToDo = fftSize - countToDo;

		vDSP_vclr(input_filtered_signal_totals[0].realp, 1, fftSizeOver2);
		vDSP_vclr(input_filtered_signal_totals[0].imagp, 1, fftSizeOver2);
		vDSP_vclr(input_filtered_signal_totals[1].realp, 1, fftSizeOver2);
		vDSP_vclr(input_filtered_signal_totals[1].imagp, 1, fftSizeOver2);

		for(size_t i = 0; i < channelCount; ++i) {
			cblas_scopy((int)prevToDo, prevInputs[i] + countToDo, 1, paddedSignal, 1);
			cblas_scopy((int)countToDo, inBuffer + i, (int)channelCount, paddedSignal + prevToDo, 1);
			cblas_scopy((int)fftSize, paddedSignal, 1, prevInputs[i], 1);

			vDSP_ctoz((DSPComplex *)paddedSignal, 2, &signal_fft, 1, fftSizeOver2);

			vDSP_DFT_Execute(dftSetupF, signal_fft.realp, signal_fft.imagp, signal_fft.realp, signal_fft.imagp);

			// One channel forward, then multiply and back twice

			float preserveIRNyq = impulse_responses[i * 2 + 0].imagp[0];
			float preserveSigNyq = signal_fft.imagp[0];
			impulse_responses[i * 2 + 0].imagp[0] = 0;
			signal_fft.imagp[0] = 0;

			vDSP_zvmul(&signal_fft, 1, &impulse_responses[i * 2 + 0], 1, &input_filtered_signal_per_channel[0], 1, fftSizeOver2, 1);

			input_filtered_signal_per_channel[0].imagp[0] = preserveIRNyq * preserveSigNyq;
			impulse_responses[i * 2 + 0].imagp[0] = preserveIRNyq;

			preserveIRNyq = impulse_responses[i * 2 + 1].imagp[0];
			impulse_responses[i * 2 + 1].imagp[0] = 0;

			vDSP_zvmul(&signal_fft, 1, &impulse_responses[i * 2 + 1], 1, &input_filtered_signal_per_channel[1], 1, fftSizeOver2, 1);

			input_filtered_signal_per_channel[1].imagp[0] = preserveIRNyq * preserveSigNyq;
			impulse_responses[i * 2 + 1].imagp[0] = preserveIRNyq;

			vDSP_zvadd(&input_filtered_signal_totals[0], 1, &input_filtered_signal_per_channel[0], 1, &input_filtered_signal_totals[0], 1, fftSizeOver2);
			vDSP_zvadd(&input_filtered_signal_totals[1], 1, &input_filtered_signal_per_channel[1], 1, &input_filtered_signal_totals[1], 1, fftSizeOver2);
		}

		vDSP_DFT_Execute(dftSetupB, input_filtered_signal_totals[0].realp, input_filtered_signal_totals[0].imagp, input_filtered_signal_totals[0].realp, input_filtered_signal_totals[0].imagp);
		vDSP_DFT_Execute(dftSetupB, input_filtered_signal_totals[1].realp, input_filtered_signal_totals[1].imagp, input_filtered_signal_totals[1].realp, input_filtered_signal_totals[1].imagp);

		vDSP_ztoc(&input_filtered_signal_totals[0], 1, (DSPComplex *)left_result, 2, fftSizeOver2);
		vDSP_ztoc(&input_filtered_signal_totals[1], 1, (DSPComplex *)right_result, 2, fftSizeOver2);

		float *left_ptr = left_result + prevToDo;
		float *right_ptr = right_result + prevToDo;

		vDSP_vsmul(left_ptr, 1, &scale, left_ptr, 1, countToDo);
		vDSP_vsmul(right_ptr, 1, &scale, right_ptr, 1, countToDo);

		cblas_scopy((int)countToDo, left_ptr, 1, outBuffer + 0, 2);
		cblas_scopy((int)countToDo, right_ptr, 1, outBuffer + 1, 2);

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
