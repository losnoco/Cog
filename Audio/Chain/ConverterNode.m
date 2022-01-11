//
//  ConverterNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "ConverterNode.h"

#import "Logging.h"

#import <audio/conversion/s16_to_float.h>
#import <audio/conversion/s32_to_float.h>

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
        
        resampler = NULL;
        resampler_data = NULL;
        inputBuffer = NULL;
        inputBufferSize = 0;
        floatBuffer = NULL;
        floatBufferSize = 0;
        
        stopping = NO;
        convertEntered = NO;
        emittingSilence = NO;
        
        skipResampler = NO;
        
        latencyStarted = -1;
        latencyEaten = 0;
        latencyPostfill = NO;

        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeScaling"		options:0 context:nil];
        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputResampling" options:0 context:nil];
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
        {0.4553F,0.0F},{0.0F,0.4553F},{0.322F,0.322F},{0.322F,0.322F},{0.2788F,0.2788F},
        {0.3943F,0.2277F},{0.2277F,0.3943F}
    },
    /*7.1*/
    {
        {0.3886F,0.0F},{0.0F,0.3886F},{0.2748F,0.2748F},{0.2748F,0.2748F},{0.3366F,0.1943F},
        {0.1943F,0.3366F},{0.3366F,0.1943F},{0.1943F,0.3366F}
    }
};

