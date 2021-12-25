//
//  ConverterNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "ConverterNode.h"

#import "Logging.h"

void PrintStreamDesc (AudioStreamBasicDescription *inDesc)
{
	if (!inDesc) {
		DLog (@"Can't print a NULL desc!\n");
		return;
	}
	DLog (@"- - - - - - - - - - - - - - - - - - - -\n");
	DLog (@"  Sample Rate:%f\n", inDesc->mSampleRate);
	DLog (@"  Format ID:%s\n", (char*)&inDesc->mFormatID);
	DLog (@"  Format Flags:%X\n", inDesc->mFormatFlags);
	DLog (@"  Bytes per Packet:%d\n", inDesc->mBytesPerPacket);
	DLog (@"  Frames per Packet:%d\n", inDesc->mFramesPerPacket);
	DLog (@"  Bytes per Frame:%d\n", inDesc->mBytesPerFrame);
	DLog (@"  Channels per Frame:%d\n", inDesc->mChannelsPerFrame);
	DLog (@"  Bits per Channel:%d\n", inDesc->mBitsPerChannel);
	DLog (@"- - - - - - - - - - - - - - - - - - - -\n");
}

@implementation ConverterNode

- (id)initWithController:(id)c previous:(id)p
{
    self = [super initWithController:c previous:p];
    if (self)
    {
        rgInfo = nil;
        
        converterFloat = NULL;
        converter = NULL;
        floatBuffer = NULL;
        floatBufferSize = 0;
        callbackBuffer = NULL;
        callbackBufferSize = 0;

        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeScaling"		options:0 context:nil];
    }
    
    return self;
}

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
    if (channels >= 3 && channels <= 8)
    for (int i = 0; i < count; ++i)
    {
        float left = 0, right = 0;
        for (int j = 0; j < channels; ++j)
        {
            left += buffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][0];
            right += buffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][1];
        }
        buffer[i * 2 + 0] = left;
        buffer[i * 2 + 1] = right;
    }
}

static void scale_by_volume(float * buffer, int count, float volume)
{
    if ( volume != 1.0 )
        for (int i = 0; i < count; ++i )
            buffer[i] *= volume;
}

