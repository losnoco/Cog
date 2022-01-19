//
//  OutputCoreAudio.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "OutputCoreAudio.h"
#import "OutputNode.h"

#import "Logging.h"

extern void scale_by_volume(float * buffer, size_t count, float volume);

@implementation OutputCoreAudio

static void fillBuffers(AudioBufferList *ioData, float * inbuffer, size_t count, size_t offset)
{
    const size_t channels = ioData->mNumberBuffers;
    for (int i = 0; i < channels; ++i)
    {
        const size_t maxCount = (ioData->mBuffers[i].mDataByteSize / sizeof(float)) - offset;
        float * output = ((float *)ioData->mBuffers[i].mData) + offset;
        const float * input = inbuffer + i;
        for (size_t j = 0, k = (count > maxCount) ? maxCount : count; j < k; ++j)
        {
            *output = *input;
            output++;
            input += channels;
        }
        ioData->mBuffers[i].mNumberChannels = 1;
    }
}

static void clearBuffers(AudioBufferList *ioData, size_t count, size_t offset)
{
    for (int i = 0; i < ioData->mNumberBuffers; ++i)
    {
        memset(ioData->mBuffers[i].mData + offset * sizeof(float), 0, count * sizeof(float));
        ioData->mBuffers[i].mNumberChannels = 1;
    }
}

static void scaleBuffersByVolume(AudioBufferList *ioData, float volume)
{
    if (volume != 1.0)
    {
        for (int i = 0; i < ioData->mNumberBuffers; ++i)
        {
            scale_by_volume((float*)ioData->mBuffers[i].mData, ioData->mBuffers[i].mDataByteSize / sizeof(float), volume);
        }
    }
}

static OSStatus renderCallback( void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData )
{
    OutputCoreAudio * _self = (__bridge OutputCoreAudio *) inRefCon;
    
    const int channels = _self->deviceFormat.mChannelsPerFrame;
    const int bytesPerPacket = channels * sizeof(float);
    
    int amountToRead, amountRead = 0;

    amountToRead = inNumberFrames * bytesPerPacket;
    
    if (_self->stopping == YES || [_self->outputController shouldContinue] == NO)
    {
        // Chain is dead, fill out the serial number pointer forever with silence
        clearBuffers(ioData, amountToRead / bytesPerPacket, 0);
        atomic_fetch_add(&_self->bytesRendered, amountToRead);
        _self->stopping = YES;
        return 0;
    }
    
    if ([[_self->outputController buffer] isEmpty] && ![_self->outputController chainQueueHasTracks])
    {
        // Hit end of last track, pad with silence until queue event stops us
        clearBuffers(ioData, amountToRead / bytesPerPacket, 0);
        atomic_fetch_add(&_self->bytesRendered, amountToRead);
        return 0;
    }
    
    void * readPtr;
    int toRead = [[_self->outputController buffer] lengthAvailableToReadReturningPointer:&readPtr];
    
    if (toRead > amountToRead)
        toRead = amountToRead;
    
    if (toRead) {
        fillBuffers(ioData, (float*)readPtr, toRead / bytesPerPacket, 0);
        amountRead = toRead;
        [[_self->outputController buffer] didReadLength:toRead];
        [_self->outputController incrementAmountPlayed:amountRead];
        atomic_fetch_add(&_self->bytesRendered, amountRead);
        [_self->writeSemaphore signal];
    }

    // Try repeatedly! Buffer wraps can cause a slight data shortage, as can
    // unexpected track changes.
    while ((amountRead < amountToRead) && [_self->outputController shouldContinue] == YES)
    {
        int amountRead2; //Use this since return type of readdata isnt known...may want to fix then can do a simple += to readdata
        amountRead2 = [[_self->outputController buffer] lengthAvailableToReadReturningPointer:&readPtr];
        if (amountRead2 > (amountToRead - amountRead))
            amountRead2 = amountToRead - amountRead;
        if (amountRead2) {
            atomic_fetch_add(&_self->bytesRendered, amountRead2);
            fillBuffers(ioData, (float*)readPtr, amountRead2 / bytesPerPacket, amountRead / bytesPerPacket);
            [[_self->outputController buffer] didReadLength:amountRead2];

            [_self->outputController incrementAmountPlayed:amountRead2];
            
            amountRead += amountRead2;
            [_self->writeSemaphore signal];
        }
        else {
            [_self->readSemaphore timedWait:500];
        }
    }

    scaleBuffersByVolume(ioData, _self->volume);
    
    if (amountRead < amountToRead)
    {
        // Either underrun, or no data at all. Caller output tends to just
        // buffer loop if it doesn't get anything, so always produce a full
        // buffer, and silence anything we couldn't supply.
        clearBuffers(ioData, (amountToRead - amountRead) / bytesPerPacket, amountRead / bytesPerPacket);
    }
    
    return 0;
};

