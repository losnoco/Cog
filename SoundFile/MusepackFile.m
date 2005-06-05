//
//  MusepackFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/23/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "MusepackFile.h"

@implementation MusepackFile

//callbacks
/*
mpc_int32_t ReadProc(void *data, void *ptr, mpc_int32_t size)
{
	DBLog(@"Reading: %i", size);
    MusepackFile *f = (MusepackFile *) data;
	
	return fread(ptr, 1, size, [f inFd]);
}

BOOL SeekProc(void *data, mpc_int32_t offset)
{
    MusepackFile *f = (MusepackFile *) data;
//	DBLog(@"Seeking: %i %i", offset, f->reader.is_seekable);

    return !fseek([f inFd], offset, SEEK_SET);
}

mpc_int32_t TellProc(void *data)
{
	DBLog(@"Tell");
    MusepackFile *f = (MusepackFile *) data;
    return ftell([f inFd]);
}

mpc_int32_t GetSizeProc(void *data)
{
	DBLog(@"Size");

    MusepackFile *f = (MusepackFile *) data;
    return [f undecodedSize];
}

BOOL CanSeekProc(void *data)
{
	DBLog(@"Can seek");

    MusepackFile *f = (MusepackFile *) data;
    return YES;
}
*/
//real ish
- (BOOL)open:(const char *)filename
{
	[self readInfo:filename];
	
    /* instantiate a decoder with our file reader */
    mpc_decoder_setup(&decoder, &reader);
    if (!mpc_decoder_initialize(&decoder, &info))
	{
        DBLog(@"Error initializing decoder.");
        return NO;
    }
//	DBLog(@"Ok to go...");

	isBigEndian = YES;
	
	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	inFd = fopen(filename, "rb");
	if (inFd == 0)
		return NO;
	
	mpc_reader_setup_file_reader(&reader , inFd);

    mpc_streaminfo_init(&info);
    if (mpc_streaminfo_read(&info, &reader) != ERROR_CODE_OK)
	{
        DBLog(@"Not a valid musepack file.");
        return NO;
    }

	bitRate = (int)(info.average_bitrate/1000.0);
	frequency = info.sample_freq;
	bitsPerSample = 16;
	channels = 2;
	
	totalSize = mpc_streaminfo_get_length_samples(&info)*channels*bitsPerSample/8;	
	
	return YES;
}

- (BOOL)writeSamplesToBuffer:(uint16_t *)sample_buffer fromBuffer:(const MPC_SAMPLE_FORMAT *)p_buffer ofSize:(unsigned)p_size
{
    unsigned n;
    int m_bps = 16;
    int clip_min = - 1 << (m_bps - 1),
		clip_max = (1 << (m_bps - 1)) - 1,
		float_scale = 1 << (m_bps - 1);

	for (n = 0; n < p_size; n++)
	{
		int val;
#ifdef MPC_FIXED_POINT
		val = shift_signed( p_buffer[n], m_bps - MPC_FIXED_POINT_SCALE_SHIFT );			
#else
		val = (int)( p_buffer[n] * float_scale );
#endif
		
		if (val < clip_min)
			val = clip_min;
		else if (val > clip_max)
			val = clip_max;

//		sample_buffer[n] = CFSwapInt16LittleToHost(val);
		sample_buffer[n] = val;
    }
	
//	DBLog(@"Samples written.");
//    m_data_bytes_written += p_size * (m_bps >> 3);
    return YES;
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numread = bufferAmount;
	int count = 0;
    MPC_SAMPLE_FORMAT sampleBuffer[MPC_DECODER_BUFFER_LENGTH];
//	DBLog(@"Fill buffer: %i", size);
	//Fill from buffer, going by bufferAmount
	//if still needs more, decode and repeat
	if (bufferAmount == 0)
	{
		/* returns the length of the samples*/

		unsigned status = mpc_decoder_decode(&decoder, sampleBuffer, 0, 0);
		if (status == (unsigned)( -1))
		{
			//decode error
			DBLog(@"Decode error");
			return 0;
		}
		else if (status == 0) //EOF
		{
//			DBLog(@"AHHHHHH EOF");
			return 0;
		}
		else //status>0 /* status == MPC_FRAME_LENGTH */
		{
			[self writeSamplesToBuffer:((uint16_t*)buffer)  fromBuffer:sampleBuffer ofSize:(status*2)];
		}
		
		bufferAmount = status*4;
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
	fclose(inFd);
}

- (double)seekToTime:(double)milliseconds
{
	BOOL r;
//	double n = milliseconds;
//	DBLog(@"Milliseconds: %f", n);
//	DBLog(@"SEEKING TO: %f", (double)milliseconds/1000.0);
	
	r = mpc_decoder_seek_sample(&decoder, frequency*((double)milliseconds/1000.0));
//	DBLog(@"SEEK RESULT: %i", r);
	
	return milliseconds;
}

//accessors
/*
- (FILE *)inFd
{
	return inFd;
}

- (int)undecodedSize
{
	return undecodedSize;
}
*/
@end
