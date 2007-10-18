//
//  MADFile.m
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "MADDecoder.h"
#undef HAVE_CONFIG_H

@implementation MADDecoder

#define LAME_HEADER_SIZE	((8 * 5) + 4 + 4 + 8 + 32 + 16 + 16 + 4 + 4 + 8 + 12 + 12 + 8 + 8 + 2 + 3 + 11 + 32 + 32 + 32)

// From vbrheadersdk:
// ========================================
// A Xing header may be present in the ancillary
// data field of the first frame of an mp3 bitstream
// The Xing header (optionally) contains
//      frames      total number of audio frames in the bitstream
//      bytes       total number of bytes in the bitstream
//      toc         table of contents

// toc (table of contents) gives seek points
// for random access
// the ith entry determines the seek point for
// i-percent duration
// seek point in bytes = (toc[i]/256.0) * total_bitstream_bytes
// e.g. half duration seek point = (toc[50]/256.0) * total_bitstream_bytes

#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008


//Scan file quickly
- (BOOL)scanFile
{
	BOOL inputEOF = NO;
	
	struct mad_stream stream;
	struct mad_frame frame;
	
	int framesDecoded = 0;
	int bytesToRead, bytesRemaining;
	int samplesPerMPEGFrame = 0;
	
	int id3_length = 0;
	
	mad_stream_init (&stream);
	mad_frame_init (&frame);	
	
	[_source seek:0 whence:SEEK_END];
	_fileSize = [_source tell];
	[_source seek:0 whence:SEEK_SET];
	
	for (;;) {
		if(NULL == stream.buffer || MAD_ERROR_BUFLEN == stream.error) {
			if(stream.next_frame) {
				bytesRemaining		= stream.bufend - stream.next_frame;
				
				memmove(_inputBuffer, stream.next_frame, bytesRemaining);
				
				bytesToRead			= INPUT_BUFFER_SIZE - bytesRemaining;
			}
			else {
				bytesToRead			= INPUT_BUFFER_SIZE,
				bytesRemaining		= 0;
			}
			
			// Read raw bytes from the MP3 file
			int bytesRead = [_source read:_inputBuffer + bytesRemaining amount: bytesToRead];
			
			if (bytesRead == 0)
			{
				memset(_inputBuffer + bytesRemaining + bytesRead, 0, MAD_BUFFER_GUARD);
				bytesRead += MAD_BUFFER_GUARD;
				inputEOF = YES;
			}
			
			mad_stream_buffer(&stream, _inputBuffer, bytesRead + bytesRemaining);
			stream.error = MAD_ERROR_NONE;
		}
		
		if (mad_frame_decode(&frame, &stream) == -1)
		{
			if (MAD_RECOVERABLE(stream.error))
			{
				// Prevent ID3 tags from reporting recoverable frame errors
				const uint8_t	*buffer			= stream.this_frame;
				unsigned		buflen			= stream.bufend - stream.this_frame;
				
				if(10 <= buflen && 0x49 == buffer[0] && 0x44 == buffer[1] && 0x33 == buffer[2]) {
					id3_length = (((buffer[6] & 0x7F) << (3 * 7)) | ((buffer[7] & 0x7F) << (2 * 7)) |
								  ((buffer[8] & 0x7F) << (1 * 7)) | ((buffer[9] & 0x7F) << (0 * 7)));
					
					// Add 10 bytes for ID3 header
					id3_length += 10;
					
					mad_stream_skip(&stream, id3_length);
				}
				
				continue;
			}
			else if (stream.error == MAD_ERROR_BUFLEN && inputEOF)
			{
				break;
			}
			else if (stream.error == MAD_ERROR_BUFLEN)
			{
				continue;
			}
			else
			{
				//NSLog(@"Unrecoverable error: %s", mad_stream_errorstr(&stream));
				break;
			}
		}
		framesDecoded++;
		
		if (framesDecoded == 1)
		{
			frequency = frame.header.samplerate;
			channels = MAD_NCHANNELS(&frame.header);
			
			if(MAD_FLAG_LSF_EXT & frame.header.flags || MAD_FLAG_MPEG_2_5_EXT & frame.header.flags) {
				switch(frame.header.layer) {
					case MAD_LAYER_I:		samplesPerMPEGFrame = 384;			break;
					case MAD_LAYER_II:		samplesPerMPEGFrame = 1152;		break;
					case MAD_LAYER_III:		samplesPerMPEGFrame = 576;			break;
				}
			}
			else {
				switch(frame.header.layer) {
					case MAD_LAYER_I:		samplesPerMPEGFrame = 384;			break;
					case MAD_LAYER_II:		samplesPerMPEGFrame = 1152;		break;
					case MAD_LAYER_III:		samplesPerMPEGFrame = 1152;		break;
				}
			}
			
			unsigned ancillaryBitsRemaining = stream.anc_bitlen;
			
			if(32 > ancillaryBitsRemaining)
				continue;
			
			uint32_t magic = mad_bit_read(&stream.anc_ptr, 32);
			ancillaryBitsRemaining -= 32;
			
			if('Xing' == magic || 'Info' == magic) {
				unsigned	i;
				uint32_t	flags = 0, frames = 0, bytes = 0, vbrScale = 0;
				
				if(32 > ancillaryBitsRemaining)
					continue;
				
				flags = mad_bit_read(&stream.anc_ptr, 32);
				ancillaryBitsRemaining -= 32;
				
				// 4 byte value containing total frames
				if(FRAMES_FLAG & flags) {
					if(32 > ancillaryBitsRemaining)
						continue;
					
					frames = mad_bit_read(&stream.anc_ptr, 32);
					ancillaryBitsRemaining -= 32;
					
					// Determine number of samples, discounting encoder delay and padding
					// Our concept of a frame is the same as CoreAudio's- one sample across all channels
					_totalSamples = frames * samplesPerMPEGFrame;
					//NSLog(@"TOTAL READ FROM XING");
				}
				
				// 4 byte value containing total bytes
				if(BYTES_FLAG & flags) {
					if(32 > ancillaryBitsRemaining)
						continue;
					
					bytes = mad_bit_read(&stream.anc_ptr, 32);
					ancillaryBitsRemaining -= 32;
				}
				
				// 100 bytes containing TOC information
				if(TOC_FLAG & flags) {
					if(8 * 100 > ancillaryBitsRemaining)
						continue;
					
					for(i = 0; i < 100; ++i)
						/*_xingTOC[i] = */ mad_bit_read(&stream.anc_ptr, 8);
					
					ancillaryBitsRemaining -= (8* 100);
				}
				
				// 4 byte value indicating encoded vbr scale
				if(VBR_SCALE_FLAG & flags) {
					if(32 > ancillaryBitsRemaining)
						continue;
					
					vbrScale = mad_bit_read(&stream.anc_ptr, 32);
					ancillaryBitsRemaining -= 32;
				}
				
				framesDecoded	= frames;
				
				_foundXingHeader = YES;
				
				// Loook for the LAME header next
				// http://gabriel.mp3-tech.org/mp3infotag.html				
				if(32 > ancillaryBitsRemaining)
					continue;
				magic = mad_bit_read(&stream.anc_ptr, 32);
				
				ancillaryBitsRemaining -= 32;
				
				if('LAME' == magic) {
					
					if(LAME_HEADER_SIZE > ancillaryBitsRemaining)
						continue;
					
					/*unsigned char versionString [5 + 1];
					memset(versionString, 0, 6);*/
					
					for(i = 0; i < 5; ++i)
						/*versionString[i] =*/ mad_bit_read(&stream.anc_ptr, 8);
					
					/*uint8_t infoTagRevision =*/ mad_bit_read(&stream.anc_ptr, 4);
					/*uint8_t vbrMethod =*/ mad_bit_read(&stream.anc_ptr, 4);
					
					/*uint8_t lowpassFilterValue =*/ mad_bit_read(&stream.anc_ptr, 8);
					
					/*float peakSignalAmplitude =*/ mad_bit_read(&stream.anc_ptr, 32);
					/*uint16_t radioReplayGain =*/ mad_bit_read(&stream.anc_ptr, 16);
					/*uint16_t audiophileReplayGain =*/ mad_bit_read(&stream.anc_ptr, 16);
					
					/*uint8_t encodingFlags =*/ mad_bit_read(&stream.anc_ptr, 4);
					/*uint8_t athType =*/ mad_bit_read(&stream.anc_ptr, 4);
					
					/*uint8_t lameBitrate =*/ mad_bit_read(&stream.anc_ptr, 8);
					
					_startPadding = mad_bit_read(&stream.anc_ptr, 12);
					_endPadding = mad_bit_read(&stream.anc_ptr, 12);
					
					_startPadding += 528 + 1; //MDCT/filterbank delay
					_totalSamples += 528 + 1;
					
					/*uint8_t misc =*/ mad_bit_read(&stream.anc_ptr, 8);
					
					/*uint8_t mp3Gain =*/ mad_bit_read(&stream.anc_ptr, 8);
					
					/*uint8_t unused =*/mad_bit_read(&stream.anc_ptr, 2);
					/*uint8_t surroundInfo =*/ mad_bit_read(&stream.anc_ptr, 3);
					/*uint16_t presetInfo =*/ mad_bit_read(&stream.anc_ptr, 11);
					
					/*uint32_t musicGain =*/ mad_bit_read(&stream.anc_ptr, 32);
					
					/*uint32_t musicCRC =*/ mad_bit_read(&stream.anc_ptr, 32);
					
					/*uint32_t tagCRC =*/ mad_bit_read(&stream.anc_ptr, 32);
					
					ancillaryBitsRemaining -= LAME_HEADER_SIZE;
					
					_foundLAMEHeader = YES;
					break;
				}
			}
		}
		else
		{
			_totalSamples = (double)frame.header.samplerate * ((_fileSize - id3_length) / (frame.header.bitrate / 8.0));
			//NSLog(@"Guestimating total samples");
			
			break;
		}
	}
	
	//Need to make sure this is correct
	bitrate = (_fileSize - id3_length) / (length * 1000.0);
	
	mad_frame_finish (&frame);
	mad_stream_finish (&stream);
	
	//Need to fix this too.
	length = 1000.0 * (_totalSamples / frequency);
	
	bitsPerSample = 16;
	
	[_source seek:0 whence:SEEK_SET];
	
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;	
}


