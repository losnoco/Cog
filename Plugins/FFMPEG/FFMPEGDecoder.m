//
//  FFMPEGDecoder.m
//  FFMPEG
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

// test
#import "FFMPEGDecoder.h"
#import "FFMPEGFileProtocols.h"

#include <pthread.h>

#import "Logging.h"

#define ST_BUFF 2048

@implementation FFMPEGDecoder


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
    if(self == [FFMPEGDecoder class])
    {
        av_log_set_flags(AV_LOG_SKIP_REPEATED);
        av_log_set_level(AV_LOG_ERROR);
        av_register_all();
        registerCogProtocols();
        av_lockmgr_register(lockmgr_callback);
    }
}

- (BOOL)open:(id<CogSource>)s
{
	int errcode, i;
	const char *filename = [[[[s url] absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String];
	
	formatCtx = NULL;
	totalFrames = 0;
	framesRead = 0;

	// register all available codecs
	
	if ((errcode = avformat_open_input(&formatCtx, filename, NULL, NULL)) < 0)
	{
        char errDescr[4096];
        av_strerror(errcode, errDescr, 4096);
        ALog(@"ERROR OPENING FILE, errcode = %d, error = %s", errcode, errDescr);
		return NO;
	}
	
    if(avformat_find_stream_info(formatCtx, NULL) < 0)
    {
        ALog(@"CAN'T FIND STREAM INFO!");
        return NO;
    }
    
    streamIndex = -1;
	for(i = 0; i < formatCtx->nb_streams; i++) {
        codecCtx = formatCtx->streams[i]->codec;
        if(codecCtx->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			DLog(@"audio codec found");
            streamIndex = i;
            break;
		}
    }
    
    if ( streamIndex < 0 ) {
        ALog(@"no audio codec found");
        return NO;
    }

    AVCodec * codec = avcodec_find_decoder(codecCtx->codec_id);
    if (!codec) {
        ALog(@"codec not found");
		return NO;
    }
    
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        ALog(@"could not open codec");
        return NO;
    }

    lastDecodedFrame = av_frame_alloc();
    av_frame_unref(lastDecodedFrame);
    lastReadPacket = malloc(sizeof(AVPacket));
    av_new_packet(lastReadPacket, 0);
    readNextPacket = YES;
    bytesConsumedFromDecodedFrame = INT_MAX;
    
    frequency = codecCtx->sample_rate;
    channels = codecCtx->channels;
    floatingPoint = NO;

    switch (codecCtx->sample_fmt) {
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
            bitsPerSample = 32;
            break;
            
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            bitsPerSample = 32;
            floatingPoint = YES;
            break;
            
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            bitsPerSample = 64;
            floatingPoint = YES;
            break;
            
        default:
            return NO;
    }
    
	totalFrames = codecCtx->sample_rate * ((float)formatCtx->duration/AV_TIME_BASE);
    bitrate = (codecCtx->bit_rate) / 1000;
    framesRead = 0;
    endOfStream = NO;
    
    if ( totalFrames < 0 )
        totalFrames = 0;
    
    seekable = [s seekable];
	
	return YES;
}

- (void)close
{
    if (lastReadPacket)
    {
        av_packet_unref(lastReadPacket);
        free(lastReadPacket);
        lastReadPacket = NULL;
    }
    
    if (lastDecodedFrame) { av_free(lastDecodedFrame); lastDecodedFrame = NULL; }
    
    if (codecCtx) { avcodec_close(codecCtx); codecCtx = NULL; }
    
    if (formatCtx) { avformat_close_input(&(formatCtx)); formatCtx = NULL; }
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    if ( totalFrames && framesRead >= totalFrames )
        return 0;

    int frameSize = channels * (bitsPerSample / 8);
    int gotFrame = 0;
    int dataSize = 0;
    
    int bytesToRead = frames * frameSize;
    int bytesRead = 0;
    
    int8_t* targetBuf = (int8_t*) buf;
    memset(buf, 0, bytesToRead);
    
    while (bytesRead < bytesToRead)
    {
        
        // buffer size needed to hold decoded samples, in bytes
        int planeSize;
        int planar = av_sample_fmt_is_planar(codecCtx->sample_fmt);
        dataSize = av_samples_get_buffer_size(&planeSize, codecCtx->channels,
                                              lastDecodedFrame->nb_samples,
                                              codecCtx->sample_fmt, 1);
        
        if ( dataSize < 0 )
            dataSize = 0;
        
        while(readNextPacket && !endOfStream)
        {
            // consume next chunk of encoded data from input stream
            av_packet_unref(lastReadPacket);
            if(av_read_frame(formatCtx, lastReadPacket) < 0)
            {
                DLog(@"End of stream");
                endOfStream = YES;
            }
            
            if (lastReadPacket->stream_index != streamIndex)
                continue;
            
            readNextPacket = NO;     // we probably won't need to consume another chunk
            bytesReadFromPacket = 0; // until this one is fully decoded
        }
        
        if (dataSize <= bytesConsumedFromDecodedFrame)
        {
            if (endOfStream)
                break;
            
            // consumed all decoded samples - decode more
            av_frame_unref(lastDecodedFrame);
            bytesConsumedFromDecodedFrame = 0;
            int len;
            do {
                len = avcodec_decode_audio4(codecCtx, lastDecodedFrame, &gotFrame, lastReadPacket);
                if (len > 0)
                {
                    if (len >= lastReadPacket->size) {
                        lastReadPacket->data -= bytesReadFromPacket;
                        lastReadPacket->size += bytesReadFromPacket;
                        readNextPacket = YES;
                        break;
                    }
                    bytesReadFromPacket += len;
                    lastReadPacket->data += len;
                    lastReadPacket->size -= len;
                }
            } while (!gotFrame && len > 0);
            if (len < 0)
            {
                char errbuf[4096];
                av_strerror(len, errbuf, 4096);
                ALog(@"Error decoding: len = %d, gotFrame = %d, strerr = %s", len, gotFrame, errbuf);
                
                dataSize = 0;
                readNextPacket = YES;
            }
            else
            {
                // Something has been successfully decoded
                dataSize = av_samples_get_buffer_size(&planeSize, codecCtx->channels,
                                                      lastDecodedFrame->nb_samples,
                                                      codecCtx->sample_fmt, 1);
                
                if ( dataSize < 0 )
                    dataSize = 0;
            }
        }

        int toConsume = FFMIN((dataSize - bytesConsumedFromDecodedFrame), (bytesToRead - bytesRead));
        
        // copy decoded samples to Cog's buffer
        if (!planar || channels == 1) {
            memmove(targetBuf + bytesRead, (lastDecodedFrame->data[0] + bytesConsumedFromDecodedFrame), toConsume);
        }
        else {
            uint8_t * out = ( uint8_t * ) targetBuf + bytesRead;
            int bytesPerSample = bitsPerSample / 8;
            int bytesConsumedPerPlane = bytesConsumedFromDecodedFrame / channels;
            int toConsumePerPlane = toConsume / channels;
            for (int s = 0; s < toConsumePerPlane; s += bytesPerSample) {
                for (int ch = 0; ch < channels; ++ch) {
                    memcpy(out, lastDecodedFrame->extended_data[ch] + bytesConsumedPerPlane + s, bytesPerSample);
                    out += bytesPerSample;
                }
            }
        }

        bytesConsumedFromDecodedFrame += toConsume;
        bytesRead += toConsume;
    }
    
    int framesReadNow = bytesRead / frameSize;
    if ( totalFrames && ( framesRead + framesReadNow > totalFrames ) )
        framesReadNow = totalFrames - framesRead;
    
    framesRead += framesReadNow;
    
    return framesReadNow;
}

- (long)seek:(long)frame
{
    if ( !totalFrames )
        return -1;
    
    if (frame >= totalFrames)
    {
        framesRead = totalFrames;
        endOfStream = YES;
        return -1;
    }
    int64_t ts = frame * (formatCtx->duration) / totalFrames;
    avformat_seek_file(formatCtx, -1, ts - 1000, ts, ts, AVSEEK_FLAG_ANY);
    avcodec_flush_buffers(codecCtx);
    readNextPacket = YES; // so we immediately read next packet
    bytesConsumedFromDecodedFrame = INT_MAX; // so we immediately begin decoding next frame
    framesRead = frame;
    endOfStream = NO;
	
    return frame;
}


- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInt:channels], @"channels",
			[NSNumber numberWithInt:bitsPerSample], @"bitsPerSample",
            [NSNumber numberWithBool:(bitsPerSample == 8)], @"Unsigned",
			[NSNumber numberWithFloat:frequency], @"sampleRate",
            [NSNumber numberWithBool:floatingPoint], @"floatingPoint",
			[NSNumber numberWithDouble:totalFrames], @"totalFrames",
			[NSNumber numberWithInt:bitrate], @"bitrate",
			[NSNumber numberWithBool:seekable], @"seekable",
            @"host", @"endian",
			nil];
}


+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"wma", @"asf", @"xwma", @"tak", @"mp3", @"mp2", @"m2a", @"mpa", @"ape", @"ac3", @"dts", @"dtshd", @"at3", @"wav", @"tta", @"vqf", @"vqe", @"vql", nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"application/wma", @"application/x-wma", @"audio/x-wma", @"audio/x-ms-wma", @"audio/x-tak", @"audio/mpeg", @"audio/x-mp3", @"audio/x-mp2", @"audio/x-ape", @"audio/x-ac3", @"audio/x-dts", @"audio/x-dtshd", @"audio/x-at3", @"audio/wav", @"audio/tta", @"audio/x-tta", @"audio/x-twinvq", nil];
}

+ (float)priority
{
    return 1.0;
}




@end
