//
//  SoundFile.m
//  Cog
//
//  Created by Vincent Spader on 1/15/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "SoundFile.h"

#import "FlacFile.h"
//#import "AACFile.h"
#import "MonkeysFile.h"
//#import "MPEGFile.h"
#import "MusepackFile.h"
#import "VorbisFile.h"
//#import "WaveFile.h"
#import "WavPackFile.h"
#import "ShnFile.h"
#import "CoreAudioFile.h"
//#import "GameFile.h"
#import "MadFile.h"


//Something is redefining BOOL
#undef BOOL

extern "C" {
BOOL hostIsBigEndian()
{
#ifdef __BIG_ENDIAN__
	return YES;
#else
	return NO;
#endif
}

extern NSArray * getCoreAudioExtensions();

};

@implementation SoundFile

/*- (void)seek:(unsigned long)position
{
	unsigned long time;
	unsigned long frame;
	
	frame = position/channels/(bitsPerSample/8);
	time = (unsigned long)(((double)frame/(frequency/1000.0)));

	currentPosition = position;
	
	time = [self seekToTime:time];
	position = time * (frequency/1000.0)*chanels*(bitsPerSample/8)
}
*/
- (double)length
{
	return (totalSize/channels/(bitsPerSample/8)/(frequency/1000.0));
}

- (int)bitrate
{
	return bitrate;
}

//this should be done by the soundfile....not seek...
- (double)seekToTime:(double)milliseconds
{
	return -1.0;
}


/*
@class FlacFile;
@class MonkeysFile;
@class MPEGFile;
@class MusepackFile;
@class VorbisFile;
@class WaveFile;
@class AACFile;
@class WavPackFile;
@class ShnFile;
*/
+ (SoundFile *)soundFileFromFilename:(NSString *)filename
{
	SoundFile	*soundFile;
	NSString	*extension;
	DBLog(@"Filename: %@", filename);
	
	extension = [filename pathExtension];
	
	/*if (([[filename pathExtension] caseInsensitiveCompare:@"wav"] == NSOrderedSame) || ([[filename pathExtension] caseInsensitiveCompare:@"aiff"] == NSOrderedSame) || ([[filename pathExtension] caseInsensitiveCompare:@"aif"] == NSOrderedSame))
	{
		soundFile = [[WaveFile alloc] init];
	}*/
	if ([[filename pathExtension] caseInsensitiveCompare:@"ogg"] == NSOrderedSame)
	{
		soundFile = [[VorbisFile alloc] init];
	}
	else if ([[filename pathExtension] caseInsensitiveCompare:@"mpc"] == NSOrderedSame)
	{
		soundFile = [[MusepackFile alloc] init];
	}
	else if ([[filename pathExtension] caseInsensitiveCompare:@"flac"] == NSOrderedSame)
	{
		soundFile = [[FlacFile alloc] init];
	}
	else if ([[filename pathExtension] caseInsensitiveCompare:@"ape"] == NSOrderedSame)
	{
		soundFile = [[MonkeysFile alloc] init];
	}
	else if ([[filename pathExtension] caseInsensitiveCompare:@"mp3"] == NSOrderedSame)
	{
		soundFile = [[MADFile alloc] init];
	}
	/*else if ([[filename pathExtension] caseInsensitiveCompare:@"aac"] == NSOrderedSame)
	{
		soundFile = [[AACFile alloc] init];
	}*/
	else if ([[filename pathExtension] caseInsensitiveCompare:@"wv"] == NSOrderedSame)
	{
		soundFile = [[WavPackFile alloc] init];
	}
	else if ([[filename pathExtension] caseInsensitiveCompare:@"shn"] == NSOrderedSame)
	{
		soundFile = [[ShnFile alloc] init];
	}
	else
	{
		unsigned i;
		NSArray *extensions = getCoreAudioExtensions();
		
		soundFile = nil;

		for(i = 0; i < [extensions count]; ++i) {
			if([[extensions objectAtIndex:i] caseInsensitiveCompare:extension]) {
				soundFile = [[CoreAudioFile alloc] init];
				break;
			}
		}
	}
	
	return soundFile;
}

+ (SoundFile *)open:(NSString *)filename
{
	SoundFile *soundFile;
	BOOL b;

	soundFile = [SoundFile soundFileFromFilename:filename];
	
	b = [soundFile open:[filename UTF8String]];
	if (b == YES)
		return soundFile;
	
	return nil;
}

+ (SoundFile *)readInfo:(NSString *)filename
{
	BOOL b;
	SoundFile *soundFile;
	
	soundFile = [SoundFile soundFileFromFilename:filename];

	b = [soundFile readInfo:[filename UTF8String]];
	if (b == NO)
		return nil;
	
	[soundFile close];

	return soundFile;
}

- (void)reset
{
	[self seekToTime:0.0];
}

- (void)getFormat:(AudioStreamBasicDescription *)sourceStreamFormat
{
//	NSLog(@"Getting format!");
	sourceStreamFormat->mFormatID = kAudioFormatLinearPCM;
	sourceStreamFormat->mFormatFlags = 0;
	
	sourceStreamFormat->mSampleRate = frequency;

	sourceStreamFormat->mBitsPerChannel = bitsPerSample;

	sourceStreamFormat->mBytesPerFrame = (bitsPerSample/8)*channels;
	sourceStreamFormat->mChannelsPerFrame = channels;
	
	sourceStreamFormat->mFramesPerPacket = 1;
	sourceStreamFormat->mBytesPerPacket = (bitsPerSample/8)*channels;	
	sourceStreamFormat->mReserved = 0;
	
	if (isBigEndian == YES)
	{
		sourceStreamFormat->mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
		sourceStreamFormat->mFormatFlags |= kLinearPCMFormatFlagIsAlignedHigh;
//		sourceStreamFormat->mFormatFlags |= kLinearPCMFormatFlagIsNonMixable;
//		NSLog(@"FUCKER IS BIG ENDIAN");
	}
	if (isUnsigned == NO)
		sourceStreamFormat->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
}

- (void)close
{
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	return 0;
}

- (BOOL)open:(const char *)filename
{
//	NSLog(@"WRONG OPEN!!!");
	return NO;
}

- (BOOL)readInfo:(const char *)filename
{
	return NO;
}

- (unsigned long)totalSize
{
	return totalSize;
}

- (UInt16)channels
{
	return channels;
}

- (UInt16)bitsPerSample
{
	return bitsPerSample;
}

- (UInt32)frequency
{
	return frequency;
}

-(BOOL)isBigEndian
{
	return isBigEndian;
}
-(BOOL)isUnsigned
{
	return isUnsigned;
}

@end
