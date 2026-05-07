//
//  HLSSegmentManager.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSSegmentManager.h"
#import "HLSPlaylist.h"
#import "HLSSegment.h"
#import "HLSMemorySource.h"
#import "HLSPlaylistParser.h"
#import "Logging.h"

// How long the fetch loop sleeps when there's nothing to do (no slot in
// the buffer and no refresh due). Short enough to react quickly to
// segment consumption / refresh deadlines, long enough to avoid
// burning CPU.
static const NSTimeInterval kFetchIdleSleep = 0.05; // 50 ms

// Maximum consecutive segment download failures before we apply
// progressive backoff and (for live streams only) skip past the
// offending segment. We never mark the memory source as EOF on a
// transient error: once the underlying decoder sees EOF it cannot be
// revived, so the right behavior on errors is to keep retrying. The
// user can stop playback if they want to give up.
static const NSInteger kMaxConsecutiveFailures = 5;

@interface HLSSegmentManager ()
@property (nonatomic, strong, readwrite) HLSPlaylist *playlist;
@property (nonatomic, copy, readwrite) NSString *lastObservedMimeType;
@end

@implementation HLSSegmentManager {
	NSThread *_fetchThread;
	NSCondition *_cond;

	// Guarded by _cond:
	NSInteger _nextFetchIndex;          // next index into playlist.segments to fetch
	BOOL _stopRequested;
	BOOL _running;

	// Live-stream playlist refresh:
	NSDate *_lastRefreshDate;
	NSMutableSet<NSNumber *> *_seenSequenceNumbers;

	// Touched only from the fetch thread:
	NSInteger _consecutiveFailures;
}

- (instancetype)initWithPlaylist:(HLSPlaylist *)playlist {
	self = [super init];
	if(self) {
		_playlist = playlist;
		_bufferSize = 5;
		_cond = [[NSCondition alloc] init];
		_nextFetchIndex = 0;
		_stopRequested = NO;
		_running = NO;
		_seenSequenceNumbers = [NSMutableSet set];
		for(HLSSegment *seg in playlist.segments) {
			[_seenSequenceNumbers addObject:@(seg.sequenceNumber)];
		}
	}
	return self;
}

- (void)dealloc {
	[self stop];
}

#pragma mark - Public

- (double)totalDuration {
	if(self.playlist.isLiveStream) return 0.0;
	double total = 0.0;
	for(HLSSegment *seg in self.playlist.segments) {
		total += seg.duration;
	}
	return total;
}

- (NSData *)downloadSegmentAtIndex:(NSInteger)index error:(NSError **)error {
	HLSSegment *seg = nil;
	if(index >= 0 && index < (NSInteger)[self.playlist.segments count]) {
		seg = self.playlist.segments[index];
	}
	if(!seg) {
		if(error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:1
			                         userInfo:@{NSLocalizedDescriptionKey: @"Segment index out of range"}];
		}
		return nil;
	}
	return [self downloadSegment:seg error:error];
}

- (NSData *)downloadSegment:(HLSSegment *)segment error:(NSError **)error {
	if(!segment || !segment.url) {
		if(error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:2
			                         userInfo:@{NSLocalizedDescriptionKey: @"Segment has no URL"}];
		}
		return nil;
	}

	Class audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> src = [audioSourceClass audioSourceForURL:segment.url];
	if(!src || ![src open:segment.url]) {
		if(error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:3
			                         userInfo:@{NSLocalizedDescriptionKey:
			    [NSString stringWithFormat:@"Failed to open segment: %@", segment.url]}];
		}
		return nil;
	}

	NSString *mime = [src mimeType];
	if([mime length]) {
		segment.mimeType = mime;
		self.lastObservedMimeType = mime;
	}

	NSMutableData *data = [NSMutableData data];
	uint8_t buf[16384];
	long got;
	while((got = [src read:buf amount:sizeof(buf)]) > 0) {
		[data appendBytes:buf length:got];
	}
	[src close];

	if([data length] == 0) {
		if(error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:4
			                         userInfo:@{NSLocalizedDescriptionKey:
			    [NSString stringWithFormat:@"Empty segment: %@", segment.url]}];
		}
		return nil;
	}

	DLog(@"HLS: fetched segment seq=%ld bytes=%lu (%@)",
	     (long)segment.sequenceNumber, (unsigned long)[data length], mime ?: @"unknown");
	return data;
}

- (void)startFetchingFromIndex:(NSInteger)index {
	[_cond lock];
	_nextFetchIndex = index;
	_stopRequested = NO;
	_lastRefreshDate = [NSDate date];
	BOOL needSpawn = !_running;
	_running = YES;
	[_cond broadcast];
	[_cond unlock];

	if(needSpawn) {
		_fetchThread = [[NSThread alloc] initWithTarget:self
		                                       selector:@selector(fetchLoop)
		                                         object:nil];
		_fetchThread.name = @"org.cogx.cog.hls-fetcher";
		[_fetchThread start];
	}
}

- (void)stop {
	[_cond lock];
	_stopRequested = YES;
	[_cond broadcast];
	NSThread *t = _fetchThread;
	[_cond unlock];

	// Wait for the thread to exit. NSThread doesn't have a direct join,
	// so spin on the `executing`/`finished` flags. The fetch loop
	// checks _stopRequested on every iteration, so this only takes one
	// in-flight HTTP request to drain in the worst case.
	if(t && t != [NSThread currentThread]) {
		while(!t.finished) {
			[NSThread sleepForTimeInterval:0.01];
		}
	}

	[_cond lock];
	_running = NO;
	_fetchThread = nil;
	[_cond unlock];
}

