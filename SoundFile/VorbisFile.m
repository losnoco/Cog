//
//  VorbisFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "VorbisFile.h"


@implementation VorbisFile

- (BOOL)open:(const char *)filename
{
	inFd = fopen(filename, "rb");
	if (inFd == 0)
		return NO;
	
	if (ov_open(inFd, &vorbisRef, NULL, 0) != 0)
		return NO;
	
	return [self readInfo];	
}

- (BOOL)readInfo
{
	vorbis_info *vi;
	
	vi = ov_info(&vorbisRef, -1);
	bitRate = (int)(vi->bitrate_nominal/1000.0);
	channels = vi->channels;
	bitsPerSample = vi->channels * 8;
	frequency = vi->rate;

	totalSize = ov_pcm_total(&vorbisRef, -1) * channels * bitsPerSample/8;

//	DBLog(@"Ok to go WITH OGG.");

	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	return [self open:filename]; //automatically invokes readInfo
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
	
	currentPosition += total;
	
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


@end
