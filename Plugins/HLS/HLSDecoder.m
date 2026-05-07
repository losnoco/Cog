//
//  HLSDecoder.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
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

#pragma mark - Plugin registration

+ (NSArray *)fileTypes {
	return @[@"m3u8", @"m3u"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/vnd.apple.mpegurl",
	         @"application/x-mpegurl",
	         @"audio/mpegurl",
	         @"audio/x-mpegurl"];
}

+ (float)priority {
	// Higher than FFmpeg's 1.5 so .m3u8 is routed here. The legacy m3u
	// container plugin (priority 1.0) handles non-HLS m3u via fileTypes,
	// but HLS playlists are signaled by their content (#EXTM3U +
	// #EXT-X-* tags), so we still get first dibs on the extension here
	// and fall back gracefully if the playlist isn't a real HLS one.
	return 16.0f;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"HTTP Live Streaming Playlist", @"playlist.icns", @"m3u8"]
	];
}

#pragma mark - Properties / metadata (delegated)

- (NSDictionary *)properties {
	NSMutableDictionary *properties = [[decoder properties] mutableCopy] ?: [NSMutableDictionary dictionary];

	if(!isLiveStream && totalFrames > 0) {
		properties[@"totalFrames"] = @(totalFrames);
	} else {
		// Live: totalFrames is unknown. Cog uses -1 to indicate that.
		properties[@"totalFrames"] = @(-1);
	}

	// Live streams can't be seeked; tell Cog so the UI disables seek.
	if(isLiveStream) {
		properties[@"seekable"] = @(NO);
	}

	return [properties copy];
}

- (NSDictionary *)metadata {
	if(decoder) return [decoder metadata];
	return @{};
}

#pragma mark - Open / Close

- (BOOL)open:(id<CogSource>)s {
	source = s;
	sourceURL = [s url];
	stateLock = [[NSLock alloc] init];

	NSString *scheme = [sourceURL scheme];
	if(![scheme isEqualToString:@"http"] && ![scheme isEqualToString:@"https"]) {
		ALog(@"HLS: refusing to open non-http(s) URL: %@", sourceURL);
		return NO;
	}

	DLog(@"HLS: opening %@", sourceURL);

	NSError *error = nil;
	playlist = [HLSPlaylistParser parsePlaylistFromSource:s error:&error];
	if(!playlist) {
		ALog(@"HLS: failed to parse playlist: %@", error.localizedDescription);
		return NO;
	}

	// If this is a master playlist, pick the highest-bandwidth variant
	// and re-fetch its media playlist.
	if(playlist.isMasterPlaylist) {
		HLSVariant *best = nil;
		for(HLSVariant *v in playlist.variants) {
			if(!best || v.bandwidth > best.bandwidth) best = v;
		}
		if(!best || !best.url) {
			ALog(@"HLS: master playlist has no usable variants");
			return NO;
		}

		DLog(@"HLS: master selected variant bandwidth=%ld url=%@",
		     (long)best.bandwidth, best.url);

		Class audioSourceClass = NSClassFromString(@"AudioSource");
		id<CogSource> variantSource = [audioSourceClass audioSourceForURL:best.url];
		if(!variantSource || ![variantSource open:best.url]) {
			ALog(@"HLS: failed to open variant playlist");
			return NO;
		}

		HLSPlaylist *media = [HLSPlaylistParser parsePlaylistFromSource:variantSource error:&error];
		[variantSource close];

		if(!media || media.isMasterPlaylist) {
			ALog(@"HLS: variant did not yield a media playlist: %@",
			     error.localizedDescription ?: @"nested master?");
			return NO;
		}
		playlist = media;
	}

	isLiveStream = playlist.isLiveStream;

	if([playlist.segments count] == 0) {
		ALog(@"HLS: media playlist has no segments");
		return NO;
	}

	// Reject encryption -- not implemented in this version (per plan).
	HLSSegment *firstSeg = playlist.segments[0];
	if(firstSeg.encrypted) {
		ALog(@"HLS: encrypted segments are not supported in this build");
		return NO;
	}

	// Build a clean URL for the in-memory source. Strip any fragment so
	// FFMPEGDecoder doesn't interpret it as a subsong index.
	NSURLComponents *comps = [NSURLComponents componentsWithURL:sourceURL resolvingAgainstBaseURL:NO];
	comps.fragment = nil;
	NSURL *cleanURL = comps.URL ?: sourceURL;

	memorySource = [[HLSMemorySource alloc] initWithURL:cleanURL mimeType:@"application/octet-stream"];

	// Synchronously fetch the first segment so FFmpeg has data to probe
	// against, and so we can detect the segment mime type before we
	// commit to opening a decoder.
	segmentManager = [[HLSSegmentManager alloc] initWithPlaylist:playlist];
	segmentManager.memorySource = memorySource;

	NSError *firstErr = nil;
	NSData *firstData = [segmentManager downloadSegmentAtIndex:0 error:&firstErr];
	if(!firstData) {
		ALog(@"HLS: failed to fetch first segment: %@", firstErr.localizedDescription);
		return NO;
	}

	NSString *segMime = segmentManager.lastObservedMimeType ?: @"audio/mpeg";
	[memorySource setMimeType:segMime];

	// Give the underlying decoder a fake filename so anything in Cog
	// that sniffs by extension behaves sanely. This is purely cosmetic
	// for FFmpeg, but harmless.
	NSString *fakeName = [self fakeFilenameForMimeType:segMime];
	if(fakeName) {
		NSURL *fakeURL = [[cleanURL URLByDeletingLastPathComponent] URLByAppendingPathComponent:fakeName];
		[memorySource setUrl:fakeURL];
	}

	[memorySource appendData:firstData];

	// Start the background fetcher from segment index 1.
	[segmentManager startFetchingFromIndex:1];

	// Open FFmpeg directly. HLS segments are most commonly MPEG-TS,
	// which the AudioDecoder.audioDecoderForSource extension lookup
	// doesn't route to anything (no plugin claims .ts), so going
	// straight to FFmpeg gives us the most coverage.
	Class ffmpegClass = NSClassFromString(@"FFMPEGDecoder");
	if(!ffmpegClass) {
		ALog(@"HLS: FFMPEGDecoder class not available");
		return NO;
	}
	decoder = [[ffmpegClass alloc] init];

	// FFMPEGDecoder branches on its `isHLS` ivar to skip duration /
	// start_time computation that doesn't make sense for streamed data.
	@try {
		[(NSObject *)decoder setValue:@YES forKey:@"isHLS"];
	} @catch(NSException *e) {
		DLog(@"HLS: couldn't set isHLS on decoder: %@", e);
	}

	if(![decoder open:memorySource]) {
		ALog(@"HLS: FFmpeg failed to open memory source");
		return NO;
	}

	// Compute totalDuration / totalFrames for VOD only.
	totalDuration = isLiveStream ? 0.0 : [segmentManager totalDuration];
	if(!isLiveStream && totalDuration > 0) {
		NSDictionary *props = [decoder properties];
		double sr = [props[@"sampleRate"] doubleValue];
		if(sr > 0) {
			totalFrames = (long)(totalDuration * sr);
		} else {
			totalFrames = 0;
		}
	} else {
		totalFrames = 0;
	}

	DLog(@"HLS: opened (live=%d duration=%.2fs frames=%ld segments=%lu)",
	     isLiveStream, totalDuration, totalFrames,
	     (unsigned long)[playlist.segments count]);

	[self registerObservers];
	return YES;
}

