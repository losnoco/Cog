//
//  MusepackFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/23/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "MusepackDecoder.h"

#import "Logging.h"

@implementation MusepackDecoder

mpc_int32_t ReadProc(void *data, void *ptr, mpc_int32_t size)
{
    MusepackDecoder *decoder = (MusepackDecoder *) data;
	
	return [[decoder source] read:ptr amount:size];
}

mpc_bool_t SeekProc(void *data, mpc_int32_t offset)
{
    MusepackDecoder *decoder = (MusepackDecoder *) data;

    return [[decoder source] seek:offset whence:SEEK_SET];
}

mpc_int32_t TellProc(void *data)
{
    MusepackDecoder *decoder = (MusepackDecoder *) data;
	
    return [[decoder source] tell];
}

mpc_int32_t GetSizeProc(void *data)
{
    MusepackDecoder *decoder = (MusepackDecoder *) data;
	
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

mpc_bool_t CanSeekProc(void *data)
{
    MusepackDecoder *decoder = (MusepackDecoder *) data;
	
	return [[decoder source] seekable];
}

- (BOOL)open:(id<CogSource>)s
{
	[self setSource: s];
	
	reader.read = ReadProc;
	reader.seek = SeekProc;
	reader.tell = TellProc;
	reader.get_size = GetSizeProc;
	reader.canseek = CanSeekProc;
	reader.data = self;
	
    mpc_streaminfo_init(&info);
    if (mpc_streaminfo_read(&info, &reader) != ERROR_CODE_OK)
	{
        DLog(@"Not a valid musepack file.");
        return NO;
    }
	
	/* instantiate a decoder with our reader */
	mpc_decoder_setup(&decoder, &reader);
	if (!mpc_decoder_initialize(&decoder, &info))
	{
		DLog(@"Error initializing decoder.");
		return NO;
	}
	

	bitrate = (int)(info.average_bitrate/1000.0);
	frequency = info.sample_freq;
		
	totalFrames = mpc_streaminfo_get_length_samples(&info);	

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}

- (BOOL)writeToBuffer:(float *)sample_buffer fromBuffer:(const MPC_SAMPLE_FORMAT *)p_buffer frames:(unsigned)frames
{
	unsigned n;
		
	unsigned p_size = frames * 2; //2 = stereo
#ifdef MPC_FIXED_POINT
    const float float_scale = 1.0 / MPC_FIXED_POINT_SCALE;
#endif
    
	for (n = 0; n < p_size; n++)
	{
		float val;
#ifdef MPC_FIXED_POINT
		val = p_buffer[n] * float_scale;
#else
		val = p_buffer[n];
#endif
		
//		sample_buffer[n] = CFSwapInt16LittleToHost(val);
		sample_buffer[n] = val;
	}

//	m_data_bytes_written += p_size * (m_bps >> 3);
	return YES;
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    MPC_SAMPLE_FORMAT sampleBuffer[MPC_DECODER_BUFFER_LENGTH];

	int framesRead = 0;
	int bytesPerFrame = sizeof(float) * 2; //bitsPerSample == 16, channels == 2
	while (framesRead < frames)
	{	
		//Fill from buffer, going by bufferFrames
		//if still needs more, decode and repeat
		if (bufferFrames == 0)
		{
			/* returns the length of the samples*/
			unsigned status = mpc_decoder_decode(&decoder, sampleBuffer, 0, 0);
			if (status == (unsigned)( -1))
			{
				//decode error
				DLog(@"Decode error");
				return 0;
			}
			else if (status == 0) //EOF
			{
				return 0;
			}
			else //status>0 /* status == MPC_FRAME_LENGTH */
			{
			}
		
			bufferFrames = status;
		}

		int framesToRead = bufferFrames;
		if (bufferFrames > frames)
		{
			framesToRead = frames;
		}

		[self writeToBuffer:((float*)(buf + (framesRead*bytesPerFrame)))  fromBuffer:sampleBuffer frames: bufferFrames];

		frames -= framesToRead;
		framesRead += framesToRead;
		bufferFrames -= framesToRead;
		
		if (bufferFrames > 0)
		{
			memmove((uint8_t *)sampleBuffer, ((uint8_t *)sampleBuffer) + (framesToRead * bytesPerFrame), bufferFrames * bytesPerFrame);
		}
	}

	return framesRead;
}

- (void)close
{
	[source close];
}

- (long)seek:(long)sample
{
	mpc_decoder_seek_sample(&decoder, sample);
	
	return sample;
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

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:bitrate], @"bitrate",
		[NSNumber numberWithFloat:frequency], @"sampleRate",
		[NSNumber numberWithDouble:totalFrames], @"totalFrames",
		[NSNumber numberWithInt:32], @"bitsPerSample",
        [NSNumber numberWithBool:YES], @"floatingPoint",
		[NSNumber numberWithInt:2], @"channels",
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host",@"endian",
		nil];
}



+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"mpc"];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-musepack", nil];
}

@end
