/*
 *  $Id$
 *
 *  Copyright (C) 2006 Stephen F. Booth <me@sbooth.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#import "CoreAudioFile.h"

@interface CoreAudioFile (Private)
- (BOOL) readInfoFromAudioFile;
- (BOOL) setupConverter: (AudioStreamBasicDescription *)inputFormat;
@end

@implementation CoreAudioFile

OSStatus readFunc(void * inRefCon, SInt64 inPosition, ByteCount requestCount, void *buffer, ByteCount* actualCount)
{
	FILE *fd = ((FILE *)inRefCon);
	fseek(fd, inPosition, SEEK_SET);

	*actualCount = fread(buffer, 1, requestCount, fd);
	
	if (*actualCount <= 0)
		return -1000; //Error?

	return noErr;
}

SInt64 getSizeFunc(void *inRefCon)
{
	FILE *fd = ((FILE *)inRefCon);
	int curPos;
	int length;

	curPos = ftell(fd);
	
	fseek(fd, 0, SEEK_END);
	length = ftell(fd);
	
	fseek(fd, curPos, SEEK_SET);

	return length;
}

OSStatus setSizeFunc(void * inRefCon, SInt64 inSize)
{
	return -1000; //Not supported at the moment
}

OSStatus writeFunc(void * inRefCon, SInt64 inPosition, ByteCount requestCount, const void *buffer, ByteCount* actualCount)
{
	return -1000; //Not supported at the moment
}

OSStatus ACInputProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
	CoreAudioFile *caf = (CoreAudioFile *)inUserData;
	OSStatus err = noErr;
	UInt32 numBytes = 0;

	if (caf->_packetCount + *ioNumberDataPackets > caf->_totalPackets)
	{
		*ioNumberDataPackets = caf->_totalPackets - caf->_packetCount;
	}

	if (*ioNumberDataPackets <= 0)
	{
		ioData->mBuffers[0].mData = NULL;
		ioData->mBuffers[0].mDataByteSize = 0;
		
		return noErr;
	}

	if (caf->_convBuf)
	{
		free(caf->_convBuf);
		caf->_convBuf = NULL;
	}
	caf->_convBuf = malloc(*ioNumberDataPackets * caf->_maxPacketSize);
	
	SInt64 localPacketCount;
	
	[caf->_countLock lock];
	localPacketCount = caf->_packetCount;
	[caf->_countLock unlock];
	
	err	= AudioFileReadPackets(caf->_in, false, &numBytes, NULL, localPacketCount, ioNumberDataPackets, caf->_convBuf);
	if(err != noErr) {
		NSLog(@"Error reading AudioFile: %i", err);
		return 0;
	}
	
	[caf->_countLock lock]; //packetcount could have changed (user could have seeked) while reading
	if (localPacketCount == caf->_packetCount)
		caf->_packetCount += *ioNumberDataPackets;
	[caf->_countLock unlock];
	
	
	ioData->mBuffers[0].mData = caf->_convBuf;
	ioData->mBuffers[0].mDataByteSize = numBytes;
	
	return err;
}

- (id)init
{
	self = [super init];
	if (self)
	{
		_countLock = [[NSLock alloc] init];
		_packetCount = 0;
		_convBuf = NULL;
		_totalPackets = 0;
		_maxPacketSize = 0;
	}
	
	return self;
}

- (BOOL) open:(const char *)filename
{
	OSStatus		err;
	
	// Open the input file
	_inFd = fopen(filename, "r");
	if (!_inFd)
	{
		NSLog(@"Error operning file: %s", filename);
		return NO;
	}
	
	//Using callbacks with fopen, ftell, fseek, fclose, because the default pread hangs when accessing the same file from multiple threads.
	err = AudioFileOpenWithCallbacks(_inFd, readFunc, writeFunc, getSizeFunc, setSizeFunc, 0, &_in);
	if(noErr != err) {
		NSLog(@"Error opening AudioFile: %i", err);
		return NO;
	}
	
	// Read properties
	return [self readInfoFromAudioFile];
}

- (void) close
{
	int	err;
	
	err = fclose(_inFd);
	if(err != 0) {
		NSLog(@"Error closing AudioFile: %i", err);
	}
}

- (BOOL) readInfo:(const char *)filename
{
	OSStatus						err;
	BOOL							result;
	
	result = YES;

	_inFd = fopen(filename, "r");
	if (!_inFd)
	{
		NSLog(@"Error operning file: %s", filename);
		return NO;
	}

	err = AudioFileOpenWithCallbacks(_inFd, readFunc, writeFunc, getSizeFunc, setSizeFunc, 0, &_in);
	if(noErr != err) {
		NSLog(@"Error opening AudioFile: %i", err);
		return NO;
	}
	
	result = [self readInfoFromAudioFile];
	
	return result;
}

- (BOOL) readInfoFromAudioFile
{
	OSStatus						err;
	UInt32							size;
	AudioStreamBasicDescription		asbd;
	SInt64							totalBytes;

	// Get input file information
	size	= sizeof(asbd);
	err		= AudioFileGetProperty(_in, kAudioFilePropertyDataFormat, &size, &asbd);
	if(err != noErr) {
		[self close];
		return NO;
	}

	size	= sizeof(_totalPackets);
	err		= AudioFileGetProperty(_in, kAudioFilePropertyAudioDataPacketCount, &size, &_totalPackets);
	if(err != noErr) {
		[self close];
		return NO;
	}
	
	size	= sizeof(totalBytes);
	err		= AudioFileGetProperty(_in, kAudioFilePropertyAudioDataByteCount, &size, &totalBytes);
	if(err != noErr) {
		[self close];
		return NO;
	}
	
	bitRate = ((totalBytes*8)/((_totalPackets * asbd.mFramesPerPacket)/asbd.mSampleRate))/1000.0;
	// Set our properties
	bitsPerSample		= asbd.mBitsPerChannel;
	channels			= asbd.mChannelsPerFrame;
	frequency			= asbd.mSampleRate;

	_framesPerPacket = asbd.mFramesPerPacket;
	
	// mBitsPerChannel will only be set for lpcm formats
	if(0 == bitsPerSample) {
		bitsPerSample = 16;
	}
	
	totalSize  = _totalPackets  *  asbd.mFramesPerPacket  *channels * (bitsPerSample/8);

	isBigEndian = YES;
	isUnsigned = NO;
	
	return [self setupConverter:&asbd];
}

- (BOOL)setupConverter: (AudioStreamBasicDescription *)inputFormat
{
	OSStatus err;
	UInt32 size;
	AudioStreamBasicDescription		result;

	// Set output format
	
	bzero(&result, sizeof(AudioStreamBasicDescription));
	
	result.mFormatID			= kAudioFormatLinearPCM;
	result.mFormatFlags			= kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian;
	
	result.mSampleRate			= frequency;
	result.mChannelsPerFrame	= channels;
	result.mBitsPerChannel		= bitsPerSample;
	
	result.mBytesPerPacket		= channels * (bitsPerSample / 8);
	result.mFramesPerPacket		= 1;
	result.mBytesPerFrame		= channels * (bitsPerSample / 8);

	err = AudioConverterNew(inputFormat, &result, &_converter);
	if (err != noErr)
	{
		[self close];

		NSLog(@"Error creating converter");
		return NO;
	}
	
    err = AudioFileGetPropertyInfo(	_in, 
									kAudioFilePropertyMagicCookieData, 
									&size, 
									NULL); 
	
	if (err == noErr) //Some data can be read without magic cookies...
    {
        void *magicCookie = malloc (size);
		//Get Magic Cookie data from Audio File
		err = AudioFileGetProperty(_in, 
								   kAudioFilePropertyMagicCookieData, 
								   &size, 
								   magicCookie);       
		
		// Give the AudioConverter the magic cookie decompression params if there are any
		if (err == noErr)
		{
			err = AudioConverterSetProperty(	_converter, 
												kAudioConverterDecompressionMagicCookie, 
												size, 
												magicCookie);
		}

		free(magicCookie);
    }
	
	size = sizeof(_maxPacketSize);
    err = AudioFileGetProperty(	_in,
								kAudioFilePropertyMaximumPacketSize, 
								&size, 
								&_maxPacketSize);
	if(err != noErr) {
		err = AudioFileClose(_in);
		NSLog(@"Error getting maximum packet size: %i", err);
		return NO;
	}
	
	return YES;
}

- (int) fillBuffer:(void *)buf ofSize:(UInt32)size
{
	OSStatus						err;
	UInt32							frameCount;
	AudioBufferList ioData;
	
	ioData.mNumberBuffers = 1;
	ioData.mBuffers[0].mData			= buf;
	ioData.mBuffers[0].mDataByteSize	= size;

	frameCount	= size / (channels * (bitsPerSample / 8));

	// Read a chunk of PCM input (converted from whatever format)
    err = AudioConverterFillComplexBuffer(_converter, ACInputProc , self , &frameCount, &ioData, NULL);
	if(err != noErr) {
		NSLog(@"Error reading converter: %i", err);
		return 0;
	}	

	return ioData.mBuffers[0].mDataByteSize;
}

- (double) seekToTime:(double)milliseconds
{
	double newTime;
	
	NSLog(@"Seeking to: %lf", milliseconds);
	NSLog(@"Max frames: %lli", _totalPackets);
	[_countLock lock];
	_packetCount = ((milliseconds / 1000.f) * frequency)/_framesPerPacket;
	NSLog(@"Seeking in coreaudio: %lli", _packetCount);
	[_countLock unlock];

	newTime = ((_packetCount * _framesPerPacket)/frequency)*1000.0;
	
	return newTime;
}

@end
