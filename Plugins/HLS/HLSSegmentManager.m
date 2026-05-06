//
//  HLSSegmentManager.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Heavily edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSSegmentManager.h"
#import "HLSPlaylist.h"
#import "HLSSegment.h"
#import "HLSPlaylistParser.h"
#import "HLSVariant.h"
#import "Logging.h"

@implementation HLSSegmentManager {
	NSMutableDictionary<NSNumber *, NSData *> *segmentCache;
	NSLock *cacheLock;

	NSMutableDictionary<NSNumber *, HLSSegment *> *playlistSegments;
}

- (id)initWithPlaylist:(HLSPlaylist *)playlist {
	self = [super init];
	if (self) {
		_playlist = playlist;
		_currentAbsoluteIndex = 0;
		_nextAbsoluteIndex = 0;
		_bufferSize = 5;
		_prefetchCount = 3;
		segmentCache = [NSMutableDictionary new];
		cacheLock = [NSLock new];
		playlistSegments = [NSMutableDictionary new];
		[self remapPlaylist];
	}
	return self;
}

- (void)remapPlaylist {
	NSUInteger index = _nextAbsoluteIndex;
	for(HLSSegment *segment in self.playlist.segments) {
		NSUInteger sequenceNumber = segment.sequenceNumber;
		playlistSegments[@(sequenceNumber)] = segment;
		if(index <= sequenceNumber)
			index = sequenceNumber + 1;
	}
	_nextAbsoluteIndex = index;
	if([playlistSegments count] > 20) {
		NSArray<NSNumber *> *keys = [playlistSegments.allKeys sortedArrayUsingSelector:@selector(compare:)];
		for(NSNumber *key in keys) {
			[playlistSegments removeObjectForKey:key];
			if([playlistSegments count] <= 10) break;
		}
	}
}

- (NSData *)downloadSegment:(HLSSegment *)segment error:(NSError **)error {
	if (!segment || !segment.url) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:1
			                         userInfo:@{NSLocalizedDescriptionKey: @"Invalid segment"}];
		}
		return nil;
	}

	Class audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> segmentSource = [audioSourceClass audioSourceForURL:segment.url];

	if (!segmentSource || ![segmentSource open:segment.url]) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:2
			                         userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Failed to open segment: %@", segment.url]}];
		}
		return nil;
	}

	segment.mimeType = [segmentSource mimeType];

	NSMutableData *segmentData = [NSMutableData data];
	char *buffer = malloc(8192);
	long bytesRead;

	if (!buffer) {
		[segmentSource close];
		if (error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:3
			                         userInfo:@{NSLocalizedDescriptionKey: @"Failed to allocate buffer"}];
		}
		return nil;
	}

	while ((bytesRead = [segmentSource read:buffer amount:8192]) > 0) {
		[segmentData appendBytes:buffer length:bytesRead];
	}

	free(buffer);
	[segmentSource close];

	if ([segmentData length] == 0) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
			                             code:3
			                         userInfo:@{NSLocalizedDescriptionKey: @"Segment is empty"}];
		}
		return nil;
	}

	DLog(@"HLS: Downloaded segment %ld - %lu bytes", (long)segment.sequenceNumber, (unsigned long)[segmentData length]);

	return segmentData;
}

- (BOOL)isSegmentBuffered:(NSInteger)index {
	NSNumber *absoluteKey = @(index);
	NSData *cachedData;

	[cacheLock lock];
	cachedData = segmentCache[absoluteKey];
	[cacheLock unlock];

	if(cachedData) return YES;

	// Find the segment in the current playlist that corresponds to this absolute index
	// For initial playlist load, map playlist index to absolute index
	HLSSegment *segment = playlistSegments[absoluteKey];

	return !!segment;
}

- (NSData *)getSegmentAtIndex:(NSInteger)index error:(NSError **)error {
	// Check if segment is already cached (using absolute index as key)
	NSNumber *absoluteKey = @(index);
	NSData *cachedData;

	[cacheLock lock];
	cachedData = segmentCache[absoluteKey];
	[cacheLock unlock];

	if (cachedData) {
		return cachedData;
	}

	HLSSegment *segment = playlistSegments[absoluteKey];

	if (!segment) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
										 code:5
									 userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Segment at absolute index %ld not read", (long)index]}];
		}
		return nil;
	}

	NSData *segmentData = [self downloadSegment:segment error:error];

	if (segmentData) {
		[cacheLock lock];
		segmentCache[absoluteKey] = segmentData;

		self.mimeType = segment.mimeType;

		// Maintain buffer size by removing oldest absolute indexes
		while ([segmentCache count] > self.bufferSize) {
			NSNumber *oldestKey = [segmentCache.allKeys sortedArrayUsingSelector:@selector(compare:)].firstObject;
			if (oldestKey) {
				[segmentCache removeObjectForKey:oldestKey];
				[playlistSegments removeObjectForKey:oldestKey];
				DLog(@"HLS: Evicted segment at absolute index %@", oldestKey);
			} else {
				break;
			}
		}

		[cacheLock unlock];
	}

	return segmentData;
}

