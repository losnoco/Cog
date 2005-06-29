//
//  MPEGFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/30/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "MPEGFile.h"


@implementation MPEGFile

- (BOOL)open:(const char *)filename
{
	int err;

//	DBLog(@"Opening: %s!!!!", filename);
	
	err = DecMPA_CreateUsingFile(&decoder, filename, DECMPA_VERSION);
	if (err != DECMPA_OK)
		return NO;
	
	DecMPA_SetParam(decoder, DECMPA_PARAM_OUTPUT, DECMPA_OUTPUT_INT16);
	
	long n;
	DecMPA_DecodeNoData(decoder, &n);
//	DBLog(@"Woot: %i", n);
	DecMPA_OutputFormat outputFormat;
	err = DecMPA_GetOutputFormat(decoder, &outputFormat);
	if (err != DECMPA_OK)
		return NO;
	
	frequency = outputFormat.nFrequency;
	channels = outputFormat.nChannels;
	bitsPerSample = 16;
	
	isBigEndian = YES;
	
	long duration;
	DecMPA_GetDuration(decoder, &duration);
	totalSize = (long int)(duration*(double)frequency/1000.0*channels*bitsPerSample/8);
	
	DecMPA_MPEGHeader mpegHeader;
	DecMPA_GetMPEGHeader(decoder, &mpegHeader);
//	int totalRate = mpegHeader.nBitRateKbps;
//	int num = 0;
/*
	while (DecMPA_DecodeNoData(decoder, &n) == DECMPA_OK)
	{
		DecMPA_GetMPEGHeader(decoder, &mpegHeader);
		totalRate += mpegHeader.nBitRateKbps;
		DBLog(@"%i %i %i", num, mpegHeader.nBitRateIndex, mpegHeader.nBitRateKbps);
		num++;
	}
	err = DecMPA_GetMPEGHeader(decoder, &mpegHeader);
*/
	bitRate = mpegHeader.nBitRateKbps;
//	DBLog(@"Bitrate? %i", mpegHeader.);
//	DBLog(@"Mpeg file opened.");
	err =  DecMPA_SeekToTime(decoder, 0);
	if (err != DECMPA_OK)
		return NO;
	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	[self open:filename];
	
	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	long numread;
    long total = 0;
	
	DecMPA_Decode(decoder, &((char *)buf)[total], size - total, &numread);
    while (total != size && numread > 0)
    {
		total += numread;
		
		DecMPA_Decode(decoder, &((char *)buf)[total], size - total, &numread);
    }
		
/*	int n;
	for (n = 0; n < total/2; n++)
	{
		((UInt16 *)buf)[n] = CFSwapInt16LittleToHost(((UInt16 *)buf)[n]);
	}
*/	
	currentPosition += total;
    return total;
}

- (void)close
{
	if (decoder)
		DecMPA_Destroy(decoder);
	decoder = NULL;
}

- (double)seekToTime:(double)milliseconds
{
	DecMPA_SeekToTime(decoder, (unsigned long)milliseconds);

	return milliseconds;
}

@end
