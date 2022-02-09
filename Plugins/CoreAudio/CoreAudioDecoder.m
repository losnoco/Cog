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

@interface CoreAudioDecoder (Private)
- (BOOL)readInfoFromExtAudioFileRef;
@end

static int ffat_get_channel_id(AudioChannelLabel label) {
	if(label == 0)
		return -1;
	else if(label <= kAudioChannelLabel_LFEScreen)
		return label - 1;
	else if(label <= kAudioChannelLabel_RightSurround)
		return label + 4;
	else if(label <= kAudioChannelLabel_CenterSurround)
		return label + 1;
	else if(label <= kAudioChannelLabel_RightSurroundDirect)
		return label + 23;
	else if(label <= kAudioChannelLabel_TopBackRight)
		return label - 1;
	else if(label < kAudioChannelLabel_RearSurroundLeft)
		return -1;
	else if(label <= kAudioChannelLabel_RearSurroundRight)
		return label - 29;
	else if(label <= kAudioChannelLabel_RightWide)
		return label - 4;
	else if(label == kAudioChannelLabel_LFE2)
		return -1;
	else if(label == kAudioChannelLabel_Mono)
		return 2; // Front center
	else
		return -1;
}

static OSStatus readProc(void *clientData,
                         SInt64 position,
                         UInt32 requestCount,
                         void *buffer,
                         UInt32 *actualCount) {
	NSObject *_handle = (__bridge NSObject *)(clientData);
	CoreAudioDecoder *__unsafe_unretained pSelf = (id)_handle;

	id<CogSource> source = pSelf->_audioSource;

	if(position != pSelf->_lastPosition) {
		[source seek:position whence:SEEK_SET];
	}

	size_t copyMax = 0;
	size_t bytesRead = 0;

	if(requestCount)
		bytesRead = [source read:(((uint8_t *)buffer) + copyMax) amount:requestCount];

	pSelf->_lastPosition = position + bytesRead;

	if(actualCount)
		*actualCount = (UInt32)(copyMax + bytesRead);

	return noErr;
}

static SInt64 getSizeProc(void *clientData) {
	NSObject *_handle = (__bridge NSObject *)(clientData);
	CoreAudioDecoder *__unsafe_unretained pSelf = (id)_handle;

	return pSelf->_fileSize;
}

@implementation CoreAudioDecoder

- (void)close {
	OSStatus err;

	if(_in_opened) {
		err = ExtAudioFileDispose(_in);
		if(noErr != err) {
			DLog(@"Error closing ExtAudioFile");
		}
		_in_opened = NO;
	}

	if(_audioFile_opened) {
		err = AudioFileClose(_audioFile);
		if(noErr != err) {
			DLog(@"Error closing AudioFile");
		}
		_audioFile_opened = NO;
	}

	_audioSource = nil;
}

- (void)dealloc {
	[self close];
}

- (BOOL)open:(id<CogSource>)source;
{
	OSStatus err;

	_audioFile_opened = NO;
	_in_opened = NO;

	if(![source seekable])
		return NO;

	_audioSource = source;
	_lastPosition = [source tell];
	[source seek:0 whence:SEEK_END];
	_fileSize = [source tell];
	[source seek:_lastPosition whence:SEEK_SET];

	err = AudioFileOpenWithCallbacks((__bridge void *)self, readProc, 0, getSizeProc, 0, 0, &_audioFile);
	if(noErr != err) {
		ALog(@"Error opening callback interface to file: %d", err);
		return NO;
	}

	_audioFile_opened = YES;

	err = ExtAudioFileWrapAudioFileID(_audioFile, false, &_in);
	if(noErr != err) {
		ALog(@"Error opening file: %d", err);
		return NO;
	}

	_in_opened = YES;

	return [self readInfoFromExtAudioFileRef];
}

