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

	
	int err, i;
	const char *filename = [[[source url] path] UTF8String];
	
	ic = NULL;
	numFrames = 0;
	samplePos = 0;
	sampleBuffer = av_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
	// register all available codecs
	av_register_all();
	
	
	err = av_open_input_file(&ic, filename, NULL, 0, NULL);
	
	if (err < 0)
	{
		NSLog(@"Opening .WMA file failed horribly: %d", err);
		return NO;
	}
	
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
	NSLog(@"sample rate: %d", c->sample_rate);
	NSLog(@"channels: %d", c->channels);
	
	channels = c->channels;
	bitrate = ic->bit_rate;
	bitsPerSample = c->channels * 8;
	totalFrames = c->sample_rate * (ic->duration/1000000LL);
	frequency = c->sample_rate;
	seekable = YES;
	
	return YES;
	
}

- (void)close
{
	avcodec_close(c);
	av_close_input_file(ic);
	av_free(sampleBuffer);
	
	[source close];
	[source release];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	uint8_t *inbuf_ptr;
	int size, out_size, len;
	AVPacket framePacket;
	int framesRead = 0;
	
	int bytesPerFrame = (bitsPerSample/8) * channels;
	

	while (frames > 0)
	{
		if (samplePos < numFrames)
		{
			int samplesLeft;
			samplesLeft = numFrames - samplePos;
			
			if (samplesLeft > frames)
				samplesLeft = frames;
			
			memcpy(buf, sampleBuffer + (samplePos * bytesPerFrame), samplesLeft * bytesPerFrame);
			buf += samplesLeft * bytesPerFrame;
			framesRead += samplesLeft;
			frames -= samplesLeft;
			samplePos += samplesLeft;
		}
		if (frames > 0)
		{
			if (av_read_frame(ic, &framePacket) < 0)
			{
				NSLog(@"Uh oh... av_read_frame returned negative");
				break;
			}
		
			size = framePacket.size;
			inbuf_ptr = framePacket.data;
	
			len = avcodec_decode_audio(c, (void *)sampleBuffer, &numFrames, 
										   inbuf_ptr, size);
			
			if (len < 0) 
				break;
			
            if (out_size <= 0) 
				continue;
			
			numFrames /= bytesPerFrame;
			samplePos = 0;
				
			// the frame packet needs to be freed before we av_read_frame a new one
			if (framePacket.data)
				av_free_packet(&framePacket);

		}
	}
	
	return framesRead;
	
}

- (long)seek:(long)frame
{
	NSLog(@"frame: %ld", frame);
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
