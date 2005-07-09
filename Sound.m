//
//  Sound.m
//  Cog
//
//  Created by Vincent Spader on 5/11/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <malloc/malloc.h>

#import "Sound.h"
#import "FlacFile.h"
#import "MonkeysFile.h"
#import "MPEGFile.h"
#import "MusepackFile.h"
#import "VorbisFile.h"
#import "WaveFile.h"

#import "DBLog.h"

//128kb of floating point data gives ya ~371 milliseconds...so 512kb should give us about 1.5 seconds
#define RING_BUFFER_SIZE (512 * 1024)
//this much data is about 56 milliseconds of floating point data
#define BUFFER_WRITE_CHUNK (16 * 1024)

//#define RING_BUFFER_SIZE 1048576
//#define BUFFER_WRITE_CHUNK 32768
#define FEEDER_THREAD_IMPORTANCE 6

//timeout should be smaller than the time itd take for the buffer to run dry...looks like were ironclad
#define TIMEOUT 1

void PrintStreamDesc (AudioStreamBasicDescription *inDesc)
{
	if (!inDesc) {
		printf ("Can't print a NULL desc!\n");
		return;
	}
	printf ("- - - - - - - - - - - - - - - - - - - -\n");
	printf ("  Sample Rate:%f\n", inDesc->mSampleRate);
	printf ("  Format ID:%s\n", (char*)&inDesc->mFormatID);
	printf ("  Format Flags:%lX\n", inDesc->mFormatFlags);
	printf ("  Bytes per Packet:%ld\n", inDesc->mBytesPerPacket);
	printf ("  Frames per Packet:%ld\n", inDesc->mFramesPerPacket);
	printf ("  Bytes per Frame:%ld\n", inDesc->mBytesPerFrame);
	printf ("  Channels per Frame:%ld\n", inDesc->mChannelsPerFrame);
	printf ("  Bits per Channel:%ld\n", inDesc->mBitsPerChannel);
	printf ("- - - - - - - - - - - - - - - - - - - -\n");
}

@implementation Sound

//called from the complexfill when the audio is converted...good clean fun
static OSStatus Sound_ACInputProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription** outDataPacketDescription, void* inUserData)
{
	Sound *sound = (Sound *)inUserData;
	OSStatus err = noErr;
	
//	DBLog(@"Convert input proc");
//	DBLog(@"Numpackets: %i %i", *ioNumberDataPackets, ioData->mNumberBuffers);
	
	int amountToWrite;
	int amountWritten;
	void *sourceBuf;

	amountToWrite = (*ioNumberDataPackets)*sound->sourceStreamFormat.mBytesPerPacket;
	sourceBuf = malloc(amountToWrite);
	sound->conversionBuffer = sourceBuf;
	
//	DBLog(@"Requesting: %i", amountToWrite);
	amountWritten = [sound->soundFile fillBuffer:sourceBuf ofSize:amountToWrite];
	
//	DBLog(@"PACKET NUMBER RECEIVED: %i", *ioNumberDataPackets);
	ioData->mBuffers[0].mData = sourceBuf;
	ioData->mBuffers[0].mDataByteSize = amountWritten;
	ioData->mBuffers[0].mNumberChannels = sound->sourceStreamFormat.mChannelsPerFrame;
	ioData->mNumberBuffers = 1;

//	DBLog(@"Input complete");
	
	return err;
}

