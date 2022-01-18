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

mpc_int32_t ReadProc(mpc_reader *p_reader, void *ptr, mpc_int32_t size)
{
    MusepackDecoder *decoder = (__bridge MusepackDecoder *) p_reader->data;
	
	return (mpc_int32_t)[[decoder source] read:ptr amount:size];
}

mpc_bool_t SeekProc(mpc_reader *p_reader, mpc_int32_t offset)
{
    MusepackDecoder *decoder = (__bridge MusepackDecoder *) p_reader->data;

    return [[decoder source] seek:offset whence:SEEK_SET];
}

mpc_int32_t TellProc(mpc_reader *p_reader)
{
    MusepackDecoder *decoder = (__bridge MusepackDecoder *) p_reader->data;
	
    return (mpc_int32_t)[[decoder source] tell];
}

mpc_int32_t GetSizeProc(mpc_reader *p_reader)
{
    MusepackDecoder *decoder = (__bridge MusepackDecoder *) p_reader->data;
	
	if ([[decoder source] seekable]) {
		long currentPos = [[decoder source] tell];
		
		[[decoder source] seek:0 whence:SEEK_END];
		long size = [[decoder source] tell];
		
		[[decoder source] seek:currentPos whence:SEEK_SET];
        
        if (size > INT32_MAX)
            size = INT32_MAX;
		
		return (mpc_int32_t)size;
	}
	else {
		return 0;
	}
}

mpc_bool_t CanSeekProc(mpc_reader *p_reader)
{
    MusepackDecoder *decoder = (__bridge MusepackDecoder *) p_reader->data;
	
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
	reader.data = (__bridge void *)(self);
	
	/* instantiate a demuxer with our reader */
    demux = mpc_demux_init(&reader);
    if (!demux)
	{
		DLog(@"Error initializing decoder.");
		return NO;
	}
    mpc_demux_get_info(demux, &info);

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
            mpc_frame_info frame;
            frame.buffer = sampleBuffer;
            mpc_status err = mpc_demux_decode(demux, &frame);
			if (frame.bits == -1)
			{
				//decode error
                if (err != MPC_STATUS_OK)
                    DLog(@"Decode error");
				return 0;
			}
			else //status>0 /* status == MPC_FRAME_LENGTH */
			{
			}
		
			bufferFrames = frame.samples;
		}

		int framesToRead = bufferFrames;
		if (bufferFrames > frames)
		{
			framesToRead = frames;
		}

		[self writeToBuffer:((float*)(buf + (framesRead	*bytesPerFrame)))  fromBuffer:sampleBuffer frames: framesToRead];

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
    if (demux)
    {
        mpc_demux_exit(demux);
        demux = NULL;
    }
}

- (void)dealloc
{
    [self close];
}

- (long)seek:(long)sample
{
	mpc_demux_seek_sample(demux, sample);
	
	return sample;
}

- (void)setSource:(id<CogSource>)s
{
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
        @"Musepack", @"codec",
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

+ (float)priority
{
    return 1.0;
}

+ (NSArray *)fileTypeAssociations
{
    return @[
        @[@"Musepack Audio Files", @"mpc.icns", @"mpc"]
    ];
}

@end
