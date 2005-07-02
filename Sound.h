//
//  Sound.h
//  Cog
//
//  Created by Vincent Spader on 5/11/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import "SoundFile/SoundFile.h"
#import "VirtualRingBuffer.h"

//Inter thread messages
//Controller to sound messages
enum
{
	kCogPauseResumeMessage = 100,
	kCogPauseMessage,
	kCogResumeMessage,
	kCogPlayFileMessage,
	kCogChangeFileMessage,
	kCogStopMessage,
	kCogSeekMessage,
	kCogEndOfPlaylistMessage,
	kCogSetVolumeMessage,
	
	//sound to controller
	kCogCheckinMessage,
	kCogRequestNextFileMessage,
	kCogBitrateUpdateMessage,
	kCogLengthUpdateMessage,
	kCogPositionUpdateMessage,
	kCogFileChangedMessage,
	kCogStatusUpdateMessage
};

enum
{
	kCogStatusPaused = 0,
	kCogStatusStopped,
	kCogStatusPlaying,
	kCogStatusEndOfFile,
	kCogStatusEndOfPlaylist,
	kCogStatusPlaybackEnded
};

@interface Sound : NSObject {
	//For communication with the soundcontroller
	NSPort *sendPort; 
	NSPort *distantPort; 
	
	SoundFile *soundFile;
	unsigned long currentPosition;
	unsigned long totalLength;
	
	//Whole lotta core audio fun
	AudioUnit outputUnit;
	AudioStreamBasicDescription deviceFormat;	// info about the default device
    AudioStreamBasicDescription sourceStreamFormat;
    AURenderCallbackStruct renderCallback;
	AudioConverterRef converter;
	
	void *conversionBuffer;
	
	int playbackStatus;
	int oldPlaybackStatus; //For resuming
	
	NSTimer *fillTimer; //used to wake up the filler thread
	//semaphore_t semaphore; //used to wake up the filler thread	
	NSTimer *positionTimer;
	
	VirtualRingBuffer *ringBuffer;
	VirtualRingBuffer *auxRingBuffer; //For changing tracks
	
	VirtualRingBuffer *readRingBuffer;
	VirtualRingBuffer *writeRingBuffer;

	//Track changing procedure...sound send changerequest to controller when it hits EOF, and sets writeringbuffer to the opposite buffer...
	//when buffer is empty, sound sends changecomplete to controller, and sets readringbuffer to the opposite
	
	NSLock *readLock;
	NSLock *writeLock;
}

- (void)launchThreadWithPort:(id)inData;
- (void)sendPortMessage:(int)msgid;
- (void)sendPortMessage:(int)msgid withData:(void *)data ofSize:(int)size;
- (void)handlePortMessage:(NSPortMessage *)portMessage;

- (void)scheduleFillTimer;
- (void)fireFillTimer;

- (void)sendPositionUpdate:(id)sender;

- (void)fillBuffer:(id)sender;
- (int)convert:(void *)destBuf packets:(int)numPackets;
- (void)resetBuffer;
- (VirtualRingBuffer *)oppositeBuffer:(VirtualRingBuffer *)buf;

//private methodss
- (BOOL)setupAudioOutput;
- (BOOL)startAudioOutput;
- (void)stopAudioOutput;
- (void)cleanUpAudioOutput;

- (BOOL)prepareSoundFile;
- (void)cleanUpSoundFile;

- (void)setThreadPolicy;

- (void)setPlaybackStatus:(int)s;

- (void)pause;
- (void)resume;
- (void)stop;

- (void)playFile:(NSString *)filename;
- (void)changeFile:(NSString *)filename;

- (BOOL)setSoundFile:(NSString *)filename;

//helper function
- (double)calculateTime:(unsigned long) pos;
- (unsigned long)calculatePos:(double) time;

- (void)setVolume:(float)v;

@end
 