//
//  AACFile.m
//  Cog
//
//  Created by Vincent Spader on 5/31/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "AACFile.h"
#import <FAAD2/aacinfo.h>

@implementation AACFile

- (BOOL)open:(const char *)filename
{
	faadAACInfo info;
	//	unsigned long cap = NeAACDecGetCapabilities();
	//Check if decoder has the needed capabilities
	
	inFd = fopen(filename, "r");
	if (!inFd)
		return NO;
	
	//Open the library
	hAac = NeAACDecOpen();
	
	//Get the current config
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hAac);
	
	conf->outputFormat = FAAD_FMT_32BIT;
	bitsPerSample = 32;
	
	//set the new configuration
	NeAACDecSetConfiguration(hAac, conf);
	
	get_AAC_format(inFd, &info, &seekTable, &seekTableLength, 1);

	fseek(inFd, 0, SEEK_SET);
	
	inputAmount = fread(inputBuffer, 1, INPUT_BUFFER_SIZE, inFd);
	
	unsigned long samplerate;
	unsigned char c;
	//Initialize the library using one of the initalization functions
	char err = NeAACDecInit(hAac, (unsigned char *)inputBuffer, inputAmount, &samplerate, &c);
	if (err < 0)
	{
		//ERROR BLARG
		DBLog(@"AAC ERRROR");
		return NO;
	}
	inputAmount -= err;
	memmove(inputBuffer, &inputBuffer[err], inputAmount);
	
	frequency = (int)samplerate;
	channels = c;
	
	bitRate = (int)((float)info.bitrate/1000.0);
	totalSize = (long int)(info.length*(double)frequency/1000.0*channels*bitsPerSample/8);
	
	isBigEndian = YES;
	
	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	[self open:filename];

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numread = bufferAmount;
	int count = 0;
    char *sampleBuffer;
	
//	DBLog(@"FILLING BUFFER : %i, %i", size, bufferAmount);
//	DBLog(@"INPUT AMOUNT: %i/%i", inputAmount, INPUT_BUFFER_SIZE);
	//	DBLog(@"Fill buffer: %i", size);
	//Fill from buffer, going by bufferAmount
	//if still needs more, decode and repeat
	if (bufferAmount == 0)
	{
		int n;

		/* returns the length of the samples*/
		if (inputAmount < INPUT_BUFFER_SIZE && !feof(inFd))
		{
			n = fread(&inputBuffer[inputAmount], 1, INPUT_BUFFER_SIZE-inputAmount, inFd);
			inputAmount += n;
		}

		sampleBuffer = NeAACDecDecode(hAac, &hInfo, (unsigned char *)inputBuffer, inputAmount);

		if (hInfo.error == 0 && hInfo.samples > 0)
		{
			inputAmount -= hInfo.bytesconsumed;
			memmove(inputBuffer, &inputBuffer[hInfo.bytesconsumed], inputAmount);
			
			//DO STUFF with decoded data
			memcpy(buffer, sampleBuffer, hInfo.samples * (bitsPerSample/8));
		}
		else if (hInfo.error != 0)
		{
			DBLog(@"FAAD2 Warning %s %i\n", NeAACDecGetErrorMessage(hInfo.error), hInfo.channels);
			if (feof(inFd) && inputAmount == 0)
			{
				return 0;
			}
		}
		else
		{
			inputAmount -= hInfo.bytesconsumed;
			memmove(inputBuffer, &inputBuffer[hInfo.bytesconsumed], inputAmount);
		}
		
		bufferAmount = hInfo.samples*(bitsPerSample/8);
//		DBLog(@"REAL BUFFER AMOUNT: %i", bufferAmount);
	}
	
	count = bufferAmount;
	if (bufferAmount > size)
	{
		count = size;
	}
	
	memcpy(buf, buffer, count);
	
	bufferAmount -= count;
	
	if (bufferAmount > 0)
		memmove(buffer, &buffer[count], bufferAmount);
	
	if (count < size)
		numread = [self fillBuffer:(&((char *)buf)[count]) ofSize:(size - count)];
	else
		numread = 0;
	
	return count + numread;
}

- (double)seekToTime:(double)milliseconds
{
	int second;
	int i;
	unsigned long pos;
	unsigned long length;
	
	if (seekTableLength <= 1)
		return -1;
	
	length = (unsigned long)(totalSize /(frequency * channels*(bitsPerSample/8)));

	second = (int)(milliseconds/1000.0);
	i = (int)(((float)second/length)*seekTableLength);

	pos = seekTable[i];
	
	fseek(inFd, pos, SEEK_SET);
	inputAmount = 0;
	NeAACDecPostSeekReset(hAac, -1);

	return second*1000.0;
}

- (void)close
{
	NeAACDecClose(hAac);
	fclose(inFd);
}

@end
