//
//  HCDecoder.h
//  HighlyComplete
//
//  Created by Christopher Snowhill on 9/30/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "Plugin.h"
#include "circular_buffer.h"
#import <Cocoa/Cocoa.h>

@interface HCDecoder : NSObject <CogDecoder, CogMetadataReader> {
	id<CogSource> currentSource;
	BOOL hintAdded;
	NSString *currentUrl;
	uint8_t *emulatorCore;
	void *emulatorExtra;

	circular_buffer<int16_t> silence_test_buffer;

	NSDictionary *metadataList;

	int tagLengthMs;
	int tagFadeMs;

	int type;

	int sampleRate;

	long totalFrames;
	long framesRead;
	long framesLength;

	BOOL usfRemoveSilence;
	long silenceSeconds; // depends on the format
}

@end