#pragma mark - Background fetch loop

- (void)fetchLoop {
	while(YES) {
		@autoreleasepool {
			[_cond lock];
			BOOL stop = _stopRequested;
			[_cond unlock];
			if(stop) break;

			BOOL didWork = NO;

			// 1. Live playlist refresh, if due.
			if(self.playlist.isLiveStream && [self isRefreshDue]) {
				[self refreshLivePlaylist];
				didWork = YES;
			}

			// 2. Fetch the next segment if we have room in the buffer.
			HLSMemorySource *mem = self.memorySource;
			NSUInteger buffered = [mem bufferedSegmentCount];
			if(buffered < self.bufferSize) {
				HLSSegment *next = nil;
				[_cond lock];
				if(_nextFetchIndex < (NSInteger)[self.playlist.segments count]) {
					next = self.playlist.segments[_nextFetchIndex];
				}
				[_cond unlock];

				if(next) {
					NSError *fetchErr = nil;
					NSData *data = [self downloadSegment:next error:&fetchErr];
					if(data) {
						[mem appendData:data];
						[_cond lock];
						_nextFetchIndex++;
						[_cond unlock];
						_consecutiveFailures = 0;
						didWork = YES;
					} else {
						ALog(@"HLS: segment fetch failed (%@): %@",
						     next.url, fetchErr.localizedDescription);
						[self handleFetchFailure];
					}
				} else if(!self.playlist.isLiveStream) {
					// VOD playlist exhausted -- nothing more will arrive.
					[mem markEndOfStream];
					DLog(@"HLS: VOD playlist exhausted, marking EOF");
					break;
				}
			}

			if(!didWork) {
				// Either the buffer is full or the live playlist has no
				// new segments yet; sleep briefly and try again.
				[NSThread sleepForTimeInterval:kFetchIdleSleep];
			}
		}
	}

	[_cond lock];
	_running = NO;
	[_cond unlock];
}

- (BOOL)isRefreshDue {
	NSTimeInterval interval = MAX((double)self.playlist.targetDuration / 2.0, 1.0);
	return _lastRefreshDate == nil
	    || [[NSDate date] timeIntervalSinceDate:_lastRefreshDate] >= interval;
}

- (void)refreshLivePlaylist {
	_lastRefreshDate = [NSDate date];

	Class audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> src = [audioSourceClass audioSourceForURL:self.playlist.url];
	if(!src || ![src open:self.playlist.url]) {
		ALog(@"HLS: failed to reopen playlist for refresh: %@", self.playlist.url);
		return;
	}

	NSError *parseErr = nil;
	HLSPlaylist *fresh = [HLSPlaylistParser parsePlaylistFromSource:src error:&parseErr];
	[src close];

	if(!fresh) {
		ALog(@"HLS: playlist refresh failed to parse: %@", parseErr.localizedDescription);
		return;
	}

	if(fresh.hasEndList) {
		// The publisher just declared the stream finite. Adopt the new
		// playlist exactly and let the loop drain it.
		self.playlist.isLiveStream = NO;
		self.playlist.hasEndList = YES;
	}

	NSMutableArray<HLSSegment *> *appended = nil;
	for(HLSSegment *seg in fresh.segments) {
		NSNumber *key = @(seg.sequenceNumber);
		if([_seenSequenceNumbers containsObject:key]) continue;

		if(!appended) appended = [NSMutableArray array];
		[appended addObject:seg];
		[_seenSequenceNumbers addObject:key];
	}

	if([appended count] == 0) return;

	NSMutableArray<HLSSegment *> *newSegments = [self.playlist.segments mutableCopy] ?: [NSMutableArray array];
	[newSegments addObjectsFromArray:appended];
	self.playlist.segments = newSegments;

	DLog(@"HLS: live refresh appended %lu new segment(s) (total: %lu)",
	     (unsigned long)[appended count], (unsigned long)[newSegments count]);
}

- (void)handleFetchFailure {
	_consecutiveFailures++;

	// Progressive backoff: 0.5s, 1s, 2s, 4s, 8s, then cap. Never longer
	// than 8s so that we recover briskly when the network returns.
	NSTimeInterval backoff = 0.5 * (NSTimeInterval)(1L << MIN(_consecutiveFailures - 1, 4L));
	if(backoff > 8.0) backoff = 8.0;
	[NSThread sleepForTimeInterval:backoff];

	if(_consecutiveFailures >= kMaxConsecutiveFailures && self.playlist.isLiveStream) {
		// Live stream: skip past the bad segment so we can keep up
		// with the broadcast. Leaves a glitch but avoids a permanent
		// hang. We deliberately do NOT mark the memory source as EOF
		// because the underlying decoder cannot be revived after EOF
		// and there's still useful data ahead.
		ALog(@"HLS: live stream skipping past unreachable segment");
		[_cond lock];
		_nextFetchIndex++;
		[_cond unlock];
		_consecutiveFailures = 0;
	}
	// VOD: keep retrying indefinitely. The user controls when to stop.
}

@end
