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

static void scale_by_volume(float * buffer, int count, float volume)
{
    if ( volume != 1.0 )
        for (int i = 0; i < count; ++i )
            buffer[i] *= volume;
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

static OSStatus ACFloatProc(AudioConverterRef inAudioConverter, UInt32* ioNumberDataPackets, AudioBufferList* ioData, AudioStreamPacketDescription** outDataPacketDescription, void* inUserData)
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
	
	amountToWrite = (*ioNumberDataPackets)*(converter->floatFormat.mBytesPerPacket);

    if ( amountToWrite + converter->floatOffset > converter->floatSize )
        amountToWrite = converter->floatSize - converter->floatOffset;
    
	ioData->mBuffers[0].mData = converter->floatBuffer + converter->floatOffset;
	ioData->mBuffers[0].mDataByteSize = amountToWrite;
	ioData->mBuffers[0].mNumberChannels = (converter->floatFormat.mChannelsPerFrame);
	ioData->mNumberBuffers = 1;
	
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
	UInt32 ioNumberFrames;
	OSStatus err;
    int amountRead = 0;
	
    if (floatOffset == floatSize) {
        ioNumberFrames = amount / outputFormat.mBytesPerFrame;
            
        floatBuffer = realloc( floatBuffer, ioNumberFrames * floatFormat.mBytesPerFrame );
        ioData.mBuffers[0].mData = floatBuffer;
        ioData.mBuffers[0].mDataByteSize = ioNumberFrames * floatFormat.mBytesPerFrame;
        ioData.mBuffers[0].mNumberChannels = floatFormat.mChannelsPerFrame;
        ioData.mNumberBuffers = 1;
            
    tryagain:
        err = AudioConverterFillComplexBuffer(converterFloat, ACInputProc, self, &ioNumberFrames, &ioData, NULL);
        amountRead += ioData.mBuffers[0].mDataByteSize;
        if (err == 100)
        {
            DLog(@"INSIZE: %i", amountRead);
            ioData.mBuffers[0].mData = floatBuffer + amountRead;
            ioNumberFrames = ( amount / outputFormat.mBytesPerFrame ) - ( amountRead / floatFormat.mBytesPerFrame );
            ioData.mBuffers[0].mDataByteSize = ioNumberFrames * floatFormat.mBytesPerFrame;
            goto tryagain;
        }
        else if (err != noErr && err != kAudioConverterErr_InvalidInputSize)
        {
            DLog(@"Error: %i", err);
            return amountRead;
        }
        
        if ( inputFormat.mChannelsPerFrame > 2 && outputFormat.mChannelsPerFrame == 2 )
            downmix_to_stereo( (float*) floatBuffer, inputFormat.mChannelsPerFrame, amountRead / floatFormat.mBytesPerFrame );
        
        scale_by_volume( (float*) floatBuffer, amountRead / sizeof(float), volumeScale);
        
        floatSize = amountRead;
        floatOffset = 0;
    }

    ioNumberFrames = amount / outputFormat.mBytesPerFrame;
    ioData.mBuffers[0].mData = dest;
    ioData.mBuffers[0].mDataByteSize = amount;
    ioData.mBuffers[0].mNumberChannels = outputFormat.mChannelsPerFrame;
    ioData.mNumberBuffers = 1;

    amountRead = 0;
        
tryagain2:
    err = AudioConverterFillComplexBuffer(converter, ACFloatProc, self, &ioNumberFrames, &ioData, NULL);
    amountRead += ioData.mBuffers[0].mDataByteSize;
    if (err == 100)
    {
        DLog(@"INSIZE: %i", amountRead);
        ioData.mBuffers[0].mData = dest + amountRead;
        ioNumberFrames = ( amount - amountRead ) / outputFormat.mBytesPerFrame;
        ioData.mBuffers[0].mDataByteSize = ioNumberFrames * outputFormat.mBytesPerFrame;
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
    
    stat = AudioConverterNew( &inputFormat, &floatFormat, &converterFloat );
    if (stat != noErr)
    {
        ALog(@"Error creating converter %i", stat);
        return NO;
    }
    
    stat = AudioConverterNew ( &floatFormat, &outputFormat, &converter );
    if (stat != noErr)
    {
        ALog(@"Error creating converter %i", stat);
        return NO;
    }

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
	else if (inputFormat.mChannelsPerFrame == 1)
	{
		SInt32 channelMap[2] = { 0, 0 };
		
		stat = AudioConverterSetProperty(converter,kAudioConverterChannelMap,sizeof(channelMap),channelMap);
		if (stat != noErr)
		{
			ALog(@"Error mapping channels %i", stat);
            return NO;
		}	
	}
	
	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);

    [self refreshVolumeScaling];
    
	return YES;
}

- (void)dealloc
{
	DLog(@"Decoder dealloc");

    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.volumeScaling"];
    
	[self cleanUp];
	[super dealloc];
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
    [rgInfo release];
    [rgi retain];
    rgInfo = rgi;
    [self refreshVolumeScaling];
}

- (void)cleanUp
{
    [rgInfo release];
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
    }
	if (callbackBuffer) {
		free(callbackBuffer);
		callbackBuffer = NULL;
	}
    floatOffset = 0;
    floatSize = 0;
}

@end
