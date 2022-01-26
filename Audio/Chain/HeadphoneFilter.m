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

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(size_t)channels {
    self = [super init];
    
    if (self) {
        id<CogSource> source = [AudioSource audioSourceForURL:url];
        if (!source)
            return nil;

        if (![source open:url])
            return nil;
        
        id<CogDecoder> decoder = [AudioDecoder audioDecoderForSource:source];

        if (decoder == nil)
            return nil;

        if (![decoder open:source])
        {
            return nil;
        }
        
        NSDictionary *properties = [decoder properties];
        
        double sampleRateOfSource = [[properties objectForKey:@"sampleRate"] floatValue];
        
        int sampleCount = [[properties objectForKey:@"totalFrames"] intValue];
        int impulseChannels = [[properties objectForKey:@"channels"] intValue];
        
        if ([[properties objectForKey:@"floatingPoint"] boolValue] != YES ||
            [[properties objectForKey:@"bitsPerSample"] intValue] != 32 ||
            !([[properties objectForKey:@"endian"] isEqualToString:@"native"] ||
              [[properties objectForKey:@"endian"] isEqualToString:@"little"]) ||
            (impulseChannels != 14 && impulseChannels != 7))
            return nil;
        
        float * impulseBuffer = calloc(sizeof(float), (sampleCount + 1024) * sizeof(float) * impulseChannels);
        [decoder readAudio:impulseBuffer frames:sampleCount];
        [decoder close];
        decoder = nil;
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
            
            float * resampledImpulse = calloc(sizeof(float), (resampledCount + resamplerLatencyOut * 2 + 128) * sizeof(float) * impulseChannels);
            
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
        
        float * deinterleavedImpulseBuffer = (float *) memalign_calloc(128, sizeof(float), fftSize * impulseChannels);
        for (size_t i = 0; i < impulseChannels; ++i) {
            for (size_t j = 0; j < sampleCount; ++j) {
                deinterleavedImpulseBuffer[i * fftSize + j] = impulseBuffer[i + impulseChannels * j];
            }
        }
        
        free(impulseBuffer);
        
        paddedBufferSize = fftSize;
        fftSizeOver2 = (fftSize + 1) / 2;
        log2n = log2f(fftSize);
        log2nhalf = log2n / 2;
        
        fftSetup = vDSP_create_fftsetup(log2n, FFT_RADIX2);
        
        paddedSignal = (float *) memalign_calloc(128, sizeof(float), paddedBufferSize);
        
        signal_fft.realp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
        signal_fft.imagp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
        
        input_filtered_signal_per_channel[0].realp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
        input_filtered_signal_per_channel[0].imagp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
        input_filtered_signal_per_channel[1].realp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
        input_filtered_signal_per_channel[1].imagp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);

        impulse_responses = (COMPLEX_SPLIT *) calloc(sizeof(COMPLEX_SPLIT), channels * 2);

        // Symmetrical / no-echo sets
        const int8_t speakers_to_hesuvi_7[8][2][8] = {
            { { 6, }, { 6, } },                 // mono/center
            { { 0, 1 }, { 1, 0 } },              // left/right
            { { 0, 1, 6 }, { 1, 0, 6 } },       // left/right/center
            { { 0, 1, 4, 5 }, { 1, 0, 5, 4 } },// left/right/left back/right back
            { { 0, 1, 6, 4, 5 }, { 1, 0, 6, 5, 4 } }, // left/right/center/back left/back right
            { { 0, 1, 6, 6, 4, 5 }, { 1, 0, 6, 6, 5, 4 } }, // left/right/center/lfe(center)/back left/back right
            { { 0, 1, 6, 6, -1, 2, 3 }, { 1, 0, 6, 6, -1, 3, 2 } }, // left/right/center/lfe(center)/back center(special)/side left/side right
            { { 0, 1, 6, 6, 4, 5, 2, 3 }, { 1, 0, 6, 6, 5, 4, 3, 2 } } // left/right/center/lfe(center)/back left/back right/side left/side right
        };

        // Asymmetrical / echo sets
        const int8_t speakers_to_hesuvi_14[8][2][8] = {
            { { 6, }, { 13, } },                 // mono/center
            { { 0, 8 }, { 1, 7 } },              // left/right
            { { 0, 8, 6 }, { 1, 7, 13 } },       // left/right/center
            { { 0, 8, 4, 12 }, { 1, 7, 5, 11 } },// left/right/left back/right back
            { { 0, 8, 6, 4, 12 }, { 1, 7, 13, 5, 11 } }, // left/right/center/back left/back right
            { { 0, 8, 6, 6, 4, 12 }, { 1, 7, 13, 13, 5, 11 } }, // left/right/center/lfe(center)/back left/back right
            { { 0, 8, 6, 6, -1, 2, 10 }, { 1, 7, 13, 13, -1, 3, 9 } }, // left/right/center/lfe(center)/back center(special)/side left/side right
            { { 0, 8, 6, 6, 4, 12, 2, 10 }, { 1, 7, 13, 13, 5, 11, 3, 9 } } // left/right/center/lfe(center)/back left/back right/side left/side right
        };
        
        const int8_t * speakers_to_hesuvi[8][2][8] = (impulseChannels == 7) ? speakers_to_hesuvi_7 : speakers_to_hesuvi_14;
        
        for (size_t i = 0; i < channels; ++i) {
            impulse_responses[i * 2 + 0].realp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
            impulse_responses[i * 2 + 0].imagp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
            impulse_responses[i * 2 + 1].realp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
            impulse_responses[i * 2 + 1].imagp = (float *) memalign_calloc(128, sizeof(float), fftSizeOver2);
            
            int leftInChannel;
            int rightInChannel;
            
            if (impulseChannels == 7) {
                leftInChannel = speakers_to_hesuvi_7[channels-1][0][i];
                rightInChannel = speakers_to_hesuvi_7[channels-1][1][i];
            }
            else {
                leftInChannel = speakers_to_hesuvi_14[channels-1][0][i];
                rightInChannel = speakers_to_hesuvi_14[channels-1][0][i];
            }
            
            if (leftInChannel == -1 || rightInChannel == -1) {
                float * temp = calloc(sizeof(float), fftSize * 2);
                if (impulseChannels == 7) {
                    for (size_t i = 0; i < fftSize; i++) {
                        temp[i + fftSize] = temp[i] = deinterleavedImpulseBuffer[i + 2 * fftSize] + deinterleavedImpulseBuffer[i + 3 * fftSize];
                    }
                }
                else {
                    for (size_t i = 0; i < fftSize; i++) {
                        temp[i] = deinterleavedImpulseBuffer[i + 2 * fftSize] + deinterleavedImpulseBuffer[i + 9 * fftSize];
                        temp[i + fftSize] = deinterleavedImpulseBuffer[i + 3 * fftSize] + deinterleavedImpulseBuffer[i + 10 * fftSize];
                    }
                }
                
                vDSP_ctoz((DSPComplex *)temp, 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
                vDSP_ctoz((DSPComplex *)(temp + fftSize), 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);
                
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
        
        left_result = (float *) memalign_calloc(128, sizeof(float), fftSize);
        right_result = (float *) memalign_calloc(128, sizeof(float), fftSize);
        
        prevOverlap[0] = (float *) memalign_calloc(128, sizeof(float), fftSize);
        prevOverlap[1] = (float *) memalign_calloc(128, sizeof(float), fftSize);

        left_mix_result = (float *) memalign_calloc(128, sizeof(float), fftSize);
        right_mix_result = (float *) memalign_calloc(128, sizeof(float), fftSize);
        
        prevOverlapLength = 0;
    }
    
    return self;
}

- (void)dealloc {
    if (paddedSignal) memalign_free(paddedSignal);
    
    if (signal_fft.realp) memalign_free(signal_fft.realp);
    if (signal_fft.imagp) memalign_free(signal_fft.imagp);

    if (input_filtered_signal_per_channel[0].realp) memalign_free(input_filtered_signal_per_channel[0].realp);
    if (input_filtered_signal_per_channel[0].imagp) memalign_free(input_filtered_signal_per_channel[0].imagp);
    if (input_filtered_signal_per_channel[1].realp) memalign_free(input_filtered_signal_per_channel[1].realp);
    if (input_filtered_signal_per_channel[1].imagp) memalign_free(input_filtered_signal_per_channel[1].imagp);

    if (impulse_responses) {
        for (size_t i = 0; i < channelCount * 2; ++i) {
            if (impulse_responses[i].realp) memalign_free(impulse_responses[i].realp);
            if (impulse_responses[i].imagp) memalign_free(impulse_responses[i].imagp);
        }
        free(impulse_responses);
    }

    memalign_free(left_result);
    memalign_free(right_result);
    
    if (prevOverlap[0]) memalign_free(prevOverlap[0]);
    if (prevOverlap[1]) memalign_free(prevOverlap[1]);

    memalign_free(left_mix_result);
    memalign_free(right_mix_result);
}

- (void)process:(const float*)inBuffer sampleCount:(size_t)count toBuffer:(float *)outBuffer {
    float scale = 1.0 / (8.0 * (float)fftSize);
    
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
            vDSP_vadd(prevOverlap[0], 1, left_mix_result, 1, left_mix_result, 1, prevOverlapLength);
            vDSP_vadd(prevOverlap[1], 1, right_mix_result, 1, right_mix_result, 1, prevOverlapLength);
        }
        
        prevOverlapLength = (int)(fftSize - countToDo);
        
        cblas_scopy(prevOverlapLength, left_mix_result + countToDo, 1, prevOverlap[0], 1);
        cblas_scopy(prevOverlapLength, right_mix_result + countToDo, 1, prevOverlap[1], 1);
        
        vDSP_vsmul(left_mix_result, 1, &scale, left_mix_result, 1, countToDo);
        vDSP_vsmul(right_mix_result, 1, &scale, right_mix_result, 1, countToDo);
        
        cblas_scopy((int)countToDo, left_mix_result, 1, outBuffer + 0, 2);
        cblas_scopy((int)countToDo, right_mix_result, 1, outBuffer + 1, 2);
        
        inBuffer += countToDo * channelCount;
        outBuffer += countToDo * 2;
        
        count -= countToDo;
    }
}

@end