//called from coreaudio, just fill the bufferlist and thats all
static OSStatus Sound_Renderer(void *inRefCon,  AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp  *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList  *ioData)
{
	Sound *sound = (Sound *)inRefCon;
	OSStatus err = noErr;
	
	int amountAvailable;
	int amountToRead;
	void *readPointer;
	
	[sound->readLock lock];
	
	amountAvailable = [sound->readRingBuffer lengthAvailableToReadReturningPointer:&readPointer];
	if (sound->playbackStatus == kCogStatusEndOfFile && amountAvailable == 0)
	{
		DBLog(@"FILE CHANGED!!!!!");
		[sound sendPortMessage:kCogFileChangedMessage];
		sound->readRingBuffer = [sound oppositeBuffer:sound->readRingBuffer];

		[sound setPlaybackStatus:kCogStatusPlaying];
		
		sound->currentPosition = 0;
		
		double time = [sound calculateTime:sound->totalLength];
		int bitrate = [sound->soundFile bitRate];
		[sound sendPortMessage:kCogLengthUpdateMessage withData:&time ofSize:(sizeof(double))];
		[sound sendPortMessage:kCogBitrateUpdateMessage withData:&bitrate ofSize:(sizeof(int))];
	}
	if (sound->playbackStatus == kCogStatusEndOfPlaylist && amountAvailable == 0)
	{
		//Stop playback
		[sound setPlaybackStatus:kCogStatusStopped];
//		return err;
	}

	if (amountAvailable < ([sound->readRingBuffer bufferLength] - BUFFER_WRITE_CHUNK))
	{
//		DBLog(@"AVAILABLE: %i", amountAvailable);
		[sound fireFillTimer];
	}
	
	if (amountAvailable > inNumberFrames*sound->deviceFormat.mBytesPerPacket)
		amountToRead = inNumberFrames*sound->deviceFormat.mBytesPerPacket;
	else
		amountToRead = amountAvailable;
	
	memcpy(ioData->mBuffers[0].mData, readPointer, amountToRead);
	ioData->mBuffers[0].mDataByteSize = amountToRead;
	
	[sound->readRingBuffer didReadLength:amountToRead];

	sound->currentPosition += amountToRead;
	
	[sound->readLock unlock];
	
	return err;
}	

- (id)init
{
	self = [super init];
	if (self)
	{
		readLock = [[NSLock alloc] init];
		writeLock = [[NSLock alloc] init];
		
		ringBuffer = [(VirtualRingBuffer *)[VirtualRingBuffer alloc] initWithLength:RING_BUFFER_SIZE];
		auxRingBuffer = [(VirtualRingBuffer *)[VirtualRingBuffer alloc] initWithLength:RING_BUFFER_SIZE];
		
		readRingBuffer = ringBuffer;
		writeRingBuffer = ringBuffer;
	}
	
	return self;
}

- (void)launchThreadWithPort:(id)inData
{
	NSAutoreleasePool *pool;
	pool = [[NSAutoreleasePool alloc] init];
	
	distantPort = (NSPort *)inData;
	
	sendPort = [NSPort port];
	if (sendPort)
	{
		[sendPort setDelegate:self];
		
		NSArray *modes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, nil];//NSEventTrackingRunLoopMode, nil];
		NSEnumerator *enumerator;
		NSString *mode;
		
		enumerator = [modes objectEnumerator];
		while ((mode = [enumerator nextObject]))
			[[NSRunLoop currentRunLoop] addPort:sendPort forMode:mode];
	}
//	DBLog(@"SENDING CHECKIN MESSAGE");
	[self sendPortMessage:kCogCheckinMessage];

	[self setupAudioOutput];
	
	[self setThreadPolicy];

	[[NSRunLoop currentRunLoop] run];
	DBLog(@"THREAD EXIT!!!!!!!!!!!");
	[pool release];
}

/*- (void)printDebugInfo:(id)sender
{
	void *ptr;
	
	DBLog(@"Available r/w: %i/%i", [ringBuffer lengthAvailableToReadReturningPointer:&ptr], [ringBuffer lengthAvailableToWriteReturningPointer:&ptr]);
}
*/
- (void)setThreadPolicy
{
    // Increase this thread's priority, and turn off timesharing.  
	
    kern_return_t error;
    thread_extended_policy_data_t extendedPolicy;
    thread_precedence_policy_data_t precedencePolicy;
	
    extendedPolicy.timeshare = 0;
    error = thread_policy_set(mach_thread_self(), THREAD_EXTENDED_POLICY,  (thread_policy_t)&extendedPolicy, THREAD_EXTENDED_POLICY_COUNT);

    if (error != KERN_SUCCESS) {
        DBLog(@"Couldnt set feeder thread's extended policy");
    }
	
    precedencePolicy.importance = FEEDER_THREAD_IMPORTANCE;
    error = thread_policy_set(mach_thread_self(), THREAD_PRECEDENCE_POLICY, (thread_policy_t)&precedencePolicy, THREAD_PRECEDENCE_POLICY_COUNT);

    if (error != KERN_SUCCESS) {
        DBLog(@"Couldn't set feeder thread's precedence policy");
    }
}

