//
//  FlacDecoder.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FlacDecoder.h"


@implementation FlacDecoder

FLAC__StreamDecoderReadStatus ReadCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;
	*bytes = [[flacDecoder source] read:buffer amount:*bytes];
	
	if(*bytes < 0) {
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
	else if(*bytes == 0) {
		[flacDecoder setEndOfStream:YES];
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else {
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
}

FLAC__StreamDecoderSeekStatus SeekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;
	
	if(![[flacDecoder source] seek:absolute_byte_offset whence:SEEK_SET])
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus TellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;

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
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;
	
	return (FLAC__bool)[flacDecoder endOfStream];
}

FLAC__StreamDecoderLengthStatus LengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;
	
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

FLAC__StreamDecoderWriteStatus WriteCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const sampleBuffer[], void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;
	
	void *buffer = [flacDecoder buffer];

	int8_t  *alias8;
	int16_t *alias16;
	int32_t *alias32;
	int sample, channel;
	int32_t	audioSample;

    switch(frame->header.bits_per_sample) {
        case 8:
            // Interleave the audio (no need for byte swapping)
            alias8 = buffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    *alias8++ = (int8_t)sampleBuffer[channel][sample];
                }
            }

            break;

        case 16:
            // Interleave the audio, converting to big endian byte order
            alias16 = buffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    *alias16++ = (int16_t)OSSwapHostToBigInt16((int16_t)sampleBuffer[channel][sample]);
                }
            }

            break;

        case 24:
            // Interleave the audio (no need for byte swapping)
            alias8 = buffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    audioSample = sampleBuffer[channel][sample];
                    *alias8++   = (int8_t)(audioSample >> 16);
                    *alias8++   = (int8_t)(audioSample >> 8);
                    *alias8++   = (int8_t)audioSample;
                }
            }

            break;

        case 32:
            // Interleave the audio, converting to big endian byte order
            alias32 = buffer;
            for(sample = 0; sample < frame->header.blocksize; ++sample) {
                for(channel = 0; channel < frame->header.channels; ++channel) {
                    *alias32++ = OSSwapHostToBigInt32(sampleBuffer[channel][sample]);
                }
            }
		default:
			NSLog(@"Error, unsupported sample size.");
	}

	[flacDecoder setBufferAmount:frame->header.blocksize * frame->header.channels * (frame->header.bits_per_sample/8)];

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void MetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;

	flacDecoder->channels = metadata->data.stream_info.channels;
	flacDecoder->frequency = metadata->data.stream_info.sample_rate;
	flacDecoder->bitsPerSample = metadata->data.stream_info.bits_per_sample;
	
	flacDecoder->length = ((double)metadata->data.stream_info.total_samples*1000.0)/flacDecoder->frequency;
	
	[flacDecoder willChangeValueForKey:@"properties"];
	[flacDecoder didChangeValueForKey:@"properties"];
}

void ErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	//Do nothing?
}

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
	
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
										 self
										 ) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		return NO;
	}
	
	FLAC__stream_decoder_process_until_end_of_metadata(decoder);

	buffer = malloc(SAMPLE_BUFFER_SIZE);

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int count;
	int numread;
	
	if (bufferAmount == 0)
	{
		if (FLAC__stream_decoder_get_state (decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		{
			return 0;
		}
		
		FLAC__stream_decoder_process_single(decoder);
	}

	count = bufferAmount;
	if (bufferAmount > size)
	{
		count = size;
	}
	memcpy(buf, buffer, count);
	
	bufferAmount -= count;
	
	if (bufferAmount > 0)
		memmove((char *)buffer, &((char *)buffer)[count], bufferAmount);
	
	if (count < size)
		numread = [self fillBuffer:(&((char *)buf)[count]) ofSize:(size - count)];
	else
		numread = 0;
	
	return count + numread;
	
}

- (void)close
{
	if (decoder)
	{
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
	}
	if (buffer)
	{
		free(buffer);
	}
	[self setSource:nil];

	decoder = NULL;
	buffer = NULL;
}

- (double)seekToTime:(double)milliseconds
{
	FLAC__stream_decoder_seek_absolute(decoder, frequency * ((double)milliseconds/1000.0));
	
	return milliseconds;
}

//bs methods
- (char *)buffer
{
	return buffer;
}
- (int)bufferAmount
{
	return bufferAmount;
}
- (void)setBufferAmount:(int)amount
{
	bufferAmount = amount;
}

- (FLAC__StreamDecoder *)decoder
{
	return decoder;
}

- (void)setSource:(id<CogSource>)s
{
	[s retain];
	[source release];
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

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithDouble:length],@"length",
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"big",@"endian",
		nil];
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"flac", @"fla", nil];
}

@end