- (id)initWithController:(OutputNode *)c
{
	self = [super init];
	if (self)
	{
		outputController = c;
        _au = nil;
        _eq = NULL;
        _bufferSize = 0;
        volume = 1.0;
        outputDeviceID = -1;
        listenerapplied = NO;
        running = NO;
        started = NO;
        
        atomic_init(&bytesRendered, 0);
        
        writeSemaphore = [[Semaphore alloc] init];
        readSemaphore = [[Semaphore alloc] init];
        
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:NULL];
        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.GraphicEQenable" options:0 context:NULL];
        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.GraphicEQpreset" options:0 context:NULL];
	}
	
	return self;
}

static OSStatus
default_device_changed(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inUserData)
{
    OutputCoreAudio *this = (__bridge OutputCoreAudio *) inUserData;
    return [this setOutputDeviceByID:-1];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"values.outputDevice"]) {

		NSDictionary *device = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];

		[self setOutputDeviceWithDeviceDict:device];
	}
    else if ([keyPath isEqualToString:@"values.GraphicEQenable"]) {
        BOOL enabled = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue];
        
        [self setEqualizerEnabled:enabled];
    }
    else if ([keyPath isEqualToString:@"values.GraphicEQpreset"]) {
        if (_eq)
            [outputController refreshEqualizer:_eq];
    }
}

- (void)signalEndOfStream
{
    [outputController resetAmountPlayed];
    [outputController endOfInputPlayed];
}

- (void)threadEntry:(id)arg
{
    running = YES;
    started = NO;
    size_t eventCount = 0;
    atomic_store(&bytesRendered, 0);
    NSMutableArray *delayedEvents = [[NSMutableArray alloc] init];
    BOOL delayedEventsPopped = YES;
    while (!stopping) {
        if (++eventCount == 128) {
            [self updateDeviceFormat];
            eventCount = 0;
        }
        
        if ([outputController shouldReset]) {
            [[outputController buffer] empty];
            [outputController setShouldReset:NO];
        }

        while ([delayedEvents count]) {
            size_t localBytesRendered = atomic_load_explicit(&bytesRendered, memory_order_relaxed);
            if (localBytesRendered >= [[delayedEvents objectAtIndex:0] longValue]) {
                if ([outputController chainQueueHasTracks])
                    delayedEventsPopped = YES;
                [self signalEndOfStream];
                [delayedEvents removeObjectAtIndex:0];
            }
            else break;
        }
        
        if (stopping)
            break;
        
        void *writePtr;
        int toWrite = [[outputController buffer] lengthAvailableToWriteReturningPointer:&writePtr];
        int bytesRead = 0;
        if (toWrite > CHUNK_SIZE)
            toWrite = CHUNK_SIZE;
        if (toWrite)
            bytesRead = [outputController readData:writePtr amount:toWrite];
        if (bytesRead) {
            [[outputController buffer] didWriteLength:bytesRead];
            [readSemaphore signal];
            continue;
        }
        else if ([outputController shouldContinue] == NO)
            break;
        else if (!toWrite) {
            if (!started) {
                started = YES;
                if (!paused) {
                    NSError *err;
                    [_au startHardwareAndReturnError:&err];
                }
            }
        }
        else {
            // End of input possibly reached
            if (delayedEventsPopped && [outputController endOfStream] == YES)
            {
                long bytesBuffered = [[outputController buffer] bufferedLength];
                bytesBuffered += atomic_load_explicit(&bytesRendered, memory_order_relaxed);
                if ([outputController chainQueueHasTracks])
                {
                    if (bytesBuffered < CHUNK_SIZE)
                        bytesBuffered = 0;
                    else
                        bytesBuffered -= CHUNK_SIZE;
                }
                [delayedEvents addObject:[NSNumber numberWithLong:bytesBuffered]];
                delayedEventsPopped = NO;
                if (!started) {
                    started = YES;
                    if (!paused) {
                        NSError *err;
                        [_au startHardwareAndReturnError:&err];
                    }
                }
            }
        }
        [readSemaphore signal];
        [writeSemaphore timedWait:5000];
    }
    stopped = YES;
    if (!stopInvoked)
        [self stop];
}

