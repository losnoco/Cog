//
//  SoundFile.m
//  Cog
//
//  Created by Vincent Spader on 1/15/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "SoundFile.h"


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

- (int)bitRate
{
	return bitRate;
}

//this should be done by the soundfile....not seek...
- (double)seekToTime:(double)milliseconds
{
}



@class FlacFile;
@class MonkeysFile;
@class MPEGFile;
@class MusepackFile;
@class VorbisFile;
@class WaveFile;
@class AACFile;
@class WavPackFile;
@class ShnFile;

+ (SoundFile *)soundFileFromFilename:(NSString *)filename
{
	SoundFile *soundFile;
	
	if ([[filename pathExtension] isEqualToString:@"wav"] || [[filename pathExtension] isEqualToString:@"aiff"] || [[filename pathExtension] isEqualToString:@"aif"])
	{
		soundFile = [[WaveFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"ogg"])
	{
		soundFile = [[VorbisFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"mpc"])
	{
		soundFile = [[MusepackFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"flac"])
	{
		soundFile = [[FlacFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"ape"])
	{
		soundFile = [[MonkeysFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"mp3"])
	{
		soundFile = [[MPEGFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"aac"])
	{
		soundFile = [[AACFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"wv"])
	{
		soundFile = [[WavPackFile alloc] init];
	}
	else if ([[filename pathExtension] isEqualToString:@"shn"])
	{
		soundFile = [[ShnFile alloc] init];
	}
	else
	{
		soundFile = nil;
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
	if (b == YES)
		return soundFile;
		
	return nil;
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

- (UInt32)fillBuffer:(void *)buf ofSize:(UInt32)size
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

- (unsigned long)currentPosition
{
	return currentPosition;
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
