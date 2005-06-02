//
//  VorbisFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"
#import <Vorbis/vorbisfile.h>
#import <Vorbis/codec.h>

@interface VorbisFile : SoundFile {
	FILE *inFd;
	OggVorbis_File vorbisRef;
	int currentSection;
}
- (BOOL)readInfo;
@end
