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
    if (whence & AVSEEK_SIZE)
    {
        if ([source seekable])
        {
            int64_t curOffset = [source tell];
            [source seek:0 whence:SEEK_END];
            int64_t size = [source tell];
            [source seek:curOffset whence:SEEK_SET];
            return size;
        }
        return -1;
    }
    whence &= ~(AVSEEK_SIZE | AVSEEK_FORCE);
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
    AVStream *stream;
    
    source = s;
	
	formatCtx = NULL;
	totalFrames = 0;
	framesRead = 0;

	// register all available codecs
    
    buffer = av_malloc(32 * 1024);
    if (!buffer)
    {
        ALog(@"Out of memory!");
        return NO;
    }
    
    ioCtx = avio_alloc_context(buffer, 32 * 1024, 0, (__bridge void *)source, ffmpeg_read, ffmpeg_write, ffmpeg_seek);
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
        stream = formatCtx->streams[i];
        codecPar = stream->codecpar;
        if(streamIndex < 0 && codecPar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			DLog(@"audio codec found");
            streamIndex = i;
		}
        else
        {
            stream->discard = AVDISCARD_ALL;
        }
    }
    
    if ( streamIndex < 0 ) {
        ALog(@"no audio codec found");
        return NO;
    }
    
    stream = formatCtx->streams[streamIndex];
    codecPar = stream->codecpar;
    
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

    enum AVCodecID codec_id = codecCtx->codec_id;
    AVCodec * codec = NULL;
    AVDictionary * dict = NULL;

    if (@available(macOS 10.15, *))
    {
        switch (codec_id)
        {
            case AV_CODEC_ID_MP3:
                codec = avcodec_find_decoder_by_name("mp3_at");
                break;
            case AV_CODEC_ID_MP2:
                codec = avcodec_find_decoder_by_name("mp2_at");
                break;
            case AV_CODEC_ID_MP1:
                codec = avcodec_find_decoder_by_name("mp1_at");
                break;
            case AV_CODEC_ID_AAC:
                codec = avcodec_find_decoder_by_name("aac_at");
                break;
            case AV_CODEC_ID_ALAC:
                codec = avcodec_find_decoder_by_name("alac_at");
                break;
            case AV_CODEC_ID_AC3:
                codec = avcodec_find_decoder_by_name("ac3_at");
                break;
            case AV_CODEC_ID_EAC3:
                codec = avcodec_find_decoder_by_name("eac3_at");
                break;
            default: break;
        }
    }
    else
    {
        switch (codec_id)
        {
            case AV_CODEC_ID_MP3:
                codec = avcodec_find_decoder_by_name("mp3float");
                break;
            case AV_CODEC_ID_MP2:
                codec = avcodec_find_decoder_by_name("mp2float");
                break;
            case AV_CODEC_ID_MP1:
                codec = avcodec_find_decoder_by_name("mp1float");
                break;
            case AV_CODEC_ID_AAC:
                codec = avcodec_find_decoder_by_name("libfdk_aac");
                av_dict_set_int(&dict, "drc_level", -2, 0); // disable DRC
                av_dict_set_int(&dict, "level_limit", 0, 0); // disable peak limiting
                break;
            case AV_CODEC_ID_ALAC:
                codec = avcodec_find_decoder_by_name("alac");
                break;
            case AV_CODEC_ID_AC3:
                codec = avcodec_find_decoder_by_name("ac3");
                break;
            case AV_CODEC_ID_EAC3:
                codec = avcodec_find_decoder_by_name("eac3");
                break;
            default: break;
        }
    }

    if (!codec)
        codec = avcodec_find_decoder(codec_id);

    if (@available(macOS 10.15, *)) {
    }
    else {
        if (codec && codec->name) {
            const char * name = codec->name;
            size_t name_len = strlen(name);
            if (name_len > 3)
            {
                name += name_len - 3;
                if (!strcmp(name, "_at"))
                {
                    ALog(@"AudioToolbox decoder picked on old macOS, disabling: %s", codec->name);
                    codec = NULL; // Disable AudioToolbox codecs on Mojave and older
                }
            }
        }
    }

    if (!codec) {
        ALog(@"codec not found");
        av_dict_free(&dict);
		return NO;
    }

    if ( (errcode = avcodec_open2(codecCtx, codec, &dict)) < 0) {
        char errDescr[4096];
        av_dict_free(&dict);
        av_strerror(errcode, errDescr, 4096);
        ALog(@"could not open codec, errcode = %d, error = %s", errcode, errDescr);
        return NO;
    }
    
    av_dict_free(&dict);
    
    lastDecodedFrame = av_frame_alloc();
    av_frame_unref(lastDecodedFrame);
    lastReadPacket = malloc(sizeof(AVPacket));
    av_new_packet(lastReadPacket, 0);
    readNextPacket = YES;
    bytesConsumedFromDecodedFrame = INT_MAX;
    seekFrame = -1;
    
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
    
    lossy = NO;
    if (floatingPoint)
        lossy = YES;
    
    if (!floatingPoint) {
        switch (codec_id) {
            case AV_CODEC_ID_MP2:
            case AV_CODEC_ID_MP3:
            case AV_CODEC_ID_AAC:
            case AV_CODEC_ID_AC3:
            // case AV_CODEC_ID_DTS: // lossy will return float, caught above, lossless will be integer
            case AV_CODEC_ID_VORBIS:
            case AV_CODEC_ID_DVAUDIO:
            case AV_CODEC_ID_WMAV1:
            case AV_CODEC_ID_WMAV2:
            case AV_CODEC_ID_MACE3:
            case AV_CODEC_ID_MACE6:
            case AV_CODEC_ID_VMDAUDIO:
            case AV_CODEC_ID_MP3ADU:
            case AV_CODEC_ID_MP3ON4:
            case AV_CODEC_ID_WESTWOOD_SND1:
            case AV_CODEC_ID_GSM:
            case AV_CODEC_ID_QDM2:
            case AV_CODEC_ID_COOK:
            case AV_CODEC_ID_TRUESPEECH:
            case AV_CODEC_ID_SMACKAUDIO:
            case AV_CODEC_ID_QCELP:
            case AV_CODEC_ID_DSICINAUDIO:
            case AV_CODEC_ID_IMC:
            case AV_CODEC_ID_MUSEPACK7:
            case AV_CODEC_ID_MLP:
            case AV_CODEC_ID_GSM_MS:
            case AV_CODEC_ID_ATRAC3:
            case AV_CODEC_ID_NELLYMOSER:
            case AV_CODEC_ID_MUSEPACK8:
            case AV_CODEC_ID_SPEEX:
            case AV_CODEC_ID_WMAVOICE:
            case AV_CODEC_ID_WMAPRO:
            case AV_CODEC_ID_ATRAC3P:
            case AV_CODEC_ID_EAC3:
            case AV_CODEC_ID_SIPR:
            case AV_CODEC_ID_MP1:
            case AV_CODEC_ID_TWINVQ:
            case AV_CODEC_ID_MP4ALS:
            case AV_CODEC_ID_ATRAC1:
            case AV_CODEC_ID_BINKAUDIO_RDFT:
            case AV_CODEC_ID_BINKAUDIO_DCT:
            case AV_CODEC_ID_AAC_LATM:
            case AV_CODEC_ID_QDMC:
            case AV_CODEC_ID_CELT:
            case AV_CODEC_ID_G723_1:
            case AV_CODEC_ID_G729:
            case AV_CODEC_ID_8SVX_EXP:
            case AV_CODEC_ID_8SVX_FIB:
            case AV_CODEC_ID_BMV_AUDIO:
            case AV_CODEC_ID_RALF:
            case AV_CODEC_ID_IAC:
            case AV_CODEC_ID_ILBC:
            case AV_CODEC_ID_OPUS:
            case AV_CODEC_ID_COMFORT_NOISE:
            case AV_CODEC_ID_METASOUND:
            case AV_CODEC_ID_PAF_AUDIO:
            case AV_CODEC_ID_ON2AVC:
            case AV_CODEC_ID_DSS_SP:
            case AV_CODEC_ID_CODEC2:
            case AV_CODEC_ID_FFWAVESYNTH:
            case AV_CODEC_ID_SONIC:
            case AV_CODEC_ID_SONIC_LS:
            case AV_CODEC_ID_EVRC:
            case AV_CODEC_ID_SMV:
            case AV_CODEC_ID_4GV:
            case AV_CODEC_ID_INTERPLAY_ACM:
            case AV_CODEC_ID_XMA1:
            case AV_CODEC_ID_XMA2:
            case AV_CODEC_ID_ATRAC3AL:
            case AV_CODEC_ID_ATRAC3PAL:
            case AV_CODEC_ID_DOLBY_E:
            case AV_CODEC_ID_APTX:
            case AV_CODEC_ID_SBC:
            case AV_CODEC_ID_ATRAC9:
            case AV_CODEC_ID_HCOM:
            case AV_CODEC_ID_ACELP_KELVIN:
            case AV_CODEC_ID_MPEGH_3D_AUDIO:
            case AV_CODEC_ID_SIREN:
            case AV_CODEC_ID_HCA:
            case AV_CODEC_ID_FASTAUDIO:
                lossy = YES;
                break;
                
            default:
                break;
        }
    }
    
	//totalFrames = codecCtx->sample_rate * ((float)formatCtx->duration/AV_TIME_BASE);
    AVRational tb = {.num = 1, .den = codecCtx->sample_rate};
    totalFrames = av_rescale_q(stream->duration, stream->time_base, tb);
    bitrate = (int)((codecCtx->bit_rate) / 1000);
    framesRead = 0;
    seekFrame = 0; // Skip preroll if necessary
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
    int seekBytesSkip = 0;
    
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

            // FFmpeg seeking by packet is usually inexact, so skip up to
            // target sample using packet timestamp
            // New: Moved here, because sometimes preroll packets also
            // trigger EAGAIN above, so ask for the next packet's timestamp
            // instead
            if (seekFrame >= 0 && errcode >= 0) {
                DLog(@"Seeking to frame %lld", seekFrame);
                AVRational tb = {.num = 1, .den = codecCtx->sample_rate};
                int64_t packetBeginFrame = av_rescale_q(
                        lastReadPacket->dts,
                        formatCtx->streams[streamIndex]->time_base,
                        tb
                );

                if (packetBeginFrame < seekFrame) {
                    seekBytesSkip += (int)((seekFrame - packetBeginFrame) * frameSize);
                }

                seekFrame = -1;
            }

            int minSkipped = FFMIN(dataSize, seekBytesSkip);
            bytesConsumedFromDecodedFrame += minSkipped;
            seekBytesSkip -= minSkipped;
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
    AVRational tb = {.num = 1, .den = codecCtx->sample_rate};
    int64_t ts = av_rescale_q(frame, tb, formatCtx->streams[streamIndex]->time_base);
    int ret = avformat_seek_file(formatCtx, streamIndex, ts - 1000, ts, ts, 0);
    avcodec_flush_buffers(codecCtx);
    if (ret < 0)
    {
        framesRead = totalFrames;
        endOfStream = YES;
        endOfAudio = YES;
        return -1;
    }
    readNextPacket = YES; // so we immediately read next packet
    bytesConsumedFromDecodedFrame = INT_MAX; // so we immediately begin decoding next frame
    framesRead = frame;
    seekFrame = frame;
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
            [NSString stringWithUTF8String:avcodec_get_name(codecCtx->codec_id)], @"codec",
            @"host", @"endian",
            lossy ? @"lossy" : @"lossless", @"encoding",
			nil];
}


