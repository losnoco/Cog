#include "AUPlayer.h"

#include <stdlib.h>

#define SF2PACK

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

AUPlayer::AUPlayer() : MIDIPlayer()
{
    samplerUnit[0] = NULL;
    samplerUnit[1] = NULL;
    samplerUnit[2] = NULL;
    bufferList = NULL;
    audioBuffer = NULL;
    
    componentSubType = kAudioUnitSubType_DLSSynth;
    componentManufacturer = kAudioUnitManufacturer_Apple;
}

AUPlayer::~AUPlayer()
{
	shutdown();
}

void AUPlayer::send_event(uint32_t b, uint32_t sample_offset)
{
	if (!(b & 0x80000000))
	{
		unsigned char event[ 3 ];
		event[ 0 ] = (unsigned char)b;
		event[ 1 ] = (unsigned char)( b >> 8 );
		event[ 2 ] = (unsigned char)( b >> 16 );
		unsigned port = (b >> 24) & 0x7F;
        if ( port > 2 ) port = 2;
        MusicDeviceMIDIEvent(samplerUnit[port], event[0], event[1], event[2], sample_offset);
    }
    else
	{
		uint32_t n = b & 0xffffff;
		const uint8_t * data;
        std::size_t size, port;
		mSysexMap.get_entry( n, data, size, port );
		if ( port > 2 ) port = 2;
        MusicDeviceSysEx(samplerUnit[port], data, (UInt32) size);
        if ( port == 0 )
        {
            MusicDeviceSysEx(samplerUnit[1], data, (UInt32) size);
            MusicDeviceSysEx(samplerUnit[2], data, (UInt32) size);
        }
	}
}

void AUPlayer::render_512(float * out)
{
    float *ptrL, *ptrR;
    memset(out, 0, 512 * sizeof(float) * 2);
	for (unsigned long i = 0; i < 3; ++i)
	{
		AudioUnitRenderActionFlags ioActionFlags = 0;
		UInt32 numberFrames = 512;
            
        for (unsigned long j = 0; j < 2; j++)
        {
            bufferList->mBuffers[j].mNumberChannels = 1;
            bufferList->mBuffers[j].mDataByteSize = (UInt32) (512 * sizeof(float));
            bufferList->mBuffers[j].mData = audioBuffer + j * 512;
            memset(bufferList->mBuffers[j].mData, 0, 512 * sizeof(float));
        }
            
        AudioUnitRender(samplerUnit[i], &ioActionFlags, &mTimeStamp, 0, numberFrames, bufferList);
            
        ptrL = (float *) bufferList->mBuffers[0].mData;
        ptrR = (float *) bufferList->mBuffers[1].mData;
        for (unsigned long j = 0; j < 512; ++j)
        {
            out[j * 2 + 0] += ptrL[j];
            out[j * 2 + 1] += ptrR[j];
        }
    }

	mTimeStamp.mSampleTime += 512.0;
}

void AUPlayer::shutdown()
{
    if ( samplerUnit[2] )
    {
        AudioUnitUninitialize( samplerUnit[2] );
        AudioComponentInstanceDispose( samplerUnit[2] );
        samplerUnit[2] = NULL;
    }
    if ( samplerUnit[1] )
    {
        AudioUnitUninitialize( samplerUnit[1] );
        AudioComponentInstanceDispose( samplerUnit[1] );
        samplerUnit[1] = NULL;
    }
    if ( samplerUnit[0] )
    {
        AudioUnitUninitialize( samplerUnit[0] );
        AudioComponentInstanceDispose( samplerUnit[0] );
        samplerUnit[0] = NULL;
    }
    if (audioBuffer)
    {
        free(audioBuffer);
        audioBuffer = NULL;
    }
    if (bufferList)
    {
        free(bufferList);
        bufferList = NULL;
    }
}

void AUPlayer::enumComponents(callback cbEnum)
{
    AudioComponentDescription cd = {0};
    cd.componentType = kAudioUnitType_MusicDevice;
    
    AudioComponent comp = NULL;
    
    const char * bytes;
    char bytesBuffer[512];
    
    comp = AudioComponentFindNext(comp, &cd);
    
    while (comp != NULL)
    {
        CFStringRef cfName;
        AudioComponentCopyName(comp, &cfName);
        bytes = CFStringGetCStringPtr(cfName, kCFStringEncodingUTF8);
        if (!bytes)
        {
            CFStringGetCString(cfName, bytesBuffer, sizeof(bytesBuffer) - 1, kCFStringEncodingUTF8);
            bytes = bytesBuffer;
        }
        AudioComponentGetDescription(comp, &cd);
        cbEnum(cd.componentSubType, cd.componentManufacturer, bytes);
        CFRelease(cfName);
        comp = AudioComponentFindNext(comp, &cd);
    }
}

void AUPlayer::setComponent(OSType uSubType, OSType uManufacturer)
{
    componentSubType = uSubType;
    componentManufacturer = uManufacturer;
    shutdown();
}

/*void AUPlayer::setSoundFont( const char * in )
{
    sSoundFontName = in;
    shutdown();
}

void AUPlayer::setFileSoundFont( const char * in )
{
    sFileSoundFontName = in;
    shutdown();
}*/

