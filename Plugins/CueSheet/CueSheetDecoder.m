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
#import "CueSheetTrack.h"

#import "Logging.h"

@implementation CueSheetDecoder

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
	[properties setObject:[NSNumber numberWithLong:(trackEnd - trackStart)] forKey:@"totalFrames"];

	return [NSDictionary dictionaryWithDictionary:properties];
}

- (NSDictionary *)metadata {
	return @{};
}

- (BOOL)open:(id<CogSource>)s {
	if(![[s url] isFileURL]) {
		return NO;
	}

	NSURL *url = [s url];

	embedded = NO;
	cuesheet = nil;
	NSDictionary *fileMetadata;

	noFragment = NO;
	observersAdded = NO;

	NSString *ext = [url pathExtension];
	if([ext caseInsensitiveCompare:@"cue"] != NSOrderedSame) {
		// Embedded cuesheet check
		fileMetadata = [NSClassFromString(@"AudioMetadataReader") metadataForURL:url skipCue:YES];
		NSString *sheet = [fileMetadata objectForKey:@"cuesheet"];
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

				NSURL *trackUrl = (embedded) ? baseURL : [track url];

				// Kind of a hackish way of accessing outside classes.
				source = [NSClassFromString(@"AudioSource") audioSourceForURL:trackUrl];

				if(![source open:trackUrl]) {
					ALog(@"Could not open cuesheet source");
					return NO;
				}

				decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source skipCue:YES];

				if(![decoder open:source]) {
					ALog(@"Could not open cuesheet decoder");
					return NO;
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
	DLog(@"REGISTERING OBSERVERS");
	[decoder addObserver:self
	          forKeyPath:@"properties"
	             options:(NSKeyValueObservingOptionNew)
	             context:NULL];

	[decoder addObserver:self
	          forKeyPath:@"metadata"
	             options:(NSKeyValueObservingOptionNew)
	             context:NULL];

	observersAdded = YES;
}

- (void)removeObservers {
	if(observersAdded) {
		[decoder removeObserver:self forKeyPath:@"properties"];
		[decoder removeObserver:self forKeyPath:@"metadata"];
		observersAdded = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	[self willChangeValueForKey:keyPath];
	[self didChangeValueForKey:keyPath];
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

	// Same file, just next track...this may be unnecessary since frame-based decoding is done now...
	if(embedded || ([[[track url] path] isEqualToString:[url path]] && [[[track url] host] isEqualToString:[url host]] && [[url fragment] intValue] == [[track track] intValue] + 1)) {
		NSArray *tracks = [cuesheet tracks];

		int i;
		for(i = 0; i < [tracks count]; i++) {
			if([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]) {
				track = [tracks objectAtIndex:i];

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

				if(embedded)
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

@end
