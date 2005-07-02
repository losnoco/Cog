//
//  WaveFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/31/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "WaveFile.h"

@implementation WaveFile

- (BOOL)open:(const char *)filename
{
	sndFile = sf_open(filename, SFM_READ, &info);
	
	if (sndFile == NULL)
		return NO;

	return [self readInfo];
}

- (BOOL)readInfo
{
	bitRate = 0;
	frequency = info.samplerate;
	channels = info.channels;
	isUnsigned = NO;

	switch (info.format & SF_FORMAT_ENDMASK)
	{
		case SF_ENDIAN_FILE:
			if (((info.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_AIFF) || ((info.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_AU))
				isBigEndian = YES;
			else
				isBigEndian = NO;

			break;
		case SF_ENDIAN_CPU:
			isBigEndian = YES;
//			DBLog(@"&CPU ENDIAN");
			break;
		case SF_ENDIAN_LITTLE:
			isBigEndian = NO;
//			DBLog(@"&LITTLE INDIAN");
			break;
		case SF_ENDIAN_BIG:
			isBigEndian = YES;
//			DBLog(@"&BIG ENDIAN");
			break;
		default:
			isBigEndian = NO;
//			DBLog(@"&WHAT THE FUCK IS GOING ON?!!!!");
	}
	
	switch (info.format & SF_FORMAT_SUBMASK)
	{
		case SF_FORMAT_PCM_S8:
			bitsPerSample = 8;
			break;
		case SF_FORMAT_PCM_16:
			bitsPerSample = 16;
			break;
		case SF_FORMAT_PCM_24:
			bitsPerSample = 24;
			break;
		case SF_FORMAT_PCM_32:
			bitsPerSample = 32;
			break;
		case SF_FORMAT_PCM_U8:
			isUnsigned = YES;
			bitsPerSample = 8;
			break;
		default:
			DBLog(@"BITS PER SAMPLE NOT DEFINED");
			return NO;
	}

	totalSize = info.frames*channels*bitsPerSample/8;

	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	[self open:filename];
	
	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numread;

	numread = sf_read_raw(sndFile, buf, size);
/*	
	if (isBigEndian == YES)
	{
		int n;
		for (n = 0; n < numread/2; n++)
		{
			((UInt16 *)buf)[n] = CFSwapInt16LittleToHost(((UInt16 *)buf)[n]);
		}
	}
*/	
	currentPosition += numread;
	
	return numread;
}

- (void)close
{
	if (sndFile)
		sf_close(sndFile);
	sndFile = NULL;
}

- (double)seekToTime:(double)milliseconds
{
	sf_seek(sndFile, frequency*((double)milliseconds/1000.0), SEEK_SET);
	
	return milliseconds;
}

@end
