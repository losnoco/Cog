//
//  jxsDecoder.h
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Syntrax_c/jxs.h>
#import <Syntrax_c/jaytrax.h>

#import "Plugin.h"

@interface jxsDecoder : NSObject <CogDecoder> {
	JT1Song *synSong;
	JT1Player *synPlayer;
	int track_num;

	double sampleRate;
	long framesLength;
	long totalFrames;
	long framesRead;

	int16_t sampleBuffer[2048 * 2];
}

// The rate the decoder plays at.
+ (double)sampleRate;
// The length of track `track` of the song `player` has loaded, in frames at
// `sampleRate`, without and with the fade a looping track gets. Measuring
// plays the track through on `player`.
+ (void)getFramesLength:(long *)framesLength totalFrames:(long *)totalFrames forPlayer:(JT1Player *)player track:(int)track sampleRate:(double)sampleRate;
// The properties of a track `totalFrames` long at `sampleRate`.
+ (NSDictionary *)propertiesWithSampleRate:(double)sampleRate totalFrames:(long)totalFrames;

@end
