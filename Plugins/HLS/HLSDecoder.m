//
//  HLSDecoder.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Heavily edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSDecoder.h"
#import "HLSPlaylistParser.h"
#import "HLSPlaylist.h"
#import "HLSSegmentManager.h"
#import "HLSMemorySource.h"
#import "HLSSegment.h"
#import "HLSVariant.h"
#import "Logging.h"

static void *kHLSDecoderContext = &kHLSDecoderContext;

@implementation HLSDecoder

+ (NSArray *)fileTypes {
	return @[@"m3u8"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/vnd.apple.mpegurl",
	         @"audio/mpegurl",
	         @"application/x-mpegURL"];
}

+ (float)priority {
	return 16.0f;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"HTTP Live Streaming Playlist", @"playlist.icns", @"m3u8"]
	];
}

- (NSDictionary *)properties {
	NSMutableDictionary *properties = [[decoder properties] mutableCopy];

	if (!isLiveStream && totalFrames > 0) {
		[properties setObject:@(totalFrames) forKey:@"totalFrames"];
	}
	else {
		[properties setObject:@(-1) forKey:@"totalFrames"];
	}

	return [NSDictionary dictionaryWithDictionary:properties];
}

- (NSDictionary *)metadata {
	if (decoder) {
		return [decoder metadata];
	}
	return @{};
}

- (BOOL)open:(id<CogSource>)s {
	source = s;
	sourceURL = [s url];

	if (![[sourceURL scheme] isEqualToString:@"http"] &&
	    ![[sourceURL scheme] isEqualToString:@"https"]) {
		return NO;
	}

	DLog(@"HLS: Opening %@", sourceURL);

	NSError *error = nil;
	playlist = [HLSPlaylistParser parsePlaylistFromSource:source error:&error];

	if (!playlist) {
		ALog(@"HLS: Failed to parse playlist: %@", error);
		return NO;
	}

	if (playlist.isMasterPlaylist) {
		DLog(@"HLS: Master playlist detected, selecting highest quality variant");

		HLSVariant *selectedVariant = nil;
		NSInteger maxBandwidth = -1;

		for (HLSVariant *variant in playlist.variants) {
			if (variant.bandwidth > maxBandwidth) {
				maxBandwidth = variant.bandwidth;
				selectedVariant = variant;
			}
		}

		if (!selectedVariant) {
			ALog(@"HLS: No variants found in master playlist");
			return NO;
		}

		DLog(@"HLS: Selected variant - bandwidth: %ld, url: %@", (long)selectedVariant.bandwidth, selectedVariant.url);

		Class audioSourceClass = NSClassFromString(@"AudioSource");
		id<CogSource> variantSource = [audioSourceClass audioSourceForURL:selectedVariant.url];

		if (!variantSource || ![variantSource open:selectedVariant.url]) {
			ALog(@"HLS: Failed to open variant playlist");
			return NO;
		}

		playlist = [HLSPlaylistParser parsePlaylistFromSource:variantSource error:&error];
		[variantSource close];

		if (!playlist || playlist.isMasterPlaylist) {
			ALog(@"HLS: Failed to parse variant playlist");
			return NO;
		}
	}

	isLiveStream = playlist.isLiveStream;
	DLog(@"HLS: Playlist loaded - segments: %lu, live: %d, targetDuration: %ld",
	     (unsigned long)[playlist.segments count], isLiveStream, (long)playlist.targetDuration);

	if ([playlist.segments count] == 0) {
		ALog(@"HLS: No segments in playlist");
		return NO;
	}

	segmentManager = [[HLSSegmentManager alloc] initWithPlaylist:playlist];
	segmentManager.source = source;

	NSString *mimeType = @"audio/mpeg";

	memorySource = [[HLSMemorySource alloc] initWithURL:sourceURL mimeType:mimeType];

	BOOL filenameSet = NO;
	for (NSInteger i = 0; i < 3 && i < [playlist.segments count]; i++) {
		NSError *segError = nil;
		NSData *segmentData = [segmentManager getSegmentAtIndex:i error:&segError];

		if (segmentData) {
			[memorySource appendData:segmentData];
			[memorySource setMimeType:segmentManager.mimeType];
			
			if(!filenameSet) {
				NSString *fakeFilename = @"";
				if([segmentManager.mimeType isEqualToString:@"audio/aac"]) {
					fakeFilename = @"stream.aac";
				} else if([segmentManager.mimeType isEqualToString:@"audio/mpeg"]) {
					fakeFilename = @"stream.mp3";
				} else if([segmentManager.mimeType isEqualToString:@"audio/mp4"]) {
					fakeFilename = @"stream.m4a";
				} else if([segmentManager.mimeType isEqualToString:@"video/mp2t"]) {
					fakeFilename = @"stream.ts";
				}
				
				NSURL *fakeSourceURL = [[sourceURL URLByDeletingLastPathComponent] URLByAppendingPathComponent:fakeFilename];
				[memorySource setUrl:fakeSourceURL];
				filenameSet = YES;
			}

			DLog(@"HLS: Buffered segment %ld", (long)i);

			segmentManager.currentAbsoluteIndex = i;
		}
		else {
			ALog(@"HLS: Failed to buffer segment %ld: %@", (long)i, segError);
		}
	}

	Class audioDecoderClass = NSClassFromString(@"AudioDecoder");
	decoder = [audioDecoderClass audioDecoderForSource:memorySource skipCue:NO];

	if (!decoder) {
		ALog(@"HLS: No decoder found for segment format");
		return NO;
	}

	if (![decoder open:memorySource]) {
		ALog(@"HLS: Failed to open decoder");

		Class ffmpegClass = NSClassFromString(@"FFMPEGDecoder");
		if (ffmpegClass) {
			decoder = [[ffmpegClass alloc] init];
			if (![decoder open:memorySource]) {
				ALog(@"HLS: FFmpeg decoder also failed");
				return NO;
			}
			DLog(@"HLS: Using FFmpeg decoder as fallback");
		}
		else {
			return NO;
		}
	}

	totalDuration = [segmentManager totalDuration];

	NSDictionary *properties = [decoder properties];
	
	totalFrames = (long)(totalDuration * [properties[@"sampleRate"] doubleValue]);

	DLog(@"HLS: Total duration: %.2f seconds, frames: %ld", totalDuration, totalFrames);

	DLog(@"HLS: Decoder opened successfully");

	if (isLiveStream) {
		[self startPlaylistRefresh];
	}

	[self registerObservers];

	return YES;
}

