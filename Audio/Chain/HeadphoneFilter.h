//
//  HeadphoneFilter.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 1/24/22.
//

#ifndef HeadphoneFilter_h
#define HeadphoneFilter_h

#import <Cocoa/Cocoa.h>
#import <Accelerate/Accelerate.h>

@interface HeadphoneFilter : NSObject {
    FFTSetup        fftSetup;
    
    size_t          fftSize;
    size_t          fftSizeOver2;
    size_t          log2n;
    size_t          log2nhalf;
    size_t          bufferSize;
    size_t          paddedBufferSize;
    size_t          channelCount;
    
    double          sampleRate;
    
    COMPLEX_SPLIT   signal_fft;
    COMPLEX_SPLIT   input_filtered_signal_per_channel[2];
    COMPLEX_SPLIT * impulse_responses;
    
    float         * left_result;
    float         * right_result;
    
    float         * left_mix_result;
    float         * right_mix_result;
    
    float         * paddedSignal;
    
    float         * prevOverlap[2];
    
    int             prevOverlapLength;
}

+ (BOOL)validateImpulseFile:(NSURL *)url;

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(size_t)channels;

- (void)process:(const float*)inBuffer sampleCount:(size_t)count toBuffer:(float *)outBuffer;

@end

#endif /* HeadphoneFilter_h */
