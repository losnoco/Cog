//
//  CueSheetDecoder.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetDecoder.h"

#import "CueSheet.h"
#import "CueSheetContainer.h"
#import "CueSheetMetadataReader.h"

#import "NSDictionary+Merge.h"

#import "Logging.h"

@implementation CueSheetDecoder

static void *kCueSheetDecoderContext = &kCueSheetDecoderContext;

+ (NSArray *)fileTypes {
	return [CueSheetContainer fileTypes];
}

+ (NSArray *)mimeTypes {
	return [CueSheetContainer mimeTypes];
}

+ (float)priority {
	return 16.0f;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"CUE Sheet File", @"cue.icns", @"cue"]
	];
}

- (NSDictionary *)properties {
	NSMutableDictionary *properties = [[decoder properties] mutableCopy];

	// Need to alter length
	if(!noFragment)
		[properties setObject:@(trackEnd - trackStart) forKey:@"totalFrames"];

	return [NSDictionary dictionaryWithDictionary:properties];
}

- (NSDictionary *)metadata {
	NSDictionary *metadata = @{};
	if(track != nil)
		metadata = [CueSheetMetadataReader processDataForTrack:track];
	if(decoder != nil)
		return [metadata dictionaryByMergingWith:[decoder metadata]];
	else
		return metadata;
}

- (BOOL)open:(id<CogSource>)s {
	if(![[s url] isFileURL]) {
		return NO;
	}

	NSURL *url = [s url];

	sourceURL = url;

	embedded = NO;
	cuesheet = nil;
	NSDictionary *fileMetadata;

	noFragment = NO;
	observersAdded = NO;

	NSString *ext = [url pathExtension];
	if([ext caseInsensitiveCompare:@"cue"] != NSOrderedSame) {
		// Embedded cuesheet check
		fileMetadata = [NSClassFromString(@"AudioMetadataReader") metadataForURL:url skipCue:YES];

		source = s;

		decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source skipCue:YES];

		if(![decoder open:source]) {
			ALog(@"Could not open cuesheet decoder");
			return NO;
		}

		NSDictionary *alsoMetadata = [decoder metadata];

		NSString *sheet = [fileMetadata objectForKey:@"cuesheet"];
		if(!sheet || ![sheet length]) sheet = [alsoMetadata objectForKey:@"cuesheet"];

		if([sheet length]) {
			cuesheet = [CueSheet cueSheetWithString:sheet withFilename:[url path]];
			embedded = YES;
		}

		baseURL = url;

		NSString *fragment = [url fragment];
		if(!fragment || [fragment isEqualToString:@""])
			noFragment = YES;
	} else
		cuesheet = [CueSheet cueSheetWithFile:[url path]];

	if(!noFragment) {
		NSArray *tracks = [cuesheet tracks];
		int i;
		for(i = 0; i < [tracks count]; i++) {
			if([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]) {
				track = [tracks objectAtIndex:i];

				// Kind of a hackish way of accessing outside classes.
				if(!embedded) {
					source = [NSClassFromString(@"AudioSource") audioSourceForURL:[track url]];

					if(![source open:[track url]]) {
						ALog(@"Could not open cuesheet source");
						return NO;
					}

					decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source skipCue:YES];

					if(![decoder open:source]) {
						ALog(@"Could not open cuesheet decoder");
						return NO;
					}
				}

				CueSheetTrack *nextTrack = nil;
				if(i + 1 < [tracks count]) {
					nextTrack = [tracks objectAtIndex:i + 1];
				}

				NSDictionary *properties = [decoder properties];
				int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
				int channels = [[properties objectForKey:@"channels"] intValue];
				float sampleRate = [[properties objectForKey:@"sampleRate"] floatValue];

				bytesPerFrame = (bitsPerSample / 8) * channels;

				double _trackStart = [track time];
				if(![track timeInSamples]) _trackStart *= sampleRate;
				trackStart = _trackStart;

				if(nextTrack && (embedded || ([[[nextTrack url] absoluteString] isEqualToString:[[track url] absoluteString]]))) {
					double _trackEnd = [nextTrack time];
					if(![nextTrack timeInSamples]) _trackEnd *= sampleRate;
					trackEnd = _trackEnd;
				} else {
					trackEnd = [[properties objectForKey:@"totalFrames"] doubleValue];
				}

				[self seek:0];

				// Note: Should register for observations of the decoder
				[self willChangeValueForKey:@"properties"];
				[self didChangeValueForKey:@"properties"];

				return YES;
			}
		}
	} else {
		// Fix for embedded cuesheet handler parsing non-embedded files,
		// or files that are already in the playlist without a fragment
		source = [NSClassFromString(@"AudioSource") audioSourceForURL:url];

		if(![source open:url]) {
			ALog(@"Could not open cuesheet source");
			return NO;
		}

		decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source skipCue:YES];

		[self registerObservers];

		if(![decoder open:source]) {
			ALog(@"Could not open cuesheet decoder");
			return NO;
		}

		NSDictionary *properties = [decoder properties];
		int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
		int channels = [[properties objectForKey:@"channels"] intValue];

		bytesPerFrame = (bitsPerSample / 8) * channels;

		trackStart = 0;

		trackEnd = [[properties objectForKey:@"totalFrames"] doubleValue];

		[self seek:0];

		return YES;
	}

	return NO;
}

