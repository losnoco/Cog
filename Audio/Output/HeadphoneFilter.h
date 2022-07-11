//
//  HeadphoneFilter.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 1/24/22.
//

#ifndef HeadphoneFilter_h
#define HeadphoneFilter_h

#import <Accelerate/Accelerate.h>
#import <Cocoa/Cocoa.h>

@interface HeadphoneFilter : NSObject {
	vDSP_DFT_Setup dftSetupF;
	vDSP_DFT_Setup dftSetupB;

	int fftSize;
	int fftSizeOver2;
	int bufferSize;
	int paddedBufferSize;
	int channelCount;

	DSPSplitComplex signal_fft;
	DSPSplitComplex input_filtered_signal_per_channel[2];
	DSPSplitComplex input_filtered_signal_totals[2];
	DSPSplitComplex *impulse_responses;

	float **prevInputs;

	float *left_result;
	float *right_result;

	float *paddedSignal;
}

+ (BOOL)validateImpulseFile:(NSURL *)url;

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(int)channels withConfig:(uint32_t)config;

- (void)process:(const float *)inBuffer sampleCount:(int)count toBuffer:(float *)outBuffer;

- (void)reset;

@end

#endif /* HeadphoneFilter_h */
