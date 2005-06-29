//
//  WavPackFile.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 6/6/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "WavPackFile.h"


@implementation WavPackFile

- (BOOL)open:(const char *)filename
{
	int open_flags = 0;
	char error[80];

	wpc = WavpackOpenFileInput(filename, error, open_flags, 0);
	if (!wpc)
		return NO;
	
	channels = WavpackGetNumChannels(wpc);
//	bitsPerSample = WavpackGetBitsPerSample(wpc);
	bitsPerSample = 32;
	
	frequency = WavpackGetSampleRate(wpc);

	int samples;
	samples = WavpackGetNumSamples(wpc);
	totalSize = samples * channels * 4;
	
	bitRate = (int)(WavpackGetAverageBitrate(wpc, TRUE)/1000.0);

	//isBigEndian = YES;

	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	[self open:filename]; //does the same damn thing

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numsamples;
	int n;
	
	numsamples = size/4/channels;
//	DBLog(@"NUM SAMPLES: %i %i", numsamples, size);
	n = WavpackUnpackSamples(wpc, buf, numsamples);
	
	n *= 4*channels;

	int i;
	for (i = 0; i < n/2; i++)
	{
//		((UInt32 *)buf)[i] = CFSwapInt32LittleToHost(((UInt32 *)buf)[i]);
		((UInt16 *)buf)[i] = CFSwapInt16LittleToHost(((UInt16 *)buf)[i]);
	}
	
	return n;
}

- (double)seekToTime:(double)milliseconds
{
	int sample;
	sample = (frequency/2)*(milliseconds/1000.0);
	DBLog(@"%lf %i", milliseconds, sample);
	WavpackSeekSample(wpc, sample);
	
	return milliseconds;
}

- (void)close
{
	WavpackCloseFile(wpc);
}

@end