- (void)close {
	[stateLock lock];

	if(segmentManager) {
		[segmentManager stop];
	}

	if(observersAdded && decoder) {
		@try {
			[(NSObject *)decoder removeObserver:self forKeyPath:@"properties" context:kHLSDecoderContext];
			[(NSObject *)decoder removeObserver:self forKeyPath:@"metadata" context:kHLSDecoderContext];
		} @catch(NSException *e) {
			DLog(@"HLS: observer removal raised: %@", e);
		}
		observersAdded = NO;
	}

	if(decoder) {
		[decoder close];
		decoder = nil;
	}

	if(memorySource) {
		[memorySource close];
		memorySource = nil;
	}

	segmentManager = nil;
	playlist = nil;
	source = nil;

	[stateLock unlock];
}

- (void)dealloc {
	[self close];
}

#pragma mark - Read / Seek

- (AudioChunk *)readAudio {
	AudioChunk *chunk = [decoder readAudio];

	// If a seek left us with frames to discard inside the new starting
	// segment, drop chunks until we've consumed enough. Since FFmpeg
	// reads at least a few hundred samples per chunk, this is at most
	// a few iterations.
	while(chunk && pendingSkipFrames > 0) {
		size_t frames = [chunk frameCount];
		if((long)frames <= pendingSkipFrames) {
			pendingSkipFrames -= (long)frames;
			chunk = [decoder readAudio];
		} else {
			// Trim the leading pendingSkipFrames frames off this chunk.
			[chunk removeSamples:(size_t)pendingSkipFrames];
			pendingSkipFrames = 0;
			break;
		}
	}

	return chunk;
}

