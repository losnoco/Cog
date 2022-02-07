//
//  CueSheetDecoder.h
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@class CueSheet;
@class CueSheetTrack;

@interface CueSheetDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	id<CogDecoder> decoder;

	int bytesPerFrame; // Number of bytes per frame, ie channels * (bitsPerSample/8)

	long framePosition; // Current position in frames.

	long trackStart; // Starting frame of track.
	long trackEnd; // Frames until end of track.

	BOOL embedded;
	BOOL noFragment;
	NSURL *baseURL;

	CueSheet *cuesheet;
	CueSheetTrack *track;
}

@end
