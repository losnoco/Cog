//
//  WavPackFile.m
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "WavPackDecoder.h"


@implementation WavPackDecoder

int32_t ReadBytesProc(void *ds, void *data, int32_t bcount)
{
	WavPackDecoder *decoder = (WavPackDecoder *)ds;
	
	return [[decoder source] read:data amount:bcount];
}

uint32_t GetPosProc(void *ds)
{
	WavPackDecoder *decoder = (WavPackDecoder *)ds;
	
	return [[decoder source] tell];
}

int SetPosAbsProc(void *ds, uint32_t pos)
{
	WavPackDecoder *decoder = (WavPackDecoder *)ds;
	
	return ([[decoder source] seek:pos whence:SEEK_SET] ? 0: -1);
}

int SetPosRelProc(void *ds, int32_t delta, int mode)
{
	WavPackDecoder *decoder = (WavPackDecoder *)ds;

	return ([[decoder source] seek:delta whence:mode] ? 0: -1);
}

int PushBackByteProc(void *ds, int c)
{
	WavPackDecoder *decoder = (WavPackDecoder *)ds;

	if ([[decoder source] seekable]) {
		[[decoder source] seek:-1 whence:SEEK_CUR];
		
		return c;
	}
	else {
		return -1;
	}
}

uint32_t GetLengthProc(void *ds)
{
	WavPackDecoder *decoder = (WavPackDecoder *)ds;
	
	if ([[decoder source] seekable]) {
		long currentPos = [[decoder source] tell];
		
		[[decoder source] seek:0 whence:SEEK_END];
		long size = [[decoder source] tell];
		
		[[decoder source] seek:currentPos whence:SEEK_SET];

		return size;
	}
	else {
		return 0;
	}	
}

int CanSeekProc(void *ds)
{
	WavPackDecoder *decoder = (WavPackDecoder *)ds;
	
	return [[decoder source] seekable];
}

int32_t WriteBytesProc(void *ds, void *data, int32_t bcount)
{
	return -1;
}


- (BOOL)open:(id<CogSource>)s
{
	int open_flags = 0;
	char error[80];
	
	[self setSource:s];

	reader.read_bytes = ReadBytesProc;
	reader.get_pos = GetPosProc;
	reader.set_pos_abs = SetPosAbsProc;
	reader.set_pos_rel = SetPosRelProc;
	reader.push_back_byte = PushBackByteProc;
	reader.get_length = GetLengthProc;
	reader.can_seek = CanSeekProc;
	reader.write_bytes = WriteBytesProc;

	//No corrections file (WVC) support at the moment.
	wpc = WavpackOpenFileInputEx(&reader, self, NULL, error, open_flags, 0);
	if (!wpc) {
		NSLog(@"Unable to open file..");
		return NO;
	}
	
	channels = WavpackGetNumChannels(wpc);
	bitsPerSample = WavpackGetBitsPerSample(wpc);
	
	frequency = WavpackGetSampleRate(wpc);

	length = ((double)WavpackGetNumSamples(wpc) * 1000.0)/frequency;
	
	bitrate = (int)(WavpackGetAverageBitrate(wpc, TRUE)/1000.0);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}
/*
- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numsamples;
	int n;
	void *sampleBuf = malloc(size*2);
	
	numsamples = size/(bitsPerSample/8)/channels;
//	DBLog(@"NUM SAMPLES: %i %i", numsamples, size);
	n = WavpackUnpackSamples(wpc, sampleBuf, numsamples);
	
	int i;
	for (i = 0; i < n*channels; i++)
	{
		((UInt16 *)buf)[i] = ((UInt32 *)sampleBuf)[i];
	}
	
	n *= (bitsPerSample/8)*channels;
	
	free(sampleBuf);
	
	return n;
}
*/
- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	uint32_t			sample;
	int32_t				audioSample;
	uint32_t			samplesRead;
	int8_t				*alias8;
	int16_t				*alias16;
	int32_t				*alias32;

	int inputBufferLength = size / (bitsPerSample/8);
	int32_t *inputBuffer = malloc(inputBufferLength*sizeof(int32_t));
	
	// Wavpack uses "complete" samples (one sample across all channels), i.e. a Core Audio frame
	samplesRead	= WavpackUnpackSamples(wpc, inputBuffer, inputBufferLength/channels);
	
	// Handle floating point files
	// Perform hard clipping and convert to integers
	if(MODE_FLOAT & WavpackGetMode(wpc) && 127 == WavpackGetFloatNormExp(wpc)) {
		float f;
		alias32 = inputBuffer;
		for(sample = 0; sample < samplesRead * channels; ++sample) {
			f =  * ((float *) alias32);
			
			if(f > 1.0)		{ f = 1.0; }
			if(f < -1.0)	{ f = -1.0; }
			
			//				*alias32++ = (int32_t) (f * 2147483647.0);
			*alias32++ = (int32_t) (f * 32767.0);
		}
	}

	switch(bitsPerSample) {
		case 8:
			// No need for byte swapping
			alias8 = buf;
			for(sample = 0; sample < samplesRead * channels; ++sample) {
				*alias8++ = (int8_t)inputBuffer[sample];
			}
			break;
		case 16:
			// Convert to big endian byte order 
			alias16 = buf;
			for(sample = 0; sample < samplesRead*channels; ++sample) {
				*alias16++ = (int16_t)inputBuffer[sample];
			}
			break;
		case 24:
			// Convert to big endian byte order 
			alias8 = buf;
			for(sample = 0; sample < samplesRead * channels; ++sample) {
				audioSample	= inputBuffer[sample];
				*alias8++	= (int8_t)(audioSample >> 16);
				*alias8++	= (int8_t)(audioSample >> 8);
				*alias8++	= (int8_t)audioSample;
			}
			break;
		case 32:
			// Convert to big endian byte order 
			alias32 = buf;
			for(sample = 0; sample < samplesRead * channels; ++sample) {
				*alias32++ = inputBuffer[sample];
			}
			break;
		default:
			NSLog(@"Unsupported sample size..");
	}
	
	free(inputBuffer);
	
	return samplesRead * channels * (bitsPerSample/8);
}

- (double)seekToTime:(double)milliseconds
{
	int sample;
	sample = frequency*(milliseconds/1000.0);

	WavpackSeekSample(wpc, sample);
	
	return milliseconds;
}

- (void)close
{
	WavpackCloseFile(wpc);
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

- (BOOL)seekable
{
	return [source seekable];
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
		[NSNumber numberWithInt:bitrate],@"bitrate",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithDouble:length],@"length",
		@"host",@"endian",
		nil];
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"wv"];
}


@end
