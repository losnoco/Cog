//
//  VorbisFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "Plugin.h"
#import <Cocoa/Cocoa.h>

// config.h things
#define __MACOSX__
#define HAVE_CONFIG_H

#import <vorbis/codec.h>
#import <vorbis/vorbisfile.h>

#undef __MACOSX__
#undef HAVE_CONFIG_H

@interface VorbisDecoder : NSObject <CogDecoder> {
	id<CogSource> source;

	OggVorbis_File vorbisRef;
	int currentSection;
	int lastSection;

	BOOL seekable;
	int bitrate;
	int channels;
	float frequency;
	long totalFrames;

	int metadataUpdateInterval;
	int metadataUpdateCount;

	NSDictionary *metaDict;
	NSDictionary *icyMetaDict;

	NSData *albumArt;
}

@end
