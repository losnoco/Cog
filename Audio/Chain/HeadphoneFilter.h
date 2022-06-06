//
//  HeadphoneFilter.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 1/24/22.
//

#ifndef HeadphoneFilter_h
#define HeadphoneFilter_h

#import <Cocoa/Cocoa.h>

#import "pffft.h"

@interface HeadphoneFilter : NSObject {
	PFFFT_Setup *fftSetup;

	size_t fftSize;
	size_t bufferSize;
	size_t paddedBufferSize;
	size_t channelCount;

	float *workBuffer;

	float **impulse_responses;

	float **prevInputs;

	float *left_result;
	float *right_result;

	float *paddedSignal;
}

+ (BOOL)validateImpulseFile:(NSURL *)url;

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(size_t)channels withConfig:(uint32_t)config;

- (void)process:(const float *)inBuffer sampleCount:(size_t)count toBuffer:(float *)outBuffer;

- (void)reset;

@end

#endif /* HeadphoneFilter_h */
