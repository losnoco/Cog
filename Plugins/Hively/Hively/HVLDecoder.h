//
//  HVLDecoder.h
//  Hively
//
//  Created by Christopher Snowhill on 10/29/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <HivelyPlayer/hvl_replay.h>

#import "Plugin.h"

@interface HVLDecoder : NSObject <CogDecoder> {
	struct hvl_tune *tune;
	int32_t *buffer;
	int trackNumber;

	double sampleRate;

	long totalFrames;
	long framesLength;
	long framesFade;
	long framesRead;
	long framesInBuffer;
	
	float sampleBuffer[1024 * 2];
}

// The rate tunes play at, from the user's settings.
+ (double)sampleRate;
// How many frames `subsong` of `tune` plays at `sampleRate` before its fade:
// twice through, as the decoder plays it. Leaves the tune at the start of
// `subsong`.
+ (long)framesLengthOfTune:(struct hvl_tune *)tune subsong:(int)subsong sampleRate:(double)sampleRate;
// The frames of fade that follow, from the user's settings.
+ (long)framesFadeForSampleRate:(double)sampleRate;
// The properties of a subsong `totalFrames` long, fade included.
+ (NSDictionary *)propertiesWithSampleRate:(double)sampleRate totalFrames:(long)totalFrames;

@end
