//
//  VorbisFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Plugin.h"

//config.h things
#define __MACOSX__ 
#define HAVE_CONFIG_H

#import <Vorbis/vorbisfile.h>
#import <Vorbis/codec.h>

#undef __MACOSX__ 
#undef HAVE_CONFIG_H

@interface VorbisDecoder : NSObject <CogDecoder>
{
	id<CogSource> source;
	
	OggVorbis_File vorbisRef;
	int currentSection;
	int lastSection;
	
	BOOL seekable;
	int bitrate;
	int channels;
	float frequency;
	long totalFrames;
}



@end