+ (NSArray *)fileTypes
{
	return @[@"wma", @"asf", @"tak", @"mp4", @"m4a", @"aac", @"mp3", @"mp2", @"m2a", @"mpa", @"ape", @"ac3", @"dts", @"dtshd", @"wav", @"tta", @"vqf", @"vqe", @"vql", @"ra", @"rm", @"rmj", @"mka", @"weba"];
}

+ (NSArray *)mimeTypes
{
	return @[@"application/wma", @"application/x-wma", @"audio/x-wma", @"audio/x-ms-wma", @"audio/x-tak", @"application/ogg", @"audio/aacp", @"audio/mpeg", @"audio/mp4", @"audio/x-mp3", @"audio/x-mp2", @"audio/x-matroska", @"audio/x-ape", @"audio/x-ac3", @"audio/x-dts", @"audio/x-dtshd", @"audio/x-at3", @"audio/wav", @"audio/tta", @"audio/x-tta", @"audio/x-twinvq"];
}

+ (NSArray *)fileTypeAssociations
{
    return @[
        @[@"Windows Media Audio File", @"song.icns", @"wma", @"asf"],
        @[@"TAK Audio File", @"song.icns", @"tak"],
        @[@"MPEG-4 Audio File", @"m4a.icns", @"mp4", @"m4a"],
        @[@"MPEG-4 AAC Audio File", @"song.icns", @"aac"],
        @[@"MPEG Audio File", @"mp3.icns", @"mp3", @"m2a", @"mpa"],
        @[@"Monkey's Audio File", @"ape.icns", @"ape"],
        @[@"AC-3 Audio File", @"song.icns", @"ac3"],
        @[@"DTS Audio File", @"song.icns", @"dts"],
        @[@"DTS-HD MA Audio File", @"song.icns", @"dtshd"],
        @[@"True Audio File", @"song.icns", @"tta"],
        @[@"TrueVQ Audio File", @"song.icns", @"vqf", @"vqe", @"vql"],
        @[@"Real Audio File", @"song.icns", @"ra", @"rm", @"rmj"],
        @[@"Matroska Audio File", @"song.icns", @"mka"],
        @[@"WebM Audio File", @"song.icns", @"weba"]
    ];
}

+ (float)priority
{
    if (@available(macOS 10.15, *))
        return 1.5;
    else
        return 1.0;
}




@end