- (BOOL)readInfoFromExtAudioFileRef {
	OSStatus err;
	UInt32 size;
	UInt32 asbdSize;
	AudioStreamBasicDescription asbd;
	AudioFileID afi;

	// Get input file information
	asbdSize = sizeof(asbd);
	err = ExtAudioFileGetProperty(_in, kExtAudioFileProperty_FileDataFormat, &asbdSize, &asbd);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}

	SInt64 total;
	size = sizeof(total);
	err = ExtAudioFileGetProperty(_in, kExtAudioFileProperty_FileLengthFrames, &size, &total);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}
	totalFrames = total;

	size = sizeof(afi);
	err = ExtAudioFileGetProperty(_in, kExtAudioFileProperty_AudioFile, &size, &afi);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}

	SInt32 formatBitsPerSample;
	size = sizeof(formatBitsPerSample);
	err = AudioFileGetProperty(afi, kAudioFilePropertySourceBitDepth, &size, &formatBitsPerSample);
	if(err != noErr) {
		if(err == kAudioFileUnsupportedPropertyError) {
			formatBitsPerSample = 0; // floating point formats apparently don't return this any more
		} else {
			err = ExtAudioFileDispose(_in);
			return NO;
		}
	}

	UInt32 _bitrate;
	size = sizeof(_bitrate);
	err = AudioFileGetProperty(afi, kAudioFilePropertyBitRate, &size, &_bitrate);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}

	err = AudioFileGetPropertyInfo(afi, kAudioFilePropertyChannelLayout, &size, NULL);
	if(err != noErr || size == 0) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}
	AudioChannelLayout *acl = malloc(size);
	err = AudioFileGetProperty(afi, kAudioFilePropertyChannelLayout, &size, acl);
	if(err != noErr) {
		free(acl);
		err = ExtAudioFileDispose(_in);
		return NO;
	}

	uint32_t config = 0;
	for(uint32_t i = 0; i < acl->mNumberChannelDescriptions; ++i) {
		int channelNumber = ffat_get_channel_id(acl->mChannelDescriptions[i].mChannelLabel);
		if(channelNumber >= 0) {
			if(config & (1 << channelNumber)) {
				free(acl);
				err = ExtAudioFileDispose(_in);
				return NO;
			}
			config |= 1 << channelNumber;
		} else {
			free(acl);
			err = ExtAudioFileDispose(_in);
			return NO;
		}
	}

	channelConfig = config;

	free(acl);

	bitrate = (_bitrate + 500) / 1000;

	CFStringRef formatName;
	size = sizeof(formatName);
	err = AudioFormatGetProperty(kAudioFormatProperty_FormatName, asbdSize, &asbd, &size, &formatName);
	if(err != noErr) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}

	codec = (__bridge NSString *)formatName;

	CFRelease(formatName);

	NSRange range = [codec rangeOfString:@","];
	if(range.location != NSNotFound) {
		codec = [codec substringToIndex:range.location];
	}

	// Set our properties
	bitsPerSample = formatBitsPerSample;
	channels = asbd.mChannelsPerFrame;
	frequency = asbd.mSampleRate;
	floatingPoint = NO;

	// mBitsPerChannel will only be set for lpcm formats
	if(bitsPerSample <= 0) {
		bitsPerSample = 32;
		floatingPoint = YES;
	}

	_audioFile_is_lossy = NO;

	if(floatingPoint || [[codec lowercaseString] containsString:@"adpcm"] || [[codec lowercaseString] containsString:@"gsm"])
		_audioFile_is_lossy = YES;

	// Set output format
	AudioStreamBasicDescription result;

	bzero(&result, sizeof(AudioStreamBasicDescription));

	result.mFormatID = kAudioFormatLinearPCM;
	if(floatingPoint) {
		result.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
	} else {
		result.mFormatFlags = kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian;
	}

	result.mSampleRate = frequency;
	result.mChannelsPerFrame = channels;
	result.mBitsPerChannel = bitsPerSample;

	result.mBytesPerPacket = channels * (bitsPerSample / 8);
	result.mFramesPerPacket = 1;
	result.mBytesPerFrame = channels * (bitsPerSample / 8);

	err = ExtAudioFileSetProperty(_in, kExtAudioFileProperty_ClientDataFormat, sizeof(result), &result);
	if(noErr != err) {
		err = ExtAudioFileDispose(_in);
		return NO;
	}

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	OSStatus err;
	AudioBufferList bufferList;
	UInt32 frameCount;

	// Set up the AudioBufferList
	bufferList.mNumberBuffers = 1;
	bufferList.mBuffers[0].mNumberChannels = channels;
	bufferList.mBuffers[0].mData = buf;
	bufferList.mBuffers[0].mDataByteSize = frames * channels * (bitsPerSample / 8);

	// Read a chunk of PCM input (converted from whatever format)
	frameCount = frames;
	err = ExtAudioFileRead(_in, &frameCount, &bufferList);
	if(err != noErr) {
		return 0;
	}

	return frameCount;
}

