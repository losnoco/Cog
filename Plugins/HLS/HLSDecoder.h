//
//  HLSDecoder.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//
//  CogDecoder for HTTP Live Streaming (.m3u8) playlists. Parses the
//  playlist, picks the highest-bandwidth variant from a master playlist
//  if needed, downloads segments via a background thread, and feeds the
//  concatenated bytes to the FFmpeg decoder for actual audio decode.
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

	long pendingSkipFrames; // discard this many frames after a seek

	NSLock *stateLock;
}

@end
