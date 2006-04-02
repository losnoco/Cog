//
//  ConverterNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "ConverterNode.h"

#define BUFFER_SIZE 512 * 1024
#define CHUNK_SIZE 16 * 1024


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

@implementation ConverterNode

//called from the complexfill when the audio is converted...good clean fun
static OSStatus ACInputProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription** outDataPacketDescription, void* inUserData)
{
	ConverterNode *converter = (ConverterNode *)inUserData;
	id previousNode = [converter previousNode];
	OSStatus err = noErr;
	void *readPtr;
	int amountToWrite;
	int availInput;
	int amountRead;
	
	if ([converter shouldContinue] == NO || [converter endOfStream] == YES)
	{
//		NSLog(@"END OF STREAM IN CONV");
		ioData->mBuffers[0].mDataByteSize = 0; 
		*ioNumberDataPackets = 0;

		return noErr;
	}
	
	amountToWrite = (*ioNumberDataPackets)*(converter->inputFormat.mBytesPerPacket);

	if (converter->callbackBuffer != NULL)
		free(converter->callbackBuffer);
	converter->callbackBuffer = malloc(amountToWrite);
	
	amountRead = [converter readData:converter->callbackBuffer amount:amountToWrite];
/*	if ([converter endOfStream] == YES)
	{
		ioData->mBuffers[0].mDataByteSize = 0; 
		*ioNumberDataPackets = 0;
		
		return noErr;
	}
*/	if (amountRead == 0)
	{
		ioData->mBuffers[0].mDataByteSize = 0; 
		*ioNumberDataPackets = 0;
		
		return 100; //Keep asking for data
	}

	/*	
	availInput = [[previousNode buffer] lengthAvailableToReadReturningPointer:&readPtr];
	if (availInput == 0 )
	{
//		NSLog(@"0 INPUT");
		ioData->mBuffers[0].mDataByteSize = 0; 
		*ioNumberDataPackets = 0;
		
		if ([previousNode endOfStream] == YES)
		{
			NSLog(@"END OF CONVERTER INPUT");
			[converter setEndOfStream:YES];
			[converter setShouldContinue:NO];
			
			return noErr;
		}
		
		return 100; //Keep asking for data
	}
		
	if (amountToWrite > availInput)
		amountToWrite = availInput;
	
	*ioNumberDataPackets = amountToWrite/(converter->inputFormat.mBytesPerPacket);
	
	if (converter->callbackBuffer != NULL)
		free(converter->callbackBuffer);
	converter->callbackBuffer = malloc(amountToWrite);
	memcpy(converter->callbackBuffer, readPtr, amountToWrite);

	if (amountToWrite > 0)
	{
		[[previousNode buffer] didReadLength:amountToWrite];
		[[previousNode semaphore] signal];
	}
*/
//	NSLog(@"Amount read: %@ %i", converter, amountRead);
	ioData->mBuffers[0].mData = converter->callbackBuffer;
	ioData->mBuffers[0].mDataByteSize = amountRead;
	ioData->mBuffers[0].mNumberChannels = (converter->inputFormat.mChannelsPerFrame);
	ioData->mNumberBuffers = 1;
	
	return err;
}

-(void)process
{
	char writeBuf[CHUNK_SIZE];
	int amountConverted;
	
	
	while ([self shouldContinue] == YES) //Need to watch EOS somehow....
	{
		amountConverted = [self convert:writeBuf amount:CHUNK_SIZE];

//		NSLog(@"Amount converted %@: %i %i", self, amountConverted, [self endOfStream]);
		if (amountConverted == 0 && [self endOfStream] == YES)
		{
//			NSLog(@"END OF STREAM FOR ZINE DINNER!!!!");
			return;
		}

		[self writeData:writeBuf amount:amountConverted];
	}
	
/*	void *writePtr;
	int availOutput;
	int amountConverted;
	
	while ([self shouldContinue] == YES)
	{
		
		availOutput = [buffer lengthAvailableToWriteReturningPointer:&writePtr];

		while (availOutput == 0)
		{
			[semaphore wait];
			
			if (shouldContinue == NO)
			{
				return;
			}
			
			availOutput = [buffer lengthAvailableToWriteReturningPointer:&writePtr];
		}
		
		amountConverted = [self convert:writePtr amount:availOutput];
		
		if (amountConverted > 0)
			[buffer didWriteLength:amountConverted];
	}
	*/
}

- (int)convert:(void *)dest amount:(int)amount
{	
	AudioBufferList ioData;
	UInt32 ioNumberFrames;
	OSStatus err;
	
	ioNumberFrames = amount/outputFormat.mBytesPerFrame;
	ioData.mBuffers[0].mData = dest;
	ioData.mBuffers[0].mDataByteSize = amount;
	ioData.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
	ioData.mNumberBuffers = 1;
	
	err = AudioConverterFillComplexBuffer(converter, ACInputProc, self, &ioNumberFrames, &ioData, NULL);
	if (err == kAudioConverterErr_InvalidInputSize) //It returns insz at EOS at times...so run it again to make sure all data is converted
	{
		return [self convert:dest amount:amount];
	}
	if (err != noErr)
		NSLog(@"Converter error: %i", err);

	return ioData.mBuffers[0].mDataByteSize;
/*
	void *readPtr;
	int availInput;

	availInput = [[previousLink buffer] lengthAvailableToReadReturningPointer:&readPtr];
//	NSLog(@"AMOUNT: %i %i", amount, availInput);

	if (availInput == 0)
	{
		if ([previousLink endOfInput] == YES)
		{
			endOfInput = YES;
			NSLog(@"EOI");
			shouldContinue = NO;
			return 0;
		}
	}
	
	
	if (availInput < amount)
		amount = availInput;

	memcpy(dest, readPtr, amount);

	if (amount > 0)
	{
//		NSLog(@"READ: %i", amount);
		[[previousLink buffer] didReadLength:amount];
		[[previousLink semaphore] signal];
	}
	
	return amount;
 */
}

- (void)setupWithInputFormat:(AudioStreamBasicDescription)inf outputFormat:(AudioStreamBasicDescription)outf
{
	//Make the converter
	OSStatus stat = noErr;
	
	inputFormat = inf;
	outputFormat = outf;

	stat = AudioConverterNew ( &inputFormat, &outputFormat, &converter);
	if (stat != noErr)
	{
		DBLog(@"Error creating converter %i", stat);
	}	
	
	if (inputFormat.mChannelsPerFrame == 1)
	{
		SInt32 channelMap[2] = { 0, 0 };
		
		stat = AudioConverterSetProperty(converter,kAudioConverterChannelMap,sizeof(channelMap),channelMap);
		if (stat != noErr)
		{
			DBLog(@"Error mapping channels %i", stat);
		}	
	}
	
	//	DBLog(@"Created converter");
	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);
}

- (void)cleanUp
{
	AudioConverterDispose(converter);
}

@end
