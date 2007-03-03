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

	ioData->mBuffers[0].mData = converter->inputBuffer;
	ioData->mBuffers[0].mDataByteSize = converter->inputBufferSize;
	ioData->mBuffers[0].mNumberChannels = (converter->inputFormat.mChannelsPerFrame);
	ioData->mNumberBuffers = 1;
	
	return err;
}

- (int)convert:(int)inputSize
{	
	AudioBufferList ioData;
	UInt32 ioNumberFrames;

	OSStatus err;
	
	ioNumberFrames = outputSize/outputFormat.mBytesPerFrame;
	ioData.mBuffers[0].mData = outputBuffer;
	ioData.mBuffers[0].mDataByteSize = outputSize;
	ioData.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
	ioData.mNumberBuffers = 1;
	
	inputBufferSize = inputSize;
	
	err = AudioConverterFillComplexBuffer(converter, ACInputProc, self, &ioNumberFrames, &ioData, NULL);
	if (err == kAudioConverterErr_InvalidInputSize) //It returns insz at EOS at times...so run it again to make sure all data is converted
	{
		return [self convert:inputSize];
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
	maxInputSize = outputSize;
    UInt32 dataSize = sizeof(maxInputSize);
    AudioConverterGetProperty(converter,
									kAudioConverterPropertyCalculateInputBufferSize,
									&dataSize,
									(void*)&maxInputSize);
	
	if (outputBuffer)
		free(outputBuffer);
	outputBuffer = malloc(outputSize);
	
	if (inputBuffer)
		free(inputBuffer);
	inputBuffer = malloc(maxInputSize);
	
	NSLog(@"Converter setup!");

	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);
}

- (int)maxInputSize
{
	return maxInputSize;
}

- (void *)outputBuffer
{
	return outputBuffer;
}

- (void *)inputBuffer
{
	return inputBuffer;
}

- (void)cleanUp
{
	if (inputBuffer)
		free(inputBuffer);
	if (outputBuffer)
		free(outputBuffer);
	
	AudioConverterDispose(converter);
}

@end
