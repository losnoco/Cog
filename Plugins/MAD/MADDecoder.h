//
//  MADFile.h
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#define HAVE_CONFIG_H
#import <Cocoa/Cocoa.h>
#undef HAVE_CONFIG_H

#import "MAD/mad.h"

#import "Plugin.h"

#define INPUT_BUFFER_SIZE 5*8192

@interface MADDecoder : NSObject <CogDecoder>
{
	struct mad_stream _stream;
	struct mad_frame _frame;
	struct mad_synth _synth;

	unsigned char _inputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];
	unsigned char *_outputBuffer;
	int _outputAvailable;
	int _fileSize;
	
	id<CogSource> _source;
	
	BOOL _firstFrame;
	
	//For gapless playback of mp3s
	BOOL _foundXingHeader;
	BOOL _foundLAMEHeader;

	long _samplesDecoded;
	long _totalSamples;
	uint16_t _startPadding;
	uint16_t _endPadding;
	
	int channels;
	int bitsPerSample;
	float frequency;
	int bitrate;
	double length;
}

@end
