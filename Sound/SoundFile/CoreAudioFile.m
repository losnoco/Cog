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

#include <unistd.h>

#import "CoreAudioFile.h"

@interface CoreAudioFile (Private)
- (BOOL) readInfoFromExtAudioFileRef;
@end

@implementation CoreAudioFile

#ifdef _USE_WRAPPER_
OSStatus readFunc(void * inRefCon, SInt64 inPosition, ByteCount requestCount, void *buffer, ByteCount* actualCount)
{
	CoreAudioFile *caf = (CoreAudioFile *)inRefCon;
	int fd = caf->_inFd;
	
	//	fseek(fd, inPosition, SEEK_SET);
//	NSLog(@"Requesting %u", requestCount);
//	NSLog(@"Currently at %lli", inPosition);
	
	*actualCount = pread(fd, buffer, requestCount, inPosition+caf->_startOffset);
	
	if (*actualCount <= 0)
	{
		return -1000; //Error?
	}
	
	return noErr;
}

SInt64 getSizeFunc(void *inRefCon)
{
	CoreAudioFile *caf = (CoreAudioFile *)inRefCon;
	int fd = caf->_inFd;
		
	if (caf->_fileSize != 0)
	{
		return caf->_fileSize;
	}
	
	/*	long curPos;	
	
	curPos = ftell(fd);
	
	fseek(fd, 0, SEEK_END);
	
	caf->_fileSize = ftell(fd);
	
	fseek(fd, curPos, SEEK_SET);
	*/

	caf->_fileSize = lseek(fd, 0, SEEK_END) - caf->_startOffset;
	NSLog(@"SIZE at %lli", caf->_fileSize);

	NSLog(@"ERROR: %i = %i %i %i", errno, EBADF, ESPIPE, EINVAL);
	return caf->_fileSize;
}

OSStatus setSizeFunc(void * inRefCon, SInt64 inSize)
{
	NSLog(@"setsize FUNC");
	
	return -1000; //Not supported at the moment
}

OSStatus writeFunc(void * inRefCon, SInt64 inPosition, ByteCount requestCount, const void *buffer, ByteCount* actualCount)
{
	NSLog(@"WRITE FUNC");
	return -1000; //Not supported at the moment
}
#endif

- (BOOL) open:(const char *)filename
{
	return [self readInfo:filename];
}

- (void) close
{
	OSStatus			err;
	
#ifdef _USE_WRAPPER_
	if (_inFd)
		close(_inFd);
	AudioFileClose(_audioID);
#endif
	
	err = ExtAudioFileDispose(_in);
	if(noErr != err) {
		NSLog(@"Error closing ExtAudioFile");
	}
}

- (BOOL) readInfo:(const char *)filename
{
	OSStatus						err;
	AudioFileTypeID					type = 0;
	NSString						*ext;
	
#ifdef _USE_WRAPPER_	
	// Open the input file
	_inFd = open(filename, O_RDONLY, 0777);
	if (_inFd < 0)
	{
		NSLog(@"Error operning file: %s", filename);
		return NO;
	}
	_startOffset = 0;
	
	ext = [[NSString stringWithCString:filename encoding:NSASCIIStringEncoding] pathExtension];
	//Find first sync frame for MP3
	if([ext caseInsensitiveCompare:@"mp3"] == NSOrderedSame) {
		size_t bytesRead;
		uint8_t buf[2];
		
		type = kAudioFileMP3Type;
		
		for(;;) {
			bytesRead = read(_inFd, buf, 2);
			
			
			if(2 != bytesRead) {
				NSLog(@"Error finding mp3 sync frame");
				close(_inFd);
				return NO;
			}
			
			
			// found some kind of data
			if(0x00 != buf[0] || 0x00 != buf[1]) {
				_startOffset = lseek(_inFd, 0, SEEK_CUR) - 2;
				NSLog(@"Found sync frame at: %llx", _startOffset);
				break;
			}
		}
	}
	else if([ext caseInsensitiveCompare:@"aac"] == NSOrderedSame) {
		type = kAudioFileAAC_ADTSType;
	}
	else if([ext caseInsensitiveCompare:@"m4a"] == NSOrderedSame) {
		type = kAudioFileM4AType;
	}
	else if([ext caseInsensitiveCompare:@"mp4"] == NSOrderedSame) {
		type = kAudioFileMPEG4Type;
	}
		
	//Using callbacks with fopen, ftell, fseek, fclose, because the default pread hangs when accessing the same file from multiple threads.
	err = AudioFileOpenWithCallbacks(self, readFunc, writeFunc, getSizeFunc, setSizeFunc, type, &_audioID);
	if(noErr != err)
	{
		NSLog(@"Error opening with callbacks, falling back: %s", (char *)&err);
		FSRef ref;
		close(_inFd);
		_inFd = 0;
		
		err = FSPathMakeRef((const UInt8 *)filename, &ref, NULL);
		if(noErr != err) {
			return NO;
		}
		
		err = AudioFileOpen(&ref, fsRdPerm, type, &_audioID);
		if(noErr != err) {
			NSLog(@"Error opening AudioFile: %s", (char *)&err);
			return NO;
		}
	}
	
	err = ExtAudioFileWrapAudioFileID(_audioID, NO, &_in);
	if(noErr != err) {
		return NO;
	}
	
#else
	FSRef							ref;
	
	// Open the input file
	err = FSPathMakeRef((const UInt8 *)filename, &ref, NULL);
	if(noErr != err) {
		return NO;
	}
	
	err = ExtAudioFileOpen(&ref, &_in);
	if(noErr != err) {
		NSLog(@"Error opening file: %s", &err);
		return NO;
	}
#endif	
	return [self readInfoFromExtAudioFileRef];
}

