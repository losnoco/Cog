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

@implementation OutputCoreAudio

- (id)initWithController:(OutputNode *)c
{
	self = [super init];
	if (self)
	{
		outputController = c;
		outputUnit = NULL;

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:NULL];
	}
	
	return self;
}

static OSStatus Sound_Renderer(void *inRefCon,  AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp  *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList  *ioData)
{
	OutputCoreAudio *output = (__bridge OutputCoreAudio *)inRefCon;
	OSStatus err = noErr;
	void *readPointer = ioData->mBuffers[0].mData;
	
	int amountToRead, amountRead;
    
    if (output->stopping == YES)
    {
        // *shrug* At least this will stop it from trying to emit data post-shutdown
        ioData->mBuffers[0].mDataByteSize = 0;
        return eofErr;
    }

	if ([output->outputController shouldContinue] == NO)
	{
        AudioOutputUnitStop(output->outputUnit);
//		[output stop];
		
		return err;
	}
	
	amountToRead = inNumberFrames*(output->deviceFormat.mBytesPerPacket);
	amountRead = [output->outputController readData:(readPointer) amount:amountToRead];

	if ((amountRead < amountToRead) && [output->outputController endOfStream] == NO) //Try one more time! for track changes!
	{
		int amountRead2; //Use this since return type of readdata isnt known...may want to fix then can do a simple += to readdata
		amountRead2 = [output->outputController readData:(readPointer+amountRead) amount:amountToRead-amountRead];
		amountRead += amountRead2;
	}
	
	ioData->mBuffers[0].mDataByteSize = amountRead;
	ioData->mBuffers[0].mNumberChannels = output->deviceFormat.mChannelsPerFrame;
	ioData->mNumberBuffers = 1;
	
	return err;
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
	
	if (deviceID == -1) {
		UInt32 size = sizeof(AudioDeviceID);
        AudioObjectPropertyAddress theAddress = {
            .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
            .mScope = kAudioObjectPropertyScopeGlobal,
            .mElement = kAudioObjectPropertyElementMaster
        };
        err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &size, &deviceID);
								
		if (err != noErr) {
			DLog(@"THERE'S NO DEFAULT OUTPUT DEVICE");
			
			return err;
		}
		else {
			outputDeviceID = deviceID;
		}
	}

	printf("DEVICE: %i\n", deviceID);
    outputDeviceID = deviceID;
	
	err = AudioUnitSetProperty(outputUnit,
							  kAudioOutputUnitProperty_CurrentDevice,
							  kAudioUnitScope_Output,
							  0,
							  &deviceID,
							  sizeof(AudioDeviceID));
	
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
		
		if (propsize < sizeof(UInt32)) continue;
		
		AudioBufferList * bufferList = (AudioBufferList *) malloc(propsize);
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, bufferList));
		UInt32 bufferCount = bufferList->mNumberBuffers;
		free(bufferList);
		
		if (!bufferCount) continue;
		
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
	if (outputUnit)
		[self stop];
	
	AudioObjectPropertyAddress propertyAddress = {
		.mElement	= kAudioObjectPropertyElementMaster
	};
    UInt32 dataSize;
	
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
	
	// change output format...
	
	// The default channel map is silence
	SInt32 deviceChannelMap [deviceFormat.mChannelsPerFrame];
	for(UInt32 i = 0; i < deviceFormat.mChannelsPerFrame; ++i)
		deviceChannelMap[i] = -1;
    
	// Determine the device's preferred stereo channels for output mapping
	if(1 == deviceFormat.mChannelsPerFrame || 2 == deviceFormat.mChannelsPerFrame) {
		propertyAddress.mSelector = kAudioDevicePropertyPreferredChannelsForStereo;
		propertyAddress.mScope = kAudioDevicePropertyScopeOutput;
        
		UInt32 preferredStereoChannels [2] = { 1, 2 };
		if(AudioObjectHasProperty(outputDeviceID, &propertyAddress)) {
			dataSize = sizeof(preferredStereoChannels);
            
			err = AudioObjectGetPropertyData(outputDeviceID, &propertyAddress, 0, nil, &dataSize, &preferredStereoChannels);
		}
        
		AudioChannelLayout stereoLayout;
		stereoLayout.mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
        
		const AudioChannelLayout *specifier [1] = { &stereoLayout };
        
		SInt32 stereoChannelMap [2] = { 1, 2 };
		dataSize = sizeof(stereoChannelMap);
		err = AudioFormatGetProperty(kAudioFormatProperty_ChannelMap, sizeof(specifier), specifier, &dataSize, stereoChannelMap);
        
		if(noErr == err) {
			deviceChannelMap[preferredStereoChannels[0] - 1] = stereoChannelMap[0];
			deviceChannelMap[preferredStereoChannels[1] - 1] = stereoChannelMap[1];
		}
		else {
			// Just use a channel map that makes sense
			deviceChannelMap[preferredStereoChannels[0] - 1] = 0;
			deviceChannelMap[preferredStereoChannels[1] - 1] = 1;
		}
	}
	// Determine the device's preferred multichannel layout
	else {
		propertyAddress.mSelector = kAudioDevicePropertyPreferredChannelLayout;
		propertyAddress.mScope = kAudioDevicePropertyScopeOutput;
        
		if(AudioObjectHasProperty(outputDeviceID, &propertyAddress)) {
			err = AudioObjectGetPropertyDataSize(outputDeviceID, &propertyAddress, 0, nil, &dataSize);
            
			AudioChannelLayout *preferredChannelLayout = (AudioChannelLayout *)(malloc(dataSize));
            
			err = AudioObjectGetPropertyData(outputDeviceID, &propertyAddress, 0, nil, &dataSize, preferredChannelLayout);
            
			const AudioChannelLayout *specifier [1] = { preferredChannelLayout };
            
			// Not all channel layouts can be mapped, so handle failure with a generic mapping
			dataSize = (UInt32)sizeof(deviceChannelMap);
			err = AudioFormatGetProperty(kAudioFormatProperty_ChannelMap, sizeof(specifier), specifier, &dataSize, deviceChannelMap);
            
			if(noErr != err) {
				// Just use a channel map that makes sense
				for(UInt32 i = 0; i < deviceFormat.mChannelsPerFrame; ++i)
					deviceChannelMap[i] = i;
			}
            
            free(preferredChannelLayout); preferredChannelLayout = nil;
		}
		else {
			// Just use a channel map that makes sense
			for(UInt32 i = 0; i < deviceFormat.mChannelsPerFrame; ++i)
				deviceChannelMap[i] = i;
		}
	}

	///Seems some 3rd party devices return incorrect stuff...or I just don't like noninterleaved data.
	deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
    // Bluetooth devices in communications mode tend to have reduced settings,
    // so let's work around that.
    // For some reason, mono output just doesn't work, so bleh.
    if (deviceFormat.mChannelsPerFrame < 2)
        deviceFormat.mChannelsPerFrame = 2;
    // And sample rate will be cruddy for the duration of playback, so fix it.
    if (deviceFormat.mSampleRate < 32000)
        deviceFormat.mSampleRate = 48000;