static OSStatus renderCallback( void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData )
{
    if ( inNumberFrames && ioData )
    {
        for ( int i = 0, j = ioData->mNumberBuffers; i < j; ++i )
        {
            int k = inNumberFrames * sizeof(float);
            if (k > ioData->mBuffers[i].mDataByteSize)
                k = ioData->mBuffers[i].mDataByteSize;
            memset( ioData->mBuffers[i].mData, 0, k);
        }
    }
    
    return noErr;
}

bool AUPlayer::startup()
{
    if (bufferList) return true;
    
    AudioComponentDescription cd = {0};
    cd.componentType = kAudioUnitType_MusicDevice;
	cd.componentSubType = componentSubType;
	cd.componentManufacturer = componentManufacturer;
    
    AudioComponent comp = NULL;
    
    comp = AudioComponentFindNext(comp, &cd);
    
    if (!comp)
        return false;
    
    OSStatus error;

    for (int i = 0; i < 3; i++)
    {
        UInt32 value = 1;
        UInt32 size = sizeof(value);

        error = AudioComponentInstanceNew(comp, &samplerUnit[i]);
        
        if (error != noErr)
            return false;
        
        {
            AudioStreamBasicDescription stream = { 0 };
            stream.mSampleRate       = uSampleRate;
            stream.mFormatID         = kAudioFormatLinearPCM;
            stream.mFormatFlags      = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagsNativeEndian;
            stream.mFramesPerPacket  = 1;
            stream.mBytesPerPacket   = 4;
            stream.mBytesPerFrame    = 4;
            stream.mBitsPerChannel   = 32;
            stream.mChannelsPerFrame = 2;
            
            AudioUnitSetProperty (samplerUnit[i], kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 0, &stream, sizeof (stream));
            
            AudioUnitSetProperty (samplerUnit[i], kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output, 0, &stream, sizeof (stream));
        }
        
        value = 512;
        AudioUnitSetProperty (samplerUnit[i], kAudioUnitProperty_MaximumFramesPerSlice,
                              kAudioUnitScope_Global, 0, &value, size);
                              
        value = 127;
        AudioUnitSetProperty (samplerUnit[i], kAudioUnitProperty_RenderQuality,
                              kAudioUnitScope_Global, 0, &value, size);
                              
        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc = renderCallback;
        callbackStruct.inputProcRefCon = 0;
        AudioUnitSetProperty (samplerUnit[i], kAudioUnitProperty_SetRenderCallback,
                              kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct));
        
        /*Float64 sampleRateIn = 0, sampleRateOut = 0;
        UInt32 sampleRateSize = sizeof (sampleRateIn);
        const Float64 sr = uSampleRate;

        AudioUnitGetProperty(samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, 0, &sampleRateIn, &sampleRateSize);
            
        if (sampleRateIn != sr)
            AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, 0, &sr, sizeof (sr));
        
        AudioUnitGetProperty (samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, 0, &sampleRateOut, &sampleRateSize);
        
        if (sampleRateOut != sr)
            AudioUnitSetProperty (samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, i, &sr, sizeof (sr));*/

        AudioUnitReset (samplerUnit[i], kAudioUnitScope_Input, 0);
        AudioUnitReset (samplerUnit[i], kAudioUnitScope_Output, 0);
        
        AudioUnitReset (samplerUnit[i], kAudioUnitScope_Global, 0);
        
        /*
        value = 1;
        AudioUnitSetProperty(samplerUnit[i], kMusicDeviceProperty_StreamFromDisk, kAudioUnitScope_Global, 0, &value, size);
        */

        error = AudioUnitInitialize(samplerUnit[i]);
        
        if (error != noErr)
            return false;
    }
    
    // Now load instruments
    /*if (sSoundFontName.length())
    {
        loadSoundFont( sSoundFontName.c_str() );
    }
    
    if ( sFileSoundFontName.length() )
    {
        loadSoundFont( sFileSoundFontName.c_str() );
    }*/
    
    bufferList = (AudioBufferList *) calloc(1, sizeof(AudioBufferList) + sizeof(AudioBuffer));
    if (!bufferList)
        return false;
    
    audioBuffer = (float *) malloc(1024 * sizeof(float));
    if (!audioBuffer)
        return false;
    
    bufferList->mNumberBuffers = 2;
    
    memset(&mTimeStamp, 0, sizeof(mTimeStamp));
    mTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;
    
	return true;
}

/*void AUPlayer::loadSoundFont(const char *name)
{
    // kMusicDeviceProperty_SoundBankURL was added in 10.5 as a replacement
    // In addition, the File Manager API became deprecated starting in 10.8
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)name, strlen(name), false);
    
    if (url) {
        for (int i = 0; i < 3; i++)
            AudioUnitSetProperty(samplerUnit[i],
                                 kMusicDeviceProperty_SoundBankURL, kAudioUnitScope_Global,
                                 0,
                                 &url, sizeof(url)
                                 );
        
        CFRelease(url);
    }
}*/
