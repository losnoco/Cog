//
//  MusepackFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/23/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <mpcdec/mpcdec.h>
#import "Plugin.h"

@interface MusepackDecoder : NSObject <CogDecoder>
{
	id<CogSource> source;
	mpc_decoder decoder;
	mpc_streaminfo info;
	mpc_reader reader;
	
	char buffer[MPC_FRAME_LENGTH*4];
	int bufferFrames;

	int bitrate;
	float frequency;	
	long totalFrames;
}
- (BOOL)writeToBuffer:(float *)sample_buffer fromBuffer:(const MPC_SAMPLE_FORMAT *)p_buffer frames:(unsigned)frames;

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

@end