- (BOOL) readInfoFromExtAudioFileRef
{
	OSStatus						err;
	UInt32							size;
	SInt64							totalFrames;
	AudioStreamBasicDescription		asbd;
	
	// Get input file information
	size	= sizeof(asbd);
	err		= ExtAudioFileGetProperty(_in, kExtAudioFileProperty_FileDataFormat, &size, &asbd);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}
	
	size	= sizeof(totalFrames);
	err		= ExtAudioFileGetProperty(_in, kExtAudioFileProperty_FileLengthFrames, &size, &totalFrames);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}
	
#ifdef _USE_WRAPPER_
	SInt64 totalBytes;
	
	size	= sizeof(totalBytes);
	err		= AudioFileGetProperty(_audioID, kAudioFilePropertyAudioDataByteCount, &size, &totalBytes);
	if(err != noErr) {
		[self close];
		return NO;
	}
	NSLog(@"BITRATE: %lli %lli %lf", totalBytes, totalFrames, asbd.mSampleRate);
	bitrate = round(((totalBytes*8.0)/((double)(totalFrames)/asbd.mSampleRate))/1000.0);
#else
	//Is there a way to get bitrate with extAudioFile?
	bitrate				= 0;
#endif
	
	// Set our properties
	bitsPerSample		= asbd.mBitsPerChannel;
	channels			= asbd.mChannelsPerFrame;
	frequency			= asbd.mSampleRate;
	
	// mBitsPerChannel will only be set for lpcm formats
	if(0 == bitsPerSample) {
		bitsPerSample = 16;
	}
	
	totalSize	= totalFrames * channels * (bitsPerSample / 8);
	
	// Set output format
	AudioStreamBasicDescription		result;
	
	bzero(&result, sizeof(AudioStreamBasicDescription));
	
	result.mFormatID			= kAudioFormatLinearPCM;
	result.mFormatFlags			= kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian;
	
	result.mSampleRate			= frequency;
	result.mChannelsPerFrame	= channels;
	result.mBitsPerChannel		= bitsPerSample;
	
	result.mBytesPerPacket		= channels * (bitsPerSample / 8);
	result.mFramesPerPacket		= 1;
	result.mBytesPerFrame		= channels * (bitsPerSample / 8);
	
	err = ExtAudioFileSetProperty(_in, kExtAudioFileProperty_ClientDataFormat, sizeof(result), &result);
	if(noErr != err) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}
	
	// Further properties
	isBigEndian			= YES;
	isUnsigned			= NO;
	
	return YES;
}

- (int) fillBuffer:(void *)buf ofSize:(UInt32)size
{
	OSStatus				err;
	AudioBufferList			bufferList;
	UInt32					frameCount;
	
	// Set up the AudioBufferList
	bufferList.mNumberBuffers				= 1;
	bufferList.mBuffers[0].mNumberChannels	= channels;
	bufferList.mBuffers[0].mData			= buf;
	bufferList.mBuffers[0].mDataByteSize	= size;
	
	// Read a chunk of PCM input (converted from whatever format)
	frameCount	= (size / (channels * (bitsPerSample / 8)));
	err			= ExtAudioFileRead(_in, &frameCount, &bufferList);
	if(err != noErr) {
		return 0;
	}	
	
	return frameCount * (channels * (bitsPerSample / 8));
}

- (double) seekToTime:(double)milliseconds
{
	OSStatus			err;
	
	err = ExtAudioFileSeek(_in, ((milliseconds / 1000.f) * frequency));
	if(noErr != err) {
		return -1.f;
	}
	
	return milliseconds;	
}

@end
