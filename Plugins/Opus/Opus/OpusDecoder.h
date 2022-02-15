//
//  OpusDecoder.h
//  Opus
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "Plugin.h"
#import <Cocoa/Cocoa.h>

#import <opusfile.h>

@interface OpusFile : NSObject <CogDecoder> {
	id<CogSource> source;

	OggOpusFile* opusRef;
	int currentSection;
	int lastSection;

	BOOL seekable;
	int bitrate;
	int channels;
	long totalFrames;

	NSString *artist;
	NSString *albumartist;
	NSString *album;
	NSString *title;
	NSString *genre;
	NSNumber *year;
	NSNumber *track;
	NSNumber *disc;
	float replayGainAlbumGain;
	float replayGainTrackGain;

	NSData *albumArt;
}

@end