- (void)registerObservers {
	if(!observersAdded) {
		DLog(@"REGISTERING OBSERVERS");
		[decoder addObserver:self
		          forKeyPath:@"properties"
		             options:(NSKeyValueObservingOptionNew)
		             context:kCueSheetDecoderContext];

		[decoder addObserver:self
		          forKeyPath:@"metadata"
		             options:(NSKeyValueObservingOptionNew)
		             context:kCueSheetDecoderContext];

		observersAdded = YES;
	}
}

- (void)removeObservers {
	if(observersAdded) {
		[decoder removeObserver:self forKeyPath:@"properties" context:kCueSheetDecoderContext];
		[decoder removeObserver:self forKeyPath:@"metadata" context:kCueSheetDecoderContext];
		observersAdded = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	if(context == kCueSheetDecoderContext) {
		[self willChangeValueForKey:keyPath];
		[self didChangeValueForKey:keyPath];
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)close {
	if(decoder) {
		[self removeObservers];
		[decoder close];
		decoder = nil;
	}

	source = nil;
	cuesheet = nil;
	track = nil;
}

- (void)dealloc {
	[self close];
}

- (BOOL)setTrack:(NSURL *)url {
	// handling the file directly
	if(noFragment)
		return NO;

	BOOL pathsAreFiles = NO;

	if([url isFileURL] && [sourceURL isFileURL]) {
		pathsAreFiles = YES;
	}

	// Same file, just next track...this may be unnecessary since frame-based decoding is done now...
	if(embedded || ([[sourceURL path] isEqualToString:[url path]] && (pathsAreFiles || [[sourceURL host] isEqualToString:[url host]]))) {
		NSArray *tracks = [cuesheet tracks];

		int i;
		for(i = 0; i < [tracks count]; i++) {
			if([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]) {
				CueSheetTrack *_track = [tracks objectAtIndex:i];

				if(![[_track url] isEqualTo:[track url]])
					return NO;

				track = _track;

				float sampleRate = [[[decoder properties] objectForKey:@"sampleRate"] floatValue];

				double _trackStart = [track time];
				if(![track timeInSamples]) _trackStart *= sampleRate;
				trackStart = _trackStart;

				CueSheetTrack *nextTrack = nil;
				if(i + 1 < [tracks count]) {
					nextTrack = [tracks objectAtIndex:i + 1];
				}

				if(nextTrack && (embedded || [[[nextTrack url] absoluteString] isEqualToString:[[track url] absoluteString]])) {
					double _trackEnd = [nextTrack time];
					if(![nextTrack timeInSamples]) _trackEnd *= sampleRate;
					trackEnd = _trackEnd;
				} else {
					trackEnd = [[[decoder properties] objectForKey:@"totalFrames"] longValue];
				}

				[self seek:0];

				DLog(@"CHANGING TRACK!");
				return YES;
			}
		}
	}

	return NO;
}

- (long)seek:(long)frame {
	if(!noFragment && frame > trackEnd - trackStart) {
		// need a better way of returning fail.
		return -1;
	}

	frame += trackStart;

	framePosition = [decoder seek:frame];

	return framePosition - trackStart;
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	if(!noFragment && framePosition + frames > trackEnd) {
		frames = (UInt32)(trackEnd - framePosition);
	}

	if(!frames) {
		DLog(@"Returning 0");
		return 0;
	}

	int n = [decoder readAudio:buf frames:frames];

	framePosition += n;

	return n;
}

- (BOOL)isSilence {
	if([decoder respondsToSelector:@selector(isSilence)])
		return [decoder isSilence];
	return NO;
}

@end
