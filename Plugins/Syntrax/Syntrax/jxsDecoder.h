//
//  jxsDecoder.h
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

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
@end
