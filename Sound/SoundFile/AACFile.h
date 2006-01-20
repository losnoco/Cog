//
//  AACFile.h
//  Cog
//
//  Created by Vincent Spader on 5/31/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <FAAD2/faad.h>

#import "SoundFile.h"

#define INPUT_BUFFER_SIZE 1024*16
#define SAMPLE_BUFFER_SIZE FAAD_MIN_STREAMSIZE*1024

@interface AACFile : SoundFile {
	FILE *inFd;
	NeAACDecHandle hAac;
	NeAACDecFrameInfo hInfo;

	unsigned long *seekTable;
	int seekTableLength;
	
	char buffer[SAMPLE_BUFFER_SIZE];
	int bufferAmount;
	
	char inputBuffer[INPUT_BUFFER_SIZE];
	int inputAmount;
}

@end