- (OSStatus)setOutputDeviceByID:(AudioDeviceID)deviceID
{
	OSStatus err;
    BOOL defaultDevice = NO;
    AudioObjectPropertyAddress theAddress = {
        .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMaster
    };

	if (deviceID == -1) {
        defaultDevice = YES;
		UInt32 size = sizeof(AudioDeviceID);
        err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &size, &deviceID);
								
		if (err != noErr) {
			DLog(@"THERE'S NO DEFAULT OUTPUT DEVICE");
			
			return err;
		}
	}

    if (_au) {
        AudioObjectPropertyAddress defaultDeviceAddress = theAddress;

        if (listenerapplied && !defaultDevice) {
            AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &defaultDeviceAddress, default_device_changed, (__bridge void * _Nullable)(self));
            listenerapplied = NO;
        }

        if (outputDeviceID != deviceID) {
            DLog(@"Device: %i\n", deviceID);
            outputDeviceID = deviceID;
            
            NSError *nserr;
            [_au setDeviceID:outputDeviceID error:&nserr];
            if (nserr != nil) {
                return (OSErr)[nserr code];
            }
        }
        
        if (!listenerapplied && defaultDevice) {
            AudioObjectAddPropertyListener(kAudioObjectSystemObject, &defaultDeviceAddress, default_device_changed, (__bridge void * _Nullable)(self));
            listenerapplied = YES;
        }
    }
    else {
        err = noErr;
    }
	
	if (err != noErr) {
		DLog(@"No output device with ID %d could be found.", deviceID);

		return err;
	}
	
	return err;
}

- (BOOL)setOutputDeviceWithDeviceDict:(NSDictionary *)deviceDict
{
	NSNumber *deviceIDNum = [deviceDict objectForKey:@"deviceID"];
	AudioDeviceID outputDeviceID = [deviceIDNum unsignedIntValue] ?: -1;
	
	__block OSStatus err = [self setOutputDeviceByID:outputDeviceID];
	
	if (err != noErr) {
		// Try matching by name.
		NSString *userDeviceName = deviceDict[@"name"];
		
		[self enumerateAudioOutputsUsingBlock:
		 ^(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop) {
            if ([deviceName isEqualToString:userDeviceName]) {
                err = [self setOutputDeviceByID:deviceID];

#if 0
                // Disable. Would cause loop by triggering `-observeValueForKeyPath:ofObject:change:context:` above.
                // Update `outputDevice`, in case the ID has changed.
                NSDictionary *deviceInfo = @{
                    @"name": deviceName,
                    @"deviceID": @(deviceID),
                };
                [[NSUserDefaults standardUserDefaults] setObject:deviceInfo forKey:@"outputDevice"];
#endif
				
				DLog(@"Found output device: \"%@\" (%d).", deviceName, deviceID);
				
                *stop = YES;
            }
		}];
	}
	
	if (err != noErr) {

		ALog(@"No output device could be found, your random error code is %d. Have a nice day!", err);
		
		return NO;
	}
	
	return YES;
}

