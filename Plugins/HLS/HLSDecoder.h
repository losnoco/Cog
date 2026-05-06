//
//  HLSDecoder.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Mildly edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

@class HLSPlaylist;
@class HLSSegmentManager;
@class HLSMemorySource;

@interface HLSDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	id<CogDecoder> decoder;

	HLSPlaylist *playlist;
	HLSSegmentManager *segmentManager;
	HLSMemorySource *memorySource;

	NSURL *sourceURL;

	BOOL isLiveStream;
	BOOL observersAdded;

	long totalFrames;
	double totalDuration;
	NSTimer *playlistRefreshTimer;
	NSLock *stateLock;
}

@end
