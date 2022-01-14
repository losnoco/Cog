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

- (id)initWithController:(OutputNode *)c
{
	self = [super init];
	if (self)
	{
		outputController = c;
        _au = nil;
        _bufferSize = 0;
        volume = 1.0;
        outputDeviceID = -1;
        listenerapplied = NO;
        running = NO;
        
        _sema = dispatch_semaphore_create(0);
        
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:NULL];
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
}

- (void)threadEntry:(id)arg
{
    running = YES;
    size_t eventCount = 0;
    while (!stopping) {
        dispatch_semaphore_wait(_sema, DISPATCH_TIME_FOREVER);
        if (++eventCount == 128) {
            [self updateDeviceFormat];
            eventCount = 0;
        }
    }
    stopped = YES;
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
    }
    
    return YES;
}

- (BOOL)setup
{
	if (_au)
		[self stop];
    
    running = NO;
    stopping = NO;
    stopped = NO;
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
    
    [self updateDeviceFormat];
    
    __block dispatch_semaphore_t sema = _sema;
    __block OutputNode * outputController = self->outputController;
    __block float * volume = &self->volume;
    
    _au.outputProvider = ^AUAudioUnitStatus(AudioUnitRenderActionFlags * actionFlags, const AudioTimeStamp * timestamp, AUAudioFrameCount frameCount, NSInteger inputBusNumber, AudioBufferList * inputData)
    {
        void *readPointer = inputData->mBuffers[0].mData;
        
        int amountToRead, amountRead;

        amountToRead = inputData->mBuffers[0].mDataByteSize;
        
        if (self->stopping == YES || [outputController shouldContinue] == NO)
        {
            memset(readPointer, 0, amountToRead);
            self->stopping = YES;
            dispatch_semaphore_signal(sema);
            return 0;
        }

        amountRead = [outputController readData:(readPointer) amount:amountToRead];

        // Try repeatedly! Buffer wraps can cause a slight data shortage, as can
        // unexpected track changes.
        while ((amountRead < amountToRead) && [outputController endOfStream] == NO && [outputController shouldContinue] == YES)
        {
            int amountRead2; //Use this since return type of readdata isnt known...may want to fix then can do a simple += to readdata
            amountRead2 = [outputController readData:(readPointer+amountRead) amount:amountToRead-amountRead];
            amountRead += amountRead2;
        }

        int framesRead = amountRead / sizeof(float);
        scale_by_volume((float*)readPointer, framesRead, *volume);
        
        if (amountRead < amountToRead)
        {
            // Either underrun, or no data at all. Caller output tends to just
            // buffer loop if it doesn't get anything, so always produce a full
            // buffer, and silence anything we couldn't supply.
            memset(readPointer + amountRead, 0, amountToRead - amountRead);
        }
        
        dispatch_semaphore_signal(sema);

        return 0;
    };
    
    [_au allocateRenderResourcesAndReturnError:&err];
    
    [NSThread detachNewThreadSelector:@selector(threadEntry:) toTarget:self withObject:nil];
    
	return (err == nil);
}

- (void)setVolume:(double)v
{
    volume = v * 0.01f;
}

- (void)start
{
    NSError *err;
    [_au startHardwareAndReturnError:&err];
}

- (void)stop
{
    stopping = YES;
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
        dispatch_semaphore_signal(_sema);
        usleep(500);
    }
}

- (void)dealloc
{
	[self stop];
    
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputDevice"];
}

- (void)pause
{
    [_au stopHardware];
}

- (void)resume
{
    NSError *err;
    [_au startHardwareAndReturnError:&err];
}

@end
