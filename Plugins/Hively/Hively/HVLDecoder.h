//
//  HVLDecoder.h
//  Hively
//
//  Created by Christopher Snowhill on 10/29/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <HivelyPlayer/hvl_replay.h>

#import "Plugin.h"

@interface HVLDecoder : NSObject <CogDecoder> {
    struct hvl_tune *tune;
    int32_t *buffer;
    long trackNumber;
    
	long totalFrames;
    long framesLength;
    long framesFade;
    long framesRead;
    long framesInBuffer;
}
@end
