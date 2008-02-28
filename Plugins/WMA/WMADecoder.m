//
//  WMADecoder.m
//  WMA
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

// test
#import "WMADecoder.h"

#define ST_BUFF 2048

@implementation WMADecoder




- (BOOL)open:(id<CogSource>)s
{
	source = [s retain];
	
	ic = NULL;
	av_register_all();
	int err, i;
	const char *filename = [[[source url] path] UTF8String];
	int st_buff;
	uint8_t *outbuf, *s_outbuf;
	
	
	NSLog(@"lolbots: %s", filename);
	
	
	err = av_open_input_file(&ic, filename, NULL, 0, NULL);
	
	if (err < 0)
		NSLog(@"Opening .WMA file failed horribly: %d", err);
	
	for(i = 0; i < ic->nb_streams; i++) {
        c = &ic->streams[i]->codec;
        if(c->codec_type == CODEC_TYPE_AUDIO)
		{
			NSLog(@"audio codec found");
            break;
		}
    }

	av_find_stream_info(ic);
	
    codec = avcodec_find_decoder(c->codec_id);        
    if (!codec) {
        NSLog(@"codec not found");
		return NO;
    }
    
    if (avcodec_open(c, codec) < 0) {
        NSLog(@"could not open codec");
        return NO;
    }
    
    st_buff = ST_BUFF;
	
    outbuf = av_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
    s_outbuf = av_malloc(st_buff);
	
    dump_format(ic, 0, filename, 0);	
	
	if (ic->title[0] != '\0')
        NSLog(@"Title: %s", ic->title);
    if (ic->author[0] != '\0')
        NSLog(@"Author: %s", ic->author);
    if (ic->album[0] != '\0')
        NSLog(@"Album: %s", ic->album);
    if (ic->year != 0)
        NSLog(@"Year: %d", ic->year);
    if (ic->track != 0)
        NSLog(@"Track: %d", ic->track);
    if (ic->genre[0] != '\0')
        NSLog(@"Genre: %s", ic->genre);
    if (ic->copyright[0] != '\0')
        NSLog(@"Copyright: %s", ic->copyright);
    if (ic->comment[0] != '\0')
        NSLog(@"Comments: %s", ic->comment);

	NSLog(@"bitrate: %d", ic->bit_rate);
	return YES;
	
}

- (void)close
{
	avcodec_close(c);
	av_close_input_file(ic);
	
	[source close];
	[source release];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	return 0;
	
}

- (long)seek:(long)frame
{
	return 0;
}


- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInt:channels], @"channels",
			[NSNumber numberWithInt:bitsPerSample], @"bitsPerSample",
			[NSNumber numberWithFloat:frequency], @"sampleRate",
			[NSNumber numberWithDouble:totalFrames], @"totalFrames",
			[NSNumber numberWithInt:bitrate], @"bitrate",
			[NSNumber numberWithBool:([source seekable] && seekable)], @"seekable",
			nil];
}


+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"wma",nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"application/wma", @"application/x-wma", @"audio/x-wma", nil];
}





@end
