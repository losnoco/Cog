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

#import "CoreAudioDecoder.h"

#import "Logging.h"

#define REWIND_SIZE 131072

@interface CoreAudioDecoder (Private)
- (BOOL) readInfoFromExtAudioFileRef;
@end

static OSStatus readProc(void* clientData,
                         SInt64 position,
                         UInt32 requestCount,
                         void* buffer,
                         UInt32* actualCount)
{
    NSObject* _handle = (__bridge NSObject *)(clientData);
    CoreAudioDecoder * __unsafe_unretained pSelf = (id) _handle;
    
    id<CogSource> source = pSelf->_audioSource;
    
    if (position != pSelf->_lastPosition) {
        if ([source seekable])
            [source seek:position whence:SEEK_SET];
        else if (position < pSelf->rewindStart)
            return seekErr;
    }

    size_t copyMax = 0;
    size_t bytesRead = 0;
    size_t rewindBytes = 0;

    if (![source seekable]) {
        long rewindStart = pSelf->rewindStart;
        rewindBytes = [pSelf->rewindBuffer length];

        if ( position < rewindStart + rewindBytes ) {
            const uint8_t * rewindBuffer = (const uint8_t *)([pSelf->rewindBuffer bytes]);
            copyMax = rewindStart + rewindBytes - position;
            if (copyMax > requestCount)
                copyMax = requestCount;
            memcpy(buffer, rewindBuffer + (position - rewindStart), copyMax);
            requestCount -= copyMax;
            position += copyMax;
        }
    }
    
    if ( requestCount )
        bytesRead = [source read:(((uint8_t *)buffer) + copyMax) amount:requestCount];
    
    if (![source seekable]) {
        size_t copyBytes = bytesRead;
        if (copyBytes) {
            ssize_t removeBytes = ((rewindBytes + copyBytes) - REWIND_SIZE);
            if (removeBytes > 0) {
                NSRange range = NSMakeRange(0, removeBytes);
                [pSelf->rewindBuffer replaceBytesInRange:range withBytes:NULL length:0];
                pSelf->rewindStart += removeBytes;
            }
            [pSelf->rewindBuffer appendBytes:buffer length:copyBytes];
        }
    }
    
    pSelf->_lastPosition = position + bytesRead;

    if(actualCount)
        *actualCount = (UInt32) (copyMax + bytesRead);

    return noErr;
}

static SInt64 getSizeProc(void* clientData) {
    NSObject* _handle = (__bridge NSObject *)(clientData);
    CoreAudioDecoder * __unsafe_unretained pSelf = (id) _handle;

    id<CogSource> source = pSelf->_audioSource;
    
    SInt64 size;
    
    if ([source seekable]) {
        [source seek:0 whence:SEEK_END];
        size = [source tell];
        [source seek:pSelf->_lastPosition whence:SEEK_SET];
    }
    else {
        size = INT64_MAX;
    }
    
    return size;
}

@implementation CoreAudioDecoder

- (void) close
{
	OSStatus			err;
	
	err = ExtAudioFileDispose(_in);
	if(noErr != err) {
		DLog(@"Error closing ExtAudioFile");
	}
    
    err = AudioFileClose(_audioFile);
    if(noErr != err) {
        DLog(@"Error closing AudioFile");
    }
    
    _audioSource = nil;
}

- (void) dealloc
{
    [self close];
}

- (BOOL)open:(id<CogSource>)source;
{
	OSStatus						err;
    
    _audioSource = source;
    _lastPosition = [source tell];
    
    rewindStart = _lastPosition;
    rewindBuffer = [[NSMutableData alloc] init];

    err = AudioFileOpenWithCallbacks((__bridge void *)self, readProc, 0, getSizeProc, 0, 0, &_audioFile);
    if(noErr != err) {
        ALog(@"Error opening callback interface to file: %d", err);
        return NO;
    }
    
    err = ExtAudioFileWrapAudioFileID(_audioFile, false, &_in);
	if(noErr != err) {
		ALog(@"Error opening file: %d", err);
		return NO;
	}
	
	return [self readInfoFromExtAudioFileRef];
}

