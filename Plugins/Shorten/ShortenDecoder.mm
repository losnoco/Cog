//
//  ShnFile.mm
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "ShortenDecoder.h"

@implementation ShortenDecoder

- (BOOL)open:(NSURL *)url
{
	decoder	= new shn_reader;
	
	if (!decoder)
	{
		return NO;
	}
	
	decoder->open([[url path] UTF8String], true);
	
	bufferSize = decoder->shn_get_buffer_block_size(512);
	buffer = malloc(bufferSize);
	
	decoder->file_info(NULL, &channels, &frequency, NULL, &bitsPerSample, NULL);
	
	length = decoder->shn_get_song_length();
	
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


- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithDouble:length],@"length",
		@"little",@"endian",
		nil];
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"shn"];
}


@end
