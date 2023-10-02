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
	int bufferSize;
	int paddedBufferSize;
	int channelCount;

	float **mirroredImpulseResponses;
	
	float **prevInputs;

	float *paddedSignal[2];
}

+ (BOOL)validateImpulseFile:(NSURL *)url;

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(int)channels withConfig:(uint32_t)config;

- (void)process:(const float *)inBuffer sampleCount:(int)count toBuffer:(float *)outBuffer;

- (void)reset;

@end

#endif /* HeadphoneFilter_h */
