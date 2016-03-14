//
//  jxsDecoder.h
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <Syntrax_c/file.h>
#import <Syntrax_c/syntrax.h>

#import "Plugin.h"

@interface jxsDecoder : NSObject <CogDecoder> {
	Song *synSong;
    Player *synPlayer;
    int track_num;
    
	long framesLength;
    long totalFrames;
    long framesRead;
}
@end
