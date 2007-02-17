//
//  MADFile.m
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "MADFile.h"
#undef HAVE_CONFIG_H
#import <ID3Tag/id3tag.h>

@implementation MADFile

/*XING FUN*/

#define XING_MAGIC	(('X' << 24) | ('i' << 16) | ('n' << 8) | 'g')
#define INFO_MAGIC	(('I' << 24) | ('n' << 16) | ('f' << 8) | 'o')
#define LAME_MAGIC  (('L' << 24) | ('A' << 16) | ('M' << 8) | 'E')

struct xing
{
	long flags;			/* valid fields (see below) */
	unsigned long frames;		/* total number of frames */
	unsigned long bytes;		/* total number of bytes */
	unsigned char toc[100];	/* 100-point seek table */
	long scale;			/* ?? */
};

struct lame
{
	long flags;
};

enum
{
	XING_FRAMES = 0x00000001L,
	XING_BYTES  = 0x00000002L,
	XING_TOC    = 0x00000004L,
	XING_SCALE  = 0x00000008L
};

int lame_parse(struct lame *lame, struct mad_bitptr *ptr, unsigned int bitlen)
{
	unsigned long magic;
	unsigned long garbage;
	
	magic = mad_bit_read(ptr, 32); //4 bytes
	
	if (magic != LAME_MAGIC)
		return 0;
	
	mad_bit_skip(ptr, 17*8); //17 bytes skipped
	garbage = mad_bit_read(ptr, 24); //3 bytes
//	_startPadding = (garbage >> 12) & 0x000FFF;
//	_endPadding = garbage & 0x000FFF;

	return 1;
}	

int xing_parse(struct xing *xing, struct mad_bitptr *ptr, unsigned int bitlen)
{
	xing->flags = 0;
	unsigned long magic;
	
	if (bitlen < 64)
		return 0;

	magic = mad_bit_read(ptr, 32);
	if (magic != INFO_MAGIC && magic != XING_MAGIC)
		return 0;
	
	xing->flags = mad_bit_read(ptr, 32);
	bitlen -= 64;
	
	if (xing->flags & XING_FRAMES) {
		if (bitlen < 32)
			return 0;
		
		xing->frames = mad_bit_read(ptr, 32);
		bitlen -= 32;
	}
	
	if (xing->flags & XING_BYTES) {
		if (bitlen < 32)
			return 0;
		
		xing->bytes = mad_bit_read(ptr, 32);
		bitlen -= 32;
	}
	
	if (xing->flags & XING_TOC) {
		int i;
		
		if (bitlen < 800)
			return 0;
		
		for (i = 0; i < 100; ++i)
			xing->toc[i] = mad_bit_read(ptr, 8);
		
		bitlen -= 800;
	}
	
	if (xing->flags & XING_SCALE) {
		if (bitlen < 32)
			return 0;
		
		xing->scale = mad_bit_read(ptr, 32);
		bitlen -= 32;
	}
	
	return 1;
}

int parse_headers(struct xing *xing, struct lame *lame, struct mad_bitptr ptr, unsigned int bitlen)
{
	xing->flags = 0;
	lame->flags = 0;
	
	if (xing_parse(xing, &ptr, bitlen))
	{
		lame_parse(lame, &ptr, bitlen);
		
		return 1;
	}
	
	return 0;
}


