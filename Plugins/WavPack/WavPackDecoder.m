//
//  WavPackFile.m
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "WavPackDecoder.h"


@implementation WavPackDecoder

- (BOOL)open:(NSURL *)url
{
	int open_flags = 0;
	char error[80];

	wpc = WavpackOpenFileInput([[url path] UTF8String], error, open_flags, 0);
	if (!wpc)
		return NO;
	
	channels = WavpackGetNumChannels(wpc);
	bitsPerSample = WavpackGetBitsPerSample(wpc);
	
	frequency = WavpackGetSampleRate(wpc);

	length = ((double)WavpackGetNumSamples(wpc) * 1000.0)/frequency;
	
	bitrate = (int)(WavpackGetAverageBitrate(wpc, TRUE)/1000.0);

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numsamples;
	int n;
	void *sampleBuf = malloc(size*2);
	
	numsamples = size/(bitsPerSample/8)/channels;
//	DBLog(@"NUM SAMPLES: %i %i", numsamples, size);
	n = WavpackUnpackSamples(wpc, sampleBuf, numsamples);
	
	int i;
	for (i = 0; i < n*channels; i++)
	{
		((UInt16 *)buf)[i] = ((UInt32 *)sampleBuf)[i];
	}
	n *= (bitsPerSample/8)*channels;
	
	free(sampleBuf);
	
	return n;
}

- (double)seekToTime:(double)milliseconds
{
	int sample;
	sample = frequency*(milliseconds/1000.0);

	WavpackSeekSample(wpc, sample);
	
	return milliseconds;
}

- (void)close
{
	WavpackCloseFile(wpc);
}


- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithInt:bitrate],@"bitrate",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithDouble:length],@"length",
		@"host",@"endian",
		nil];
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"wv"];
}


@end