- (long)seek:(long)frame {
	OSStatus err;

	err = ExtAudioFileSeek(_in, frame);
	if(noErr != err) {
		return -1;
	}

	return frame;
}

+ (NSArray *)fileTypes {
	OSStatus err;
	UInt32 size;
	NSArray *sAudioExtensions;

	size = sizeof(sAudioExtensions);
	err = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AllExtensions, 0, NULL, &size, &sAudioExtensions);
	if(noErr != err) {
		return nil;
	}

	return sAudioExtensions;
}

+ (NSArray *)mimeTypes {
	OSStatus err;
	UInt32 size;
	NSArray *sAudioMIMETypes;

	size = sizeof(sAudioMIMETypes);
	err = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AllMIMETypes, 0, NULL, &size, &sAudioMIMETypes);
	if(noErr != err) {
		return nil;
	}

	return sAudioMIMETypes;
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"WAVE File", @"wav.icns", @"wav", @"w64"],
		@[@"AIFF File", @"aiff.icns", @"aif", @"aiff", @"aifc"],
		@[@"CAF File", @"song.icns", @"caf"],
		@[@"AU File", @"song.icns", @"au"],
		@[@"MPEG Audio File", @"mp3.icns", @"mp3", @"mp2", @"mp1", @"m2a", @"mpa"],
		@[@"MPEG Stream File", @"song.icns", @"mpeg"],
		@[@"MPEG-4 Audio File", @"m4a.icns", @"m4a", @"mp4", @"m4b", @"m4r"],
		@[@"MPEG-4 AAC Audio File", @"song.icns", @"aac", @"adts"],
		@[@"AMR Audio File", @"song.icns", @"amr"],
		@[@"USAC Audio File", @"song.icns", @"xhe"],
		@[@"AC-3 Audio File", @"song.icns", @"ac3"],
		@[@"FLAC Audio File", @"flac.icns", @"flac"],
		@[@"SND Audio File", @"song.icns", @"snd"]
	];
}

- (NSDictionary *)properties {
	return @{@"channels": [NSNumber numberWithInt:channels],
			 @"channelConfig": [NSNumber numberWithUnsignedInt:channelConfig],
			 @"bitsPerSample": [NSNumber numberWithInt:bitsPerSample],
			 @"floatingPoint": [NSNumber numberWithBool:floatingPoint],
			 @"bitrate": [NSNumber numberWithInt:bitrate],
			 @"sampleRate": [NSNumber numberWithFloat:frequency],
			 @"totalFrames": [NSNumber numberWithLong:totalFrames],
			 @"seekable": [NSNumber numberWithBool:YES],
			 @"codec": codec,
			 @"endian": floatingPoint ? @"host" : @"big",
			 @"encoding": _audioFile_is_lossy ? @"lossy" : @"lossless"};
}

@end
