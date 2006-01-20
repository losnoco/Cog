//
//  WaveFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/31/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SndFile/sndfile.h"
#import "SoundFile.h"

@interface WaveFile : SoundFile {
	SNDFILE *sndFile;
	SF_INFO info;
}

- (BOOL)readInfo;

@end
