//
//  MADFile.m
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "MADDecoder.h"

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
			sampleRate = frame.header.samplerate;
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
					totalFrames = frames * samplesPerMPEGFrame;
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
					totalFrames += 528 + 1;
					
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
			totalFrames = (double)frame.header.samplerate * ((_fileSize - id3_length) / (frame.header.bitrate / 8.0));
			//NSLog(@"Guestimating total samples");
			
			break;
		}
	}
	
	bitrate = ((double)((_fileSize - id3_length)*8)/1000.0) * (sampleRate/(double)totalFrames);
	
	mad_frame_finish (&frame);
	mad_stream_finish (&stream);
	
	bitsPerSample = 16;
	
	bytesPerFrame = (bitsPerSample/8) * channels;
	
	[_source seek:0 whence:SEEK_SET];
	inputEOF = NO;
	
	NSLog(@"Mad properties: %@", [self properties]);
	
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

	inputEOF = NO;
	
	if (![_source seekable])
	{
		//Decode the first frame to get the channels, samplerate, etc.
		int r;
		do {
			r = [self decodeMPEGFrame];
			NSLog(@"Decoding first frame: %i", r);
		} while (r == 0);
		
		return (r == -1 ? NO : YES);
	}

	return [self scanFile];
}


// Clipping and rounding code from madplay(audio.c):
/*
 * madplay - MPEG audio decoder and player
 * Copyright (C) 2000-2004 Robert Leslie
 */
static int32_t 
audio_linear_round(unsigned int bits, 
				   mad_fixed_t sample)
{
	enum {
		MIN = -MAD_F_ONE,
		MAX =  MAD_F_ONE - 1
	};
	
	/* round */
	sample += (1L << (MAD_F_FRACBITS - bits));
	
	/* clip */
	if(MAX < sample)
		sample = MAX;
	else if(MIN > sample)
		sample = MIN;
	
	/* quantize and scale */
	return sample >> (MAD_F_FRACBITS + 1 - bits);
}
// End madplay code


- (void)writeOutput
{
	unsigned int startingSample = 0;
	unsigned int sampleCount = _synth.pcm.length;
	
	//NSLog(@"Position: %li/%li", _framesDecoded, totalFrames);
	//NSLog(@"<%i, %i>", _startPadding, _endPadding);

	//NSLog(@"Counts: %i, %i", startingSample, sampleCount);
	if (_foundLAMEHeader) {
		if (_framesDecoded < _startPadding) {
			//NSLog(@"Skipping start.");
			startingSample = _startPadding - _framesDecoded;
		}
		
		if (_framesDecoded > totalFrames - _endPadding + startingSample) {
			//NSLog(@"End of file. Not writing.");
			return;
		}

		if (_framesDecoded + (sampleCount - startingSample) > totalFrames - _endPadding) {
			sampleCount = totalFrames - _endPadding - _framesDecoded + startingSample;
			//NSLog(@"End of file. %li",  totalFrames - _endPadding - _framesDecoded);
		}

		if (startingSample > sampleCount) {
			_framesDecoded += _synth.pcm.length;
			//NSLog(@"Skipping entire sample");
			return;
		}
		
	}
	
	//NSLog(@"Revised: %i, %i", startingSample, sampleCount);
	
	_framesDecoded += _synth.pcm.length;
	
	_outputFrames = (sampleCount - startingSample);
	
	if (_outputBuffer)
		free(_outputBuffer);
	
	_outputBuffer = (unsigned char *) malloc (_outputFrames * bytesPerFrame * sizeof (char));
	
	unsigned int i, j; 
	unsigned int stride = channels * 2; 
	for (j = 0; j < channels; j++) 
	{ 
		/* output sample(s) in 16-bit signed little-endian PCM */  
		mad_fixed_t const *channel = _synth.pcm.samples[j]; 
		unsigned char *outputPtr = _outputBuffer + (j * 2); 
		
		for (i = startingSample; i < sampleCount; i++) 
		{  
			signed short sample = audio_linear_round(bitsPerSample, channel[i]); 
			
			outputPtr[0] = sample>>8;  
			outputPtr[1] = sample & 0xff;  
			outputPtr += stride; 
		} 
	} 
}

- (int)decodeMPEGFrame
{
	if (_stream.buffer == NULL || _stream.error == MAD_ERROR_BUFLEN)
	{
		int inputToRead;
		int inputRemaining;

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

			NSLog(@"recoverable error");
			return 0;
		}
		else if (MAD_ERROR_BUFLEN == _stream.error && inputEOF)
		{
			NSLog(@"EOF");
			return -1;
		}
		else if (MAD_ERROR_BUFLEN == _stream.error)
		{
			//NSLog(@"Bufferlen");
			return 0;
		}
		else
		{
			//NSLog(@"Unrecoverable stream error: %s", mad_stream_errorstr(&_stream));
			return -1;
		}
	}

	//NSLog(@"Decoded buffer.");
	mad_synth_frame (&_synth, &_frame);
	//NSLog(@"first frame: %i", _firstFrame);
	if (_firstFrame)
	{
		_firstFrame = NO;

		if (![_source seekable]) {
			sampleRate = _frame.header.samplerate;
			channels = MAD_NCHANNELS(&_frame.header);
			bitsPerSample = 16;
			bytesPerFrame = (bitsPerSample/8) * channels;

			[self willChangeValueForKey:@"properties"];
			[self didChangeValueForKey:@"properties"];
		}
		//NSLog(@"FIRST FRAME!!! %i %i", _foundXingHeader, _foundLAMEHeader);
		if (_foundXingHeader) {
			//NSLog(@"Skipping xing header.");
			return 0;
		}
	}

	return 1;
}

- (int)readAudio:(void *)buffer frames:(UInt32)frames
{
	int framesRead = 0;
	
	for (;;)
	{
		int framesRemaining = frames - framesRead;
		int framesToCopy = (_outputFrames > framesRemaining ? framesRemaining : _outputFrames);
		
		if (framesToCopy) {
			memcpy(buffer + (framesRead * bytesPerFrame), _outputBuffer, framesToCopy*bytesPerFrame);
			framesRead += framesToCopy;
		
			if (framesToCopy != _outputFrames) {
				memmove(_outputBuffer, _outputBuffer + (framesToCopy*bytesPerFrame), (_outputFrames - framesToCopy)*bytesPerFrame);
			}

			_outputFrames -= framesToCopy;
		}
		
		if (framesRead == frames)
			break;
		
		int r = [self decodeMPEGFrame];
		//NSLog(@"Decoding frame: %i", r);
		if (r == 0) //Recoverable error.
			continue;
		else if (r == -1) //Unrecoverable error
			break;
		
		[self writeOutput];
		//NSLog(@"Wrote output");
	}
	
	//NSLog(@"Read: %i/%i", bytesRead, size);
	return framesRead;
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

- (long)seek:(long)frame
{
	if (frame > totalFrames)
		frame = totalFrames;
	
	unsigned long new_position = ((double)frame / totalFrames) * _fileSize;
	[_source seek:new_position whence:SEEK_SET];
	
	mad_stream_buffer(&_stream, NULL, 0);
	
	//Gapless busted after seek. Mp3 just doesn't have sample-accurate seeking. Maybe xing toc?
	_framesDecoded = frame;
	
	return frame;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithFloat:sampleRate],@"sampleRate",
		[NSNumber numberWithInt:bitrate],@"bitrate",
		[NSNumber numberWithLong:totalFrames],@"totalFrames",
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