- (AudioChunk *)readAudio {
	// Proactively maintain buffer level - fetch next segments if needed
	if ([segmentManager needsMoreSegments]) {
		NSInteger nextIndex = segmentManager.currentAbsoluteIndex + 1;
		if ([segmentManager isSegmentBuffered:nextIndex]) {
			NSError *error = nil;
			NSData *segmentData = [segmentManager getSegmentAtIndex:nextIndex error:&error];
			if (segmentData) {
				[memorySource appendData:segmentData];
				segmentManager.currentAbsoluteIndex = nextIndex;
				DLog(@"HLS: Prefetched and buffered segment %ld", (long)nextIndex);
			} else {
				ALog(@"HLS: Failed to prefetch segment %ld: %@", (long)nextIndex, error);
			}
		}
	}

	// Check if buffer is running low and needs immediate refill
	if (memorySource.segmentsBuffered < 2) {
		NSInteger nextIndex = segmentManager.currentAbsoluteIndex + 1;
		if ([segmentManager isSegmentBuffered:nextIndex]) {
			NSError *error = nil;
			NSData *segmentData = [segmentManager getSegmentAtIndex:nextIndex error:&error];

			if (segmentData) {
				[memorySource appendData:segmentData];
				segmentManager.currentAbsoluteIndex = nextIndex;
				DLog(@"HLS: Appended segment %ld due to low buffer", (long)nextIndex);
			} else {
				ALog(@"HLS: Failed to get segment for buffer refill: %@", error);
			}
		} else if (isLiveStream) {
			// Try to refresh playlist for live streams
			NSError *error = nil;
			[segmentManager refreshPlaylist:&error];

			nextIndex = segmentManager.currentAbsoluteIndex + 1;
			if ([segmentManager isSegmentBuffered:nextIndex]) {
				NSError *segError = nil;
				NSData *segmentData = [segmentManager getSegmentAtIndex:nextIndex error:&segError];

				if (segmentData) {
					[memorySource appendData:segmentData];
					segmentManager.currentAbsoluteIndex = nextIndex;
					DLog(@"HLS: Live stream appended segment %ld", (long)nextIndex);
				}
			}
		}
	}

	// Try to read from decoder
	return [decoder readAudio];
}

