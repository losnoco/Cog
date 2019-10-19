//
//  VGMDecoder.h
//  vgmstream
//
//  Created by Christopher Snowhill on 02/25/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <libvgmstream/vgmstream.h>
#import <libvgmstream/streamfile.h>

#import "Plugin.h"

@interface VGMInfoCache : NSObject {
    NSMutableDictionary *storage;
}

+(id)sharedCache;

-(void)stuffURL:(NSURL *)url stream:(VGMSTREAM *)stream;
-(NSDictionary*)getPropertiesForURL:(NSURL *)url;
-(NSDictionary*)getMetadataForURL:(NSURL *)url;

@end

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