- (BOOL)scanFileFast:(BOOL)fast useXing:(BOOL)use_xing
{
	const int BUFFER_SIZE = 16*1024;
	const int N_AVERAGE_FRAMES = 10;

	struct mad_stream stream;
	struct mad_header header;
	struct mad_frame frame; /* to read xing data */
	struct xing xing;
	struct lame lame;
	int remainder = 0;
	int data_used = 0;
	int len = 0;
	int tagsize = 0;
	int frames = 0;
	unsigned char buffer[BUFFER_SIZE];
	BOOL has_xing = NO;
	BOOL vbr = NO;
	
	mad_stream_init (&stream);
	mad_header_init (&header);
	mad_frame_init (&frame);	
	
	bitrate = 0;
	_duration = mad_timer_zero;
	
	frames = 0;
	
	fseek(_inFd, 0, SEEK_END);
	_fileSize = ftell(_inFd);
	fseek(_inFd, 0, SEEK_SET);
	
	BOOL done = NO;
	
	while (!done)
    {
		remainder = stream.bufend - stream.next_frame;
				
		memcpy (buffer, stream.this_frame, remainder);
		len = fread(buffer + remainder, 1, BUFFER_SIZE - remainder, _inFd);
		
		if (len <= 0)
			break;
		
		mad_stream_buffer (&stream, buffer, len + remainder);
		
		while (1)
		{
			if (mad_header_decode (&header, &stream) == -1)
			{
				if (stream.error == MAD_ERROR_BUFLEN)
				{
					break;
				}
				if (!MAD_RECOVERABLE (stream.error))
				{
					break;
				}
				if (stream.error == MAD_ERROR_LOSTSYNC)
				{
					/* ignore LOSTSYNC due to ID3 tags */
					tagsize = id3_tag_query (stream.this_frame,
											 stream.bufend -
											 stream.this_frame);
					if (tagsize > 0)
					{
						mad_stream_skip (&stream, tagsize);
						continue;
					}

				}

				continue;
			}
			frames++;

			mad_timer_add (&_duration, header.duration);
			data_used += stream.next_frame - stream.this_frame;
			if (frames == 1)
			{
				/* most of these *should* remain constant */
				bitrate = header.bitrate;
				frequency = header.samplerate;
				channels = MAD_NCHANNELS(&header);

				if (use_xing)
				{
					frame.header = header;
					if (mad_frame_decode(&frame, &stream) == -1)
						continue;

					if (parse_headers(&xing, &lame, stream.anc_ptr, stream.anc_bitlen)) 
					{
						has_xing = YES;
						vbr = YES;

						frames = xing.frames;
						mad_timer_multiply (&_duration, frames);

						bitrate = 8.0 * xing.bytes / mad_timer_count(_duration, MAD_UNITS_SECONDS); 
						done = YES;
						break;
					}
				}
				
			}
			else
			{
				/* perhaps we have a VBR file */
				if (bitrate != header.bitrate)
					vbr = YES;
				if (vbr)
					bitrate += header.bitrate;
			}
			
			if ((!vbr || (vbr && !has_xing)) && fast && frames >= N_AVERAGE_FRAMES)
			{
				float frame_size = ((double)data_used) / N_AVERAGE_FRAMES;
				frames = (_fileSize - tagsize) / frame_size;

				_duration.seconds /= N_AVERAGE_FRAMES;
				_duration.fraction /= N_AVERAGE_FRAMES;
				mad_timer_multiply (&_duration, frames);

				done = YES;
				break;
			}
		}
		if (stream.error != MAD_ERROR_BUFLEN)
			break;
    }
	
	if (vbr && !has_xing)
		bitrate = bitrate / frames;
	
	mad_frame_finish (&frame);
	mad_header_finish (&header);
	mad_stream_finish (&stream);
	
	totalSize = (mad_timer_count(_duration, MAD_UNITS_MILLISECONDS)*(frequency/1000.0))*channels*(bitsPerSample/8);	

	bitrate /= 1000;
	NSLog(@"BITRATE: %i %i", bitrate, vbr);
	fseek(_inFd, 0, SEEK_SET);
	
	return frames != 0;	
}


- (BOOL)open:(const char *)filename
{	
	/* First the structures used by libmad must be initialized. */
	mad_stream_init(&_stream);
	mad_frame_init(&_frame);
	mad_synth_init(&_synth);
	mad_timer_reset(&_timer);
	
	_inFd = fopen(filename, "r");
	if (!_inFd)
		return NO;
	
	bitsPerSample = 16;
	isBigEndian=YES;
	isUnsigned=NO;
	
	return [self scanFileFast:YES useXing:YES];
}

