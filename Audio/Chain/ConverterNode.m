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
	printf ("  Format Flags:%X\n", inDesc->mFormatFlags);
	printf ("  Bytes per Packet:%d\n", inDesc->mBytesPerPacket);
	printf ("  Frames per Packet:%d\n", inDesc->mFramesPerPacket);
	printf ("  Bytes per Frame:%d\n", inDesc->mBytesPerFrame);
	printf ("  Channels per Frame:%d\n", inDesc->mChannelsPerFrame);
	printf ("  Bits per Channel:%d\n", inDesc->mBitsPerChannel);
	printf ("- - - - - - - - - - - - - - - - - - - -\n");
}

@implementation ConverterNode

static const float STEREO_DOWNMIX[8-2][8][2]={
    /*3.0*/
    {
        {0.5858F,0.0F},{0.0F,0.5858F},{0.4142F,0.4142F}
    },
    /*quadrophonic*/
    {
        {0.4226F,0.0F},{0.0F,0.4226F},{0.366F,0.2114F},{0.2114F,0.336F}
    },
    /*5.0*/
    {
        {0.651F,0.0F},{0.0F,0.651F},{0.46F,0.46F},{0.5636F,0.3254F},
        {0.3254F,0.5636F}
    },
    /*5.1*/
    {
        {0.529F,0.0F},{0.0F,0.529F},{0.3741F,0.3741F},{0.3741F,0.3741F},{0.4582F,0.2645F},
        {0.2645F,0.4582F}
    },
    /*6.1*/
    {
        {0.4553F,0.0F},{0.0F,0.4553F},{0.322F,0.322F},{0.322F,0.322F},{0.3943F,0.2277F},
        {0.2277F,0.3943F},{0.2788F,0.2788F}
    },
    /*7.1*/
    {
        {0.3886F,0.0F},{0.0F,0.3886F},{0.2748F,0.2748F},{0.2748F,0.2748F},{0.3366F,0.1943F},
        {0.1943F,0.3366F},{0.3366F,0.1943F},{0.1943F,0.3366F}
    }
};

static void downmix_to_stereo(float * buffer, int channels, int count)
{
    if (channels >= 3 && channels < 8)
    for (int i = 0; i < count; ++i)
    {
        float left = 0, right = 0;
        for (int j = 0; j < channels; ++j)
        {
            left += buffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][0];
            right += buffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][1];
        }
        buffer[i * channels + 0] = left;
        buffer[i * channels + 1] = right;
    }
}

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

