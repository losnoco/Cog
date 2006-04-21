//
//  WavPackFile.m
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
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
	bitsPerSample = WavpackGetBitsPerSample(wpc);
//	bitsPerSample = 32;
	NSLog(@"BYTES PER SAMPLE: %i", WavpackGetBitsPerSample(wpc));
	NSLog(@"BYTES PER SAMPLE: %i", WavpackGetBytesPerSample(wpc));
	
	frequency = WavpackGetSampleRate(wpc);

	int samples;
	samples = WavpackGetNumSamples(wpc);
	totalSize = samples * channels * 4;
	
	bitRate = (int)(WavpackGetAverageBitrate(wpc, TRUE)/1000.0);

	isBigEndian = hostIsBigEndian();

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
	
	return n;
}

- (double)seekToTime:(double)milliseconds
{
	int sample;
	sample = (frequency/2)*(milliseconds/1000.0);

	WavpackSeekSample(wpc, sample);
	
	return milliseconds;
}

- (void)close
{
	WavpackCloseFile(wpc);
}

@end