- (void)sendPortMessage:(int)msgid
{
	NSPortMessage *portMessage = [[NSPortMessage alloc] initWithSendPort:distantPort receivePort:sendPort components:nil];
	
	if (portMessage)
	{
		NSDate *date = [[NSDate alloc] initWithTimeIntervalSinceNow:20.0];//[[NSDate alloc] init];
		
		[portMessage setMsgid:msgid];
		
		[portMessage sendBeforeDate:date];
		
		[date release];
		[portMessage release];
	}
}

- (void)sendPortMessage:(int)msgid withData:(void *)data ofSize:(int)size
{
	NSPortMessage *portMessage;
	NSData *d = [[NSData alloc] initWithBytes:data length:size];
	NSArray *a = [[NSArray alloc] initWithObjects:d,nil];
	portMessage = [[NSPortMessage alloc] initWithSendPort:distantPort receivePort:sendPort components:a];
	
	[a release];
	[d release];
	
	if (portMessage)
	{
		NSDate *date = [[NSDate alloc] initWithTimeIntervalSinceNow:20.0];//give shit a little time to send, just in case...may come back to bite me

		[portMessage setMsgid:msgid];
		
		NS_DURING
			[portMessage sendBeforeDate:date];
		NS_HANDLER
			NSRunAlertPanel(@"Error Panel", @"%@", @"OK", nil, nil, localException);
		NS_ENDHANDLER

		[date release];
		[portMessage release];
	}
	else {
		DBLog(@"DIDNT SEND! ERROR");
	}
	
}

// Worker thread message handler method.
- (void)handlePortMessage:(NSPortMessage *)portMessage
{
	unsigned int msgid = [portMessage msgid];
		
	// Handle messages from the main thread.
	if (msgid == kCogPauseResumeMessage)
	{
		if (playbackStatus == kCogStatusPaused)
		{
			[self resume];
		}
		else
		{
			[self pause];
		}
	}
	else if (msgid == kCogPauseMessage)
	{
		[self pause];
	}
	else if (msgid == kCogResumeMessage)
	{
		[self resume];
	}
	else if (msgid == kCogStopMessage)
	{
		[self stop];
	}
	else if (msgid == kCogPlayFileMessage || msgid == kCogChangeFileMessage)
	{
		NSArray* components = [portMessage components];
		NSData *data = [components objectAtIndex:0];
		NSString *s = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];		
		
		if (msgid == kCogPlayFileMessage) //PLAY THE FARKING FILE NOW
		{
			[self playFile:s];
		}
		else if (msgid == kCogChangeFileMessage) //change the file, usually in response to a nexttrack request
		{
			[self changeFile:s];
		}
		
		[s release];
	}
	else if (msgid == kCogSeekMessage)
	{
//		DBLog(@"MESAGE RECEIVED");
		NSArray* components = [portMessage components];
		NSData *data = [components objectAtIndex:0];
		double time;
		double newTime;
		unsigned long pos;
		
		time = (*(double *)[data bytes]);
		pos = [self calculatePos:time];
		
		newTime = [soundFile seekToTime:time];
		if (newTime >= 0.0)
		{
			[self resetBuffer];
			
			pos = [self calculatePos:newTime];

			[readLock lock];
			currentPosition = pos;
			[readLock unlock];
		}
		else
		{
			DBLog(@"Not resetting: %f", newTime);
			newTime = [self calculateTime:currentPosition];
		}
		//send a message with newTime
		DBLog(@"RESETING TIME TO: %f", newTime);
		[self sendPortMessage:kCogPositionUpdateMessage withData:&newTime ofSize:(sizeof(double))];
	}
	else if (msgid == kCogEndOfPlaylistMessage)
	{
		[self setPlaybackStatus:kCogStatusEndOfPlaylist];
	}
	else if (msgid == kCogSetVolumeMessage)
	{
		NSArray* components = [portMessage components];
		NSData *data = [components objectAtIndex:0];
		float vol;
		
		vol = (*(float *)[data bytes]);
		
		[self setVolume:vol];
	}		
}

