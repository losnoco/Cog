//
//  CueSheetDecoder.h
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@class CueSheet;
@class CueSheetTrack;

@interface CueSheetDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	id<CogDecoder> decoder;

	NSURL *sourceURL;

	BOOL seekedToStart;
	BOOL isDSD;

	int bytesPerFrame; // Number of bytes per frame, ie channels * (bitsPerSample/8)

	long framePosition; // Current position in frames.

	long trackStart; // Starting frame of track.
	long trackEnd; // Frames until end of track.

	BOOL embedded;
	BOOL noFragment;
	BOOL observersAdded;
	NSURL *baseURL;

	CueSheet *cuesheet;
	CueSheetTrack *track;
}

// The frames of its audio file that track `index` of `tracks` plays, from
// its start to the next track's, or to the end of the file, whose
// properties are `properties`.
+ (void)getStart:(long *)start end:(long *)end forTrackAtIndex:(NSUInteger)index ofTracks:(NSArray *)tracks embedded:(BOOL)embedded fileProperties:(NSDictionary *)properties;
// The properties of a track playing those frames of a file with
// `fileProperties`.
+ (NSDictionary *)propertiesWithFileProperties:(NSDictionary *)fileProperties start:(long)start end:(long)end;

@end
