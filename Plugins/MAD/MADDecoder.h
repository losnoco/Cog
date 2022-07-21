//
//  MADFile.h
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "MAD/mad.h"

#import "Plugin.h"

#define INPUT_BUFFER_SIZE 5 * 8192

@interface MADDecoder : NSObject <CogDecoder> {
	struct mad_stream _stream;
	struct mad_frame _frame;
	struct mad_synth _synth;

	unsigned char _inputBuffer[INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];
	float *_outputBuffer;
	long _outputFrames;
	long _currentOutputFrames;
	long _fileSize;

	id<CogSource> _source;

	BOOL _firstFrame;

	// For gapless playback of mp3s
	BOOL _foundXingHeader;
	BOOL _foundLAMEHeader;
	BOOL _foundVBRIHeader;
	BOOL _foundID3v2;
	BOOL _foundiTunSMPB;

	long _framesDecoded;
	uint16_t _startPadding;
	uint16_t _endPadding;

	BOOL inputEOF;

	int channels;
	float sampleRate;
	int bitrate;
	long totalFrames;
	long framesToSkip;
	int layer;

	int metadataUpdateInterval;
	int metadataUpdateCount;

	NSString *genre;
	NSString *album;
	NSString *artist;
	NSString *title;
}

- (int)decodeMPEGFrame;

@end