- (void)startPositionTimer
{
	positionTimer = [NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(sendPositionUpdate:) userInfo:nil repeats:YES];
}

- (void)stopPositionTimer
{
	[positionTimer invalidate];
	positionTimer = nil;
}

- (void)scheduleFillTimer
{
	fillTimer = [NSTimer scheduledTimerWithTimeInterval:TIMEOUT target:self selector:@selector(fillBuffer:) userInfo:nil repeats:NO];
}

//the only method in the whole fucking thing that should be called by an outside thread
- (void)fireFillTimer
{
	NSDate *date = [[NSDate alloc] init];
//	DBLog(@"FIRED");
	[fillTimer  setFireDate:date];

	[date release];
}

- (void)fillBuffer:(id)sender
{
	int amountAvailable;
	int convertedSize;
	void *writePointer;
	
	if (playbackStatus == kCogStatusStopped)
	{
		DBLog(@"STOPPING");
		[self stop];
		return;
	}
	
	[writeLock lock];
	
	//We do this so uhm, newly started songs dont have to wait for the entire buffer to fill for data to become available.
	amountAvailable = [writeRingBuffer lengthAvailableToWriteReturningPointer:&writePointer];

	int i;
	for (i = 0; i < amountAvailable; i =+ BUFFER_WRITE_CHUNK)
	{
		int amountToWrite;
		if (amountAvailable > BUFFER_WRITE_CHUNK)
			amountToWrite = BUFFER_WRITE_CHUNK;
		else
			amountToWrite = amountAvailable;
		
		convertedSize = [self convert:writePointer packets:(amountToWrite/deviceFormat.mBytesPerPacket)];
//	convertedSize = [self convert:writePointer packets:(amountAvailable/deviceFormat.mBytesPerPacket)];
		if (playbackStatus == kCogStatusPlaying && convertedSize == 0)
		{
			DBLog(@"NEXT!!!!");
			[self sendPortMessage:kCogRequestNextFileMessage];
			writeRingBuffer = [self oppositeBuffer:writeRingBuffer];
			
			[self setPlaybackStatus:kCogStatusEndOfFile];
		}
			
//		writePointer = (char *)writePointer + convertedSize;
		[writeRingBuffer didWriteLength:convertedSize];

		amountAvailable = [writeRingBuffer lengthAvailableToWriteReturningPointer:&writePointer];
	}
	[writeLock unlock];
//	DBLog(@"WRote: %i", convertedSize);
		
	[self scheduleFillTimer];
}

- (VirtualRingBuffer *)oppositeBuffer:(VirtualRingBuffer *)buf
{
	if (buf == ringBuffer)
		return auxRingBuffer;
	else
		return ringBuffer;
}

- (double)calculateTime:(unsigned long) pos
{
	return ((pos/deviceFormat.mBytesPerPacket)/(deviceFormat.mSampleRate/1000.0));
}

- (unsigned long)calculatePos:(double) time
{
	return (unsigned long)(time * (deviceFormat.mSampleRate/1000.0)) * deviceFormat.mBytesPerPacket;
}

- (void)sendPositionUpdate:(id)sender
{
	//this may be a bad idea
	[readLock lock];
	unsigned long pos = currentPosition;
	[readLock unlock];

	double time = [self calculateTime:pos];
	
	[self sendPortMessage:kCogPositionUpdateMessage withData:&time ofSize:(sizeof(double))];
}