- (BOOL)readInfo:(const char *)filename
{
	return [self open:filename];
}

/**
* Scale PCM data
 */
static inline signed int scale (mad_fixed_t sample)
{
	BOOL hard_limit = YES;
//	BOOL replaygain = NO;
	/* replayGain by SamKR */
	double scale = -1;
/*	if (replaygain)
    {
		if (file_info->has_replaygain)
        {
			scale = file_info->replaygain_track_scale;
			if (file_info->replaygain_album_scale != -1
				&& (scale==-1 || ! xmmsmad_config.replaygain.track_mode))
            {
				scale = file_info->replaygain_album_scale;
            }
        }
        if (scale == -1)
            scale = xmmsmad_config.replaygain.default_scale;
    }
*/
	if (scale == -1)
		scale = 1.0;
	
	/* hard-limit (clipping-prevention) */
	if (hard_limit)
    {
		/* convert to double before computation, to avoid mad_fixed_t wrapping */
		double x = mad_f_todouble(sample) * scale;
		static const double k = 0.5; // -6dBFS
		if (x > k)
		{
			x = tanh((x - k) / (1-k)) * (1-k) + k;
		}
		else if(x < -k)
		{
			x = tanh((x + k) / (1-k)) * (1-k) - k;
		}
		sample = x * (MAD_F_ONE);
    }
	else
		sample *= scale;
	
	int n_bits_to_loose = MAD_F_FRACBITS + 1 - 16;
	
	/* round */
	/* add half of the bits_to_loose range to round */
	sample += (1L << (n_bits_to_loose - 1));
			
	/* clip */
	/* make sure we are between -1 and 1 */
	if (sample >= MAD_F_ONE)
    {
		sample = MAD_F_ONE - 1;
    }
	else if (sample < -MAD_F_ONE)
    {
		sample = -MAD_F_ONE;
    }
	
	/* quantize */
	/*
	 * Turn our mad_fixed_t into an integer.
	 * Shift all but 16-bits of the fractional part
	 * off the right hand side and shift an extra place
	 * to get the sign bit.
	 */
	sample >>= n_bits_to_loose;

	return sample;
}


