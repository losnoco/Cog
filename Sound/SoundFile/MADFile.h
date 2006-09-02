//
//  MADFile.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 6/17/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"
#undef HAVE_CONFIG_H
#import "MAD/mad.h"

#define INPUT_BUFFER_SIZE 5*8192


@interface MADFile : SoundFile {
	struct mad_stream _stream;
	struct mad_frame _frame;
	struct mad_synth _synth;
	mad_timer_t _timer;
	mad_timer_t _duration;
	unsigned char _inputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];
	unsigned char *_outputBuffer;
	int _outputAvailable;
	int _fileSize;
	
	FILE *_inFd;
	
	BOOL _seekSkip;

	//For gapless playback of mp3s
	BOOL _gapless;
	long _currentFrame;
	int _startPadding;
	int _endPadding;
}

@end