- (BOOL) readInfoFromExtAudioFileRef
{
	OSStatus						err;
	UInt32							size;
	AudioStreamBasicDescription		asbd;
	
	// Get input file information
	size	= sizeof(asbd);
	err		= ExtAudioFileGetProperty(_in, kExtAudioFileProperty_FileDataFormat, &size, &asbd);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}
	
	SInt64 total;
	size	= sizeof(total);
	err		= ExtAudioFileGetProperty(_in, kExtAudioFileProperty_FileLengthFrames, &size, &total);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}
	totalFrames = total;
	
	//Is there a way to get bitrate with extAudioFile?
	bitrate				= 0;
	
	// Set our properties
	bitsPerSample		= asbd.mBitsPerChannel;
	channels			= asbd.mChannelsPerFrame;
	frequency			= asbd.mSampleRate;
	floatingPoint       = NO;
    
	// mBitsPerChannel will only be set for lpcm formats
	if(0 == bitsPerSample) {
		bitsPerSample = 32;
        floatingPoint = YES;
	}
	
	// Set output format
	AudioStreamBasicDescription		result;
	
	bzero(&result, sizeof(AudioStreamBasicDescription));
	
	result.mFormatID			= kAudioFormatLinearPCM;
    if (floatingPoint) {
        result.mFormatFlags     = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    }
    else {
        result.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian;
    }
	
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
	
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}

- (int) readAudio:(void *)buf frames:(UInt32)frames
{
	OSStatus				err;
	AudioBufferList			bufferList;
	UInt32					frameCount;
	
	// Set up the AudioBufferList
	bufferList.mNumberBuffers				= 1;
	bufferList.mBuffers[0].mNumberChannels	= channels;
	bufferList.mBuffers[0].mData			= buf;
	bufferList.mBuffers[0].mDataByteSize	= frames * channels * (bitsPerSample/8);
	
	// Read a chunk of PCM input (converted from whatever format)
	frameCount	= frames;
	err			= ExtAudioFileRead(_in, &frameCount, &bufferList);
	if(err != noErr) {
		return 0;
	}	
	
	return frameCount;
}

- (long) seek:(long)frame
{
	OSStatus			err;
	
	err = ExtAudioFileSeek(_in, frame);
	if(noErr != err) {
		return -1;
	}
	
	return frame;
}

+ (NSArray *)fileTypes
{
	OSStatus			err;
	UInt32				size;
	NSArray *sAudioExtensions;
	
	size	= sizeof(sAudioExtensions);
	err		= AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AllExtensions, 0, NULL, &size, &sAudioExtensions);
	if(noErr != err) {
		return nil;
	}
	
	return sAudioExtensions;
}

+ (NSArray *)mimeTypes
{
    OSStatus           err;
    UInt32             size;
    NSArray *sAudioMIMETypes;
    
    size    = sizeof(sAudioMIMETypes);
    err     = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AllMIMETypes, 0, NULL, &size, &sAudioMIMETypes);
    if(noErr != err) {
        return nil;
    }
    
	return sAudioMIMETypes;
}

+ (float)priority
{
    return 1.0;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels],@"channels",
		[NSNumber numberWithInt:bitsPerSample],@"bitsPerSample",
        [NSNumber numberWithBool:floatingPoint],@"floatingPoint",
		[NSNumber numberWithInt:bitrate],@"bitrate",
		[NSNumber numberWithFloat:frequency],@"sampleRate",
		[NSNumber numberWithLong:totalFrames],@"totalFrames",
		[NSNumber numberWithBool:[_audioSource seekable]], @"seekable",
        floatingPoint ? @"host" : @"big", @"endian",
		nil];
}


@end
