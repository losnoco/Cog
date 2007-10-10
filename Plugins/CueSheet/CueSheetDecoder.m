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

	CueSheet *cuesheet = [CueSheet cueSheetWithFile:[url path]];

	NSArray *tracks = [cuesheet tracks];
	int i;
	for (i = 0; i < [tracks count]; i++) 
	{
		if ([[[tracks objectAtIndex:i] track] isEqualToString:[url fragment]]){
			track = [tracks objectAtIndex:i];

			//Kind of a hackish way of accessing outside classes.
			source = [NSClassFromString(@"AudioSource") audioSourceForURL:[track url]];
			[source retain];

			if (![source open:[track url]]) {
				NSLog(@"Could not open cuesheet source");
				return NO;
			}

			decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForURL:[source url]];
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

			bytesPerSecond = (int)((bitsPerSample/8) * channels * sampleRate);

			[decoder seekToTime: [track time] * 1000.0];

			if (nextTrack && [[[nextTrack url] absoluteString] isEqualToString:[[track url] absoluteString]]) {
				trackEnd = [nextTrack time];
			}
			else {
				trackEnd = [[properties objectForKey:@"length"] doubleValue]/1000.0;
			}
			
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

	return [decoder seekToTime:time];
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	long trackByteEnd = trackEnd * bytesPerSecond;	
	
	if (bytePosition + size > trackByteEnd) {
		size = trackByteEnd - bytePosition;
	}

	if (!size) {
		return 0;
	}

	int n = [decoder fillBuffer:buf ofSize:size];
	
	bytePosition += n;
	
	return n;
}


@end
