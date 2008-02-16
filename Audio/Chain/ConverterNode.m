//
//  ConverterNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "ConverterNode.h"

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
	OSStatus err = noErr;
	int amountToWrite;
	int amountRead;
	
	if ([converter shouldContinue] == NO || [converter endOfStream] == YES)
	{
		ioData->mBuffers[0].mDataByteSize = 0; 
		*ioNumberDataPackets = 0;

		return noErr;
	}
	
	amountToWrite = (*ioNumberDataPackets)*(converter->inputFormat.mBytesPerPacket);

	if (converter->callbackBuffer != NULL)
		free(converter->callbackBuffer);
	converter->callbackBuffer = malloc(amountToWrite);

	amountRead = [converter readData:converter->callbackBuffer amount:amountToWrite];
	if (amountRead == 0 && [converter endOfStream] == NO)
	{
		ioData->mBuffers[0].mDataByteSize = 0; 
		*ioNumberDataPackets = 0;
		
		return 100; //Keep asking for data
	}

	ioData->mBuffers[0].mData = converter->callbackBuffer;
	ioData->mBuffers[0].mDataByteSize = amountRead;
	ioData->mBuffers[0].mNumberChannels = (converter->inputFormat.mChannelsPerFrame);
	ioData->mNumberBuffers = 1;
	
	return err;
}

-(void)process
{
	char writeBuf[CHUNK_SIZE];	
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO) //Need to watch EOS somehow....
	{
		int amountConverted = [self convert:writeBuf amount:CHUNK_SIZE];
		[self writeData:writeBuf amount:amountConverted];
	}
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
	int amountRead = ioData.mBuffers[0].mDataByteSize;
	if (err == kAudioConverterErr_InvalidInputSize) //It returns insz at EOS at times...so run it again to make sure all data is converted
	{
		NSLog(@"INSIZE: %i", amountRead);
		amountRead += [self convert:dest + amountRead amount:amount - amountRead];
	}
	else if (err != noErr && err != 100) {
		NSLog(@"Error: %i", err);
	}
	
	return amountRead;
}

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inf outputFormat:(AudioStreamBasicDescription)outf
{
	//Make the converter
	OSStatus stat = noErr;
	
	inputFormat = inf;
	outputFormat = outf;

	stat = AudioConverterNew ( &inputFormat, &outputFormat, &converter);
	if (stat != noErr)
	{
		NSLog(@"Error creating converter %i", stat);
	}	
	
	if (inputFormat.mChannelsPerFrame == 1)
	{
		SInt32 channelMap[2] = { 0, 0 };
		
		stat = AudioConverterSetProperty(converter,kAudioConverterChannelMap,sizeof(channelMap),channelMap);
		if (stat != noErr)
		{
			NSLog(@"Error mapping channels %i", stat);
		}	
	}
	
	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);
	
	return YES;
}

- (void)dealloc
{
	NSLog(@"Decoder dealloc");
	[self cleanUp];
	[super dealloc];
}


- (void)setOutputFormat:(AudioStreamBasicDescription)format
{
	NSLog(@"SETTING OUTPUT FORMAT!");
	outputFormat = format;
}

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format
{
	NSLog(@"FORMAT CHANGED");
	[self cleanUp];
	[self setupWithInputFormat:format outputFormat:outputFormat];
}

- (void)cleanUp
{
	if (converter)
	{
		AudioConverterDispose(converter);
		converter = NULL;
	}
	if (callbackBuffer) {
		free(callbackBuffer);
		callbackBuffer = NULL;
	}
}

@end
