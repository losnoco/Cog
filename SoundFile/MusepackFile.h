//
//  MusepackFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/23/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <MPCDec/mpcdec.h>
#import "SoundFile.h"


@interface MusepackFile : SoundFile {
	FILE *inFd;
	mpc_decoder decoder;
	mpc_reader_file reader;
    mpc_streaminfo info;

//	int undecodedSize;
	
    char buffer[MPC_FRAME_LENGTH*4];
	int bufferAmount;
}
- (BOOL)writeSamplesToBuffer:(uint16_t *)sample_buffer fromBuffer:(const MPC_SAMPLE_FORMAT *)p_buffer ofSize:(unsigned)p_size;

//- (FILE *)inFd;
//- (int)undecodedSize;

@end