- (void)writeOutput
{
	unsigned int nsamples;
	mad_fixed_t const *left_ch, *right_ch;

//	if (_outputAvailable) {
//		NSLog(@"Losing Output: %i", _outputAvailable);
//	}
	nsamples = _synth.pcm.length;
	left_ch = _synth.pcm.samples[0];
	right_ch = _synth.pcm.samples[1];
	_outputAvailable = nsamples * channels * (bitsPerSample/8);

	if (_outputBuffer)
		free(_outputBuffer);

	_outputBuffer = (unsigned char *) malloc (_outputAvailable * sizeof (char));
	
	unsigned char *outputPtr = _outputBuffer;
	
	int i;
	for (i=0; i < nsamples; i++)
    {
		signed short sample;
		/* output sample(s) in 16-bit signed little-endian PCM */
		sample = scale(left_ch[i]);
		*(outputPtr++) = sample>>8;
		*(outputPtr++) = sample & 0xff;

		if (channels == 2)
		{
			sample = scale(right_ch[i]);
			*(outputPtr++) = sample>>8;
			*(outputPtr++) = sample & 0xff;
		}
    }	
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int remainder;
	int len;
	BOOL eof = NO;
	int amountToCopy = size;
	int amountRemaining = size;
	
	if (amountToCopy > _outputAvailable)
		amountToCopy = _outputAvailable;

	if (amountToCopy) {
		memcpy(buf, _outputBuffer, amountToCopy);
		memmove(_outputBuffer, _outputBuffer + amountToCopy, _outputAvailable - amountToCopy);

		amountRemaining -= amountToCopy;
		_outputAvailable -= amountToCopy;
	}

	while (amountRemaining > 0 && !eof) {
		if (_stream.buffer == NULL || _stream.error == MAD_ERROR_BUFLEN)
		{
			if (!_seekSkip)
			{
				remainder = _stream.bufend - _stream.next_frame;
				if (remainder)
					memmove(_inputBuffer, _stream.this_frame, remainder);
			}
			else
			{
				remainder = 0;
			}

			len = fread(_inputBuffer+remainder, 1, INPUT_BUFFER_SIZE-remainder, _inFd);
			if (len <= 0)
			{
				eof = YES;
				break;
			}

			len += remainder;
			if (len < MAD_BUFFER_GUARD) {
				int i;
				for (i = len; i < MAD_BUFFER_GUARD; i++)
					_inputBuffer[i] = 0;
				len = MAD_BUFFER_GUARD;
			}

			mad_stream_buffer(&_stream, _inputBuffer, len);
			_stream.error = 0;
			
			if (_seekSkip)
			{
				int skip = 2;
				do
				{
					if (mad_frame_decode (&_frame, &_stream) == 0)
					{
						mad_timer_add (&_timer, _frame.header.duration);
						if (--skip == 0)
							mad_synth_frame (&_synth, &_frame);
					}
					else if (!MAD_RECOVERABLE (_stream.error))
						break;
				} while (skip);
				
				_seekSkip = NO;
			}
			
		}		
		if (mad_frame_decode(&_frame, &_stream) == -1) {
			if (!MAD_RECOVERABLE (_stream.error))
			{
				if(_stream.error==MAD_ERROR_BUFLEN) {
					continue;
				}

				eof = YES;
			}

			if (_stream.error == MAD_ERROR_LOSTSYNC)
			{
				// ignore LOSTSYNC due to ID3 tags
				int tagsize = id3_tag_query (_stream.this_frame,
											 _stream.bufend -
											 _stream.this_frame);
				if (tagsize > 0)
				{
					mad_stream_skip (&_stream, tagsize);
				}
			}

			continue;
		}
		mad_timer_add (&_timer, _frame.header.duration);
		
		mad_synth_frame (&_synth, &_frame);
		
		[self writeOutput];
		amountToCopy = amountRemaining;
		if (amountToCopy > _outputAvailable) {
			amountToCopy = _outputAvailable;
		}
		if (amountRemaining < amountToCopy) {
			amountToCopy = amountRemaining;
		}
		
		memcpy(((char *)buf)+(size-amountRemaining), _outputBuffer, amountToCopy);
		memmove(_outputBuffer, _outputBuffer + amountToCopy, _outputAvailable - amountToCopy); 
		amountRemaining -= amountToCopy;
		_outputAvailable -= amountToCopy;
	}

	return (size - amountRemaining);
}

- (void)close
{
	fclose(_inFd);
	
	mad_synth_finish(&_synth);
	mad_frame_finish(&_frame);
	mad_stream_finish(&_stream);	
}

- (double)seekToTime:(double)milliseconds
{
	int new_position;
	int seconds = milliseconds/1000.0;
	int total_seconds = mad_timer_count(_duration, MAD_UNITS_SECONDS);

	if (seconds > total_seconds)
		seconds = total_seconds;
	
	mad_timer_set(&_timer, seconds, 0, 0);
	new_position = ((double) seconds / (double) total_seconds) * _fileSize;

	fseek(_inFd, new_position, SEEK_SET);
	mad_stream_sync(&_stream);
	_stream.error = MAD_ERROR_BUFLEN;
	_stream.sync = 0;
	_outputAvailable = 0;

	mad_frame_mute(&_frame);
	mad_synth_mute(&_synth);

	_seekSkip = YES;
	
	return seconds*1000.0;
}

@end

