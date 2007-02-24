//
//  VorbisFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"

//config.h things
#define __MACOSX__ 
#define HAVE_CONFIG_H

#import <Vorbis/vorbisfile.h>
#import <Vorbis/codec.h>

#undef __MACOSX__ 
#undef HAVE_CONFIG_H

@interface VorbisFile : SoundFile {
	FILE *inFd;
	OggVorbis_File vorbisRef;
	int currentSection;
}
- (BOOL)readInfo;
@end
