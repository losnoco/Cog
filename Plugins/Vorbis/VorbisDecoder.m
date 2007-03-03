//
//  VorbisFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "VorbisDecoder.h"


@implementation VorbisDecoder

size_t sourceRead(void *buf, size_t size, size_t nmemb, void *datasource)
{
	id source = (id)datasource;

	return [source read:buf amount:(size*nmemb)];
}

int sourceSeek(void *datasource, ogg_int64_t offset, int whence)
{
	id source = (id)datasource;
	return ([source seek:offset whence:whence] ? 0 : -1);
}

int sourceClose(void *datasource)
{
	id source = (id)datasource;
	[source close];

	return 0;
}

long sourceTell(void *datasource)
{
	id source = (id)datasource;

	return [source tell];
}

- (BOOL)open:(id<CogSource>)s
{
	source = [s retain];
	
	ov_callbacks callbacks = {
		read_func: sourceRead,
		seek_func: sourceSeek,
		close_func: sourceClose,
		tell_func: sourceTell
	};
	
	if (ov_open_callbacks(source, &vorbisRef, NULL, 0, callbacks) != 0)
	{
		NSLog(@"FAILED TO OPEN VORBIS FILE");
		return NO;
	}
	
	vorbis_info *vi;
	
	vi = ov_info(&vorbisRef, -1);
	
	bitsPerSample = vi->channels * 8;
	bitrate = (vi->bitrate_nominal/1000.0);
	channels = vi->channels;
	frequency = vi->rate;
	NSLog(@"INFO: %i", bitsPerSample);
	
	seekable = ov_seekable(&vorbisRef);
	
	length = 0.0;//((double)ov_pcm_total(&vorbisRef, -1) * 1000.0)/frequency;

	NSLog(@"Ok to go WITH OGG. %i", seekable);

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numread;
    int total = 0;
	
    do {
		lastSection = currentSection;
		numread = ov_read(&vorbisRef, &((char *)buf)[total], size - total, 0, bitsPerSample/8, 1, &currentSection);
		if (numread > 0) {
			total += numread;
		}
	
		if (currentSection != lastSection) {
			vorbis_info *vi;
			vi = ov_info(&vorbisRef, -1);
			
			bitsPerSample = vi->channels * 8;
			bitrate = (vi->bitrate_nominal/1000.0);
			channels = vi->channels;
			frequency = vi->rate;
			
			NSLog(@"Format changed...");
		}
		
    } while (total != size && numread != 0);

    return total;
}

- (void)close
{
	ov_clear(&vorbisRef);
	[source release];
}

- (double)seekToTime:(double)milliseconds
{
	ov_time_seek(&vorbisRef, (double)milliseconds/1000.0);
	
	return milliseconds;
}

- (BOOL) seekable
{
	return [source seekable];
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
