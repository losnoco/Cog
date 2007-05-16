//
//  ShnFile.mm
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "ShortenDecoder.h"

@implementation ShortenDecoder

- (BOOL)open:(id<CogSource>)source
{
	NSURL *url = [source url];
	if (![[url scheme] isEqualToString:@"file"])
		return NO;
	
	[source close];
	
	decoder	= new shn_reader;
	
	if (!decoder)
	{
		return NO;
	}
	
	decoder->open([[url path] UTF8String], true);
	
	bufferSize = decoder->shn_get_buffer_block_size(NUM_DEFAULT_BUFFER_BLOCKS);
	
	decoder->file_info(NULL, &channels, &frequency, NULL, &bitsPerSample, NULL);
	
	length = decoder->shn_get_song_length();
	
	decoder->go();

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
		
	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	long totalRead, amountRead, amountToRead;
	
	totalRead = 0;
	
	//For some reason the busy loop is causing pops when output is set to 48000. Probably CPU starvation, since the SHN decoder seems to use a multithreaded approach.
//	while (totalRead < size) {
		amountToRead = size - totalRead;
		if (amountToRead > bufferSize) { 
			amountToRead = bufferSize;
		}
	
		do
		{
			amountRead = decoder->read(buf, amountToRead);
		} while(amountRead == -1 && totalRead == 0);
		
//		if (amountRead <= 0) {
//			return totalRead;
//		}

		totalRead += amountRead;
//		buf = (void *)((char *)buf + amountRead);
//	}

	return totalRead;
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

	/*if (shn_cleanup_decoder(handle))
		shn_unload(handle);*/
}

- (BOOL)seekable
{
	return YES;
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
