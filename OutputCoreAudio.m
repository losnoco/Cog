//
//  OutputCoreAudio.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "OutputCoreAudio.h"


@implementation OutputCoreAudio

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

- (void)setup
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
	
	// change output format...
	///Seems some 3rd party devices return incorrect stuff...or I just don't like noninterleaved data.
	deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
	deviceFormat.mBytesPerFrame = deviceFormat.mChannelsPerFrame*(deviceFormat.mBitsPerChannel/8);
	deviceFormat.mBytesPerPacket = deviceFormat.mBytesPerFrame * deviceFormat.mFramesPerPacket;
	//	DBLog(@"stuff: %i %i %i %i", deviceFormat.mBitsPerChannel, deviceFormat.mBytesPerFrame, deviceFormat.mBytesPerPacket, deviceFormat.mFramesPerPacket);
	err = AudioUnitSetProperty (outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Output,
								0,
								&deviceFormat,
								size);
	
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

- (void)start
{
	AudioOutputUnitStart(outputUnit);
}

- (void)stop
{
	if (outputUnit)
        AudioOutputUnitStop(outputUnit);
}

@end
