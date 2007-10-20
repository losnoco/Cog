//
//  CueSheetDecoder.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "APLDecoder.h"
#import "APLFile.h"

@implementation APLDecoder

+ (NSArray *)fileTypes {	
	return [NSArray arrayWithObject:@"apl"];
}

+ (NSArray *)mimeTypes {	
	return [NSArray arrayWithObjects:@"application/x-apl", nil];
}

- (NSDictionary *)properties {
	NSMutableDictionary *properties = [[decoder properties] mutableCopy];
	
	//Need to alter length
	[properties setObject:[NSNumber numberWithDouble:trackLength] forKey:@"length"];
	return [properties autorelease];
}

- (BOOL)open:(id<CogSource>)s
{
	NSLog(@"Loading apl...");
	if (![[s url] isFileURL])
		return NO;
	
	NSURL *url = [s url];
	[s close];
	
	apl = [APLFile createWithFile:[url path]];
	[apl retain];
	
	//Kind of a hackish way of accessing outside classes.
	source = [NSClassFromString(@"AudioSource") audioSourceForURL:[apl file]];
	[source retain];
	
	if (![source open:[apl file]]) {
		NSLog(@"Could not open source for file '%@' referenced in apl", [apl file]);
		return NO;
	}
	decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForSource:source];
	[decoder retain];
	
	if (![decoder open:source]) {
		NSLog(@"Could not open decoder for source for apl");
		return NO;
	}
	
	NSDictionary *properties = [decoder properties];
	int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
	int channels = [[properties objectForKey:@"channels"] intValue];
	float sampleRate = [[properties objectForKey:@"sampleRate"] floatValue];
	
	
	bytesPerFrame = (bitsPerSample/8) * channels;
	bytesPerSecond = (int)(bytesPerFrame * sampleRate);
	
	if ([apl endBlock] > [apl startBlock]) trackEnd = ([apl endBlock] / sampleRate) * 1000.0;
	else trackEnd = [[properties objectForKey:@"length"] doubleValue];
	trackStart = ([apl startBlock]/sampleRate) * 1000.0;
	trackLength = trackEnd - trackStart;
	
	[decoder seekToTime: trackStart];
	
	//Note: Should register for observations of the decoder, but laziness consumes all.
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	return YES;
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
	if (apl) {
		[apl release];
		apl = nil;
	}
}

#if 0
- (BOOL)setTrack_1111:(NSURL *)url
{
	// see if next apl is next track...
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
#endif

- (double)seekToTime:(double)time //milliseconds
{
	if (time > trackLength || time < 0) {
		//need a better way of returning fail.
		return -1.0;
	}
	
	time += trackStart;
	
	bytePosition = (time/1000.0) * bytesPerSecond;
	
	NSLog(@"Before: %li", bytePosition);
	bytePosition -= bytePosition % bytesPerFrame;
	NSLog(@"After: %li", bytePosition);
	
	return [decoder seekToTime:time];
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	long trackByteEnd = (trackEnd/1000.0) * bytesPerSecond;	
	trackByteEnd -= trackByteEnd % (bytesPerFrame);
	
	if (bytePosition + size > trackByteEnd)
		size = trackByteEnd - bytePosition;
	
	if (!size)
		return 0;
	
	int n = [decoder fillBuffer:buf ofSize:size];
	bytePosition += n;
	return n;
}


@end