- (BOOL)moveToNextSegment {
	// Always allow moving to next absolute index - this increases monotonically
	_currentAbsoluteIndex++;
	return YES;
}

- (BOOL)hasSegmentAtIndex:(NSInteger)index {
	NSNumber *key = @(index);
	BOOL hasCached;

	[cacheLock lock];
	hasCached = segmentCache[key] != nil;
	[cacheLock unlock];

	return hasCached;
}

- (NSInteger)segmentIndexForFrame:(long)frame {
	double totalDuration = 0.0;

	for (NSInteger i = 0; i < [self.playlist.segments count]; i++) {
		HLSSegment *segment = self.playlist.segments[i];
		double segmentDurationFrames = segment.duration * 44100.0;

		if (frame < totalDuration + segmentDurationFrames) {
			return i;
		}

		totalDuration += segmentDurationFrames;
	}

	return [self.playlist.segments count] - 1;
}

- (long)offsetInFrame:(long)frame forSegment:(NSInteger)segmentIndex {
	double totalDuration = 0.0;

	for (NSInteger i = 0; i < segmentIndex && i < [self.playlist.segments count]; i++) {
		HLSSegment *segment = self.playlist.segments[i];
		totalDuration += segment.duration;
	}

	long offset = (long)((frame - totalDuration * 44100.0));
	return MAX(0, offset);
}

- (BOOL)needsMoreSegments {
	// Check if we have downloaded enough segments ahead of our current absolute position
	NSInteger downloadedAhead = 0;

	[cacheLock lock];
	for (NSInteger i = _nextAbsoluteIndex; i < _currentAbsoluteIndex + _prefetchCount; i++) {
		if (!segmentCache[@(i)]) {
			break;
		}
		downloadedAhead++;
	}
	[cacheLock unlock];

	return downloadedAhead < _prefetchCount;
}

- (void)refreshPlaylist:(NSError **)error {
	if (!self.playlist.isLiveStream) {
		return;
	}
	
	Class audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> playlistSource = [audioSourceClass audioSourceForURL:self.playlist.url];
	
	if (!playlistSource || ![playlistSource open:self.playlist.url]) {
		if (error) {
			*error = [NSError errorWithDomain:@"HLSSegmentManager"
										 code:4
									 userInfo:@{NSLocalizedDescriptionKey: @"Failed to refresh playlist"}];
		}
		return;
	}
	
	HLSPlaylist *newPlaylist = [HLSPlaylistParser parsePlaylistFromSource:playlistSource error:error];
	
	[playlistSource close];
	
	if (newPlaylist && ![newPlaylist.segments isEqualToArray:self.playlist.segments]) {
		// Build URL set from existing playlist segments and currently loaded cached segments
		NSMutableSet<NSString *> *existingURLs = [NSMutableSet set];
		for (HLSSegment *seg in self.playlist.segments) {
			[existingURLs addObject:[seg.url absoluteString]];
		}
		
		// Also include URLs from currently cached segments
		[cacheLock lock];
		for (NSNumber *cacheKey in segmentCache) {
			NSInteger segIndex = [cacheKey integerValue];
			if (segIndex < [self.playlist.segments count]) {
				HLSSegment *cachedSeg = self.playlist.segments[segIndex];
				[existingURLs addObject:[cachedSeg.url absoluteString]];
			}
		}
		[cacheLock unlock];
		
		NSMutableArray *updatedSegments = [NSMutableArray arrayWithArray:self.playlist.segments];
		NSInteger trulyNewSegments = 0;
		NSInteger duplicateSegments = 0;
		
		for (NSInteger i = 0; i < [newPlaylist.segments count]; i++) {
			HLSSegment *newSegment = newPlaylist.segments[i];
			NSString *newURL = [newSegment.url absoluteString];

			if (![existingURLs containsObject:newURL]) {
				[updatedSegments addObject:newSegment];
				[existingURLs addObject:newURL];
				trulyNewSegments++;
			} else {
				DLog(@"HLS: Refresh skipping duplicate (already in playlist or cache): %@", newURL);
				duplicateSegments++;
			}
		}
		
		if (trulyNewSegments > 0) {
			self.playlist.segments = updatedSegments;
			[self remapPlaylist];
			DLog(@"HLS: Playlist refreshed - added %ld new segments, skipped %ld duplicates, total: %lu",
				 (long)trulyNewSegments, (long)duplicateSegments, (unsigned long)[updatedSegments count]);
		}
	}
}

- (double)totalDuration {
	double total = 0.0;

	for (HLSSegment *segment in self.playlist.segments) {
		total += segment.duration;
	}

	return total;
}

- (void)dealloc {
	[cacheLock lock];
	[segmentCache removeAllObjects];
	[cacheLock unlock];
}

@end
