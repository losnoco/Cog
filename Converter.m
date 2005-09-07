//
//  Converter.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "Converter.h"


@implementation Converter

//called from the complexfill when the audio is converted...good clean fun
static OSStatus ACInputProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription** outDataPacketDescription, void* inUserData)
{
	SoundController *soundController = (SoundController *)inUserData;
	OSStatus err = noErr;
	
	int amountToWrite;
	int amountWritten;
	
	amountToWrite = (*ioNumberDataPackets)*[[soundController input] format].mBytesPerPacket;
	
	availInput = [[[soundController input] buffer] amountAvailableToReadReturningBuffer:&readPtr];
	while (availInput == 0)
	{
		[conversionSemaphore wait];
		
		availInput = [[[soundController input] buffer] amountAvailableToReadReturningBuffer:&readPtr];
	}
	
	if (availInput > amountToWrite)
		availInput = amountToWrite;
	
	*ioNumberDataPackets = availInput/format->mBytesPerPacket;
	
	ioData->mBuffers[0].mData = readPtr;
	ioData->mBuffers[0].mDataByteSize = availInput;
	ioData->mBuffers[0].mNumberChannels = [[soundController input] format].mChannelsPerFrame;
	ioData->mNumberBuffers = 1;
	
	return err;
}

-(void)process
{
	void *writePtr;
	int availOutput;
	int amountConverted;
	
	while ([soundController shouldContinue] == YES)
	{
		[[soundController outputLock] lock];
		availOutput = [[[soundController output] buffer] amountAvailableToWriteReturningBuffer:&writePtr];
		while (availOutput == 0)
		{
			[[soundController outputLock] unlock];
			[conversionSemaphore wait];
			
			[[soundController outputLock] lock];
			availOutput = [[[soundController output] buffer] amountAvailableToWriteReturningBuffer:&writePtr];
		}
		
		amountConverted = [self convert:writePtr amount:availOutput];
		
		[[[soundController output] buffer] didWriteAmount:amountConverted];
		[[soundController outputLock] unlock];
	}
}

- (int)convert:(void *)dest amount:(int)amount
{	
	AudioBufferList ioData;
	UInt32 ioNumberFrames;
	OSStatus err;
	
	ioNumberFrames = amount/[[soundController output] format].mBytesPerFrame;
	ioData.mBuffers[0].mData = dest;
	ioData.mBuffers[0].mDataByteSize = amount;
	ioData.mBuffers[0].mNumberChannels = [[soundController output] format].mChannelsPerFrame;
	ioData.mNumberBuffers = 1;
	
	err = AudioConverterFillComplexBuffer(converter, ACInputProc, &[[soundController input] format], &ioNumberFrames, &ioData, NULL);
	if (err != noErr)
		DBLog(@"Converter error: %i", err);
	
	[[soundController inputBuffer] didReadLength:(ioNumberFrames * [[[soundController input] format].mBytesPerFrame]);
	[[soundController ioSemaphore] signal];
	
	return ioData.mBuffers[0].mDataByteSize;
}

- (void)setup
{
	//Make the converter
	OSStatus stat = noErr;
	stat = AudioConverterNew ( &sourceStreamFormat, &deviceFormat, &converter);
	//	DBLog(@"Created converter");
	if (stat != noErr)
	{
		DBLog(@"Error creating converter %i", stat);
	}	
}

- (void)cleanUp
{
	AudioConverterDispose(converter);
}

@end