// The following is largely a copy pasta of -awakeFromNib from "OutputsArrayController.m".
// TODO: Share the code. (How to do this across xcodeproj?)
- (void)enumerateAudioOutputsUsingBlock:(void (NS_NOESCAPE ^ _Nonnull)(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop))block
{
	UInt32 propsize;
	AudioObjectPropertyAddress theAddress = {
		.mSelector = kAudioHardwarePropertyDevices,
		.mScope = kAudioObjectPropertyScopeGlobal,
		.mElement = kAudioObjectPropertyElementMaster
	};
	
	__Verify_noErr(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize));
	UInt32 nDevices = propsize / (UInt32)sizeof(AudioDeviceID);
	AudioDeviceID *devids = malloc(propsize);
	__Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, devids));
	
	theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
	AudioDeviceID systemDefault;
	propsize = sizeof(systemDefault);
	__Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, &systemDefault));
	
	theAddress.mScope = kAudioDevicePropertyScopeOutput;
	
	for (UInt32 i = 0; i < nDevices; ++i) {
		CFStringRef name = NULL;
		propsize = sizeof(name);
		theAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &name));
		
		propsize = 0;
		theAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
		__Verify_noErr(AudioObjectGetPropertyDataSize(devids[i], &theAddress, 0, NULL, &propsize));
		
        if (propsize < sizeof(UInt32)) {
            CFRelease(name);
            continue;
        }
		
		AudioBufferList * bufferList = (AudioBufferList *) malloc(propsize);
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, bufferList));
		UInt32 bufferCount = bufferList->mNumberBuffers;
		free(bufferList);
		
        if (!bufferCount) {
            CFRelease(name);
            continue;
        }
            
		BOOL stop = NO;
		block([NSString stringWithString:(__bridge NSString *)name],
			  devids[i],
			  systemDefault,
			  &stop);
		
		CFRelease(name);
		
		if (stop) {
			break;
		}
	}
	
	free(devids);
}

- (BOOL)updateDeviceFormat
{
    AVAudioFormat *format = _au.outputBusses[0].format;
    
    if (!_deviceFormat || ![_deviceFormat isEqual:format])
    {
        NSError *err;
        AVAudioFormat *renderFormat;

        [outputController incrementAmountPlayed:[[outputController buffer] bufferedLength]];
        [[outputController buffer] empty];

        _deviceFormat = format;
        deviceFormat = *(format.streamDescription);

        ///Seems some 3rd party devices return incorrect stuff...or I just don't like noninterleaved data.
        deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
    //    deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsFloat;
    //    deviceFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
        // We don't want more than 8 channels
        if (deviceFormat.mChannelsPerFrame > 8) {
            deviceFormat.mChannelsPerFrame = 8;
        }
        deviceFormat.mBytesPerFrame = deviceFormat.mChannelsPerFrame*(deviceFormat.mBitsPerChannel/8);
        deviceFormat.mBytesPerPacket = deviceFormat.mBytesPerFrame * deviceFormat.mFramesPerPacket;

        /* Set the channel layout for the audio queue */
        AudioChannelLayoutTag tag = 0;
        switch (deviceFormat.mChannelsPerFrame) {
        case 1:
            tag = kAudioChannelLayoutTag_Mono;
            break;
        case 2:
            tag = kAudioChannelLayoutTag_Stereo;
            break;
        case 3:
            tag = kAudioChannelLayoutTag_DVD_4;
            break;
        case 4:
            tag = kAudioChannelLayoutTag_Quadraphonic;
            break;
        case 5:
            tag = kAudioChannelLayoutTag_MPEG_5_0_A;
            break;
        case 6:
            tag = kAudioChannelLayoutTag_MPEG_5_1_A;
            break;
        case 7:
            tag = kAudioChannelLayoutTag_MPEG_6_1_A;
            break;
        case 8:
            tag = kAudioChannelLayoutTag_MPEG_7_1_A;
            break;
        }

        renderFormat = [[AVAudioFormat alloc] initWithStreamDescription:&deviceFormat channelLayout:[[AVAudioChannelLayout alloc] initWithLayoutTag:tag]];
        [_au.inputBusses[0] setFormat:renderFormat error:&err];
        if (err != nil)
            return NO;
        
        [outputController setFormat:&deviceFormat];
        
        AudioStreamBasicDescription asbd = deviceFormat;
        
        asbd.mFormatFlags &= ~kAudioFormatFlagIsPacked;
        
        AudioUnitSetProperty (_eq, kAudioUnitProperty_StreamFormat,
                              kAudioUnitScope_Input, 0, &asbd, sizeof (asbd));
        
        AudioUnitSetProperty (_eq, kAudioUnitProperty_StreamFormat,
                              kAudioUnitScope_Output, 0, &asbd, sizeof (asbd));
        AudioUnitReset (_eq, kAudioUnitScope_Input, 0);
        AudioUnitReset (_eq, kAudioUnitScope_Output, 0);
        
        AudioUnitReset (_eq, kAudioUnitScope_Global, 0);
        
        eqEnabled = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue];
    }
    
    return YES;
}