- (long)seek:(long)frame {
	if (isLiveStream) {
		return -1;
	}

	NSInteger targetSegment = [segmentManager segmentIndexForFrame:frame];
	long offsetInSegment = [segmentManager offsetInFrame:frame forSegment:targetSegment];

	DLog(@"HLS: Seeking to frame %ld (segment %ld, offset %ld)", frame, (long)targetSegment, offsetInSegment);

	if (targetSegment < 0 || targetSegment >= [playlist.segments count]) {
		return -1;
	}

	NSError *error = nil;
	NSData *segmentData = [segmentManager getSegmentAtIndex:targetSegment error:&error];

	if (!segmentData) {
		ALog(@"HLS: Failed to get segment for seek: %@", error);
		return -1;
	}

	[memorySource resetPosition];
	[memorySource appendData:segmentData];

	for (NSInteger i = targetSegment + 1; i < targetSegment + 3 && i < [playlist.segments count]; i++) {
		NSData *nextSegmentData = [segmentManager getSegmentAtIndex:i error:nil];
		if (nextSegmentData) {
			[memorySource appendData:nextSegmentData];
		}
	}

	segmentManager.currentAbsoluteIndex = targetSegment;

	if (![decoder open:memorySource]) {
		ALog(@"HLS: Failed to reopen decoder after seek");
		return -1;
	}

	long result = [decoder seek:offsetInSegment];

	if (result < 0) {
		ALog(@"HLS: Decoder seek failed");
		return -1;
	}

	DLog(@"HLS: Seek successful to frame %ld", result);
	return result;
}

- (void)close {
	[self stopPlaylistRefresh];

	if (observersAdded && decoder) {
		[decoder removeObserver:self forKeyPath:@"properties"];
		[decoder removeObserver:self forKeyPath:@"metadata"];
		observersAdded = NO;
	}

	if (decoder) {
		[decoder close];
		decoder = nil;
	}

	if (segmentManager) {
		segmentManager = nil;
	}

	if (memorySource) {
		[memorySource close];
		memorySource = nil;
	}

	if (playlist) {
		playlist = nil;
	}

	source = nil;
}

- (void)dealloc {
	[self close];
}

- (void)startPlaylistRefresh {
	if (!isLiveStream || playlistRefreshTimer) {
		return;
	}

	NSTimeInterval refreshInterval = (double)playlist.targetDuration / 2.0;
	if (refreshInterval < 1.0) {
		refreshInterval = 1.0;
	}

	DLog(@"HLS: Starting playlist refresh timer (%.1f seconds)", refreshInterval);

	playlistRefreshTimer = [NSTimer scheduledTimerWithTimeInterval:refreshInterval
	                                                        target:self
	                                                      selector:@selector(refreshPlaylist)
	                                                      userInfo:nil
	                                                       repeats:YES];
}

- (void)stopPlaylistRefresh {
	if (playlistRefreshTimer) {
		[playlistRefreshTimer invalidate];
		playlistRefreshTimer = nil;
	}
}

- (void)refreshPlaylist {
	if (!isLiveStream) {
		return;
	}

	DLog(@"HLS: Refreshing playlist");

	NSError *error = nil;
	[segmentManager refreshPlaylist:&error];

	if (error) {
		ALog(@"HLS: Playlist refresh failed: %@", error);
	}
}

- (void)registerObservers {
	if (!observersAdded && decoder) {
		[decoder addObserver:self forKeyPath:@"properties" options:0 context:kHLSDecoderContext];
		[decoder addObserver:self forKeyPath:@"metadata" options:0 context:kHLSDecoderContext];
		observersAdded = YES;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if (context == kHLSDecoderContext) {
		[self willChangeValueForKey:keyPath];
		[self didChangeValueForKey:keyPath];
	}
	else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

@end
