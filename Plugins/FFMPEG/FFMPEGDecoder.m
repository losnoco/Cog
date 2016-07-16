//
//  FFMPEGDecoder.m
//  FFMPEG
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

// test
#import "FFMPEGDecoder.h"

#include <pthread.h>

#import "Logging.h"

#define ST_BUFF 2048

int ffmpeg_read(void *opaque, uint8_t *buf, int buf_size)
{
    id source = (__bridge id) opaque;
    return (int) [source read:buf amount:buf_size];
}

int ffmpeg_write(void *opaque, uint8_t *buf, int buf_size)
{
    return -1;
}

int64_t ffmpeg_seek(void *opaque, int64_t offset, int whence)
{
    id source = (__bridge id) opaque;
    return [source seekable] ? ([source seek:offset whence:whence] ? [source tell] : -1) : -1;
}

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
        av_lockmgr_register(lockmgr_callback);
    }
}

- (id)init
{
    self = [super init];
    if (self) {
        lastReadPacket = NULL;
        lastDecodedFrame = NULL;
        codecCtx = NULL;
        formatCtx = NULL;
        ioCtx = NULL;
        buffer = NULL;
    }
    return self;
}

- (BOOL)open:(id<CogSource>)s
{
	int errcode, i;
    
    source = s;
	
	formatCtx = NULL;
	totalFrames = 0;
	framesRead = 0;

	// register all available codecs
    
    buffer = av_malloc(128 * 1024);
    if (!buffer)
    {
        ALog(@"Out of memory!");
        return NO;
    }
    
    ioCtx = avio_alloc_context(buffer, 128 * 1024, 0, (__bridge void *)source, ffmpeg_read, ffmpeg_write, ffmpeg_seek);
    if (!ioCtx)
    {
        ALog(@"Unable to create AVIO context");
        return NO;
    }
    
    formatCtx = avformat_alloc_context();
    if (!formatCtx)
    {
        ALog(@"Unable to allocate AVFormat context");
        return NO;
    }
    
    formatCtx->pb = ioCtx;
	
	if ((errcode = avformat_open_input(&formatCtx, "", NULL, NULL)) < 0)
	{
        char errDescr[4096];
        av_strerror(errcode, errDescr, 4096);
        ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
		return NO;
	}
	
    if((errcode = avformat_find_stream_info(formatCtx, NULL)) < 0)
    {
        char errDescr[4096];
        av_strerror(errcode, errDescr, 4096);
        ALog(@"Can't find stream info, errcode = %d, error = %s", errcode, errDescr);
        return NO;
    }
    
    streamIndex = -1;
    AVCodecParameters *codecPar;
    
	for(i = 0; i < formatCtx->nb_streams; i++) {
        codecPar = formatCtx->streams[i]->codecpar;
        if(codecPar->codec_type == AVMEDIA_TYPE_AUDIO)
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
    
    codecCtx = avcodec_alloc_context3(NULL);
    if (!codecCtx)
    {
        ALog(@"could not allocate codec context");
        return NO;
    }
    
    if ( (errcode = avcodec_parameters_to_context(codecCtx, codecPar)) < 0 )
    {
        char errDescr[4096];
        av_strerror(errcode, errDescr, 4096);
        ALog(@"Can't copy codec parameters to context, errcode = %d, error = %s", errcode, errDescr);
        return NO;
    }
    
    av_codec_set_pkt_timebase(codecCtx, formatCtx->streams[streamIndex]->time_base);

    AVCodec * codec = avcodec_find_decoder(codecCtx->codec_id);
    if (!codec) {
        ALog(@"codec not found");
		return NO;
    }
    
    if ( (errcode = avcodec_open2(codecCtx, codec, NULL)) < 0) {
        char errDescr[4096];
        av_strerror(errcode, errDescr, 4096);
        ALog(@"could not open codec, errcode = %d, error = %s", errcode, errDescr);
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
    
	//totalFrames = codecCtx->sample_rate * ((float)formatCtx->duration/AV_TIME_BASE);
    AVRational tb = (AVRational) { 1, codecCtx->sample_rate };
    totalFrames = av_rescale_q(formatCtx->streams[streamIndex]->duration, formatCtx->streams[streamIndex]->time_base, tb);
    bitrate = (int)((codecCtx->bit_rate) / 1000);
    framesRead = 0;
    endOfStream = NO;
    endOfAudio = NO;
    
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
    
    if (codecCtx) { avcodec_close(codecCtx); avcodec_free_context(&codecCtx); codecCtx = NULL; }
    
    if (formatCtx) { avformat_close_input(&(formatCtx)); formatCtx = NULL; }
    
    if (ioCtx) { buffer = ioCtx->buffer; av_free(ioCtx); ioCtx = NULL; }
    
    if (buffer) { av_free(buffer); buffer = NULL; }
}

- (void)dealloc
{
    [self close];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    if ( totalFrames && framesRead >= totalFrames )
        return 0;

    int frameSize = channels * (bitsPerSample / 8);
    int dataSize = 0;
    
    int bytesToRead = frames * frameSize;
    int bytesRead = 0;
    
    int errcode;
    
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
        
        while(readNextPacket && !endOfAudio)
        {
            // consume next chunk of encoded data from input stream
            if (!endOfStream)
            {
                av_packet_unref(lastReadPacket);
                if((errcode = av_read_frame(formatCtx, lastReadPacket)) < 0)
                {
                    if (errcode == AVERROR_EOF)
                    {
                        DLog(@"End of stream");
                        endOfStream = YES;
                    }
                    if (formatCtx->pb && formatCtx->pb->error) break;
                }
                if (lastReadPacket->stream_index != streamIndex)
                    continue;
            }
            
            if ((errcode = avcodec_send_packet(codecCtx, endOfStream ? NULL : lastReadPacket)) < 0)
            {
                if (errcode != AVERROR(EAGAIN))
                {
                    char errDescr[4096];
                    av_strerror(errcode, errDescr, 4096);
                    ALog(@"Error sending packet to codec, errcode = %d, error = %s", errcode, errDescr);
                    return 0;
                }
            }
            
            readNextPacket = NO;     // we probably won't need to consume another chunk
        }

        if (dataSize <= bytesConsumedFromDecodedFrame)
        {
            if (endOfStream && endOfAudio)
                break;
            
            bytesConsumedFromDecodedFrame = 0;

            if ((errcode = avcodec_receive_frame(codecCtx, lastDecodedFrame)) < 0)
            {
                if (errcode == AVERROR_EOF)
                {
                    endOfAudio = YES;
                    break;
                }
                else if (errcode == AVERROR(EAGAIN))
                {
                   // Read another packet
                    readNextPacket = YES;
                    continue;
                }
                else
                {
                    char errDescr[4096];
                    av_strerror(errcode, errDescr, 4096);
                    ALog(@"Error receiving frame, errcode = %d, error = %s", errcode, errDescr);
                    return 0;
                }
            }

            // Something has been successfully decoded
            dataSize = av_samples_get_buffer_size(&planeSize, codecCtx->channels,
                                                  lastDecodedFrame->nb_samples,
                                                  codecCtx->sample_fmt, 1);
                
            if ( dataSize < 0 )
                dataSize = 0;
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
        framesReadNow = (int)(totalFrames - framesRead);
    
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
        endOfAudio = YES;
        return -1;
    }
    int64_t ts = frame * (formatCtx->duration) / totalFrames;
    avformat_seek_file(formatCtx, -1, ts - 1000, ts, ts, AVSEEK_FLAG_ANY);
    avcodec_flush_buffers(codecCtx);
    readNextPacket = YES; // so we immediately read next packet
    bytesConsumedFromDecodedFrame = INT_MAX; // so we immediately begin decoding next frame
    framesRead = frame;
    endOfStream = NO;
    endOfAudio = NO;
	
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
	return [NSArray arrayWithObjects:@"wma", @"asf", @"xwma", @"xma", @"tak", @"mp3", @"mp2", @"m2a", @"mpa", @"ape", @"ac3", @"dts", @"dtshd", @"at3", @"wav", @"tta", @"vqf", @"vqe", @"vql", nil];
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