static void downmix_to_stereo(float * buffer, int channels, size_t count)
{
    if (channels >= 3 && channels <= 8)
    for (size_t i = 0; i < count; ++i)
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

static void downmix_to_mono(float * buffer, int channels, size_t count)
{
    if (channels >= 3 && channels <= 8)
    {
        downmix_to_stereo(buffer, channels, count);
        channels = 2;
    }
    float invchannels = 1.0 / (float)channels;
    for (size_t i = 0; i < count; ++i)
    {
        float sample = 0;
        for (int j = 0; j < channels; ++j)
        {
            sample += buffer[i * channels + j];
        }
        buffer[i] = sample * invchannels;
    }
}

static void upmix(float * buffer, int inchannels, int outchannels, size_t count)
{
    for (ssize_t i = count - 1; i >= 0; --i)
    {
        if (inchannels == 1 && outchannels == 2)
        {
            // upmix mono to stereo
            float sample = buffer[i];
            buffer[i * 2 + 0] = sample;
            buffer[i * 2 + 1] = sample;
        }
        else if (inchannels == 1 && outchannels == 4)
        {
            // upmix mono to quad
            float sample = buffer[i];
            buffer[i * 4 + 0] = sample;
            buffer[i * 4 + 1] = sample;
            buffer[i * 4 + 2] = 0;
            buffer[i * 4 + 3] = 0;
        }
        else if (inchannels == 1 && (outchannels == 3 || outchannels >= 5))
        {
            // upmix mono to center channel
            float sample = buffer[i];
            buffer[i * outchannels + 2] = sample;
            for (int j = 0; j < 2; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
            for (int j = 3; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
        else if (inchannels == 4 && outchannels >= 5)
        {
            float fl = buffer[i * 4 + 0];
            float fr = buffer[i * 4 + 1];
            float bl = buffer[i * 4 + 2];
            float br = buffer[i * 4 + 3];
            const int skipclfe = (outchannels == 5) ? 1 : 2;
            buffer[i * outchannels + 0] = fl;
            buffer[i * outchannels + 1] = fr;
            buffer[i * outchannels + skipclfe + 2] = bl;
            buffer[i * outchannels + skipclfe + 3] = br;
            for (int j = 0; j < skipclfe; ++j)
            {
                buffer[i * outchannels + 2 + j] = 0;
            }
            for (int j = 4 + skipclfe; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
        else if (inchannels == 5 && outchannels >= 6)
        {
            float fl = buffer[i * 5 + 0];
            float fr = buffer[i * 5 + 1];
            float c = buffer[i * 5 + 2];
            float bl = buffer[i * 5 + 3];
            float br = buffer[i * 5 + 4];
            buffer[i * outchannels + 0] = fl;
            buffer[i * outchannels + 1] = fr;
            buffer[i * outchannels + 2] = c;
            buffer[i * outchannels + 3] = 0;
            buffer[i * outchannels + 4] = bl;
            buffer[i * outchannels + 5] = br;
            for (int j = 6; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
        else if (inchannels == 7 && outchannels == 8)
        {
            float fl = buffer[i * 7 + 0];
            float fr = buffer[i * 7 + 1];
            float c = buffer[i * 7 + 2];
            float lfe = buffer[i * 7 + 3];
            float sl = buffer[i * 7 + 4];
            float sr = buffer[i * 7 + 5];
            float bc = buffer[i * 7 + 6];
            buffer[i * 8 + 0] = fl;
            buffer[i * 8 + 1] = fr;
            buffer[i * 8 + 2] = c;
            buffer[i * 8 + 3] = lfe;
            buffer[i * 8 + 4] = bc;
            buffer[i * 8 + 5] = bc;
            buffer[i * 8 + 6] = sl;
            buffer[i * 8 + 7] = sr;
        }
        else
        {
            // upmix N channels to N channels plus silence the empty channels
            float samples[inchannels];
            for (int j = 0; j < inchannels; ++j)
            {
                samples[j] = buffer[i * inchannels + j];
            }
            for (int j = 0; j < inchannels; ++j)
            {
                buffer[i * outchannels + j] = samples[j];
            }
            for (int j = inchannels; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
    }
}

void scale_by_volume(float * buffer, size_t count, float volume)
{
    if ( volume != 1.0 )
        for (size_t i = 0; i < count; ++i )
            buffer[i] *= volume;
}

static void convert_u8_to_s16(int16_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i )
    {
        uint16_t sample = (input[i] << 8) | input[i];
        sample ^= 0x8080;
        output[i] = (int16_t)(sample);
    }
}

static void convert_s8_to_s16(int16_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i )
    {
        uint16_t sample = (input[i] << 8) | input[i];
        output[i] = (int16_t)(sample);
    }
}

static void convert_u16_to_s16(int16_t *buffer, size_t count)
{
    for (size_t i = 0; i < count; ++i )
    {
        buffer[i] ^= 0x8000;
    }
}

static void convert_s24_to_s32(int32_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i )
    {
        int32_t sample = (input[i * 3] << 8) | (input[i * 3 + 1] << 16) | (input[i * 3 + 2] << 24);
        output[i] = sample;
    }
}

static void convert_u24_to_s32(int32_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i )
    {
        int32_t sample = (input[i * 3] << 8) | (input[i * 3 + 1] << 16) | (input[i * 3 + 2] << 24);
        output[i] = sample ^ 0x80000000;
    }
}

static void convert_u32_to_s32(int32_t *buffer, size_t count)
{
    for (size_t i = 0; i < count; ++i )
    {
        buffer[i] ^= 0x80000000;
    }
}

static void convert_be_to_le(uint8_t *buffer, size_t bitsPerSample, size_t bytes)
{
    size_t i;
    uint8_t temp;
    bitsPerSample = (bitsPerSample + 7) / 8;
    switch (bitsPerSample) {
        case 2:
            for (i = 0; i < bytes; i += 2)
            {
                temp = buffer[1];
                buffer[1] = buffer[0];
                buffer[0] = temp;
                buffer += 2;
            }
            break;
            
        case 3:
            for (i = 0; i < bytes; i += 3)
            {
                temp = buffer[2];
                buffer[2] = buffer[0];
                buffer[0] = temp;
                buffer += 3;
            }
            break;
            
        case 4:
            for (i = 0; i < bytes; i += 4)
            {
                temp = buffer[3];
                buffer[3] = buffer[0];
                buffer[0] = temp;
                temp = buffer[2];
                buffer[2] = buffer[1];
                buffer[1] = temp;
                buffer += 4;
            }
            break;
    }
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
	UInt32 ioNumberPackets;
    int amountReadFromFC;
    int amountRead = 0;
    
    if (emittingSilence)
    {
        memset(dest, 0, amount);
        return amount;
    }
    
    if (stopping)
        return 0;
    
    convertEntered = YES;
    
tryagain:
    if (stopping || [self shouldContinue] == NO)
    {
        convertEntered = NO;
        return amountRead;
    }
    
    amountReadFromFC = 0;
    
    if (floatOffset == floatSize) // skip this step if there's still float buffered
    while (inpOffset == inpSize) {
        size_t samplesRead = 0;
        
        // Approximately the most we want on input
        ioNumberPackets = (amount - amountRead) / outputFormat.mBytesPerPacket;
        
        size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
        if (!inputBuffer || inputBufferSize < newSize)
            inputBuffer = realloc( inputBuffer, inputBufferSize = newSize * 3 );
        
        if (stopping || [self shouldContinue] == NO || [self endOfStream] == YES)
        {
            if (!skipResampler && !latencyPostfill)
            {
                ioNumberPackets = (int)resampler->latency(resampler_data);
                newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
                if (!inputBuffer || inputBufferSize < newSize)
                    inputBuffer = realloc( inputBuffer, inputBufferSize = newSize * 64);
                
                inpSize = newSize;
                inpOffset = 0;
                latencyPostfill = YES;
                break;
            }
            else
            {
                convertEntered = NO;
                return amountRead;
            }
        }

        size_t amountToWrite = ioNumberPackets * inputFormat.mBytesPerPacket;
        size_t amountToSkip = 0;
        
        if (!skipResampler)
        {
            if (latencyStarted < 0)
            {
                latencyStarted = resampler->latency(resampler_data);
            }

            if (latencyStarted)
            {
                size_t latencyToWrite = latencyStarted * inputFormat.mBytesPerPacket;
                if (latencyToWrite > amountToWrite)
                    latencyToWrite = amountToWrite;
            
                if (inputFormat.mBitsPerChannel <= 8)
                    memset(inputBuffer, 0x80, latencyToWrite);
                else
                    memset(inputBuffer, 0, latencyToWrite);

                amountToSkip = latencyToWrite;
                amountToWrite -= amountToSkip;

                latencyEaten = latencyStarted * sampleRatio;

                latencyStarted -= latencyToWrite / inputFormat.mBytesPerPacket;
            }
        }
                
        size_t bytesReadFromInput = [self readData:inputBuffer + amountToSkip amount:(int)amountToWrite] + amountToSkip;
        
        if (bytesReadFromInput &&
            (inputFormat.mFormatFlags & kAudioFormatFlagIsBigEndian))
        {
            // Time for endian swap!
            convert_be_to_le(inputBuffer, inputFormat.mBitsPerChannel, bytesReadFromInput);
        }
        
        if (bytesReadFromInput &&
            !(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat))
        {
            size_t bitsPerSample = inputFormat.mBitsPerChannel;
            BOOL isUnsigned = !(inputFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger);
            BOOL isFloat = NO;
            if (bitsPerSample <= 8) {
                samplesRead = bytesReadFromInput;
                if (isUnsigned)
                    convert_s8_to_s16(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                else
                    convert_u8_to_s16(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * 2);
                bitsPerSample = 16;
                bytesReadFromInput = samplesRead * 2;
                isUnsigned = NO;
            }
            if (bitsPerSample <= 16) {
                samplesRead = bytesReadFromInput / 2;
                if (isUnsigned)
                    convert_u16_to_s16(inputBuffer, samplesRead);
                convert_s16_to_float(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead, 1.0);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * sizeof(float));
                bitsPerSample = 32;
                bytesReadFromInput = samplesRead * sizeof(float);
                isUnsigned = NO;
                isFloat = YES;
            }
            else if (bitsPerSample <= 24) {
                samplesRead = bytesReadFromInput / 3;
                if (isUnsigned)
                    convert_u24_to_s32(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                else
                    convert_s24_to_s32(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * 4);
                bitsPerSample = 32;
                bytesReadFromInput = samplesRead * 4;
                isUnsigned = NO;
            }
            if (!isFloat && bitsPerSample <= 32) {
                samplesRead = bytesReadFromInput / 4;
                if (isUnsigned)
                    convert_u32_to_s32(inputBuffer, samplesRead);
                convert_s32_to_float(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead, 1.0);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * sizeof(float));
                bitsPerSample = 32;
                bytesReadFromInput = samplesRead * sizeof(float);
                isUnsigned = NO;
                isFloat = YES;
            }
        }
        
        // Input now contains bytesReadFromInput worth of floats, in the input sample rate
        inpSize = bytesReadFromInput;
        inpOffset = 0;
    }
    
    if (floatOffset == floatSize)
    {
        struct resampler_data src_data;
        
        size_t inputSamples = (inpSize - inpOffset) / floatFormat.mBytesPerPacket;
        
        ioNumberPackets = (UInt32)inputSamples;
        
        ioNumberPackets = (UInt32)((float)ioNumberPackets * sampleRatio);
        ioNumberPackets = (ioNumberPackets + 255) & ~255;
        
        size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
        if (newSize < (ioNumberPackets * dmFloatFormat.mBytesPerPacket))
            newSize = ioNumberPackets * dmFloatFormat.mBytesPerPacket;
        if (!floatBuffer || floatBufferSize < newSize)
            floatBuffer = realloc( floatBuffer, floatBufferSize = newSize * 3 );
        
        if (stopping)
        {
            convertEntered = NO;
            return 0;
        }

        src_data.data_out      = floatBuffer;
        src_data.output_frames = 0;
        
        src_data.data_in       = (float*)(((uint8_t*)inputBuffer) + inpOffset);
        src_data.input_frames  = inputSamples;
        
        src_data.ratio         = sampleRatio;
        
        if (!skipResampler)
        {
            resampler->process(resampler_data, &src_data);
        }
        else
        {
            memcpy(src_data.data_out, src_data.data_in, inputSamples * floatFormat.mBytesPerPacket);
            src_data.output_frames = inputSamples;
        }
        
        inpOffset += inputSamples * floatFormat.mBytesPerPacket;
        
        if (!skipResampler && latencyEaten)
        {
            if (src_data.output_frames > latencyEaten)
            {
                src_data.output_frames -= latencyEaten;
                memmove(src_data.data_out, src_data.data_out + latencyEaten * inputFormat.mChannelsPerFrame, src_data.output_frames * floatFormat.mBytesPerPacket);
                latencyEaten = 0;
            }
            else
            {
                latencyEaten -= src_data.output_frames;
                src_data.output_frames = 0;
            }
        }
        
        amountReadFromFC = (int)(src_data.output_frames * floatFormat.mBytesPerPacket);
        
        scale_by_volume( (float*) floatBuffer, amountReadFromFC / sizeof(float), volumeScale);
        
        if ( inputFormat.mChannelsPerFrame > 2 && outputFormat.mChannelsPerFrame == 2 )
        {
            int samples = amountReadFromFC / floatFormat.mBytesPerFrame;
            downmix_to_stereo( (float*) floatBuffer, inputFormat.mChannelsPerFrame, samples );
            amountReadFromFC = samples * sizeof(float) * 2;
        }
        else if ( inputFormat.mChannelsPerFrame > 1 && outputFormat.mChannelsPerFrame == 1 )
        {
            int samples = amountReadFromFC / floatFormat.mBytesPerFrame;
            downmix_to_mono( (float*) floatBuffer, inputFormat.mChannelsPerFrame, samples );
            amountReadFromFC = samples * sizeof(float);
        }
        else if ( inputFormat.mChannelsPerFrame < outputFormat.mChannelsPerFrame )
        {
            int samples = amountReadFromFC / floatFormat.mBytesPerFrame;
            upmix( (float*) floatBuffer, inputFormat.mChannelsPerFrame, outputFormat.mChannelsPerFrame, samples );
            amountReadFromFC = samples * sizeof(float) * outputFormat.mChannelsPerFrame;
        }
        
        floatSize = amountReadFromFC;
        floatOffset = 0;
    }
    
    if (floatOffset == floatSize)
        goto tryagain;

    ioNumberPackets = (amount - amountRead);
    if (ioNumberPackets > (floatSize - floatOffset))
        ioNumberPackets = (UInt32)(floatSize - floatOffset);
    
    memcpy(dest + amountRead, floatBuffer + floatOffset, ioNumberPackets);
    
    floatOffset += ioNumberPackets;
    amountRead += ioNumberPackets;
    
    convertEntered = NO;
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
    else if ([keyPath isEqual:@"values.outputResampling"]) {
        // Reset resampler
        if (resampler && resampler_data)
            [self inputFormatDidChange:inputFormat];
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
	inputFormat = inf;
	outputFormat = outf;
    
    // These are really placeholders, as we're doing everything internally now
    
    floatFormat = inputFormat;
    floatFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    floatFormat.mBitsPerChannel = 32;
    floatFormat.mBytesPerFrame = (32/8)*floatFormat.mChannelsPerFrame;
    floatFormat.mBytesPerPacket = floatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;
    
    inpOffset = 0;
    inpSize = 0;
    
    floatOffset = 0;
    floatSize = 0;
    
    // This is a post resampler, post-down/upmix format
    
    dmFloatFormat = floatFormat;
    dmFloatFormat.mSampleRate = outputFormat.mSampleRate;
    dmFloatFormat.mChannelsPerFrame = outputFormat.mChannelsPerFrame;
    dmFloatFormat.mBytesPerFrame = (32/8)*dmFloatFormat.mChannelsPerFrame;
    dmFloatFormat.mBytesPerPacket = dmFloatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;

    convert_s16_to_float_init_simd();
    convert_s32_to_float_init_simd();
    
    skipResampler = outputFormat.mSampleRate == inputFormat.mSampleRate;
    
    sampleRatio = (double)outputFormat.mSampleRate / (double)inputFormat.mSampleRate;
    
    if (!skipResampler)
    {
        enum resampler_quality quality = RESAMPLER_QUALITY_DONTCARE;
        
        NSString * resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"outputResampling"];
        if ([resampling isEqualToString:@"lowest"])
            quality = RESAMPLER_QUALITY_LOWEST;
        else if ([resampling isEqualToString:@"lower"])
            quality = RESAMPLER_QUALITY_LOWER;
        else if ([resampling isEqualToString:@"normal"])
            quality = RESAMPLER_QUALITY_NORMAL;
        else if ([resampling isEqualToString:@"higher"])
            quality = RESAMPLER_QUALITY_HIGHER;
        else if ([resampling isEqualToString:@"highest"])
            quality = RESAMPLER_QUALITY_HIGHEST;
        
        if (!retro_resampler_realloc(&resampler_data, &resampler, "sinc", quality, inputFormat.mChannelsPerFrame, sampleRatio))
        {
            return NO;
        }
    
        latencyStarted = -1;
        latencyEaten = 0;
        latencyPostfill = NO;
    }

	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);

    [self refreshVolumeScaling];
    
    // Move this here so process call isn't running the resampler until it's allocated
    stopping = NO;
    convertEntered = NO;
    emittingSilence = NO;
    
	return YES;
}

- (void)dealloc
{
	DLog(@"Decoder dealloc");

    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.volumeScaling"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputResampling"];
    
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
    emittingSilence = YES;
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
    stopping = YES;
    while (convertEntered)
    {
        usleep(500);
    }
    if (resampler && resampler_data)
    {
        resampler->free(resampler_data);
        resampler = NULL;
        resampler_data = NULL;
    }
    if (floatBuffer)
    {
        free(floatBuffer);
        floatBuffer = NULL;
        floatBufferSize = 0;
    }
	if (inputBuffer) {
		free(inputBuffer);
		inputBuffer = NULL;
        inputBufferSize = 0;
	}
    floatOffset = 0;
    floatSize = 0;
}

@end
