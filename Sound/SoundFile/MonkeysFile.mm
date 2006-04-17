//
//  MonkeysFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/30/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "MonkeysFile.h"
#import "MAC/ApeInfo.h"
#import "MAC/CharacterHelper.h"

@implementation MonkeysFile

- (BOOL)open:(const char *)filename
{
	int n;
	str_utf16 *chars = NULL;

	chars = GetUTF16FromUTF8((const unsigned char *)filename);
	if(chars == NULL)
		return NO;
	
	decompress = CreateIAPEDecompress(chars, &n);
	free(chars);

	if (decompress == NULL)
	{
		DBLog(@"ERROR OPENING FILE");
		return NO;
	}
	
	frequency = decompress->GetInfo(APE_INFO_SAMPLE_RATE);
	bitsPerSample = decompress->GetInfo(APE_INFO_BITS_PER_SAMPLE);
	channels = decompress->GetInfo(APE_INFO_CHANNELS);

	totalSize = decompress->GetInfo(APE_INFO_TOTAL_BLOCKS)*bitsPerSample/8*channels;

	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	return [self open:filename];
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

- (double)seekToTime:(double)milliseconds
{
	int r;
//	DBLog(@"HELLO: %i", int(frequency*((double)milliseconds/1000.0)));
	r = decompress->Seek(int(frequency*((double)milliseconds/1000.0)));
	
	return milliseconds;
}

@end
