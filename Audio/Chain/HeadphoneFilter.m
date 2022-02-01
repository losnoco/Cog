//
//  HeadphoneFilter.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 1/24/22.
//

#import "HeadphoneFilter.h"
#import "AudioSource.h"
#import "AudioDecoder.h"

#import <audio/audio_resampler.h>
#import <memalign.h>

@implementation HeadphoneFilter

// Symmetrical / no-reverb sets
static const int8_t speakers_to_hesuvi_7[8][2][8] = {
    // mono/center
    { { 6 }, { 6 } },
    // left/right
    { { 0, 1 }, { 1, 0 } },
    // left/right/center
    { { 0, 1, 6 }, { 1, 0, 6 } },
    // left/right/back lef/back right
    { { 0, 1, 4, 5 }, { 1, 0, 5, 4 } },
    // left/right/center/back left/back right
    { { 0, 1, 6, 4, 5 }, { 1, 0, 6, 5, 4 } },
    // left/right/center/lfe(center)/back left/back right
    { { 0, 1, 6, 6, 4, 5 }, { 1, 0, 6, 6, 5, 4 } },
    // left/right/center/lfe(center)/back center(special)/side left/side right
    { { 0, 1, 6, 6, -1, 2, 3 }, { 1, 0, 6, 6, -1, 3, 2 } },
    // left/right/center/lfe(center)/back left/back right/side left/side right
    { { 0, 1, 6, 6, 4, 5, 2, 3 }, { 1, 0, 6, 6, 5, 4, 3, 2 } }
};

// Asymmetrical / reverb sets
static const int8_t speakers_to_hesuvi_14[8][2][8] = {
    // mono/center
    { { 6 }, { 13 } },
    // left/right
    { { 0, 8 }, { 1, 7 } },
    // left/right/center
    { { 0, 8, 6 }, { 1, 7, 13 } },
    // left/right/back left/back right
    { { 0, 8, 4, 12 }, { 1, 7, 5, 11 } },
    // left/right/center/back left/back right
    { { 0, 8, 6, 4, 12 }, { 1, 7, 13, 5, 11 } },
    // left/right/center/lfe(center)/back left/back right
    { { 0, 8, 6, 6, 4, 12 }, { 1, 7, 13, 13, 5, 11 } },
    // left/right/center/lfe(center)/back center(special)/side left/side right
    { { 0, 8, 6, 6, -1, 2, 10 }, { 1, 7, 13, 13, -1, 3, 9 } },
    // left/right/center/lfe(center)/back left/back right/side left/side right
    { { 0, 8, 6, 6, 4, 12, 2, 10 }, { 1, 7, 13, 13, 5, 11, 3, 9 } }
};