- (int)convert:(void *)destBuf packets:(int)numPackets
{	
	AudioBufferList ioData;
	UInt32 ioNumberFrames;
	UInt32 destSize;
	OSStatus err;
	
	destSize = numPackets*deviceFormat.mBytesPerPacket;
	
	ioNumberFrames = numPackets;
	ioData.mBuffers[0].mData = destBuf;
	ioData.mBuffers[0].mDataByteSize = destSize;
	ioData.mBuffers[0].mNumberChannels = deviceFormat.mChannelsPerFrame;
	ioData.mNumberBuffers = 1;

//	DBLog(@"THIS IS RETARDED: %i", ioData.mBuffers[0].mData);
//	DBLog(@"NUMBER OF PACKETS REQUESTED: %i", ioNumberFrames);
	err = AudioConverterFillComplexBuffer(converter, Sound_ACInputProc, self, &ioNumberFrames, &ioData, NULL);
	if (err != noErr)
		DBLog(@"Converter error: %i", err);
//	DBLog(@"THIS IS ANNOYING: %i", ioData.mBuffers[0].mData);
	free(conversionBuffer);

	
//	DBLog(@"HERE: %i/%i", destSize,  ioData.mBuffers[0].mDataByteSize);

//	DBLog(@"%i/%i",old, inNumberFrames);
	return ioData.mBuffers[0].mDataByteSize;
}

- (BOOL)setupAudioOutput
{
	ComponentDescription desc;  
	OSStatus err;
	
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	Component comp = FindNextComponent(NULL, &desc);  //Finds an component that meets the desc spec's
	if (comp == NULL)
		return NO;
	
	err = OpenAComponent(comp, &outputUnit);  //gains access to the services provided by the component
	if (err)
		return NO;
	
	// Initialize AudioUnit 
	err = AudioUnitInitialize(outputUnit);
	if (err != noErr)
		return NO;
	
	
	UInt32 size = sizeof (AudioStreamBasicDescription);
	Boolean outWritable;
	//Gets the size of the Stream Format Property and if it is writable
	AudioUnitGetPropertyInfo(outputUnit,  
							 kAudioUnitProperty_StreamFormat,
							 kAudioUnitScope_Output, 
							 0, 
							 &size, 
							 &outWritable);
	//Get the current stream format of the output
	err = AudioUnitGetProperty (outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Output,
								0,
								&deviceFormat,
								&size);

	if (err != noErr)
		return NO;
	
/*	// change output format...
	deviceFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
	deviceFormat.mBytesPerFrame = 4;
	deviceFormat.mBitsPerChannel = 16;
	deviceFormat.mBytesPerPacket = 4;
	DBLog(@"stuff: %i %i %i %i", deviceFormat.mBitsPerChannel, deviceFormat.mBytesPerFrame, deviceFormat.mBytesPerPacket, deviceFormat.mFramesPerPacket);
	err = AudioUnitSetProperty (outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Output,
								0,
								&deviceFormat,
								size);
*/
	//Set the stream format of the output to match the input
	err = AudioUnitSetProperty (outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Input,
								0,
								&deviceFormat,
								size);
	
	//setup render callbacks
	renderCallback.inputProc = Sound_Renderer;
	renderCallback.inputProcRefCon = self;
	
	AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &renderCallback, sizeof(AURenderCallbackStruct));	
	
//	DBLog(@"Audio output successfully initialized");
	return (err == noErr);
}	

- (BOOL)startAudioOutput
{
	return (noErr == AudioOutputUnitStart(outputUnit));    
}

- (void)cleanUpAudioOutput
{
	AudioOutputUnitStop(outputUnit);//you must stop the audio unit
	AudioUnitUninitialize (outputUnit);
	CloseComponent(outputUnit);
}		

- (void)stopAudioOutput
{
	if (outputUnit)
        AudioOutputUnitStop(outputUnit);
}

- (BOOL)prepareSoundFile
{
//	DBLog(@"This fucker hates me");
	[soundFile getFormat:&sourceStreamFormat];
	
#ifdef DEBUG
	PrintStreamDesc(&sourceStreamFormat);
	PrintStreamDesc(&deviceFormat);
#endif
	
	//Make the converter
	OSStatus stat = noErr;
	stat = AudioConverterNew ( &sourceStreamFormat, &deviceFormat, &converter);
//	DBLog(@"Created converter");
	if (stat != noErr)
	{
		DBLog(@"Error creating converter %i", stat);
	}

	unsigned long length;
	length = (unsigned long)([soundFile totalSize] * (deviceFormat.mSampleRate/sourceStreamFormat.mSampleRate) * (deviceFormat.mBytesPerPacket/sourceStreamFormat.mBytesPerPacket));
	
	//again, may be bad
	[readLock lock];
	totalLength = length;
	[readLock unlock];
//	DBLog(@"AM I CRASHING?");
	return (stat == noErr);
}

