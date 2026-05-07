//
//  HLSSegmentManager.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//
//  Background segment fetcher. Owns an NSThread that:
//
//    1. Downloads the next playlist segment using HTTPSource and pushes
//       its bytes into the supplied HLSMemorySource.
//    2. Pauses when the memory source already has `bufferSize` segments
//       queued (backpressure).
//    3. Refreshes the playlist on `targetDuration / 2` second intervals
//       when the playlist is live, splicing newly-announced segments
//       onto the end of the existing segment list.
//    4. Marks the memory source EOF when a non-live playlist is fully
//       consumed, so the decoder can drain cleanly.
//
//  All public methods are safe to call from any thread.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@class HLSPlaylist;
@class HLSSegment;
@class HLSMemorySource;

@interface HLSSegmentManager : NSObject

@property (nonatomic, strong, readonly) HLSPlaylist *playlist;
@property (nonatomic, weak) HLSMemorySource *memorySource;

// Number of segments to keep buffered ahead of the decoder before
// applying backpressure. Default: 5.
@property (nonatomic) NSUInteger bufferSize;

// MIME type observed on the most recently downloaded segment, populated
// after the first segment arrives. Useful for choosing the fake
// filename to expose to the underlying decoder.
@property (nonatomic, copy, readonly) NSString *lastObservedMimeType;

- (instancetype)initWithPlaylist:(HLSPlaylist *)playlist;

// Synchronously download a single segment. The HLSDecoder uses this for
// the initial bytes it needs to feed FFmpeg's open_input. Returns nil
// on error (and fills *error if non-NULL).
- (NSData *)downloadSegment:(HLSSegment *)segment error:(NSError **)error;

// Convenience: download the segment at the given index in the
// playlist's current segments array. Updates lastObservedMimeType.
- (NSData *)downloadSegmentAtIndex:(NSInteger)index error:(NSError **)error;

// Start the background fetcher, beginning at the given playlist index.
// `index` is the next segment to fetch (typically 0 on initial open, or
// the last synchronously-downloaded index + 1).
- (void)startFetchingFromIndex:(NSInteger)index;

// Stop the background fetcher. Blocks until the thread exits.
- (void)stop;

// Total VOD duration, summed from all segment durations. Returns 0 for
// live playlists.
- (double)totalDuration;

@end