- (BOOL)open:(id<CogSource>)source
{	
	[source retain];
	[_source release];
	_source = source;
	
	/* First the structures used by libmad must be initialized. */
	mad_stream_init(&_stream);
	mad_frame_init(&_frame);
	mad_synth_init(&_synth);
	
	_firstFrame = YES;
	//NSLog(@"OPEN: %i", _firstFrame);
	
	if ([_source seekable]) {
		return [self scanFile];
	}
	
	return YES;
}


/**
* Scale PCM data
 */
static inline signed int scale (mad_fixed_t sample)
{
	BOOL hard_limit = YES;
	
	double scale = 1.0;
	
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
	unsigned int startingSample = 0;
	unsigned int sampleCount = _synth.pcm.length;
	
	//NSLog(@"Position: %li/%li", _samplesDecoded, _totalSamples);
	//NSLog(@"<%i, %i>", _startPadding, _endPadding);

	//NSLog(@"Counts: %i, %i", startingSample, sampleCount);
	if (_foundLAMEHeader) {
		if (_samplesDecoded < _startPadding) {
			//NSLog(@"Skipping start.");
			startingSample = _startPadding - _samplesDecoded;
		}
		
		if (_samplesDecoded > _totalSamples - _endPadding + startingSample) {
			//NSLog(@"End of file. Not writing.");
			return;
		}

		if (_samplesDecoded + (sampleCount - startingSample) > _totalSamples - _endPadding) {
			sampleCount = _totalSamples - _endPadding - _samplesDecoded + startingSample;
			//NSLog(@"End of file. %li",  _totalSamples - _endPadding - _samplesDecoded);
		}

		if (startingSample > sampleCount) {
			_samplesDecoded += _synth.pcm.length;
			//NSLog(@"Skipping entire sample");
			return;
		}
		
	}
	
	//NSLog(@"Revised: %i, %i", startingSample, sampleCount);
	
	_samplesDecoded += _synth.pcm.length;
	
	_outputAvailable = (sampleCount - startingSample) * channels * (bitsPerSample/8);
	
	if (_outputBuffer)
		free(_outputBuffer);
	
	_outputBuffer = (unsigned char *) malloc (_outputAvailable * sizeof (char));
	
	unsigned int i, j; 
	unsigned int stride = channels * 2; 
	for (j = 0; j < channels; j++) 
	{ 
		/* output sample(s) in 16-bit signed little-endian PCM */  
		mad_fixed_t const *channel = _synth.pcm.samples[j]; 
		unsigned char *outputPtr = _outputBuffer + (j * 2); 
		
		for (i = startingSample; i < sampleCount; i++) 
		{  
			signed short sample = scale(channel[i]); 
			
			outputPtr[0] = sample>>8;  
			outputPtr[1] = sample & 0xff;  
			outputPtr += stride; 
		} 
	} 
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int inputToRead;
	int inputRemaining;
	BOOL inputEOF = NO;
	
	int bytesRead = 0;
	
	for (;;)
	{
		int bytesRemaining = size - bytesRead;
		int bytesToCopy = (_outputAvailable > bytesRemaining ? bytesRemaining : _outputAvailable);
		
		memcpy(buf + bytesRead, _outputBuffer, bytesToCopy);
		bytesRead += bytesToCopy;
		
		if (bytesToCopy != _outputAvailable) {
			memmove(_outputBuffer, _outputBuffer + bytesToCopy, _outputAvailable - bytesToCopy);
		}
		
		_outputAvailable -= bytesToCopy;
		
		if (bytesRead == size)
			break;
		
		if (_stream.buffer == NULL || _stream.error == MAD_ERROR_BUFLEN)
		{
			if (_stream.next_frame != NULL)
			{
				inputRemaining = _stream.bufend - _stream.next_frame;
				
				memmove(_inputBuffer, _stream.next_frame, inputRemaining);
				
				inputToRead = INPUT_BUFFER_SIZE - inputRemaining;
			}
			else
			{
				inputToRead = INPUT_BUFFER_SIZE;
				inputRemaining = 0;
			}
			
			int inputRead = [_source read:_inputBuffer + inputRemaining amount:INPUT_BUFFER_SIZE - inputRemaining];
			if (inputRead == 0)
			{
				memset(_inputBuffer + inputRemaining + inputRead, 0, MAD_BUFFER_GUARD);
				inputRead += MAD_BUFFER_GUARD;
				inputEOF = YES;
			}
			
			mad_stream_buffer(&_stream, _inputBuffer, inputRead + inputRemaining);
			_stream.error = MAD_ERROR_NONE;
			//NSLog(@"Read stream.");
		}
		
		if (mad_frame_decode(&_frame, &_stream) == -1) {
			if (MAD_RECOVERABLE (_stream.error))
			{
				const uint8_t	*buffer			= _stream.this_frame;
				unsigned		buflen			= _stream.bufend - _stream.this_frame;
				uint32_t		id3_length		= 0;
				
				//No longer need ID3Tag framework
				if(10 <= buflen && 0x49 == buffer[0] && 0x44 == buffer[1] && 0x33 == buffer[2]) {
					id3_length = (((buffer[6] & 0x7F) << (3 * 7)) | ((buffer[7] & 0x7F) << (2 * 7)) |
								  ((buffer[8] & 0x7F) << (1 * 7)) | ((buffer[9] & 0x7F) << (0 * 7)));
					
					// Add 10 bytes for ID3 header
					id3_length += 10;
					
					mad_stream_skip(&_stream, id3_length);
				}
				
				//NSLog(@"recoverable error");
				continue;
			}
			else if (MAD_ERROR_BUFLEN == _stream.error && inputEOF)
			{
				//NSLog(@"EOF");
				break;
			}
			else if (MAD_ERROR_BUFLEN == _stream.error)
			{
				//NSLog(@"Bufferlen");
				continue;
			}
			else
			{
				//NSLog(@"Unrecoverable stream error: %s", mad_stream_errorstr(&_stream));
				break;
			}
		}
		
		//NSLog(@"Decoded buffer.");
		mad_synth_frame (&_synth, &_frame);
		//NSLog(@"first frame: %i", _firstFrame);
		if (_firstFrame)
		{
			_firstFrame = NO;
			
			if (![_source seekable]) {
				frequency = _frame.header.samplerate;
				channels = MAD_NCHANNELS(&_frame.header);
				bitsPerSample = 16;
				
				[self willChangeValueForKey:@"properties"];
				[self didChangeValueForKey:@"properties"];
			}
			//NSLog(@"FIRST FRAME!!! %i %i", _foundXingHeader, _foundLAMEHeader);
			if (_foundXingHeader) {
				//NSLog(@"Skipping xing header.");
				continue;
			}
		}
		
		[self writeOutput];
		//NSLog(@"Wrote output");
	}
	
	//NSLog(@"Read: %i/%i", bytesRead, size);
	return bytesRead;
}

- (void)close
{
	if (_source)
	{
		[_source close];
		[_source release];
		_source = nil;
	}
	
	if (_outputBuffer)
	{
		free(_outputBuffer);
		_outputBuffer = NULL;
	}
	
	mad_synth_finish(&_synth);
	mad_frame_finish(&_frame);
	mad_stream_finish(&_stream);	
}

- (double)seekToTime:(double)milliseconds
{
	if (milliseconds > length)
		milliseconds = length;
	
	unsigned long new_position = (milliseconds / length) * _fileSize;
	[_source seek:new_position whence:SEEK_SET];
	
	mad_stream_buffer(&_stream, NULL, 0);
	
	//Gapless busted after seek. Mp3 just doesn't have sample-accurate seeking. Maybe xing toc?
	_samplesDecoded = (milliseconds / length) * _totalSamples;
	
	return milliseconds;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithInt:bitrate],@"bitrate",
		[NSNumber numberWithDouble:length],@"length",
		[NSNumber numberWithBool:[_source seekable]], @"seekable",
		@"big", @"endian",
		nil];
}


+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"mp3",nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/mpeg", @"audio/x-mp3", nil];
}

@end

