//
//  QuicktimeDecoder.h
//  Quicktime
//
//  Created by Vincent Spader on 6/10/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <AudioToolbox/AudioToolbox.h>
#import "Quicktime/Movies.h"

#import "Plugin.h"

@interface QuicktimeDecoder : NSObject <CogDecoder> {
	AudioStreamBasicDescription	_asbd;
	MovieAudioExtractionRef _extractionSessionRef;

	int bitrate;
	int bitsPerSample;
	int channels;
	float frequency;
	double length;
	
	unsigned long _totalFrames;
}

@end
