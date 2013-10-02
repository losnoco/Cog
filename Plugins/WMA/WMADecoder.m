//
//  WMADecoder.m
//  WMA
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

// test
#import "WMADecoder.h"

#include <pthread.h>

#define ST_BUFF 2048

@implementation WMADecoder


int lockmgr_callback(void ** mutex, enum AVLockOp op)
{
    switch (op)
    {
        case AV_LOCK_CREATE:
            *mutex = malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(*mutex, NULL);
            break;
            
        case AV_LOCK_DESTROY:
            pthread_mutex_destroy(*mutex);
            free(*mutex);
            *mutex = NULL;
            break;
            
        case AV_LOCK_OBTAIN:
            pthread_mutex_lock(*mutex);
            break;
            
        case AV_LOCK_RELEASE:
            pthread_mutex_unlock(*mutex);
            break;
    }
    return 0;
}

+ (void)initialize
{
    av_register_all();
    av_lockmgr_register(lockmgr_callback);
}

- (BOOL)open:(id<CogSource>)s
{
	source = [s retain];

	
	int err, i;
	const char *filename = [[[source url] path] UTF8String];
	
	ic = NULL;
	numFrames = 0;
	samplePos = 0;
	sampleBuffer = NULL;
	// register all available codecs
	
	err = avformat_open_input(&ic, filename, NULL, NULL);
	
	if (err < 0)
	{
		NSLog(@"Opening .WMA file failed horribly: %d", err);
		return NO;
	}
	
	for(i = 0; i < ic->nb_streams; i++) {
        c = ic->streams[i]->codec;
        if(c->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			NSLog(@"audio codec found");
            break;
		}
    }

	avformat_find_stream_info(ic, NULL);
	
    codec = avcodec_find_decoder(c->codec_id);        
    if (!codec) {
        NSLog(@"codec not found");
		return NO;
    }
    
    if (avcodec_open2(c, codec, NULL) < 0) {
        NSLog(@"could not open codec");
        return NO;
    }
    
    av_dump_format(ic, 0, filename, 0);
	
    AVDictionary * metadata = ic->metadata;
    AVDictionaryEntry * entry;
    
	if ((entry = av_dict_get(metadata, "title", NULL, 0)))
        NSLog(@"Title: %s", entry->value);
	if ((entry = av_dict_get(metadata, "author", NULL, 0)))
        NSLog(@"Author: %s", entry->value);
	if ((entry = av_dict_get(metadata, "album", NULL, 0)))
        NSLog(@"Album: %s", entry->value);
	if ((entry = av_dict_get(metadata, "year", NULL, 0)))
        NSLog(@"Year: %d", entry->value);
	if ((entry = av_dict_get(metadata, "track", NULL, 0)))
        NSLog(@"Track: %d", entry->value);
	if ((entry = av_dict_get(metadata, "genre", NULL, 0)))
        NSLog(@"Genre: %s", entry->value);
	if ((entry = av_dict_get(metadata, "copyright", NULL, 0)))
        NSLog(@"Copyright: %s", entry->value);
	if ((entry = av_dict_get(metadata, "comment", NULL, 0)))
        NSLog(@"Comments: %s", entry->value);

	NSLog(@"bitrate: %d", ic->bit_rate);
	NSLog(@"sample rate: %d", c->sample_rate);
	NSLog(@"channels: %d", c->channels);
	
	channels = c->channels;
	bitrate = ic->bit_rate;
    switch (c->sample_fmt) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
            bitsPerSample = 8;
            break;
            
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            bitsPerSample = 16;
            break;
            
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            bitsPerSample = 32;
            break;
    }
	totalFrames = c->sample_rate * (ic->duration/1000000LL);
	frequency = c->sample_rate;
	seekable = YES;
	
	return YES;
	
}

- (void)close
{
	avcodec_close(c);
	avformat_close_input(&ic);
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
            size_t sampleBufferOffset = 0;
            
			if (av_read_frame(ic, &framePacket) < 0)
			{
				NSLog(@"Uh oh... av_read_frame returned negative");
				break;
			}
            
            AVFrame * frame = av_frame_alloc();
            int ret, got_frame = 0;
            
            while ( framePacket.size && (ret = avcodec_decode_audio4(c, frame, &got_frame, &framePacket)) >= 0 )
            {
                ret = FFMIN(ret, framePacket.size);
                framePacket.data += ret;
                framePacket.size -= ret;
                
                if ( !got_frame ) continue;
                
                int plane_size;
                int planar    = av_sample_fmt_is_planar(c->sample_fmt);
                int data_size = av_samples_get_buffer_size(&plane_size, c->channels,
                                                           frame->nb_samples,
                                                           c->sample_fmt, 1);
                
                sampleBuffer = av_realloc(sampleBuffer, sampleBufferOffset + data_size);
                
                if (!planar) {
                    memcpy((uint8_t *)sampleBuffer + sampleBufferOffset, frame->extended_data[0], plane_size);
                }
                else if (channels > 1) {
                    uint8_t * out = (uint8_t *)sampleBuffer + sampleBufferOffset;
                    int bytesPerSample = bitsPerSample / 8;
                    for (int s = 0; s < plane_size; s += bytesPerSample) {
                        for (int ch = 0; ch < channels; ++ch) {
                            memcpy(out, frame->extended_data[ch] + s, bytesPerSample);
                            out += bytesPerSample;
                        }
                    }
                }
                
                sampleBufferOffset += plane_size * channels;
            }
            
            av_frame_free(&frame);
            
			if (framePacket.data)
				av_free_packet(&framePacket);
            
            if ( !sampleBufferOffset ) {
                if ( ret < 0 ) break;
                else continue;
            }
			
			numFrames = sampleBufferOffset / bytesPerFrame;
			samplePos = 0;
            
            if ( numFrames ) {
                switch ( c->sample_fmt ) {
                    case AV_SAMPLE_FMT_FLT:
                    case AV_SAMPLE_FMT_FLTP:
                    {
                        float * input = (float *) sampleBuffer;
                        int32_t * output = (int32_t *) sampleBuffer;
                        
                        for (int i = 0; i < numFrames * channels; ++i)
                        {
                            float sample = input[i];
                            if (sample > 1.0) sample = 1.0;
                            else if (sample < -1.0) sample = -1.0;
                            output[i] = sample * INT_MAX;
                        }
                    }
                    break;
                        
                    case AV_SAMPLE_FMT_DBL:
                    case AV_SAMPLE_FMT_DBLP:
                    {
                        double * input = (double *) sampleBuffer;
                        int32_t * output = (int32_t *) sampleBuffer;
                        
                        for (int i = 0; i < numFrames * channels; ++i)
                        {
                            double sample = input[i];
                            if (sample > 1.0) sample = 1.0;
                            else if (sample < -1.0) sample = -1.0;
                            output[i] = sample * INT_MAX;
                        }
                    }
                    break;
                }
            }
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
