//
//  FlacDecoder.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FlacDecoder.h"


@implementation FlacDecoder

FLAC__StreamDecoderWriteStatus WriteProc(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const sampleBuffer[], void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;
	unsigned wide_samples = frame->header.blocksize;
	unsigned channels = frame->header.channels;
	
	UInt16 *buffer = (UInt16 *)[flacDecoder buffer];
	int i, j, c;

	for (i = j = 0; i < wide_samples; i++)
	{
		for (c = 0; c < channels; c++, j++)
		{
//			buffer[j] = CFSwapInt16LittleToHost(sampleBuffer[c][i]);
			buffer[j] = sampleBuffer[c][i];
		}
	}
	
//	memcpy([flacDecoder buffer], sampleBuffer, wide_samples*[flacDecoder bitsPerSample]/8);
	[flacDecoder setBufferAmount:(wide_samples * channels*2)];

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void MetadataProc(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	FlacDecoder *flacDecoder = (FlacDecoder *)client_data;

	flacDecoder->channels = metadata->data.stream_info.channels;
	flacDecoder->frequency = metadata->data.stream_info.sample_rate;
	flacDecoder->bitsPerSample = metadata->data.stream_info.bits_per_sample;
	
	flacDecoder->length = ((double)metadata->data.stream_info.total_samples*1000.0)/flacDecoder->frequency;
}

void ErrorProc(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	//Do nothing?
}

- (BOOL)open:(NSURL *)url
{
	FLAC__bool status;
	
	decoder = FLAC__file_decoder_new();
	if (decoder == NULL)
		return NO;
		
	status = FLAC__file_decoder_set_filename(decoder, [[url path] UTF8String]);
	if (status == false)
		return NO;
	
	status = FLAC__file_decoder_set_write_callback(decoder, WriteProc);
	if (status == false)
		return NO;

	status = FLAC__file_decoder_set_metadata_callback(decoder, MetadataProc);
	if (status == false)
		return NO;

	status = FLAC__file_decoder_set_error_callback(decoder, ErrorProc);
	if (status == false)
		return NO;
	
	status = FLAC__file_decoder_set_client_data(decoder, self);
	if (status == false)
		return NO;

	if (FLAC__file_decoder_init(decoder) != FLAC__FILE_DECODER_OK)
		return NO;

	FLAC__file_decoder_process_until_end_of_metadata(decoder);

	return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int count;
	int numread;
	
	if (bufferAmount == 0)
	{
		int i;
		if (FLAC__file_decoder_get_state (decoder) == FLAC__FILE_DECODER_END_OF_FILE)
		{
			return 0;
		}
		
		i = FLAC__file_decoder_process_single(decoder);// != FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE);
			//return 0;
//		[self writeSamplesToBuffer:buffer  fromBuffer:sampleBuffer ofSize:(status*2)];
//			write callback sets bufferAmount...frickin weird, also sets sampleBuffer
//		bufferAmount = status*4;
	}

	count = bufferAmount;
	if (bufferAmount > size)
	{
		count = size;
	}
	memcpy(buf, buffer, count);
	
	bufferAmount -= count;
	
	if (bufferAmount > 0)
		memmove(buffer, &buffer[count], bufferAmount);
	
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
		FLAC__file_decoder_finish(decoder);
		FLAC__file_decoder_delete(decoder);
	}
	decoder = NULL;
	
}

- (double)seekToTime:(double)milliseconds
{
	FLAC__file_decoder_seek_absolute(decoder, frequency * ((double)milliseconds/1000.0));
	
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

- (FLAC__FileDecoder *)decoder
{
	return decoder;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithDouble:length],@"length",
		@"host",@"endian",
		nil];
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"flac"];
}

@end
