//
//  OpusDecoder.h
//  Opus
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "Plugin.h"
#import <Cocoa/Cocoa.h>

// config.h things
#define __MACOSX__
#define HAVE_CONFIG_H

#import <Opus/opusfile.h>

#undef __MACOSX__
#undef HAVE_CONFIG_H

@interface OpusFile : NSObject <CogDecoder> {
	id<CogSource> source;

	OggOpusFile* opusRef;
	int currentSection;
	int lastSection;

	BOOL seekable;
	int bitrate;
	int channels;
	long totalFrames;

	double track_gain;
	double album_gain;

	NSString *genre;
	NSString *album;
	NSString *artist;
	NSString *title;
}

@end