- (BOOL)setup
{
	if (_au)
		[self stop];
    
    stopInvoked = NO;
    running = NO;
    stopping = NO;
    stopped = NO;
    paused = NO;
    outputDeviceID = -1;
	
    AudioComponentDescription desc;
    NSError *err;
	
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_HALOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
    _au = [[AUAudioUnit alloc] initWithComponentDescription:desc error:&err];
    if (err != nil)
        return NO;

    // Setup the output device before mucking with settings
    NSDictionary *device = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];
    if (device) {
        BOOL ok = [self setOutputDeviceWithDeviceDict:device];
        if (!ok) {
            //Ruh roh.
            [self setOutputDeviceWithDeviceDict:nil];
            
            [[[NSUserDefaultsController sharedUserDefaultsController] defaults] removeObjectForKey:@"outputDevice"];
        }
    }
    else {
        [self setOutputDeviceWithDeviceDict:nil];
    }

    _deviceFormat = nil;
        
    AudioComponent comp = NULL;
    
    desc.componentType = kAudioUnitType_Effect;
    desc.componentSubType = kAudioUnitSubType_GraphicEQ;
    
    comp = AudioComponentFindNext(comp, &desc);
    if (!comp)
        return NO;
    
    OSStatus _err = AudioComponentInstanceNew(comp, &_eq);
    if (err)
        return NO;

    [self updateDeviceFormat];
    
    __block AudioUnit eq = _eq;
    __block AudioStreamBasicDescription *format = &deviceFormat;
    __block BOOL *eqEnabled = &self->eqEnabled;
    __block void *refCon = (__bridge void *)self;

    _au.outputProvider = ^AUAudioUnitStatus(AudioUnitRenderActionFlags * _Nonnull actionFlags, const AudioTimeStamp * _Nonnull timestamp, AUAudioFrameCount frameCount, NSInteger inputBusNumber, AudioBufferList * _Nonnull inputData)
    {
        // This expects multiple buffers, so:
        int i;
        const int channels = format->mChannelsPerFrame;
        const int channelsminusone = channels - 1;
        float buffers[frameCount * format->mChannelsPerFrame];
        uint8_t bufferlistbuffer[sizeof(AudioBufferList) + sizeof(AudioBuffer) * channelsminusone];
        AudioBufferList * ioData = (AudioBufferList *)(bufferlistbuffer);
        
        ioData->mNumberBuffers = channels;
        
        memset(buffers, 0, sizeof(buffers));
        
        for (i = 0; i < channels; ++i) {
            ioData->mBuffers[i].mNumberChannels = 1;
            ioData->mBuffers[i].mData = buffers + frameCount * i;
            ioData->mBuffers[i].mDataByteSize = frameCount * sizeof(float);
        }
        
        OSStatus ret;
        
        if (*eqEnabled)
            ret = AudioUnitRender(eq, actionFlags, timestamp, (UInt32) inputBusNumber, frameCount, ioData);
        else
            ret = renderCallback(refCon, actionFlags, timestamp, (UInt32) inputBusNumber, frameCount, ioData);
        
        if (ret)
            return ret;
        
        for (i = 0; i < channels; ++i) {
            float * outBuffer = ((float*)inputData->mBuffers[0].mData) + i;
            const float * inBuffer = ((float*)ioData->mBuffers[i].mData);
            const int frameCount = ioData->mBuffers[i].mDataByteSize / sizeof(float);
            for (int j = 0; j < frameCount; ++j) {
                *outBuffer = *inBuffer;
                inBuffer++;
                outBuffer += channels;
            }
        }
        
        inputData->mBuffers[0].mNumberChannels = channels;
        
        return 0;
    };

    UInt32 value;
    UInt32 size = sizeof(value);

    value = CHUNK_SIZE;
    AudioUnitSetProperty (_eq, kAudioUnitProperty_MaximumFramesPerSlice,
                          kAudioUnitScope_Global, 0, &value, size);
                          
    value = 127;
    AudioUnitSetProperty (_eq, kAudioUnitProperty_RenderQuality,
                          kAudioUnitScope_Global, 0, &value, size);

    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProcRefCon = (__bridge void *)self;
    callbackStruct.inputProc = renderCallback;
    AudioUnitSetProperty (_eq, kAudioUnitProperty_SetRenderCallback,
                          kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct));

    AudioUnitReset (_eq, kAudioUnitScope_Input, 0);
    AudioUnitReset (_eq, kAudioUnitScope_Output, 0);
    
    AudioUnitReset (_eq, kAudioUnitScope_Global, 0);

    _err = AudioUnitInitialize(_eq);
    if (_err)
        return NO;
    
    [outputController beginEqualizer:_eq];
    
    [_au allocateRenderResourcesAndReturnError:&err];
    
	return (err == nil);
}

