//
//  VGMDecoder.h
//  vgmstream
//
//  Created by Christopher Snowhill on 02/25/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <libvgmstream/libvgmstream.h>

#import "Plugin.h"

@interface VGMInfoCache : NSObject {
	NSMutableDictionary *storage;
}

+ (id)sharedCache;

- (void)stuffURL:(NSURL *)url stream:(libvgmstream_t *)stream;
- (NSDictionary *)getPropertiesForURL:(NSURL *)url;
- (NSDictionary *)getMetadataForURL:(NSURL *)url;

@end

@interface VGMDecoder : NSObject <CogDecoder> {
	libvgmstream_t *stream;

	BOOL formatFloat;

	BOOL playForever;
	BOOL canPlayForever;
	int loopCount;
	double fadeTime;
	int sampleRate;
	int channels;
	int bps;
	int bitrate;
	long totalFrames;
	long framesRead;
}
@end
