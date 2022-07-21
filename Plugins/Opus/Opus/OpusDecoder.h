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

	int metadataUpdateInterval;
	int metadataUpdateCount;

	NSDictionary *metaDict;
	NSDictionary *icyMetaDict;

	NSData *albumArt;

	float replayGainAlbumGain;
	float replayGainTrackGain;
}

@end
