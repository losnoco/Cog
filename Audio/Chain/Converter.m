//
//  ConverterNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "Converter.h"
#import "Node.h"

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

@implementation Converter

//called from the complexfill when the audio is converted...good clean fun
static OSStatus ACInputProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription** outDataPacketDescription, void* inUserData)
{
	Converter *converter = (Converter *)inUserData;
	OSStatus err = noErr;

	if (converter->inputBufferSize > 0) {
		int amountConverted = *ioNumberDataPackets * converter->inputFormat.mBytesPerPacket;
		if (amountConverted > converter->inputBufferSize) {
			amountConverted = converter->inputBufferSize;
		}
		
		ioData->mBuffers[0].mData = converter->inputBuffer;
		ioData->mBuffers[0].mDataByteSize = amountConverted;
		ioData->mBuffers[0].mNumberChannels = (converter->inputFormat.mChannelsPerFrame);
		ioData->mNumberBuffers = 1;

		*ioNumberDataPackets = amountConverted / converter->inputFormat.mBytesPerPacket;
		
		converter->inputBufferSize -= amountConverted;
		converter->inputBuffer = ((char *)converter->inputBuffer) + amountConverted;
	}
	else {
		ioData->mBuffers[0].mData = NULL;
		ioData->mBuffers[0].mDataByteSize = 0;
		ioData->mNumberBuffers = 1;
		*ioNumberDataPackets = 0;
		
		//Reset the converter's internal bufferrs.
		converter->needsReset = YES;
	}
	
	return err;
}

- (int)convert:(void *)input amount:(int)inputSize
{	
	AudioBufferList ioData;
	UInt32 ioNumberFrames;

	if (inputSize <= 0) {
		return inputSize;
	}

	OSStatus err;
	
	needsReset = NO;
	
	ioNumberFrames = inputSize/inputFormat.mBytesPerFrame;
	ioData.mBuffers[0].mData = outputBuffer;
	ioData.mBuffers[0].mDataByteSize = outputSize;
	ioData.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
	ioData.mNumberBuffers = 1;
	inputBuffer = input;
	inputBufferSize = inputSize;
	
	err = AudioConverterFillComplexBuffer(converter, ACInputProc, self, &ioNumberFrames, &ioData, NULL);
	if (err != noErr || needsReset) //It returns insz at EOS at times...so run it again to make sure all data is converted
	{
		AudioConverterReset(converter);
	}

	return ioData.mBuffers[0].mDataByteSize;
}

- (void)setupWithInputFormat:(AudioStreamBasicDescription)inf outputFormat:(AudioStreamBasicDescription)outf
{
	NSLog(@"CREATING THE CONVERTER");
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
	
	outputSize = CHUNK_SIZE;
    UInt32 dataSize = sizeof(outputSize);
    AudioConverterGetProperty(converter,
									kAudioConverterPropertyCalculateOutputBufferSize,
									&dataSize,
									(void*)&outputSize);
	NSLog(@"Output size: %i %i", outputSize, CHUNK_SIZE);
	if (outputBuffer)
	{
		NSLog(@"FREEING");
		free(outputBuffer);
		NSLog(@"FREED");
	}
	outputBuffer = malloc(outputSize);
	
	
	NSLog(@"Converter setup!");

	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);
}


- (void *)outputBuffer
{
	return outputBuffer;
}

- (void)cleanUp
{
	if (outputBuffer) {
		free(outputBuffer);
		outputBuffer = NULL;
	}
	AudioConverterDispose(converter);
}

@end