+ (BOOL)validateImpulseFile:(NSURL *)url {
    id<CogSource> source = [AudioSource audioSourceForURL:url];
    if (!source)
        return NO;

    if (![source open:url])
        return NO;
    
    id<CogDecoder> decoder = [AudioDecoder audioDecoderForSource:source];

    if (decoder == nil) {
        [source close];
        source = nil;
        return NO;
    }

    if (![decoder open:source])
    {
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
    
    if ([[properties objectForKey:@"floatingPoint"] boolValue] != YES ||
        [[properties objectForKey:@"bitsPerSample"] intValue] != 32 ||
        !([[properties objectForKey:@"endian"] isEqualToString:@"host"] ||
          [[properties objectForKey:@"endian"] isEqualToString:@"little"]) ||
        (impulseChannels != 14 && impulseChannels != 7))
        return NO;

    return YES;
}

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(size_t)channels {
    self = [super init];
    
    if (self) {
        id<CogSource> source = [AudioSource audioSourceForURL:url];
        if (!source)
            return nil;

        if (![source open:url])
            return nil;
        
        id<CogDecoder> decoder = [AudioDecoder audioDecoderForSource:source];

        if (decoder == nil) {
            [source close];
            source = nil;
            return nil;
        }

        if (![decoder open:source])
        {
            decoder = nil;
            [source close];
            source = nil;
            return nil;
        }
        
        NSDictionary *properties = [decoder properties];
        
        double sampleRateOfSource = [[properties objectForKey:@"sampleRate"] floatValue];
        
        int sampleCount = [[properties objectForKey:@"totalFrames"] intValue];
        int impulseChannels = [[properties objectForKey:@"channels"] intValue];
        
        if ([[properties objectForKey:@"floatingPoint"] boolValue] != YES ||
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

        float * impulseBuffer = (float *) malloc(sampleCount * sizeof(float) * impulseChannels);
        if (!impulseBuffer) {
            [decoder close];
            decoder = nil;
            [source close];
            source = nil;
            return nil;
        }
        
        if ([decoder readAudio:impulseBuffer frames:sampleCount] != sampleCount) {
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
        
        if (sampleRateOfSource != sampleRate) {
            double sampleRatio = sampleRate / sampleRateOfSource;
            int resampledCount = (int)ceil((double)sampleCount * sampleRatio);

            void *resampler_data = NULL;
            const retro_resampler_t *resampler = NULL;

            if (!retro_resampler_realloc(&resampler_data, &resampler, "sinc", RESAMPLER_QUALITY_NORMAL, impulseChannels, sampleRatio)) {
                free(impulseBuffer);
                return nil;
            }

            int resamplerLatencyIn = (int) resampler->latency(resampler_data);
            int resamplerLatencyOut = (int)ceil(resamplerLatencyIn * sampleRatio);
            
            float * tempImpulse = (float *) realloc(impulseBuffer, (sampleCount + resamplerLatencyIn * 2 + 1024) * sizeof(float) * impulseChannels);
            if (!tempImpulse) {
                free(impulseBuffer);
                return nil;
            }
            
            impulseBuffer = tempImpulse;
            
            resampledCount += resamplerLatencyOut * 2 + 1024;
            
            float * resampledImpulse = (float *) malloc(resampledCount * sizeof(float) * impulseChannels);
            if (!resampledImpulse) {
                free(impulseBuffer);
                return nil;
            }

            memmove(impulseBuffer + resamplerLatencyIn * impulseChannels, impulseBuffer, sampleCount * sizeof(float) * impulseChannels);
            memset(impulseBuffer, 0, resamplerLatencyIn * sizeof(float) * impulseChannels);
            memset(impulseBuffer + (resamplerLatencyIn + sampleCount) * impulseChannels, 0, resamplerLatencyIn * sizeof(float) * impulseChannels);

            struct resampler_data src_data;
            src_data.data_in = impulseBuffer;
            src_data.input_frames = sampleCount + resamplerLatencyIn * 2;
            src_data.data_out = resampledImpulse;
            src_data.output_frames = 0;
            src_data.ratio = sampleRatio;
            
            resampler->process(resampler_data, &src_data);

            resampler->free(resampler, resampler_data);

            src_data.output_frames -= resamplerLatencyOut * 2;

            memmove(resampledImpulse, resampledImpulse + resamplerLatencyOut * impulseChannels, src_data.output_frames * sizeof(float) * impulseChannels);

            free(impulseBuffer);
            impulseBuffer = resampledImpulse;
            sampleCount = (int) src_data.output_frames;
        }

        channelCount = channels;

        bufferSize = 512;
        fftSize = sampleCount + bufferSize;

        int pow = 1;
        while (fftSize > 2) { pow++; fftSize /= 2; }
        fftSize = 2 << pow;

        float * deinterleavedImpulseBuffer = (float *) memalign_alloc(16, fftSize * sizeof(float) * impulseChannels);
        if (!deinterleavedImpulseBuffer) {
            free(impulseBuffer);
            return nil;
        }
        
        for (size_t i = 0; i < impulseChannels; ++i) {
            cblas_scopy(sampleCount, impulseBuffer + i, impulseChannels, deinterleavedImpulseBuffer + i * fftSize, 1);
            vDSP_vclr(deinterleavedImpulseBuffer + i * fftSize + sampleCount, 1, fftSize - sampleCount);
        }

        free(impulseBuffer);

        paddedBufferSize = fftSize;
        fftSizeOver2 = (fftSize + 1) / 2;
        log2n = log2f(fftSize);
        log2nhalf = log2n / 2;

        fftSetup = vDSP_create_fftsetup(log2n, FFT_RADIX2);
        if (!fftSetup) {
            memalign_free(deinterleavedImpulseBuffer);
            return nil;
        }

        paddedSignal = (float *) memalign_alloc(16, sizeof(float) * paddedBufferSize);
        if (!paddedSignal) {
            memalign_free(deinterleavedImpulseBuffer);
            return nil;
        }
        
        signal_fft.realp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
        signal_fft.imagp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
        if (!signal_fft.realp || !signal_fft.imagp) {
            memalign_free(deinterleavedImpulseBuffer);
            return nil;
        }
        
        input_filtered_signal_per_channel[0].realp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
        input_filtered_signal_per_channel[0].imagp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
        if (!input_filtered_signal_per_channel[0].realp ||
            !input_filtered_signal_per_channel[0].imagp) {
            memalign_free(deinterleavedImpulseBuffer);
            return nil;
        }
        
        input_filtered_signal_per_channel[1].realp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
        input_filtered_signal_per_channel[1].imagp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
        if (!input_filtered_signal_per_channel[1].realp ||
            !input_filtered_signal_per_channel[1].imagp) {
            memalign_free(deinterleavedImpulseBuffer);
            return nil;
        }

        impulse_responses = (COMPLEX_SPLIT *) calloc(sizeof(COMPLEX_SPLIT), channels * 2);
        if (!impulse_responses) {
            memalign_free(deinterleavedImpulseBuffer);
            return nil;
        }

        for (size_t i = 0; i < channels; ++i) {
            impulse_responses[i * 2 + 0].realp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
            impulse_responses[i * 2 + 0].imagp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
            impulse_responses[i * 2 + 1].realp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
            impulse_responses[i * 2 + 1].imagp = (float *) memalign_alloc(16, sizeof(float) * fftSizeOver2);
            
            if (!impulse_responses[i * 2 + 0].realp || !impulse_responses[i * 2 + 0].imagp ||
                !impulse_responses[i * 2 + 1].realp || !impulse_responses[i * 2 + 1].imagp) {
                memalign_free(deinterleavedImpulseBuffer);
                return nil;
            }
            
            int leftInChannel;
            int rightInChannel;
            
            if (impulseChannels == 7) {
                leftInChannel = speakers_to_hesuvi_7[channels-1][0][i];
                rightInChannel = speakers_to_hesuvi_7[channels-1][1][i];
            }
            else {
                leftInChannel = speakers_to_hesuvi_14[channels-1][0][i];
                rightInChannel = speakers_to_hesuvi_14[channels-1][1][i];
            }
            
            if (leftInChannel == -1 || rightInChannel == -1) {
                float * temp;
                if (impulseChannels == 7) {
                    temp = (float *) malloc(sizeof(float) * fftSize);
                    if (!temp) {
                        memalign_free(deinterleavedImpulseBuffer);
                        return nil;
                    }

                    cblas_scopy((int)fftSize, deinterleavedImpulseBuffer + 4 * fftSize, 1, temp, 1);
                    vDSP_vadd(temp, 1, deinterleavedImpulseBuffer + 5 * fftSize, 1, temp, 1, fftSize);
                    
                    vDSP_ctoz((DSPComplex *)temp, 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
                    vDSP_ctoz((DSPComplex *)temp, 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);
                }
                else {
                    temp = (float *) malloc(sizeof(float) * fftSize * 2);
                    if (!temp) {
                        memalign_free(deinterleavedImpulseBuffer);
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
            }
            else {
                vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + leftInChannel * fftSize), 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
                vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + rightInChannel * fftSize), 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);
            }
            
            vDSP_fft_zrip(fftSetup, &impulse_responses[i * 2 + 0], 1, log2n, FFT_FORWARD);
            vDSP_fft_zrip(fftSetup, &impulse_responses[i * 2 + 1], 1, log2n, FFT_FORWARD);
        }
        
        memalign_free(deinterleavedImpulseBuffer);
        
        left_result = (float *) memalign_alloc(16, sizeof(float) * fftSize);
        right_result = (float *) memalign_alloc(16, sizeof(float) * fftSize);
        if (!left_result || !right_result)
            return nil;
        
        prevOverlapLeft = (float *) memalign_alloc(16, sizeof(float) * fftSize);
        prevOverlapRight = (float *) memalign_alloc(16, sizeof(float) * fftSize);
        if (!prevOverlapLeft || !prevOverlapRight)
            return nil;

        left_mix_result = (float *) memalign_alloc(16, sizeof(float) * fftSize);
        right_mix_result = (float *) memalign_alloc(16, sizeof(float) * fftSize);
        if (!left_mix_result || !right_mix_result)
            return nil;
        
        prevOverlapLength = 0;
    }
    
    return self;
}

- (void)dealloc {
    if (fftSetup) vDSP_destroy_fftsetup(fftSetup);
    
    memalign_free(paddedSignal);
    
    memalign_free(signal_fft.realp);
    memalign_free(signal_fft.imagp);

    memalign_free(input_filtered_signal_per_channel[0].realp);
    memalign_free(input_filtered_signal_per_channel[0].imagp);
    memalign_free(input_filtered_signal_per_channel[1].realp);
    memalign_free(input_filtered_signal_per_channel[1].imagp);

    if (impulse_responses) {
        for (size_t i = 0; i < channelCount * 2; ++i) {
            memalign_free(impulse_responses[i].realp);
            memalign_free(impulse_responses[i].imagp);
        }
        free(impulse_responses);
    }

    memalign_free(left_result);
    memalign_free(right_result);
    
    memalign_free(prevOverlapLeft);
    memalign_free(prevOverlapRight);

    memalign_free(left_mix_result);
    memalign_free(right_mix_result);
}

- (void)process:(const float*)inBuffer sampleCount:(size_t)count toBuffer:(float *)outBuffer {
    const float scale = 1.0 / (8.0 * (float)fftSize);

    while (count > 0) {
        size_t countToDo = (count > bufferSize) ? bufferSize : count;

        vDSP_vclr(left_mix_result, 1, fftSize);
        vDSP_vclr(right_mix_result, 1, fftSize);

        for (size_t i = 0; i < channelCount; ++i) {
            cblas_scopy((int)countToDo, inBuffer + i, (int)channelCount, paddedSignal, 1);

            vDSP_vclr(paddedSignal + countToDo, 1, paddedBufferSize - countToDo);

            vDSP_ctoz((DSPComplex *)paddedSignal, 2, &signal_fft, 1, fftSizeOver2);

            vDSP_fft_zrip(fftSetup, &signal_fft, 1, log2n, FFT_FORWARD);

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

            vDSP_fft_zrip(fftSetup, &input_filtered_signal_per_channel[0], 1, log2n, FFT_INVERSE);
            vDSP_fft_zrip(fftSetup, &input_filtered_signal_per_channel[1], 1, log2n, FFT_INVERSE);

            vDSP_ztoc(&input_filtered_signal_per_channel[0], 1, (DSPComplex *)left_result, 2, fftSizeOver2);
            vDSP_ztoc(&input_filtered_signal_per_channel[1], 1, (DSPComplex *)right_result, 2, fftSizeOver2);

            vDSP_vadd(left_mix_result, 1, left_result, 1, left_mix_result, 1, fftSize);
            vDSP_vadd(right_mix_result, 1, right_result, 1, right_mix_result, 1, fftSize);
        }

        // Integrate previous overlap
        if (prevOverlapLength) {
            vDSP_vadd(prevOverlapLeft, 1, left_mix_result, 1, left_mix_result, 1, prevOverlapLength);
            vDSP_vadd(prevOverlapRight, 1, right_mix_result, 1, right_mix_result, 1, prevOverlapLength);
        }

        prevOverlapLength = (int)(fftSize - countToDo);

        cblas_scopy(prevOverlapLength, left_mix_result + countToDo, 1, prevOverlapLeft, 1);
        cblas_scopy(prevOverlapLength, right_mix_result + countToDo, 1, prevOverlapRight, 1);

        vDSP_vsmul(left_mix_result, 1, &scale, left_mix_result, 1, countToDo);
        vDSP_vsmul(right_mix_result, 1, &scale, right_mix_result, 1, countToDo);

        cblas_scopy((int)countToDo, left_mix_result, 1, outBuffer + 0, 2);
        cblas_scopy((int)countToDo, right_mix_result, 1, outBuffer + 1, 2);

        inBuffer += countToDo * channelCount;
        outBuffer += countToDo * 2;

        count -= countToDo;
    }
}

- (void)reset {
    prevOverlapLength = 0;
}

@end
