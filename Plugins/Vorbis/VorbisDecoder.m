//
//  VorbisFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "VorbisDecoder.h"


@implementation VorbisDecoder

- (BOOL)open:(NSURL *)url
{
	inFd = fopen([[url path] UTF8String], "rb");
	if (inFd == 0)
		return NO;
	
	if (ov_open(inFd, &vorbisRef, NULL, 0) != 0)
		return NO;
	
	vorbis_info *vi;
	
	vi = ov_info(&vorbisRef, -1);

	bitsPerSample = vi->channels * 8;
	bitrate = (vi->bitrate_nominal/1000.0);
	channels = vi->channels;
	frequency = vi->rate;

	length = ((double)ov_pcm_total(&vorbisRef, -1) * 1000.0)/frequency;

//	DBLog(@"Ok to go WITH OGG.");

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numread;
    int total = 0;
	
    numread = ov_read(&vorbisRef, &((char *)buf)[total], size - total, 0, bitsPerSample/8, 1, &currentSection);
    while (total != size && numread > 0)
    {
		total += numread;
		
		numread = ov_read(&vorbisRef, &((char *)buf)[total], size - total, 0, bitsPerSample/8, 1, &currentSection);
    }
	
    return total;
}

- (void)close
{
	ov_clear(&vorbisRef);
}

- (double)seekToTime:(double)milliseconds
{
	ov_time_seek(&vorbisRef, (double)milliseconds/1000.0);
	
	return milliseconds;
}


- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels], @"channels",
		[NSNumber numberWithInt:bitsPerSample], @"bitsPerSample",
		[NSNumber numberWithFloat:frequency], @"sampleRate",
		[NSNumber numberWithDouble:length], @"length",
		[NSNumber numberWithInt:bitrate], @"bitrate",
		nil];
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"ogg",nil];
}


@end
