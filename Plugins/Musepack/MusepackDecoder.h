//
//  MusepackFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/23/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <MPCDec/mpcdec.h>
#import "Plugin.h"

@interface MusepackDecoder : NSObject <CogDecoder>
{
	FILE *inFd;
	mpc_decoder decoder;
	mpc_reader_file reader;
	mpc_streaminfo info;

	char buffer[MPC_FRAME_LENGTH*4];
	int bufferAmount;

	int bitrate;
	float frequency;	
	double length;
}
- (BOOL)writeSamplesToBuffer:(uint16_t *)sample_buffer fromBuffer:(const MPC_SAMPLE_FORMAT *)p_buffer ofSize:(unsigned)p_size;

- (NSDictionary *)properties;

@end
