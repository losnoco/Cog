//
//  ShnFile.m
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
	
	bufferSize = shn_get_buffer_block_size(handle, 512);
	buffer = malloc(bufferSize);
	
	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	shn_config conf;
	int r;
	
	conf.error_output_method =  ERROR_OUTPUT_DEVNULL;
	conf.seek_tables_path = NULL;
	conf.relative_seek_tables_path = ".";
	conf.verbose = 0;
	conf.swap_bytes = 0;
	
	handle = shn_load((char *)filename, conf);
	if (!handle)
		return NO;
	
	r = shn_init_decoder(handle);
	if (r < 0)
		return NO;
	
	channels = shn_get_channels(handle);
	frequency = shn_get_samplerate(handle);
	bitsPerSample = shn_get_bitspersample(handle);
	
	unsigned int length;
	length = shn_get_song_length(handle);
	totalSize = (((double)(length)*frequency)/1000.0) * channels * (bitsPerSample/8);
	
	bitRate = (int)((double)totalSize/((double)length/1000.0));
	
	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numread = bufferAmount;
	int count = 0;
	
	//Fill from buffer, going by bufferAmount
	//if still needs more, decode and repeat
	if (bufferAmount == 0)
	{
		bufferAmount = shn_read(handle, buffer, bufferSize);
		if (bufferAmount == 0)
			return 0;
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
	unsigned int sec;
	
	if (!shn_seekable(handle))
		return -1.0;
	
	sec = (int)(milliseconds/1000.0);
	
	shn_seek(handle, sec);
	
	return (sec * 1000.0);
}

- (void)close
{
	if (buffer)
		free(buffer);
	
	if (shn_cleanup_decoder(handle))
		shn_unload(handle);
}

@end
