//
//  FSurroundFilter.h
//  CogAudio
//
//  Created by Christopher Snowhill on 7/9/22.
//

#ifndef FSurroundFilter_h
#define FSurroundFilter_h

#import <Cocoa/Cocoa.h>

#import <stdint.h>

#define FSurroundChunkSize 4096

@interface FSurroundFilter : NSObject {
	void *decoder;
	void *params;
	double srate;
	uint32_t channelCount;
	uint32_t channelConfig;
}

- (id)initWithSampleRate:(double)srate;

- (uint32_t)channelCount;
- (uint32_t)channelConfig;
- (double)srate;

- (void)process:(const float *)samplesIn output:(float *)samplesOut count:(uint32_t)count;

@end

#endif /* FSurround_h */
