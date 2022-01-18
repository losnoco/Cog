//
//  FlacDecoder.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FlacDecoder.h"

#import "Logging.h"

@implementation FlacDecoder

FLAC__StreamDecoderReadStatus ReadCallback(const FLAC__StreamDecoder *decoder, FLAC__byte blockBuffer[], size_t *bytes, void *client_data)
{
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;
    long bytesRead = [[flacDecoder source] read:blockBuffer amount:*bytes];
	
	if(bytesRead < 0) {
        *bytes = 0;
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
	else if(bytesRead == 0) {
        *bytes = 0;
		[flacDecoder setEndOfStream:YES];
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else {
        *bytes = bytesRead;
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
}

FLAC__StreamDecoderSeekStatus SeekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;
	
	if(![[flacDecoder source] seek:absolute_byte_offset whence:SEEK_SET])
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus TellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

	off_t pos;
	if((pos = [[flacDecoder source] tell]) < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__bool EOFCallback(const FLAC__StreamDecoder *decoder, void *client_data)
{
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;
	
	return (FLAC__bool)[flacDecoder endOfStream];
}

FLAC__StreamDecoderLengthStatus LengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;
	
	if ([[flacDecoder source] seekable]) {
		long currentPos = [[flacDecoder source] tell];
		
		[[flacDecoder source] seek:0 whence:SEEK_END];
		*stream_length = [[flacDecoder source] tell];
		
		[[flacDecoder source] seek:currentPos whence:SEEK_SET];
		
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
	else {
		*stream_length = 0;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	}
}

FLAC__StreamDecoderWriteStatus WriteCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const sampleblockBuffer[], void *client_data)
{
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;
	
	void *blockBuffer = [flacDecoder blockBuffer];

	int8_t  *alias8;
	int16_t *alias16;
	int32_t *alias32;
	int sample, channel;
	int32_t	audioSample;

    switch(frame->header.bits_per_sample) {
        case 8:
            // Interleave the audio (no need for byte swapping)
            alias8 = blockBuffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    *alias8++ = (int8_t)sampleblockBuffer[channel][sample];
                }
            }

            break;

        case 16:
            // Interleave the audio, converting to big endian byte order
            alias16 = blockBuffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    *alias16++ = (int16_t)OSSwapHostToBigInt16((int16_t)sampleblockBuffer[channel][sample]);
                }
            }

            break;

        case 24:
            // Interleave the audio (no need for byte swapping)
            alias8 = blockBuffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    audioSample = sampleblockBuffer[channel][sample];
                    *alias8++   = (int8_t)(audioSample >> 16);
                    *alias8++   = (int8_t)(audioSample >> 8);
                    *alias8++   = (int8_t)audioSample;
                }
            }

            break;

        case 32:
            // Interleave the audio, converting to big endian byte order
            alias32 = blockBuffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    *alias32++ = OSSwapHostToBigInt32(sampleblockBuffer[channel][sample]);
                }
            }
		default:
            // Time for some nearest byte padding up to 32
            alias8 = blockBuffer;
            int sampleSize = frame->header.bits_per_sample;
            int sampleBit;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    int32_t sampleExtended = sampleblockBuffer[channel][sample];
                    for(sampleBit = sampleSize - 8; sampleBit >= -8; sampleBit -= 8) {
                        if (sampleBit >= 0)
                            *alias8++ = (uint8_t)((sampleExtended >> sampleBit) & 0xFF);
                        else
                            *alias8++ = (uint8_t)((sampleExtended << -sampleBit) & 0xFF);
                    }
                }
            }
            break;
	}

	[flacDecoder setBlockBufferFrames:frame->header.blocksize];

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

// This callback is only called for STREAMINFO blocks
void MetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    // Some flacs observed in the wild have multiple STREAMINFO metadata blocks,
    // of which only first one has sane values, so only use values from the first STREAMINFO
    // to determine stream format (this seems to be consistent with flac spec: http://flac.sourceforge.net/format.html)
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

    if (!flacDecoder->hasStreamInfo) {
        flacDecoder->channels = metadata->data.stream_info.channels;
        flacDecoder->frequency = metadata->data.stream_info.sample_rate;
        flacDecoder->bitsPerSample = metadata->data.stream_info.bits_per_sample;
	
        flacDecoder->totalFrames = metadata->data.stream_info.total_samples;
	
        [flacDecoder willChangeValueForKey:@"properties"];
        [flacDecoder didChangeValueForKey:@"properties"];
        
        flacDecoder->hasStreamInfo = YES;
    }
}

void ErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	//Do nothing?
}

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
    [self setSize:0];
    
    if ([s seekable])
    {
        [s seek:0 whence:SEEK_END];
        [self setSize:[s tell]];
        [s seek:0 whence:SEEK_SET];
    }
	
	decoder = FLAC__stream_decoder_new();
	if (decoder == NULL)
		return NO;
		
	if (FLAC__stream_decoder_init_stream(decoder,
										 ReadCallback,
										 ([source seekable] ? SeekCallback : NULL),
										 ([source seekable] ? TellCallback : NULL),
										 ([source seekable] ? LengthCallback : NULL),
										 ([source seekable] ? EOFCallback : NULL),
										 WriteCallback,
										 MetadataCallback,
										 ErrorCallback,
										 (__bridge void *)(self)
										 ) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		return NO;
	}
	
	FLAC__stream_decoder_process_until_end_of_metadata(decoder);

	blockBuffer = malloc(SAMPLE_blockBuffer_SIZE);

	return YES;
}

- (int)readAudio:(void *)buffer frames:(UInt32)frames
{
	int framesRead = 0;
	int bytesPerFrame = ((bitsPerSample+7)/8) * channels;
	while (framesRead < frames)
	{	
		if (blockBufferFrames == 0)
		{
			if (FLAC__stream_decoder_get_state (decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
			{
				break;
			}
		
			FLAC__stream_decoder_process_single(decoder);
		}
	
		int framesToRead = blockBufferFrames;
		if (blockBufferFrames > frames)
		{
			framesToRead = frames;
		}

		memcpy(((uint8_t *)buffer) + (framesRead * bytesPerFrame), (uint8_t *)blockBuffer, framesToRead * bytesPerFrame);

		frames -= framesToRead;
		framesRead += framesToRead;
		blockBufferFrames -= framesToRead;
		
		if (blockBufferFrames > 0)
		{
			memmove((uint8_t *)blockBuffer, ((uint8_t *)blockBuffer) + (framesToRead * bytesPerFrame), blockBufferFrames * bytesPerFrame);
		}
	}	
	
	return framesRead;
}

- (void)close
{
	if (decoder)
	{
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
	}
	if (blockBuffer)
	{
		free(blockBuffer);
	}

	decoder = NULL;
	blockBuffer = NULL;
}

- (void)dealloc
{
    [self close];
}

- (long)seek:(long)sample
{
	if (!FLAC__stream_decoder_seek_absolute(decoder, sample))
        return -1;
	
	return sample;
}

//bs methods
- (char *)blockBuffer
{
	return blockBuffer;
}
- (int)blockBufferFrames
{
	return blockBufferFrames;
}
- (void)setBlockBufferFrames:(int)frames
{
	blockBufferFrames = frames;
}

- (FLAC__StreamDecoder *)decoder
{
	return decoder;
}

- (void)setSource:(id<CogSource>)s
{
	source = s;
}
- (id<CogSource>)source
{
	return source;
}

- (void)setEndOfStream:(BOOL)eos
{
	endOfStream = eos;
}

- (BOOL)endOfStream
{
	return endOfStream;
}

- (void)setSize:(long)size
{
    fileSize = size;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithDouble:totalFrames],@"totalFrames",
		[NSNumber numberWithBool:[source seekable]], @"seekable",
        [NSNumber numberWithInt:fileSize ? (fileSize * 8 / ((totalFrames + (frequency / 2)) / frequency)) / 1000 : 0], @"bitrate",
        @"FLAC",@"codec",
		@"big",@"endian",
		nil];
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"flac", nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-flac", nil];
}

+ (float)priority
{
    return 2.0;
}

+ (NSArray *)fileTypeAssociations
{
    return @[
        @[@"FLAC Audio File", @"flac.icns", @"flac"]
    ];
}

@end
