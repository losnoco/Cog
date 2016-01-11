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
    
    mComponentName = NULL;
}

AUPlayer::~AUPlayer()
{
	shutdown();
}

void AUPlayer::send_event(uint32_t b)
{
	if (!(b & 0x80000000))
	{
		unsigned char event[ 3 ];
		event[ 0 ] = (unsigned char)b;
		event[ 1 ] = (unsigned char)( b >> 8 );
		event[ 2 ] = (unsigned char)( b >> 16 );
		unsigned port = (b >> 24) & 0x7F;
        if ( port > 2 ) port = 2;
        MusicDeviceMIDIEvent(samplerUnit[port], event[0], event[1], event[2], 0);
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

void AUPlayer::render(float * out, unsigned long count)
{
    float *ptrL, *ptrR;
    while (count)
    {
        unsigned long todo = count;
        if (todo > 512)
            todo = 512;
        memset(out, 0, todo * sizeof(float) * 2);
        for (unsigned long i = 0; i < 3; ++i)
        {
            AudioUnitRenderActionFlags ioActionFlags = 0;
            UInt32 numberFrames = (UInt32) todo;
            
            for (unsigned long j = 0; j < 2; j++)
            {
                bufferList->mBuffers[j].mNumberChannels = 1;
                bufferList->mBuffers[j].mDataByteSize = (UInt32) (todo * sizeof(float));
                bufferList->mBuffers[j].mData = audioBuffer + j * 512;
                memset(bufferList->mBuffers[j].mData, 0, todo * sizeof(float));
            }
            
            AudioUnitRender(samplerUnit[i], &ioActionFlags, &mTimeStamp, 0, numberFrames, bufferList);
            
            ptrL = (float *) bufferList->mBuffers[0].mData;
            ptrR = (float *) bufferList->mBuffers[1].mData;
            for (unsigned long j = 0; j < todo; ++j)
            {
                out[j * 2 + 0] += ptrL[j];
                out[j * 2 + 1] += ptrR[j];
            }
        }
        
        mTimeStamp.mSampleTime += (Float64) todo;
        
        out += todo * 2;
        count -= todo;
    }
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
        cbEnum(bytes);
        CFRelease(cfName);
        comp = AudioComponentFindNext(comp, &cd);
    }
}

void AUPlayer::setComponent(const char *name)
{
    if (mComponentName)
    {
        free(mComponentName);
        mComponentName = NULL;
    }
    
    size_t size = strlen(name) + 1;
    mComponentName = (char *) malloc(size);
    memcpy(mComponentName, name, size);
}

bool AUPlayer::startup()
{
    if (bufferList) return true;
    
    AudioComponentDescription cd = {0};
    cd.componentType = kAudioUnitType_MusicDevice;
    
    AudioComponent comp = NULL;
    
    const char * pComponentName = mComponentName;
    
    const char * bytes;
    char bytesBuffer[512];
    
    comp = AudioComponentFindNext(comp, &cd);
    
    if (pComponentName == NULL)
    {
        pComponentName = "Roland: SOUND Canvas VA";
        //pComponentName = "Apple: DLSMusicDevice";
    }

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
        if (!strcmp(bytes, pComponentName))
        {
            CFRelease(cfName);
            break;
        }
        CFRelease(cfName);
        comp = AudioComponentFindNext(comp, &cd);
    }
    
    if (!comp)
        return false;
    
    OSStatus error;

    for (int i = 0; i < 3; i++)
    {
        error = AudioComponentInstanceNew(comp, &samplerUnit[i]);
        
        if (error != noErr)
            return false;
        
        Float64 sampleRateIn = 0, sampleRateOut = 0;
        UInt32 sampleRateSize = sizeof (sampleRateIn);
        const Float64 sr = uSampleRate;

        AudioUnitGetProperty(samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, 0, &sampleRateIn, &sampleRateSize);
            
        if (sampleRateIn != sr)
            AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, 0, &sr, sizeof (sr));
        
        AudioUnitGetProperty (samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, 0, &sampleRateOut, &sampleRateSize);
        
        if (sampleRateOut != sr)
            AudioUnitSetProperty (samplerUnit[i], kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, i, &sr, sizeof (sr));

        AudioUnitReset (samplerUnit[i], kAudioUnitScope_Input, 0);
        AudioUnitReset (samplerUnit[i], kAudioUnitScope_Output, 0);
        
        AudioUnitReset (samplerUnit[i], kAudioUnitScope_Global, 0);
        
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

        error = AudioUnitInitialize(samplerUnit[i]);
        
        if (error != noErr)
            return false;
    }
    
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
