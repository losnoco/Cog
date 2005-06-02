//
//  AACFile.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 5/31/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "AACFile.h"


@implementation AACFile

- (BOOL)open:(const char *)filename
{
	unsigned long cap = NeAACDecGetCapabilities();
	//Check if decoder has the needed capabilities
	
	//Open the library
	hAac = NeAACDecOpen();
	
	//Get the current config
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hAac);
	
//	conf->useOldADTSFormat = 1;
	DBLog(@"CONFIG: %i", conf->useOldADTSFormat);
	//if needed, change some of the values in conf
	conf->outputFormat = FAAD_FMT_32BIT;
//	conf->downMatrix = 1;
//	channels = 1;
	bitsPerSample = 32;

	//set the new configuration
	NeAACDecSetConfiguration(hAac, conf);
	
	inFd = fopen(filename, "r");
	if (!inFd)
		return NO;
	
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

	isBigEndian = YES;
	
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

- (void)close
{
}

@end