//called from the complexfill when the audio is converted...good clean fun
static OSStatus ACInputProc(AudioConverterRef inAudioConverter,
                            UInt32* ioNumberDataPackets,
                            AudioBufferList* ioData,
                            AudioStreamPacketDescription** outDataPacketDescription,
                            void* inUserData)
{
	ConverterNode *converter = (__bridge ConverterNode *)inUserData;
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

    if (!converter->callbackBuffer || converter->callbackBufferSize < amountToWrite)
        converter->callbackBuffer = realloc(converter->callbackBuffer, converter->callbackBufferSize = amountToWrite + 1024);

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

static OSStatus ACFloatProc(AudioConverterRef inAudioConverter,
                            UInt32* ioNumberDataPackets,
                            AudioBufferList* ioData,
                            AudioStreamPacketDescription** outDataPacketDescription,
                            void* inUserData)
{
	ConverterNode *converter = (__bridge ConverterNode *)inUserData;
	OSStatus err = noErr;
	int amountToWrite;
	
	if ([converter shouldContinue] == NO)
	{
		ioData->mBuffers[0].mDataByteSize = 0;
		*ioNumberDataPackets = 0;
        
		return noErr;
	}
	
    amountToWrite = (*ioNumberDataPackets) * (converter->dmFloatFormat.mBytesPerPacket);

    if ( amountToWrite + converter->floatOffset > converter->floatSize )
    {
        amountToWrite = converter->floatSize - converter->floatOffset;
        *ioNumberDataPackets = amountToWrite / (converter->dmFloatFormat.mBytesPerPacket);
    }
    
	ioData->mBuffers[0].mData = converter->floatBuffer + converter->floatOffset;
	ioData->mBuffers[0].mDataByteSize = amountToWrite;
	ioData->mBuffers[0].mNumberChannels = (converter->dmFloatFormat.mChannelsPerFrame);
	ioData->mNumberBuffers = 1;
    
    if (amountToWrite == 0)
        return 100;
	
    converter->floatOffset += amountToWrite;
    
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
	UInt32 ioNumberPackets;
	OSStatus err;
    int amountReadFromFC;
    int amountRead = 0;
    
tryagain2:
    amountReadFromFC = 0;
	
    if (floatOffset == floatSize) {
        UInt32 ioWantedNumberPackets;
        
        ioNumberPackets = amount / outputFormat.mBytesPerPacket;
        
        ioNumberPackets = (UInt32)((float)ioNumberPackets * sampleRatio);
        ioNumberPackets = (ioNumberPackets + 255) & ~255;
        
        ioWantedNumberPackets = ioNumberPackets;
        
        size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
        if (!floatBuffer || floatBufferSize < newSize)
            floatBuffer = realloc( floatBuffer, floatBufferSize = newSize + 1024 );
        ioData.mBuffers[0].mData = floatBuffer;
        ioData.mBuffers[0].mDataByteSize = ioNumberPackets * floatFormat.mBytesPerPacket;
        ioData.mBuffers[0].mNumberChannels = floatFormat.mChannelsPerFrame;
        ioData.mNumberBuffers = 1;
            
    tryagain:
        err = AudioConverterFillComplexBuffer(converterFloat, ACInputProc, (__bridge void * _Nullable)(self), &ioNumberPackets, &ioData, NULL);
        amountReadFromFC += ioNumberPackets * floatFormat.mBytesPerPacket;
        if (err == 100)
        {
            ioData.mBuffers[0].mData = (void *)(((uint8_t*)floatBuffer) + amountReadFromFC);
            ioNumberPackets = ioWantedNumberPackets - ioNumberPackets;
            ioWantedNumberPackets = ioNumberPackets;
            ioData.mBuffers[0].mDataByteSize = ioNumberPackets * floatFormat.mBytesPerPacket;
            usleep(10000);
            goto tryagain;
        }
        else if (err != noErr && err != kAudioConverterErr_InvalidInputSize)
        {
            DLog(@"Error: %i", err);
            return amountRead;
        }
        
        if ( inputFormat.mChannelsPerFrame > 2 && outputFormat.mChannelsPerFrame == 2 )
        {
            int samples = amountReadFromFC / floatFormat.mBytesPerFrame;
            downmix_to_stereo( (float*) floatBuffer, inputFormat.mChannelsPerFrame, samples );
            amountReadFromFC = samples * sizeof(float) * 2;
        }
        
        scale_by_volume( (float*) floatBuffer, amountReadFromFC / sizeof(float), volumeScale);
        
        floatSize = amountReadFromFC;
        floatOffset = 0;
    }

    ioNumberPackets = amount / outputFormat.mBytesPerPacket;
    ioData.mBuffers[0].mData = dest + amountRead;
    ioData.mBuffers[0].mDataByteSize = amount - amountRead;
    ioData.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
    ioData.mNumberBuffers = 1;

    err = AudioConverterFillComplexBuffer(converter, ACFloatProc, (__bridge void *)(self), &ioNumberPackets, &ioData, NULL);
    amountRead += ioNumberPackets * outputFormat.mBytesPerPacket;
    if (err == 100)
    {
        goto tryagain2;
    }
    else if (err != noErr && err != kAudioConverterErr_InvalidInputSize)
    {
        DLog(@"Error: %i", err);
    }
	
	return amountRead;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
	DLog(@"SOMETHING CHANGED!");
    if ([keyPath isEqual:@"values.volumeScaling"]) {
        //User reset the volume scaling option
        [self refreshVolumeScaling];
    }
}

static float db_to_scale(float db)
{
    return pow(10.0, db / 20);
}

- (void)refreshVolumeScaling
{
    if (rgInfo == nil)
    {
        volumeScale = 1.0;
        return;
    }
    
    NSString * scaling = [[NSUserDefaults standardUserDefaults] stringForKey:@"volumeScaling"];
    BOOL useAlbum = [scaling hasPrefix:@"albumGain"];
    BOOL useTrack = useAlbum || [scaling hasPrefix:@"trackGain"];
    BOOL useVolume = useAlbum || useTrack || [scaling isEqualToString:@"volumeScale"];
    BOOL usePeak = [scaling hasSuffix:@"WithPeak"];
    float scale = 1.0;
    float peak = 0.0;
    if (useVolume) {
        id pVolumeScale = [rgInfo objectForKey:@"volume"];
        if (pVolumeScale != nil)
            scale = [pVolumeScale floatValue];
    }
    if (useTrack) {
        id trackGain = [rgInfo objectForKey:@"replayGainTrackGain"];
        id trackPeak = [rgInfo objectForKey:@"replayGainTrackPeak"];
        if (trackGain != nil)
            scale = db_to_scale([trackGain floatValue]);
        if (trackPeak != nil)
            peak = [trackPeak floatValue];
    }
    if (useAlbum) {
        id albumGain = [rgInfo objectForKey:@"replayGainAlbumGain"];
        id albumPeak = [rgInfo objectForKey:@"replayGainAlbumPeak"];
        if (albumGain != nil)
            scale = db_to_scale([albumGain floatValue]);
        if (albumPeak != nil)
            peak = [albumPeak floatValue];
    }
    if (usePeak) {
        if (scale * peak > 1.0)
            scale = 1.0 / peak;
    }
    volumeScale = scale;
}


- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inf outputFormat:(AudioStreamBasicDescription)outf
{
	//Make the converter
	OSStatus stat = noErr;
	
	inputFormat = inf;
	outputFormat = outf;
    
    floatFormat = inputFormat;
    floatFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    floatFormat.mBitsPerChannel = 32;
    floatFormat.mBytesPerFrame = (32/8)*floatFormat.mChannelsPerFrame;
    floatFormat.mBytesPerPacket = floatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;
    
    floatOffset = 0;
    floatSize = 0;
    
    stat = AudioConverterNew( &inputFormat, &floatFormat, &converterFloat );
    if (stat != noErr)
    {
        ALog(@"Error creating converter %i", stat);
        return NO;
    }
    
    dmFloatFormat = floatFormat;
    floatFormat.mChannelsPerFrame = outputFormat.mChannelsPerFrame;
    floatFormat.mBytesPerFrame = (32/8)*floatFormat.mChannelsPerFrame;
    floatFormat.mBytesPerPacket = floatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;
    
    stat = AudioConverterNew ( &floatFormat, &outputFormat, &converter );
    if (stat != noErr)
    {
        ALog(@"Error creating converter %i", stat);
        return NO;
    }

#if 0
    // These mappings don't do what I want, so avoid them.
    if (inputFormat.mChannelsPerFrame > 2 && outputFormat.mChannelsPerFrame == 2)
    {
        SInt32 channelMap[2] = { 0, 1 };
        
		stat = AudioConverterSetProperty(converter,kAudioConverterChannelMap,sizeof(channelMap),channelMap);
		if (stat != noErr)
		{
			ALog(@"Error mapping channels %i", stat);
            return NO;
		}
    }
    else if (inputFormat.mChannelsPerFrame > 1 && outputFormat.mChannelsPerFrame == 1)
    {
        SInt32 channelMap[1] = { 0 };
        
        stat = AudioConverterSetProperty(converter,kAudioConverterChannelMap,(int)sizeof(channelMap),channelMap);
        if (stat != noErr)
        {
            ALog(@"Error mapping channels %i", stat);
            return NO;
        }
    }
	else
#endif
    if (inputFormat.mChannelsPerFrame == 1 && outputFormat.mChannelsPerFrame > 1)
	{
		SInt32 channelMap[outputFormat.mChannelsPerFrame];
        
        memset(channelMap, 0, sizeof(channelMap));
		
		stat = AudioConverterSetProperty(converter,kAudioConverterChannelMap,(int)sizeof(channelMap),channelMap);
		if (stat != noErr)
		{
			ALog(@"Error mapping channels %i", stat);
            return NO;
		}	
	}
	
	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);

    [self refreshVolumeScaling];
    
    sampleRatio = (float)inputFormat.mSampleRate / (float)outputFormat.mSampleRate;
    
	return YES;
}

- (void)dealloc
{
	DLog(@"Decoder dealloc");

    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.volumeScaling"];
    
	[self cleanUp];
}


- (void)setOutputFormat:(AudioStreamBasicDescription)format
{
	DLog(@"SETTING OUTPUT FORMAT!");
	outputFormat = format;
}

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format
{
	DLog(@"FORMAT CHANGED");
	[self cleanUp];
	[self setupWithInputFormat:format outputFormat:outputFormat];
}

- (void)setRGInfo:(NSDictionary *)rgi
{
    DLog(@"Setting ReplayGain info");
    rgInfo = rgi;
    [self refreshVolumeScaling];
}

- (void)cleanUp
{
    rgInfo = nil;
    if (converterFloat)
    {
        AudioConverterDispose(converterFloat);
        converterFloat = NULL;
    }
	if (converter)
	{
		AudioConverterDispose(converter);
		converter = NULL;
	}
    if (floatBuffer)
    {
        free(floatBuffer);
        floatBuffer = NULL;
        floatBufferSize = 0;
    }
	if (callbackBuffer) {
		free(callbackBuffer);
		callbackBuffer = NULL;
        callbackBufferSize = 0;
	}
    floatOffset = 0;
    floatSize = 0;
}

@end
