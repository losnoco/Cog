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

@implementation CueSheetDecoder

+ (NSArray *)fileTypes 
{	
	return [NSArray arrayWithObject:@"cue"];
}

- (NSDictionary *)properties 
{
	return [decoder properties];
}

- (BOOL)open:(id<CogSource>)source
{
	//Kind of a hackish way of accessing AudioDecoder
	decoder = [NSClassFromString(@"AudioDecoder") audioDecoderForURL:[source url]];
	[decoder retain];
	
	BOOL r = [decoder open:source];
	if (r)
	{
		CueSheet *cuesheet = [CueSheet cueSheetWithFile:[[source url] path]];
		
		track = [cuesheet track:[[source url] fragment]];
		
		NSDictionary *properties = [decoder properties];
		int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
		int channels = [[properties objectForKey:@"channels"] intValue];
		float sampleRate = [[properties objectForKey:@"sampleRate"] floatValue];
		
		bytesPerSecond = (int)((bitsPerSample/8) * channels * sampleRate);
		
		[decoder seekToTime: [track start] * 1000.0];
	}
	
	return r;
}

- (double)seekToTime:(double)time //milliseconds
{
	double trackStartMs = [track start] * 1000.0;
	double trackEndMs =  [track end] * 1000.0;
	
	if (time > trackEndMs - trackStartMs) {
		//need a better way of returning fail.
		return -1.0;
	}
	
	time += trackStartMs;
	
	trackPosition = (time/1000.0) * bytesPerSecond;

	return [decoder seekToTime:time];
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int n;
	
	n = [decoder fillBuffer:buf ofSize:size];
	
	trackPosition += n;
	
	int trackEnd = [track end] * bytesPerSecond;
		
	if (trackPosition + n > trackEnd) {
		return trackEnd - trackPosition;
	}
	
	return n;
}

- (void)dealloc
{
	[decoder release];
	
	[super dealloc];
}


@end