- (void)setVolume:(double)v
{
    volume = v * 0.01f;
}

- (void)setEqualizerEnabled:(BOOL)enabled
{
    if (enabled && !eqEnabled) {
        if (_eq) {
            AudioUnitReset (_eq, kAudioUnitScope_Input, 0);
            AudioUnitReset (_eq, kAudioUnitScope_Output, 0);
            AudioUnitReset (_eq, kAudioUnitScope_Global, 0);
        }
    }
    
    eqEnabled = enabled;
}

- (void)start
{
    [self threadEntry:nil];
}

- (void)stop
{
    stopInvoked = YES;
    stopping = YES;
    paused = NO;
    [writeSemaphore signal];
    [readSemaphore signal];
    if (listenerapplied) {
        AudioObjectPropertyAddress theAddress = {
            .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
            .mScope = kAudioObjectPropertyScopeGlobal,
            .mElement = kAudioObjectPropertyElementMaster
        };
        AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &theAddress, default_device_changed, (__bridge void * _Nullable)(self));
        listenerapplied = NO;
    }
    if (_au) {
        [_au stopHardware];
        _au = nil;
    }
    if (running)
    while (!stopped)
    {
        stopping = YES;
        [readSemaphore signal];
        [writeSemaphore timedWait:5000];
    }
    if (_eq)
    {
        [outputController endEqualizer:_eq];
        AudioUnitUninitialize(_eq);
        AudioComponentInstanceDispose(_eq);
        _eq = NULL;
    }
}

- (void)dealloc
{
	[self stop];
    
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputDevice"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.GraphicEQenable"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.GraphicEQpreset"];
}

- (void)pause
{
    paused = YES;
    [_au stopHardware];
}

- (void)resume
{
    NSError *err;
    [_au startHardwareAndReturnError:&err];
    paused = NO;
}

@end