//	deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsFloat;
//	deviceFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
	deviceFormat.mBytesPerFrame = deviceFormat.mChannelsPerFrame*(deviceFormat.mBitsPerChannel/8);
	deviceFormat.mBytesPerPacket = deviceFormat.mBytesPerFrame * deviceFormat.mFramesPerPacket;
	
	err = AudioUnitSetProperty (outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Output,
								0,
								&deviceFormat,
								size);
	
	//Set the stream format of the output to match the input
	err = AudioUnitSetProperty (outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Input,
								0,
								&deviceFormat,
								size);
    
    size = (unsigned int) sizeof(deviceChannelMap);
    err = AudioUnitSetProperty(outputUnit,
                               kAudioOutputUnitProperty_ChannelMap,
                               kAudioUnitScope_Output,
                               0,
                               deviceChannelMap,
                               size);
	
	//setup render callbacks
    stopping = NO;
	renderCallback.inputProc = Sound_Renderer;
	renderCallback.inputProcRefCon = (__bridge void * _Nullable)(self);
	
	AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &renderCallback, sizeof(AURenderCallbackStruct));	
	
	[outputController setFormat:&deviceFormat];
	
	return (err == noErr);	
}

- (void)setVolume:(double)v
{
	AudioUnitSetParameter (outputUnit,
							kHALOutputParam_Volume,
							kAudioUnitScope_Global,
							0,
							v * 0.01f,
							0);
}	

- (void)start
{
	AudioOutputUnitStart(outputUnit);
}

- (void)stop
{
	if (outputUnit)
	{
        stopping = YES;
        AudioOutputUnitStop(outputUnit);
		AudioUnitUninitialize (outputUnit);
		AudioComponentInstanceDispose(outputUnit);
		outputUnit = NULL;
	}
}

- (void)dealloc
{
	[self stop];
	
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputDevice"];
}

- (void)pause
{
	AudioOutputUnitStop(outputUnit);
}

- (void)resume
{
	AudioOutputUnitStart(outputUnit);
}

@end
