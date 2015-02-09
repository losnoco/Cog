//
//  VGMDecoder.h
//  vgmstream
//
//  Created by Christopher Snowhill on 02/25/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <vgmstream/vgmstream.h>
#import <vgmstream/streamfile.h>

#import "Plugin.h"

@interface VGMDecoder : NSObject <CogDecoder> {
    VGMSTREAM *stream;

    int sampleRate;
    int channels;
    int bitrate;
	long totalFrames;
    long framesLength;
    long framesFade;
    long framesRead;
}
@end
