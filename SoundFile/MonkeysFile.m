//
//  MonkeysFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/30/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "MonkeysFile.h"
#import "MAC/ApeInfo.h"

@implementation MonkeysFile

- (BOOL)open:(const char *)filename
{
	int n;
	
	decompress = CreateIAPEDecompress(filename, &n);

	if (decompress == NULL)
	{
		DBLog(@"ERROR OPENING FILE");
		return NO;
	}
	
	frequency = decompress->GetInfo(APE_INFO_SAMPLE_RATE);
	bitsPerSample = decompress->GetInfo(APE_INFO_BITS_PER_SAMPLE);
	channels = decompress->GetInfo(APE_INFO_CHANNELS);

	totalSize = decompress->GetInfo(APE_INFO_TOTAL_BLOCKS)*bitsPerSample/8*channels;
	DBLog(@"APE OPENED: %i %i %i %i", frequency, bitsPerSample, channels, totalSize);
	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	int err;
	CAPEInfo apeInfo(&err, filename, NULL);
	
	frequency = apeInfo.GetInfo(APE_INFO_SAMPLE_RATE);
	bitsPerSample = apeInfo.GetInfo(APE_INFO_BITS_PER_SAMPLE);
	channels = apeInfo.GetInfo(APE_INFO_CHANNELS);
	
	totalSize = apeInfo.GetInfo(APE_INFO_TOTAL_BLOCKS)*bitsPerSample/8*channels;	
	bitRate = apeInfo.GetInfo(APE_INFO_AVERAGE_BITRATE);
	
	DBLog(@"INFO READ: %i %i %i %i", frequency, bitsPerSample, channels, totalSize);

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int n;
	int numread;
	int blockAlign = decompress->GetInfo(APE_INFO_BLOCK_ALIGN);
	n = decompress->GetData((char *)buf, size/blockAlign, &numread);
	if (n != ERROR_SUCCESS)
	{
		DBLog(@"ERROR: %i", n);
		return 0;
	}
	numread *= blockAlign;
//	DBLog(@"READ DATA: %i", numread);
	
//	DBLog(@"NUMREAD: %i", numread);
	return numread;
}

- (void)close
{
//	DBLog(@"CLOSE");
	if (decompress)
		delete decompress;
	
	decompress = NULL;
}

- (void)seekToTime:(double)milliseconds
{
	int r;
//	DBLog(@"HELLO: %i", int(frequency*((double)milliseconds/1000.0)));
	r = decompress->Seek(int(frequency*((double)milliseconds/1000.0)));
}

@end
