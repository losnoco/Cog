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
- (BOOL) readInfoFromExtAudioFileRef:(ExtAudioFileRef)file;
@end

@implementation CoreAudioFile

- (BOOL) open:(const char *)filename
{
	OSStatus		err;
	FSRef			ref;
	
	// Open the input file
	err = FSPathMakeRef((const UInt8 *)filename, &ref, NULL);
	if(noErr != err) {
		NSLog(@"Error opening ExtAudioFile: %i", err);
		return NO;
	}
	
	err = ExtAudioFileOpen(&ref, &_in);
	if(noErr != err) {
		NSLog(@"Error opening ExtAudioFile: %i", err);
		return NO;
	}
	
	// Read properties
	return [self readInfoFromExtAudioFileRef:_in];
}

- (void) close
{
	OSStatus			err;
	
	err = ExtAudioFileDispose(_in);
	if(noErr != err) {
		NSLog(@"Error closing ExtAudioFile: %i", err);
	}
}

- (BOOL) readInfo:(const char *)filename
{
	OSStatus						err;
	FSRef							ref;
	BOOL							result;
	
	result = YES;

	// Open the input file
	err = FSPathMakeRef((const UInt8 *)filename, &ref, NULL);
	if(noErr != err) {
		NSLog(@"Error closing ExtAudioFile: %i", err);
		return NO;
	}
	
	err = ExtAudioFileOpen(&ref, &_in);
	if(noErr != err) {
		NSLog(@"Error closing ExtAudioFile: %i", err);
		return NO;
	}
	
	result = [self readInfoFromExtAudioFileRef:_in];
	
	return result;
}

- (BOOL) readInfoFromExtAudioFileRef:(ExtAudioFileRef)file
{
	OSStatus						err;
	UInt32							size;
	SInt64							totalFrames;
	AudioStreamBasicDescription		asbd;

	// Get input file information
	size	= sizeof(asbd);
	err		= ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileDataFormat, &size, &asbd);
	if(err != noErr) {
		err = ExtAudioFileDispose(file);
		NSLog(@"Error closing ExtAudioFile: %i", err);
		return NO;
	}

	size	= sizeof(totalFrames);
	err		= ExtAudioFileGetProperty(file, kExtAudioFileProperty_FileLengthFrames, &size, &totalFrames);
	if(err != noErr) {
		err = ExtAudioFileDispose(file);
		NSLog(@"Error closing ExtAudioFile: %i", err);
		return NO;
	}
		
	// Set our properties
	bitsPerSample		= asbd.mBitsPerChannel;
	channels			= asbd.mChannelsPerFrame;
	frequency			= asbd.mSampleRate;
	
	// mBitsPerChannel will only be set for lpcm formats
	if(0 == bitsPerSample) {
		bitsPerSample = 16;
	}
	
	totalSize			= totalFrames * channels * (bitsPerSample / 8);
	currentPosition		= 0;
	bitRate				= 0;

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
		
	err = ExtAudioFileSetProperty(file, kExtAudioFileProperty_ClientDataFormat, sizeof(result), &result);
	if(noErr != err) {
		err = ExtAudioFileDispose(file);
		NSLog(@"Error closing ExtAudioFile: %i", err);
		return NO;
	}
	
	// Further properties
	isBigEndian			= YES;
	isUnsigned			= NO;
	
	NSLog(@"Successfully read file");
	
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
		NSLog(@"Error reading ExtAudioFile: %i", err);
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