- (long)seek:(long)frame {
	if(isLiveStream) {
		DLog(@"HLS: seek requested on a live stream -- denied");
		return -1;
	}
	if(frame < 0) frame = 0;

	NSDictionary *props = [decoder properties];
	double sr = [props[@"sampleRate"] doubleValue];
	if(sr <= 0) {
		ALog(@"HLS: seek failed -- decoder hasn't reported a sample rate");
		return -1;
	}

	double targetTime = (double)frame / sr;

	// Find the segment that contains this time, and the start time of
	// that segment.
	double accum = 0.0;
	NSInteger targetIndex = 0;
	double segmentStartTime = 0.0;
	NSArray<HLSSegment *> *segs = playlist.segments;
	for(NSInteger i = 0; i < (NSInteger)[segs count]; i++) {
		HLSSegment *seg = segs[i];
		if(targetTime < accum + seg.duration) {
			targetIndex = i;
			segmentStartTime = accum;
			break;
		}
		accum += seg.duration;
		// If we run off the end, clamp to the last segment.
		targetIndex = i;
		segmentStartTime = accum - seg.duration;
	}

	long segmentStartFrame = (long)(segmentStartTime * sr);
	long offsetFrames = frame - segmentStartFrame;
	if(offsetFrames < 0) offsetFrames = 0;

	DLog(@"HLS: seek to frame=%ld -> segment=%ld start_frame=%ld offset=%ld",
	     frame, (long)targetIndex, segmentStartFrame, offsetFrames);

	// Try to fetch the new starting segment BEFORE tearing anything
	// down. If the network fetch fails we'd rather leave the existing
	// decoder running than drop the user into a broken state.
	NSError *err = nil;
	NSData *segData = [segmentManager downloadSegmentAtIndex:targetIndex error:&err];
	if(!segData) {
		ALog(@"HLS: seek fetch failed for segment %ld: %@",
		     (long)targetIndex, err.localizedDescription);
		return -1;
	}

	[stateLock lock];

	// Pause the fetcher and clear the in-flight memory buffer.
	[segmentManager stop];
	[memorySource reset];

	// Tear down the old FFmpeg decoder (also drops KVO observers).
	if(observersAdded && decoder) {
		@try {
			[(NSObject *)decoder removeObserver:self forKeyPath:@"properties" context:kHLSDecoderContext];
			[(NSObject *)decoder removeObserver:self forKeyPath:@"metadata" context:kHLSDecoderContext];
		} @catch(NSException *e) {
		}
		observersAdded = NO;
	}
	[decoder close];
	decoder = nil;

	// Feed the new starting segment and resume fetching one segment later.
	[memorySource appendData:segData];
	[segmentManager startFetchingFromIndex:targetIndex + 1];

	// Reopen FFmpeg against the freshly-primed memory source.
	Class ffmpegClass = NSClassFromString(@"FFMPEGDecoder");
	decoder = [[ffmpegClass alloc] init];
	@try {
		[(NSObject *)decoder setValue:@YES forKey:@"isHLS"];
	} @catch(NSException *e) {
	}
	if(![decoder open:memorySource]) {
		ALog(@"HLS: FFmpeg reopen after seek failed");
		[stateLock unlock];
		return -1;
	}

	[self registerObservers];

	// Discard the leading offsetFrames within the new segment so we
	// land on the requested frame.
	pendingSkipFrames = offsetFrames;

	[stateLock unlock];

	return frame;
}

#pragma mark - Helpers

- (NSString *)fakeFilenameForMimeType:(NSString *)mime {
	NSString *m = [mime lowercaseString];
	if([m hasPrefix:@"audio/aac"] || [m hasPrefix:@"audio/aacp"]) return @"stream.aac";
	if([m hasPrefix:@"audio/mpeg"] || [m hasPrefix:@"audio/mp3"]) return @"stream.mp3";
	if([m hasPrefix:@"audio/mp4"] || [m hasPrefix:@"audio/m4a"]) return @"stream.m4a";
	if([m hasPrefix:@"video/mp2t"] || [m hasPrefix:@"audio/mp2t"]) return @"stream.ts";
	if([m hasPrefix:@"video/mp4"]) return @"stream.mp4";
	return nil;
}

- (void)registerObservers {
	if(observersAdded || !decoder) return;
	@try {
		[(NSObject *)decoder addObserver:self
		                      forKeyPath:@"properties"
		                         options:0
		                         context:kHLSDecoderContext];
		[(NSObject *)decoder addObserver:self
		                      forKeyPath:@"metadata"
		                         options:0
		                         context:kHLSDecoderContext];
		observersAdded = YES;
	} @catch(NSException *e) {
		DLog(@"HLS: observer registration raised: %@", e);
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	if(context == kHLSDecoderContext) {
		[self willChangeValueForKey:keyPath];
		[self didChangeValueForKey:keyPath];
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

@end
