//
//  SoundFile.h
//  Cog
//
//  Created by Vincent Spader on 1/15/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import "DBLog.h"

#ifdef __cplusplus
extern "C" {
#endif
BOOL hostIsBigEndian();
#ifdef __cplusplus
}
#endif

@interface SoundFile : NSObject {
	UInt16 bitsPerSample;
	UInt16 channels;
	UInt32 frequency;
	UInt32 bitRate;
	BOOL isBigEndian;
	BOOL isUnsigned;

	unsigned long totalSize;
}

- (unsigned long)totalSize;

- (double)length;

- (int)bitRate;

+ (SoundFile *)soundFileFromFilename:(NSString *)filename; //PRIVATE
+ (SoundFile *)open:(NSString *)filename;
+ (SoundFile *)readInfo:(NSString *)filename;

- (BOOL)open:(const char *)filename;
- (void)getFormat:(AudioStreamBasicDescription *)sourceStreamFormat;

- (BOOL)readInfo:(const char *)filename; //for getting information

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size;

//- (BOOL)seek:(unsigned long)position;
- (double)seekToTime:(double)milliseconds;
- (void)close;
- (void)reset; //START AGAIN

- (UInt16)channels;
- (UInt16)bitsPerSample;
- (UInt32)frequency;
-(BOOL)isBigEndian;
-(BOOL)isUnsigned;

@end