- (void)cleanUpSoundFile
{
	AudioConverterDispose(converter);
	[soundFile close];
}

- (void)setPlaybackStatus:(int)s
{
	playbackStatus = s;

	[self sendPortMessage:kCogStatusUpdateMessage withData:&s ofSize:(sizeof(int))];
}

- (void)pause
{
	[self stopAudioOutput];

	oldPlaybackStatus = playbackStatus;
	[self setPlaybackStatus:kCogStatusPaused];

	[self stopPositionTimer];
}

- (void)resume
{
	[self setPlaybackStatus:oldPlaybackStatus];
	
	[self startAudioOutput];

	[self startPositionTimer];
}

- (void)stop
{
	[self stopAudioOutput];
	DBLog(@"Audio output stopped");
	[self resetBuffer];
	DBLog(@"BUFFERS RESET");
	
	[self setPlaybackStatus:kCogStatusStopped];

//	DBLog(@"HERE? PORT CONFLICT...FUCK");
	unsigned long pos = 0;
	[self sendPortMessage:kCogPositionUpdateMessage withData:&pos ofSize:(sizeof(unsigned long))];
//	DBLog(@"THIS IS UBER SHITE: %@", positionTimer);
	
	[self stopPositionTimer];
	
	//	DBLog(@"INVALIDATED");
}

- (void)playFile:(NSString *)filename
{
	[self resetBuffer];
	[self stopPositionTimer];
//	[self stop];

	DBLog(@"PLAYING FILE");
	if (![self setSoundFile:filename])
	{
		DBLog(@"NOT PLAYING FILE");
		[self stop];

		return;
	}
	
	DBLog(@"DONT LIKE THIS, HUH?");

	[readLock lock];
	unsigned long length = totalLength;
	int bitrate = [soundFile bitRate];
	[readLock unlock];
	
	double time = [self calculateTime:length];
	
	[self sendPortMessage:kCogLengthUpdateMessage withData:&time ofSize:(sizeof(double))];
	[self sendPortMessage:kCogBitrateUpdateMessage withData:&bitrate ofSize:(sizeof(int))];
	
	[self setPlaybackStatus:kCogStatusPlaying];

	[self fillBuffer:self];
	[self startAudioOutput];

	[self startPositionTimer];	
}

- (void)changeFile:(NSString *)filename
{
	if ([self setSoundFile:filename])
		[self fireFillTimer];
}

- (void)resetBuffer
{
	[writeLock lock];
	[readLock lock];
	
	[ringBuffer empty];
	[auxRingBuffer empty];
	
	readRingBuffer = ringBuffer;
	writeRingBuffer = ringBuffer;
	
	currentPosition = 0;
	
	[readLock unlock];
	[writeLock unlock];
}

- (BOOL)setSoundFile:(NSString *)filename
{
	[self cleanUpSoundFile];
	[soundFile release];
	
	//GO THROUGH HELLA SHIT TO DETERMINE FILE...NEED TO MAKE SOME KIND OF REGISTERING MECHANISM
	soundFile = [SoundFile open:filename];
	if (!soundFile)
	{
		DBLog(@"NEW SONG SETSOUNDFILE");
		[self sendPortMessage:kCogFileChangedMessage];
		[self setPlaybackStatus:kCogStatusEndOfPlaylist];
//		[self sendPortMessage:kCogRequestNextFileMessage];
		return NO;
	}

//	DBLog(@"File opened: %s", [filename UTF8String]);
	[self prepareSoundFile];
	
	return YES;
}

- (void)setVolume:(float)v
{
//	DBLog(@"Setting volume to: %f", v);
	//Get the current stream format of the output
	OSStatus err = AudioUnitSetParameter (outputUnit,
								kHALOutputParam_Volume,
								kAudioUnitScope_Global,
								0,
								v * 0.01f,
								0);
}


@end
