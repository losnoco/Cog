//
//  st3Decoder.h
//  st3play
//
//  Created by Christopher Snowhill on 03/17/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <st3play/st3play.h>

#import "Plugin.h"

@interface st3Decoder : NSObject <CogDecoder> {
	void *st3play;
    void *data;
    long size;
    int track_num;
    
	long framesLength;
    long totalFrames;
    long framesRead;
}
@end
