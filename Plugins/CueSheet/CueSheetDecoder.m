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

@implementation CueSheetDecoder

+ (NSArray *)fileTypes 
{	
	return [CueSheetContainer fileTypes];
}

+ (NSArray *)mimeTypes
{	
	return [CueSheetContainer mimeTypes];
}

- (NSDictionary *)properties 
{
	NSMutableDictionary *properties = [[decoder properties] mutableCopy];

	//Need to alter length
	[properties setObject:[NSNumber numberWithDouble:((trackEnd - [track time]) * 1000)] forKey:@"length"];

	return [properties autorelease];
}

- (BOOL)open:(id<CogSource>)s
{
	if (![[s url] isFileURL]) {
		return NO;
	}

	NSURL *url = [s url];
	[s close];

	cuesheet = [CueSheet cueSheetWithFile:[url path]];
	[cuesheet retain];
	
	NSArray *tracks = [cuesheet tracks];
	int i;
	for (i = 0; i < [tracks count]; i++) 
	{
		if ([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]){
			track = [tracks objectAtIndex:i];
			[track retain];

			//Kind of a hackish way of accessing outside classes.
			source = [NSClassFromString(@"AudioSource") audioSourceForURL:[track url]];
			[source retain];

			if (![source open:[track url]]) {
				NSLog(@"Could not open cuesheet source");
				return NO;
			}

			decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source];
			[decoder retain];

			if (![decoder open:source]) {
				NSLog(@"Could not open cuesheet decoder");
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
			bytesPerSecond = (int)(bytesPerFrame * sampleRate);

			if (nextTrack && [[[nextTrack url] absoluteString] isEqualToString:[[track url] absoluteString]]) {
				trackEnd = [nextTrack time];
			}
			else {
				trackEnd = [[properties objectForKey:@"length"] doubleValue]/1000.0;
			}
			
                       [self seekToTime: 0.0];

			//Note: Should register for observations of the decoder, but laziness consumes all.
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
		[decoder release];
		decoder = nil;
	}

	if (source) {
		[source release];
		source = nil;
	}
	if (cuesheet) {
		[cuesheet release];
		cuesheet = nil;
	}
	if (track) {
		[track release];
		track = nil;
	}
}

- (BOOL)setTrack:(NSURL *)url
{
	if ([[url fragment] intValue] == [[track track] intValue] + 1) {
		NSArray *tracks = [cuesheet tracks];
		
		int i;
		for (i = 0; i < [tracks count]; i++) {
			if ([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]){
				[track release];
				track = [tracks objectAtIndex:i];
				[track retain];
				
				CueSheetTrack *nextTrack = nil;
				if (i + 1 < [tracks count]) {
					nextTrack = [tracks objectAtIndex:i + 1];
				}

				if (nextTrack && [[[nextTrack url] absoluteString] isEqualToString:[[track url] absoluteString]]) {
					trackEnd = [nextTrack time];
				}
				else {
					trackEnd = [[[decoder properties] objectForKey:@"length"] doubleValue]/1000.0;
				}
				
				NSLog(@"CHANGING TRACK!");
				return YES;
			}
		}
	}
	
	return NO;
}

- (double)seekToTime:(double)time //milliseconds
{
	double trackStartMs = [track time] * 1000.0;
	double trackEndMs =  trackEnd * 1000.0;
	
	if (time > trackEndMs - trackStartMs) {
		//need a better way of returning fail.
		return -1.0;
	}
	
	time += trackStartMs;
	
	bytePosition = (time/1000.0) * bytesPerSecond;
	
	NSLog(@"Before: %li", bytePosition);
	bytePosition -= bytePosition % bytesPerFrame;
	NSLog(@"After: %li", bytePosition);

	return [decoder seekToTime:time];
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	long trackByteEnd = trackEnd * bytesPerSecond;	
	trackByteEnd -= trackByteEnd % (bytesPerFrame);
	
//     NSLog(@"Position: %i/%i", bytePosition, trackByteEnd);
//     NSLog(@"Requested: %i", size);
	if (bytePosition + size > trackByteEnd) {
		size = trackByteEnd - bytePosition;
	}

//     NSLog(@"Revised size: %i", size);

	if (!size) {
               NSLog(@"Returning 0");
		return 0;
	}

	int n = [decoder fillBuffer:buf ofSize:size];
	
//     NSLog(@"Received: %i", n);

	bytePosition += n;
	
	return n;
}


@end