static OSStatus ACDownmixProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription** outDataPacketDescription, void* inUserData)
{
	ConverterNode *converter = (ConverterNode *)inUserData;
	OSStatus err = noErr;
	int amountToWrite;
	
	if ([converter shouldContinue] == NO || [converter endOfStream] == YES)
	{
		ioData->mBuffers[0].mDataByteSize = 0;
		*ioNumberDataPackets = 0;
        
		return noErr;
	}
	
	amountToWrite = (*ioNumberDataPackets)*(converter->downmixFormat.mBytesPerPacket);

    if ( amountToWrite + converter->downmixOffset > converter->downmixSize )
        amountToWrite = converter->downmixSize - converter->downmixOffset;
    
	ioData->mBuffers[0].mData = converter->downmixBuffer + converter->downmixOffset;
	ioData->mBuffers[0].mDataByteSize = amountToWrite;
	ioData->mBuffers[0].mNumberChannels = (converter->downmixFormat.mChannelsPerFrame);
	ioData->mNumberBuffers = 1;
	
    converter->downmixOffset += amountToWrite;
    
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
    int amountRead = 0;
	
    if ( converterDownmix )
    {
        if (downmixOffset == downmixSize) {
            ioNumberFrames = amount / outputFormat.mBytesPerFrame;
            
            downmixBuffer = realloc( downmixBuffer, ioNumberFrames * downmixFormat.mBytesPerFrame );
            ioData.mBuffers[0].mData = downmixBuffer;
            ioData.mBuffers[0].mDataByteSize = ioNumberFrames * downmixFormat.mBytesPerFrame;
            ioData.mBuffers[0].mNumberChannels = downmixFormat.mChannelsPerFrame;
            ioData.mNumberBuffers = 1;
            
        tryagain:
            err = AudioConverterFillComplexBuffer(converter, ACInputProc, self, &ioNumberFrames, &ioData, NULL);
            amountRead += ioData.mBuffers[0].mDataByteSize;
            if (err == 100)
            {
                NSLog(@"INSIZE: %i", amountRead);
                ioData.mBuffers[0].mData = downmixBuffer + amountRead;
                ioNumberFrames = ( amount / outputFormat.mBytesPerFrame ) - ( amountRead / downmixFormat.mBytesPerFrame );
                ioData.mBuffers[0].mDataByteSize = ioNumberFrames * downmixFormat.mBytesPerFrame;
                goto tryagain;
            }
            else if (err != noErr)
            {
                NSLog(@"Error: %i", err);
            }
            
            downmix_to_stereo( (float*) downmixBuffer, downmixFormat.mChannelsPerFrame, amountRead / downmixFormat.mBytesPerFrame );
        
            downmixSize = amountRead;
            downmixOffset = 0;
        }
        
        ioNumberFrames = amount / outputFormat.mBytesPerFrame;
        ioData.mBuffers[0].mData = dest;
        ioData.mBuffers[0].mDataByteSize = amount;
        ioData.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
        ioData.mNumberBuffers = 1;

        amountRead = 0;
        
    tryagain2:
        err = AudioConverterFillComplexBuffer(converterDownmix,  ACDownmixProc, self, &ioNumberFrames, &ioData, NULL);
        amountRead += ioData.mBuffers[0].mDataByteSize;
        if (err == 100)
        {
            NSLog(@"INSIZE: %i", amountRead);
            ioData.mBuffers[0].mData = dest + amountRead;
            ioNumberFrames = ( amount - amountRead ) / outputFormat.mBytesPerFrame;
            ioData.mBuffers[0].mDataByteSize = ioNumberFrames * outputFormat.mBytesPerFrame;
            goto tryagain2;
        }
        else if (err != noErr && err != kAudioConverterErr_InvalidInputSize)
        {
            NSLog(@"Error: %i", err);
        }
    }
    else
    {
        ioNumberFrames = amount/outputFormat.mBytesPerFrame;
        ioData.mBuffers[0].mData = dest;
        ioData.mBuffers[0].mDataByteSize = amount;
        ioData.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
        ioData.mNumberBuffers = 1;
	
    tryagain3:
        err = AudioConverterFillComplexBuffer(converter, ACInputProc, self, &ioNumberFrames, &ioData, NULL);
        amountRead += ioData.mBuffers[0].mDataByteSize;
        if (err == 100) //It returns insz at EOS at times...so run it again to make sure all data is converted
        {
            NSLog(@"INSIZE: %i", amountRead);
            ioData.mBuffers[0].mData = dest + amountRead;
            ioNumberFrames = ( amount - amountRead ) / outputFormat.mBytesPerFrame;
            ioData.mBuffers[0].mDataByteSize = ioNumberFrames * outputFormat.mBytesPerFrame;
            goto tryagain3;
        }
        else if (err != noErr) {
            NSLog(@"Error: %i", err);
        }
    }
	
	return amountRead;
}

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inf outputFormat:(AudioStreamBasicDescription)outf
{
	//Make the converter
	OSStatus stat = noErr;
	
	inputFormat = inf;
	outputFormat = outf;

    if (inputFormat.mChannelsPerFrame > 2 && outputFormat.mChannelsPerFrame == 2)
    {
        downmixFormat = inputFormat;
        downmixFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
        downmixFormat.mBitsPerChannel = 32;
        downmixFormat.mBytesPerFrame = (32/8)*downmixFormat.mChannelsPerFrame;
        downmixFormat.mBytesPerPacket = downmixFormat.mBytesPerFrame * downmixFormat.mFramesPerPacket;
        stat = AudioConverterNew( &inputFormat, &downmixFormat, &converter );
        if (stat != noErr)
        {
            NSLog(@"Error creating converter %i", stat);
        }
        stat = AudioConverterNew ( &downmixFormat, &outputFormat, &converterDownmix );
        if (stat != noErr)
        {
            NSLog(@"Error creating converter %i", stat);
        }
            
        SInt32 channelMap[2] = { 0, 1 };
        
		stat = AudioConverterSetProperty(converterDownmix,kAudioConverterChannelMap,sizeof(channelMap),channelMap);
		if (stat != noErr)
		{
			NSLog(@"Error mapping channels %i", stat);
		}
    }
    else
    {

        stat = AudioConverterNew ( &inputFormat, &outputFormat, &converter);
        if (stat != noErr)
        {
            NSLog(@"Error creating converter %i", stat);
        }
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
    if (converterDownmix)
    {
        AudioConverterDispose(converterDownmix);
        converterDownmix = NULL;
    }
	if (converter)
	{
		AudioConverterDispose(converter);
		converter = NULL;
	}
    if (downmixBuffer)
    {
        free(downmixBuffer);
        downmixBuffer = NULL;
    }
	if (callbackBuffer) {
		free(callbackBuffer);
		callbackBuffer = NULL;
	}
}

@end
