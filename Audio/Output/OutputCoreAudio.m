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

@interface OutputCoreAudio (Private)
- (void)prime;
@end

@implementation OutputCoreAudio

- (id)initWithController:(OutputNode *)c
{
	self = [super init];
	if (self)
	{
		outputController = c;
		outputUnit = NULL;
        audioQueue = NULL;
        buffers = NULL;
        numberOfBuffers = 0;
        volume = 1.0;
        outputDeviceID = -1;
        listenerapplied = NO;

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:NULL];
	}
	
	return self;
}

static void Sound_Renderer(void *userData, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
	OutputCoreAudio *output = (__bridge OutputCoreAudio *)userData;
	void *readPointer = buffer->mAudioData;
	
	int amountToRead, amountRead;

    int framesToRead = buffer->mAudioDataByteSize / (output->deviceFormat.mBytesPerPacket);
    
    amountToRead = framesToRead * (output->deviceFormat.mBytesPerPacket);
    
    if (output->stopping == YES)
    {
        output->stopped = YES;
        return;
    }

	if ([output->outputController shouldContinue] == NO)
	{
//		[output stop];
        memset(readPointer, 0, amountToRead);
        buffer->mAudioDataByteSize = amountToRead;
        AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
        return;
	}
	
	amountRead = [output->outputController readData:(readPointer) amount:amountToRead];

	if ((amountRead < amountToRead) && [output->outputController endOfStream] == NO) //Try one more time! for track changes!
	{
		int amountRead2; //Use this since return type of readdata isnt known...may want to fix then can do a simple += to readdata
		amountRead2 = [output->outputController readData:(readPointer+amountRead) amount:amountToRead-amountRead];
		amountRead += amountRead2;
	}
    
    if (amountRead < amountToRead)
    {
        // Either underrun, or no data at all. Caller output tends to just
        // buffer loop if it doesn't get anything, so always produce a full
        // buffer, and silence anything we couldn't supply.
        memset(readPointer + amountRead, 0, amountToRead - amountRead);
        amountRead = amountToRead;
    }
	
    buffer->mAudioDataByteSize = amountRead;
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
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


- (OSStatus)setOutputDeviceByID:(AudioDeviceID)deviceID
{
	OSStatus err;
    BOOL defaultDevice = NO;
    UInt32 thePropSize;
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

    if (audioQueue) {
        AudioObjectPropertyAddress defaultDeviceAddress = theAddress;

        if (listenerapplied && !defaultDevice) {
            AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &defaultDeviceAddress, default_device_changed, (__bridge void * _Nullable)(self));
            listenerapplied = NO;
        }

        if (outputDeviceID != deviceID) {
            printf("DEVICE: %i\n", deviceID);
            outputDeviceID = deviceID;

            CFStringRef theDeviceUID;
            theAddress.mSelector = kAudioDevicePropertyDeviceUID;
            theAddress.mScope = kAudioDevicePropertyScopeOutput;
            thePropSize = sizeof(theDeviceUID);
            err = AudioObjectGetPropertyData(outputDeviceID, &theAddress, 0, NULL, &thePropSize, &theDeviceUID);
	
            if (err) {
                DLog(@"Error getting device UID as string");
                return err;
            }
    
            err = AudioQueueStop(audioQueue, true);
            if (err) {
                DLog(@"Error stopping stream to set device");
                CFRelease(theDeviceUID);
                return err;
            }
            primed = NO;
            err = AudioQueueSetProperty(audioQueue, kAudioQueueProperty_CurrentDevice, &theDeviceUID, sizeof(theDeviceUID));
            CFRelease(theDeviceUID);
            if (running)
                [self start];
        }
        
        if (!listenerapplied && defaultDevice) {
            AudioObjectAddPropertyListener(kAudioObjectSystemObject, &defaultDeviceAddress, default_device_changed, (__bridge void * _Nullable)(self));
            listenerapplied = YES;
        }
    }
    else if (outputUnit) {
        err = AudioUnitSetProperty(outputUnit,
                                  kAudioOutputUnitProperty_CurrentDevice,
                                  kAudioUnitScope_Output,
                                  0,
                                  &deviceID,
                                  sizeof(AudioDeviceID));

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

- (BOOL)setup
{
	if (outputUnit || audioQueue)
		[self stop];
    
    stopping = NO;
    stopped = NO;
    outputDeviceID = -1;
	
    AudioComponentDescription desc;
	OSStatus err;
	
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	AudioComponent comp = AudioComponentFindNext(NULL, &desc);  //Finds an component that meets the desc spec's
	if (comp == NULL)
		return NO;
	
	err = AudioComponentInstanceNew(comp, &outputUnit);  //gains access to the services provided by the component
	if (err)
		return NO;
	
	// Initialize AudioUnit 
	err = AudioUnitInitialize(outputUnit);
	if (err != noErr)
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
	
	UInt32 size = sizeof (AudioStreamBasicDescription);
	Boolean outWritable;
	//Gets the size of the Stream Format Property and if it is writable
	AudioUnitGetPropertyInfo(outputUnit,  
							 kAudioUnitProperty_StreamFormat,
							 kAudioUnitScope_Output, 
							 0, 
							 &size, 
							 &outWritable);
	//Get the current stream format of the output
	err = AudioUnitGetProperty (outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Output,
								0,
								&deviceFormat,
								&size);
	
	if (err != noErr)
		return NO;
    
    AudioUnitUninitialize (outputUnit);
    AudioComponentInstanceDispose(outputUnit);
    outputUnit = NULL;
	
	///Seems some 3rd party devices return incorrect stuff...or I just don't like noninterleaved data.
	deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
//	deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsFloat;
//	deviceFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
	deviceFormat.mBytesPerFrame = deviceFormat.mChannelsPerFrame*(deviceFormat.mBitsPerChannel/8);
	deviceFormat.mBytesPerPacket = deviceFormat.mBytesPerFrame * deviceFormat.mFramesPerPacket;

    err = AudioQueueNewOutput(&deviceFormat, Sound_Renderer, (__bridge void * _Nullable)(self), NULL, NULL, 0, &audioQueue);
    
    if (err != noErr)
        return NO;
    
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

    /* Set the channel layout for the audio queue */
    AudioChannelLayout layout = {0};
    switch (deviceFormat.mChannelsPerFrame) {
    case 1:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_Mono;
        break;
    case 2:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
        break;
    case 3:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_DVD_4;
        break;
    case 4:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_Quadraphonic;
        break;
    case 5:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_5_0_A;
        break;
    case 6:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_5_1_A;
        break;
    case 7:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_6_1_A;
        break;
    case 8:
        layout.mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_7_1_A;
        break;
    }
    if (layout.mChannelLayoutTag != 0) {
        err = AudioQueueSetProperty(audioQueue, kAudioQueueProperty_ChannelLayout, &layout, sizeof(layout));
        if (err != noErr) {
            return NO;
        }
    }

    numberOfBuffers = 4;
    bufferByteSize = deviceFormat.mBytesPerPacket * 512;
    
    buffers = calloc(sizeof(buffers[0]), numberOfBuffers);
    
    if (!buffers)
    {
        AudioQueueDispose(audioQueue, true);
        audioQueue = NULL;
        return NO;
    }
    
    for (UInt32 i = 0; i < numberOfBuffers; ++i)
    {
        err = AudioQueueAllocateBuffer(audioQueue, bufferByteSize, buffers + i);
        if (err != noErr || buffers[i] == NULL)
        {
            err = AudioQueueDispose(audioQueue, true);
            audioQueue = NULL;
            return NO;
        }
        
        buffers[i]->mAudioDataByteSize = bufferByteSize;
    }
    
    [self prime];
    
	[outputController setFormat:&deviceFormat];
	
	return (err == noErr);	
}

- (void)prime
{
    for (UInt32 i = 0; i < numberOfBuffers; ++i)
        Sound_Renderer((__bridge void * _Nullable)(self), audioQueue, buffers[i]);
    primed = YES;
}

- (void)setVolume:(double)v
{
    volume = v * 0.01f;
    AudioQueueSetParameter(audioQueue, kAudioQueueParam_VolumeRampTime, 0);
    AudioQueueSetParameter(audioQueue, kAudioQueueParam_Volume, volume);
}

- (void)start
{
    AudioQueueSetParameter(audioQueue, kAudioQueueParam_VolumeRampTime, 0);
    AudioQueueSetParameter(audioQueue, kAudioQueueParam_Volume, volume);
    AudioQueueStart(audioQueue, NULL);
    running = YES;
    if (!primed)
        [self prime];
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
	if (outputUnit) {
		AudioUnitUninitialize (outputUnit);
		AudioComponentInstanceDispose(outputUnit);
		outputUnit = NULL;
	}
    if (audioQueue && buffers) {
        AudioQueuePause(audioQueue);
        AudioQueueStop(audioQueue, true);
        running = NO;

        for (UInt32 i = 0; i < numberOfBuffers; ++i) {
            if (buffers[i])
                AudioQueueFreeBuffer(audioQueue, buffers[i]);
            buffers[i] = NULL;
        }
        free(buffers);
        buffers = NULL;
    }
    if (audioQueue) {
        AudioQueueDispose(audioQueue, true);
        audioQueue = NULL;
    }
}

- (void)dealloc
{
	[self stop];
	
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputDevice"];
}

- (void)pause
{
    AudioQueuePause(audioQueue);
    running = NO;
}

- (void)resume
{
    AudioQueueStart(audioQueue, NULL);
    running = YES;
    if (!primed)
        [self prime];
}

@end
