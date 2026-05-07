//
//  HLSMemorySource.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//
//  An in-memory CogSource implementation backed by a queue of NSData
//  chunks. The producer (HLSSegmentManager's fetcher thread) appends
//  segment payloads with -appendData:; the consumer (the underlying
//  audio decoder) calls -read: and the call blocks until enough data
//  is available or the stream is marked as ended / closed.
//
//  The source is non-seekable. Callers that need to "seek" should call
//  -reset and then re-feed segment data starting from the new playback
//  position; HLSDecoder does this when handling -seek:.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@interface HLSMemorySource : NSObject <CogSource>

- (instancetype)initWithURL:(NSURL *)url mimeType:(NSString *)mimeType;

// Append a freshly-fetched segment's bytes. Called from the segment
// manager's fetch thread; signals any blocked readers.
- (void)appendData:(NSData *)data;

// Mark that no further data will arrive (VOD reached the end of the
// playlist). A blocked reader will then return whatever is left and
// then return 0 to signal EOF.
- (void)markEndOfStream;

// Drop all queued data and reset position trackers. Called from the
// decoder thread when handling a seek.
- (void)reset;

// How many appended chunks are currently sitting in the queue (i.e.
// fetched but not yet fully consumed by the decoder). Used by the
// segment manager for backpressure.
- (NSUInteger)bufferedSegmentCount;

// Mutable URL / mime-type setters so HLSDecoder can decide what fake
// filename to expose to the underlying decoder once the first segment
// has been fetched and its content type is known.
- (void)setUrl:(NSURL *)url;
- (void)setMimeType:(NSString *)mimeType;

@end
