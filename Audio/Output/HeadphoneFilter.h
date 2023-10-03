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

#import <simd/simd.h>

@interface HeadphoneFilter : NSObject {
	NSURL *URL;

	int bufferSize;
	int paddedBufferSize;
	int channelCount;
	uint32_t config;

	float **mirroredImpulseResponses;
	
	float **prevInputs;

	float *paddedSignal[2];
}

+ (BOOL)validateImpulseFile:(NSURL *)url;

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(int)channels withConfig:(uint32_t)config withMatrix:(simd_float4x4)matrix;

- (void)reloadWithMatrix:(simd_float4x4)matrix;

- (void)process:(const float *)inBuffer sampleCount:(int)count toBuffer:(float *)outBuffer;

- (void)reset;

@end

#endif /* HeadphoneFilter_h */
