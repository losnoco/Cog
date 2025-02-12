//
//  MusepackFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/23/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "Plugin.h"
#import <Cocoa/Cocoa.h>
#import <mpcdec/decoder.h>
#import <mpcdec/internal.h>
#import <mpcdec/mpcdec.h>

@interface MusepackDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	mpc_reader reader;
	mpc_demux *demux;
	mpc_streaminfo info;

	char buffer[MPC_FRAME_LENGTH * 4];
	int bufferFrames;
	
	MPC_SAMPLE_FORMAT sampleBuffer[MPC_DECODER_BUFFER_LENGTH];
	float floatBuffer[1024 * 2];

	int bitrate;
	float frequency;
	long totalFrames;
	long frame;
}
- (BOOL)writeToBuffer:(float *)sample_buffer fromBuffer:(const MPC_SAMPLE_FORMAT *)p_buffer frames:(unsigned)frames;

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

@end
