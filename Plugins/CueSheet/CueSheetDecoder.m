//
//  CueSheetDecoder.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetDecoder.h"

#import "CueSheet.h"
#import "CueSheetTrack.h"
#import "CueSheetContainer.h"

#import "Logging.h"

@implementation CueSheetDecoder

+ (NSArray *)fileTypes 
{	
	return [CueSheetContainer fileTypes];
}

+ (NSArray *)mimeTypes
{	
	return [CueSheetContainer mimeTypes];
}

+ (float)priority
{
    return 1.0;
}

- (NSDictionary *)properties 
{
	NSMutableDictionary *properties = [[decoder properties] mutableCopy];

	//Need to alter length
	[properties setObject:[NSNumber numberWithLong:(trackEnd - trackStart)] forKey:@"totalFrames"];

	return properties;
}

- (BOOL)open:(id<CogSource>)s
{
	if (![[s url] isFileURL]) {
		return NO;
	}

	NSURL *url = [s url];
	[s close];

	cuesheet = [CueSheet cueSheetWithFile:[url path]];
	
	NSArray *tracks = [cuesheet tracks];
	int i;
	for (i = 0; i < [tracks count]; i++) 
	{
		if ([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]){
			track = [tracks objectAtIndex:i];

			//Kind of a hackish way of accessing outside classes.
			source = [NSClassFromString(@"AudioSource") audioSourceForURL:[track url]];

			if (![source open:[track url]]) {
				ALog(@"Could not open cuesheet source");
				return NO;
			}

			decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source];

			if (![decoder open:source]) {
				ALog(@"Could not open cuesheet decoder");
				return NO;
			}

			CueSheetTrack *nextTrack = nil;
			if (i + 1 < [tracks count]) {
				nextTrack = [tracks objectAtIndex:i + 1];
			}

			NSDictionary *properties = [decoder properties];
			int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
			int channels = [[properties objectForKey:@"channels"] intValue];
			float sampleRate = [[properties objectForKey:@"sampleRate"] floatValue];

			bytesPerFrame = (bitsPerSample/8) * channels;
			
			trackStart = [track time] * sampleRate;

			if (nextTrack && [[[nextTrack url] absoluteString] isEqualToString:[[track url] absoluteString]]) {
				trackEnd = [nextTrack time] * sampleRate;
			}
			else {
				trackEnd = [[properties objectForKey:@"totalFrames"] doubleValue];
			}
			
			[self seek: 0];

			//Note: Should register for observations of the decoder
			[self willChangeValueForKey:@"properties"];
			[self didChangeValueForKey:@"properties"];

			return YES;
		}
	}

	return NO;
}

- (void)close {
	if (decoder) {
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

- (BOOL)setTrack:(NSURL *)url
{
	//Same file, just next track...this may be unnecessary since frame-based decoding is done now...
	if ([[[track url] path] isEqualToString:[url path]] && [[[track url] host] isEqualToString:[url host]] && [[url fragment] intValue] == [[track track] intValue] + 1) {
		NSArray *tracks = [cuesheet tracks];
		
		int i;
		for (i = 0; i < [tracks count]; i++) {
			if ([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]){
				track = [tracks objectAtIndex:i];
				
				float sampleRate = [[[decoder properties] objectForKey:@"sampleRate"] floatValue];
				
				trackStart = [track time] * sampleRate;
				
				CueSheetTrack *nextTrack = nil;
				if (i + 1 < [tracks count]) {
					nextTrack = [tracks objectAtIndex:i + 1];
				}

				if (nextTrack && [[[nextTrack url] absoluteString] isEqualToString:[[track url] absoluteString]]) {
					trackEnd = [nextTrack time] * sampleRate;
				}
				else {
					trackEnd = [[[decoder properties] objectForKey:@"totalFrames"] longValue];
				}
				
				DLog(@"CHANGING TRACK!");
				return YES;
			}
		}
	}
	
	return NO;
}

- (long)seek:(long)frame
{
	if (frame > trackEnd - trackStart) {
		//need a better way of returning fail.
		return -1;
	}
	
	frame += trackStart;
	
	framePosition = [decoder seek:frame];

	return framePosition;
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	if (framePosition + frames > trackEnd) {
		frames = (UInt32)(trackEnd - framePosition);
	}

	if (!frames)
	{
		DLog(@"Returning 0");
		return 0;
	}

	int n = [decoder readAudio:buf frames:frames];
	
	framePosition += n;
	
	return n;
}


@end
