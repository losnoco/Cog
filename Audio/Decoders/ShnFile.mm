//
//  ShnFile.mm
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "ShnFile.h"

@implementation ShnFile

- (BOOL)open:(const char *)filename
{
	if ([self readInfo:filename] == NO)
		return NO;
	
	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	decoder	= new shn_reader;
	
	if (!decoder)
	{
		return NO;
	}
	
	decoder->open(filename, true);
	
	bufferSize = decoder->shn_get_buffer_block_size(512);
	buffer = malloc(bufferSize);
	
	unsigned int length;
	int chan;
	float freq;
	int bps;
	
	decoder->file_info(NULL, &chan, &freq, NULL, &bps, NULL);
	
	/*NSLog(@"chan: %d",chan);
	NSLog(@"freq: %f",freq);
	NSLog(@"bps: %d",bps);*/

	channels = chan;
	frequency = (int)freq;
	bitsPerSample = bps;
	
	/*NSLog(@"channels: %d",channels);
	NSLog(@"frequency: %f",(double)frequency);
	NSLog(@"bitsPerSample: %d",bitsPerSample);*/

	length = decoder->shn_get_song_length();
	//NSLog(@"length: %d",length);
	totalSize = (((double)(length)*frequency)/1000.0) * channels * (bitsPerSample/8);
	bitrate = (int)((double)totalSize/((double)length/1000.0));
	
	/*NSLog(@"totalSize: %d",totalSize);
	NSLog(@"bitrate: %d",bitrate);*/
	
	decoder->go();
	
	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{

	//long numread = bufferAmount;
	//long count = 0;
	long numread, count;
	bufferAmount = 0;
	inputBuffer = malloc(bufferSize);
	
	//Fill from buffer, going by bufferAmount
	//if still needs more, decode and repeat
	if (bufferAmount == 0)
	{
		//bufferAmount = shn_read(handle, buffer, bufferSize);
		while((bufferAmount = decoder->read(inputBuffer, bufferSize)) == (unsigned)(-1))
		{
			bufferAmount = decoder->read(inputBuffer, bufferSize);
		}
		if (bufferAmount == 0)
			return 0;
		else if(bufferAmount == (unsigned)( -2))
		{
			//NSLog(@"closing file, eof");
			return -2;
		}
		else
		{
			memcpy(buffer, inputBuffer, bufferAmount);
			free(inputBuffer);
		}
	}
	
	//NSLog(@"bufferAmount: %d",bufferAmount);
	
	
	count = bufferAmount;
	if (bufferAmount > size)
	{
		count = size;
	}
	
	memcpy(buf, buffer, count);
	
	bufferAmount -= count;
	
	if (bufferAmount > 0)
		memmove(buffer, (&((char *)buffer)[count]), bufferAmount);
	
	if (count < size)
		numread = [self fillBuffer:(&((char *)buf)[count]) ofSize:(size - count)];
	else
		numread = 0;
	
	return count + numread;
}

- (double)seekToTime:(double)milliseconds
{
	unsigned int sec;
	
	/*if (!shn_seekable(handle))
		return -1.0;*/
	
	sec = (int)(milliseconds/1000.0);
	
	//shn_seek(handle, sec);
	
	decoder->seek(sec);
	
	return (sec * 1000.0);
}

- (void)close
{
	if(decoder)
	{
		decoder->exit();
		delete decoder;
		decoder	= NULL;
	}

	if (buffer)
	{
		free(buffer);
		buffer = NULL;
	}
	
	/*if (shn_cleanup_decoder(handle))
		shn_unload(handle);*/
}

@end
