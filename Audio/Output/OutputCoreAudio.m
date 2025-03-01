//
//  OutputCoreAudio.m
//  Cog
//
//  Created by Christopher Snowhill on 7/25/23.
//  Copyright 2023-2024 Christopher Snowhill. All rights reserved.
//

#import "OutputCoreAudio.h"
#import "OutputNode.h"

#ifdef _DEBUG
#import "BadSampleCleaner.h"
#endif

#import "Logging.h"

#import <Accelerate/Accelerate.h>

#import <CogAudio/VisualizationController.h>

extern void scale_by_volume(float *buffer, size_t count, float volume);

static NSString *CogPlaybackDidBeginNotificiation = @"CogPlaybackDidBeginNotificiation";

@implementation OutputCoreAudio {
	VisualizationController *visController;
}

static void *kOutputCoreAudioContext = &kOutputCoreAudioContext;

- (int)renderInput:(int)amountToRead toBuffer:(float *)buffer {
	int amountRead = 0;

	if(stopping == YES || [outputController shouldContinue] == NO) {
		// Chain is dead, fill out the serial number pointer forever with silence
		stopping = YES;
		return 0;
	}

	AudioStreamBasicDescription format;
	uint32_t config;
	if([outputController peekFormat:&format channelConfig:&config]) {
		AudioStreamBasicDescription origFormat;
		uint32_t origConfig = config;
		origFormat = format;

		if(!streamFormatStarted || config != realStreamChannelConfig || memcmp(&realStreamFormat, &format, sizeof(format)) != 0) {
			realStreamFormat = format;
			realStreamChannelConfig = config;
			streamFormatStarted = YES;
			streamFormatChanged = YES;
		}
	}

	if(streamFormatChanged) {
		return 0;
	}

	AudioChunk *chunk;
	
	if(!chunkRemain) {
		chunk = [outputController readChunk:amountToRead];
		streamTimestamp = [chunk streamTimestamp];
	} else {
		chunk = chunkRemain;
		chunkRemain = nil;
	}

	int frameCount = (int)[chunk frameCount];
	format = [chunk format];
	config = [chunk channelConfig];
	double chunkDuration = 0;

	if(frameCount) {
		chunkDuration = [chunk duration];

		NSData *samples = [chunk removeSamples:frameCount];
#ifdef _DEBUG
		[BadSampleCleaner cleanSamples:(float *)[samples bytes]
								amount:frameCount * format.mChannelsPerFrame
							  location:@"pre downmix"];
#endif
		const float *outputPtr = (const float *)[samples bytes];

		cblas_scopy((int)(frameCount * realStreamFormat.mChannelsPerFrame), outputPtr, 1, &buffer[0], 1);
		amountRead = frameCount;
	} else {
		return 0;
	}

	if(stopping) return 0;

	float volumeScale = 1.0;
	double sustained;
	sustained = secondsHdcdSustained;
	if(sustained > 0) {
		if(sustained < amountRead) {
			secondsHdcdSustained = 0;
		} else {
			secondsHdcdSustained -= chunkDuration;
			volumeScale = 0.5;
		}
	}

	scale_by_volume(&buffer[0], amountRead * realStreamFormat.mChannelsPerFrame, volumeScale * volume);

	return amountRead;
}

- (id)initWithController:(OutputNode *)c {
	self = [super init];
	if(self) {
		outputController = c;
		volume = 1.0;
		outputDeviceID = -1;

		secondsHdcdSustained = 0;

		outputLock = [[NSLock alloc] init];

#ifdef OUTPUT_LOG
		NSString *logName = [NSTemporaryDirectory() stringByAppendingPathComponent:@"CogAudioLog.raw"];
		_logFile = fopen([logName UTF8String], "wb");
#endif
	}

	return self;
}

static OSStatus
default_device_changed(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inUserData) {
	OutputCoreAudio *_self = (__bridge OutputCoreAudio *)inUserData;
	return [_self setOutputDeviceByID:-1];
}

static OSStatus
current_device_listener(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inUserData) {
	OutputCoreAudio *_self = (__bridge OutputCoreAudio *)inUserData;
	for(UInt32 i = 0; i < inNumberAddresses; ++i) {
		switch(inAddresses[i].mSelector) {
			case kAudioDevicePropertyDeviceIsAlive:
				return [_self setOutputDeviceByID:-1];

			case kAudioDevicePropertyNominalSampleRate:
			case kAudioDevicePropertyStreamFormat:
				_self->outputdevicechanged = YES;
				return noErr;
		}
	}
	return noErr;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kOutputCoreAudioContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.outputDevice"]) {
		NSDictionary *device = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];

		[self setOutputDeviceWithDeviceDict:device];
	}
}

- (BOOL)signalEndOfStream:(double)latency {
	stopped = YES;
	BOOL ret = [outputController selectNextBuffer];
	stopped = ret;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC * latency)), dispatch_get_main_queue(), ^{
		[self->outputController endOfInputPlayed];
		[self->outputController resetAmountPlayed];
	});
	return ret;
}

- (BOOL)processEndOfStream {
	if(stopping || ([outputController endOfStream] == YES && [self signalEndOfStream:[outputController getTotalLatency]])) {
		stopping = YES;
		return YES;
	}
	return NO;
}

- (void)threadEntry:(id)arg {
	@autoreleasepool {
		NSThread *currentThread = [NSThread currentThread];
		[currentThread setThreadPriority:0.75];
		[currentThread setQualityOfService:NSQualityOfServiceUserInitiated];
	}

	running = YES;
	started = NO;
	shouldPlayOutBuffer = NO;

	while(!stopping) {
		@autoreleasepool {
			if(outputdevicechanged) {
				[self updateDeviceFormat];
				outputdevicechanged = NO;
			}

			if([outputController shouldReset]) {
				[outputController setShouldReset:NO];
				[outputLock lock];
				started = NO;
				restarted = NO;
				inputRemain = 0;
				[outputLock unlock];
			}

			if(stopping)
				break;

			if(!started && !paused) {
				[self resume];
			}

			if([outputController shouldContinue] == NO) {
				break;
			}
		}

		usleep(5000);
	}

	stopped = YES;
	if(!stopInvoked) {
		[self doStop];
	}
}

- (OSStatus)setOutputDeviceByID:(AudioDeviceID)deviceID {
	OSStatus err;
	BOOL defaultDevice = NO;
	AudioObjectPropertyAddress theAddress = {
		.mSelector = kAudioHardwarePropertyDefaultOutputDevice,
		.mScope = kAudioObjectPropertyScopeGlobal,
		.mElement = kAudioObjectPropertyElementMaster
	};

	if(deviceID == -1) {
		defaultDevice = YES;
		UInt32 size = sizeof(AudioDeviceID);
		err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &size, &deviceID);

		if(err != noErr) {
			DLog(@"THERE'S NO DEFAULT OUTPUT DEVICE");

			return err;
		}
	}

	if(_au) {
		if(defaultdevicelistenerapplied && !defaultDevice) {
			/* Already set above
			 * theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice; */
			AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &theAddress, default_device_changed, (__bridge void *_Nullable)(self));
			defaultdevicelistenerapplied = NO;
		}

		outputdevicechanged = NO;

		if(outputDeviceID != deviceID) {
			if(currentdevicelistenerapplied) {
				if(devicealivelistenerapplied) {
					theAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;
					AudioObjectRemovePropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
					devicealivelistenerapplied = NO;
				}
				theAddress.mSelector = kAudioDevicePropertyStreamFormat;
				AudioObjectRemovePropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
				theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
				AudioObjectRemovePropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
				currentdevicelistenerapplied = NO;
			}

			DLog(@"Device: %i\n", deviceID);
			outputDeviceID = deviceID;

			NSError *nserr;
			[_au setDeviceID:outputDeviceID error:&nserr];
			if(nserr != nil) {
				return (OSErr)[nserr code];
			}

			outputdevicechanged = YES;
		}

		if(!currentdevicelistenerapplied) {
			if(!devicealivelistenerapplied && !defaultDevice) {
				theAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;
				AudioObjectAddPropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
				devicealivelistenerapplied = YES;
			}
			theAddress.mSelector = kAudioDevicePropertyStreamFormat;
			AudioObjectAddPropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
			theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
			AudioObjectAddPropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
			currentdevicelistenerapplied = YES;
		}

		if(!defaultdevicelistenerapplied && defaultDevice) {
			theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
			AudioObjectAddPropertyListener(kAudioObjectSystemObject, &theAddress, default_device_changed, (__bridge void *_Nullable)(self));
			defaultdevicelistenerapplied = YES;
		}
	} else {
		err = noErr;
	}

	if(err != noErr) {
		DLog(@"No output device with ID %d could be found.", deviceID);

		return err;
	}

	return err;
}

- (BOOL)setOutputDeviceWithDeviceDict:(NSDictionary *)deviceDict {
	NSNumber *deviceIDNum = [deviceDict objectForKey:@"deviceID"];
	AudioDeviceID outputDeviceID = [deviceIDNum unsignedIntValue] ?: -1;

	__block OSStatus err = [self setOutputDeviceByID:outputDeviceID];

	if(err != noErr) {
		// Try matching by name.
		NSString *userDeviceName = deviceDict[@"name"];

		[self enumerateAudioOutputsUsingBlock:
			  ^(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop) {
				  if([deviceName isEqualToString:userDeviceName]) {
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

	if(err != noErr) {
		ALog(@"No output device could be found, your random error code is %d. Have a nice day!", err);

		return NO;
	}

	return YES;
}

// The following is largely a copy pasta of -awakeFromNib from "OutputsArrayController.m".
// TODO: Share the code. (How to do this across xcodeproj?)
- (void)enumerateAudioOutputsUsingBlock:(void(NS_NOESCAPE ^ _Nonnull)(NSString *deviceName, AudioDeviceID deviceID, AudioDeviceID systemDefaultID, BOOL *stop))block {
	UInt32 propsize;
	AudioObjectPropertyAddress theAddress = {
		.mSelector = kAudioHardwarePropertyDevices,
		.mScope = kAudioObjectPropertyScopeGlobal,
		.mElement = kAudioObjectPropertyElementMaster
	};

	__Verify_noErr(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize));
	UInt32 nDevices = propsize / (UInt32)sizeof(AudioDeviceID);
	AudioDeviceID *devids = (AudioDeviceID *)malloc(propsize);
	__Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, devids));

	theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
	AudioDeviceID systemDefault;
	propsize = sizeof(systemDefault);
	__Verify_noErr(AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, &systemDefault));

	theAddress.mScope = kAudioDevicePropertyScopeOutput;

	for(UInt32 i = 0; i < nDevices; ++i) {
		UInt32 isAlive = 0;
		propsize = sizeof(isAlive);
		theAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &isAlive));
		if(!isAlive) continue;

		CFStringRef name = NULL;
		propsize = sizeof(name);
		theAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &name));

		propsize = 0;
		theAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
		__Verify_noErr(AudioObjectGetPropertyDataSize(devids[i], &theAddress, 0, NULL, &propsize));

		if(propsize < sizeof(UInt32)) {
			if(name) CFRelease(name);
			continue;
		}

		AudioBufferList *bufferList = (AudioBufferList *)malloc(propsize);
		__Verify_noErr(AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, bufferList));
		UInt32 bufferCount = bufferList->mNumberBuffers;
		free(bufferList);

		if(!bufferCount) {
			if(name) CFRelease(name);
			continue;
		}

		BOOL stop = NO;
		NSString *deviceName = name ? [NSString stringWithString:(__bridge NSString *)name] : [NSString stringWithFormat:@"Unknown device %u", (unsigned int)devids[i]];

		block(deviceName,
			  devids[i],
			  systemDefault,
			  &stop);

		CFRelease(name);

		if(stop) {
			break;
		}
	}

	free(devids);
}

- (BOOL)updateDeviceFormat {
	AVAudioFormat *format = _au.outputBusses[0].format;

	if(!_deviceFormat || ![_deviceFormat isEqual:format]) {
		NSError *err;
		AVAudioFormat *renderFormat;

		_deviceFormat = format;
		deviceFormat = *(format.streamDescription);

		/// Seems some 3rd party devices return incorrect stuff...or I just don't like noninterleaved data.
		deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
		//    deviceFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsFloat;
		//    deviceFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
		// We don't want more than 8 channels
		if(deviceFormat.mChannelsPerFrame > 8) {
			deviceFormat.mChannelsPerFrame = 8;
		}
		deviceFormat.mBytesPerFrame = deviceFormat.mChannelsPerFrame * (deviceFormat.mBitsPerChannel / 8);
		deviceFormat.mBytesPerPacket = deviceFormat.mBytesPerFrame * deviceFormat.mFramesPerPacket;

		/* Set the channel layout for the audio queue */
		AudioChannelLayoutTag tag = 0;
		switch(deviceFormat.mChannelsPerFrame) {
			case 1:
				tag = kAudioChannelLayoutTag_Mono;
				deviceChannelConfig = AudioConfigMono;
				break;
			case 2:
				tag = kAudioChannelLayoutTag_Stereo;
				deviceChannelConfig = AudioConfigStereo;
				break;
			case 3:
				tag = kAudioChannelLayoutTag_DVD_4;
				deviceChannelConfig = AudioConfig3Point0;
				break;
			case 4:
				tag = kAudioChannelLayoutTag_Quadraphonic;
				deviceChannelConfig = AudioConfig4Point0;
				break;
			case 5:
				tag = kAudioChannelLayoutTag_MPEG_5_0_A;
				deviceChannelConfig = AudioConfig5Point0;
				break;
			case 6:
				tag = kAudioChannelLayoutTag_MPEG_5_1_A;
				deviceChannelConfig = AudioConfig5Point1;
				break;
			case 7:
				tag = kAudioChannelLayoutTag_MPEG_6_1_A;
				deviceChannelConfig = AudioConfig6Point1;
				break;
			case 8:
				tag = kAudioChannelLayoutTag_MPEG_7_1_A;
				deviceChannelConfig = AudioConfig7Point1;
				break;
		}

		renderFormat = [[AVAudioFormat alloc] initWithStreamDescription:&deviceFormat channelLayout:[[AVAudioChannelLayout alloc] initWithLayoutTag:tag]];
		[_au.inputBusses[0] setFormat:renderFormat error:&err];
		if(err != nil)
			return NO;

		[outputController setFormat:&deviceFormat channelConfig:deviceChannelConfig];
	}

	return YES;
}

- (void)updateStreamFormat {
	/* Set the channel layout for the audio queue */
	resetStreamFormat = NO;

	uint32_t channels = realStreamFormat.mChannelsPerFrame;
	uint32_t channelConfig = realStreamChannelConfig;

	streamFormat = realStreamFormat;
	streamFormat.mChannelsPerFrame = channels;
	streamFormat.mBytesPerFrame = sizeof(float) * channels;
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mBytesPerPacket = sizeof(float) * channels;
	streamChannelConfig = channelConfig;

	AudioChannelLayoutTag tag = 0;

	AudioChannelLayout layout = { 0 };
	switch(streamChannelConfig) {
		case AudioConfigMono:
			tag = kAudioChannelLayoutTag_Mono;
			break;
		case AudioConfigStereo:
			tag = kAudioChannelLayoutTag_Stereo;
			break;
		case AudioConfig3Point0:
			tag = kAudioChannelLayoutTag_WAVE_3_0;
			break;
		case AudioConfig4Point0:
			tag = kAudioChannelLayoutTag_WAVE_4_0_A;
			break;
		case AudioConfig5Point0:
			tag = kAudioChannelLayoutTag_WAVE_5_0_A;
			break;
		case AudioConfig5Point1:
			tag = kAudioChannelLayoutTag_WAVE_5_1_A;
			break;
		case AudioConfig6Point1:
			tag = kAudioChannelLayoutTag_WAVE_6_1;
			break;
		case AudioConfig7Point1:
			tag = kAudioChannelLayoutTag_WAVE_7_1;
			break;

		default:
			tag = 0;
			break;
	}

	if(tag) {
		layout.mChannelLayoutTag = tag;
	} else {
		layout.mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelBitmap;
		layout.mChannelBitmap = streamChannelConfig;
	}
}

- (int)renderAndConvert {
	int inputRendered = 0;
	int bytesRendered = inputRendered * realStreamFormat.mBytesPerPacket;

	if(resetStreamFormat) {
		[self updateStreamFormat];
		if([self processEndOfStream]) {
			return 0;
		}
	}

	while(inputRendered < 4096) {
		int maxToRender = MIN(4096 - inputRendered, 512);
		int rendered = [self renderInput:maxToRender toBuffer:&tempBuffer[0]];
		if(rendered > 0) {
			memcpy(((uint8_t*)&inputBuffer[0]) + bytesRendered, &tempBuffer[0], rendered * realStreamFormat.mBytesPerPacket);
		}
		inputRendered += rendered;
		bytesRendered += rendered * realStreamFormat.mBytesPerPacket;
		if(streamFormatChanged) {
			streamFormatChanged = NO;
			if(inputRendered) {
				resetStreamFormat = YES;
				break;
			} else {
				[self updateStreamFormat];
			}
		}
		if([self processEndOfStream]) break;
	}

	int samplesRendered = inputRendered;

	samplePtr = &inputBuffer[0];

#ifdef OUTPUT_LOG
	if(samplesRendered) {
		size_t dataByteSize = samplesRendered * sizeof(float) * deviceFormat.mChannelsPerFrame;

		fwrite(samplePtr, 1, dataByteSize, _logFile);
	}
#endif

	return samplesRendered;
}

- (void)audioOutputBlock {
	__block AudioStreamBasicDescription *format = &deviceFormat;
	__block void *refCon = (__bridge void *)self;

#ifdef OUTPUT_LOG
	__block FILE *logFile = _logFile;
#endif

	_au.outputProvider = ^AUAudioUnitStatus(AudioUnitRenderActionFlags *_Nonnull actionFlags, const AudioTimeStamp *_Nonnull timestamp, AUAudioFrameCount frameCount, NSInteger inputBusNumber, AudioBufferList *_Nonnull inputData) {
		if(!frameCount) return 0;

		const int channels = format->mChannelsPerFrame;
		if(!channels) return 0;

		OutputCoreAudio *_self = (__bridge OutputCoreAudio *)refCon;
		int renderedSamples = 0;

		@autoreleasepool {
			while(renderedSamples < frameCount) {
				int inputRemain = _self->inputRemain;
				while(!inputRemain) {
					inputRemain = [_self renderAndConvert];
					if(_self->stopping) {
						inputData->mBuffers[0].mDataByteSize = frameCount * format->mBytesPerPacket;
						inputData->mBuffers[0].mNumberChannels = channels;
						bzero(inputData->mBuffers[0].mData, inputData->mBuffers[0].mDataByteSize);
						return 0;
					}
				}
				if(inputRemain) {
					int inputTodo = MIN(inputRemain, frameCount - renderedSamples);
					cblas_scopy(inputTodo * channels, _self->samplePtr, 1, ((float *)inputData->mBuffers[0].mData) + renderedSamples * channels, 1);
					_self->samplePtr += inputTodo * channels;
					inputRemain -= inputTodo;
					renderedSamples += inputTodo;
				}
				_self->inputRemain = inputRemain;
			}

			inputData->mBuffers[0].mDataByteSize = renderedSamples * format->mBytesPerPacket;
			inputData->mBuffers[0].mNumberChannels = channels;

			double secondsRendered = (double)renderedSamples / format->mSampleRate;
			[_self updateLatency:secondsRendered];
		}

#ifdef _DEBUG
		[BadSampleCleaner cleanSamples:(float *)inputData->mBuffers[0].mData
								amount:inputData->mBuffers[0].mDataByteSize / sizeof(float)
							  location:@"final output"];
#endif

		return 0;
	};
}

- (BOOL)setup {
	if(_au)
		[self stop];

	@synchronized(self) {
		stopInvoked = NO;
		stopCompleted = NO;
		commandStop = NO;
		shouldPlayOutBuffer = NO;

		resetStreamFormat = NO;
		streamFormatChanged = NO;
		streamFormatStarted = NO;

		running = NO;
		stopping = NO;
		stopped = NO;
		paused = NO;
		outputDeviceID = -1;
		restarted = NO;

		inputRemain = 0;

		chunkRemain = nil;

		AudioComponentDescription desc;
		NSError *err;

		desc.componentType = kAudioUnitType_Output;
		desc.componentSubType = kAudioUnitSubType_HALOutput;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;

		_au = [[AUAudioUnit alloc] initWithComponentDescription:desc error:&err];
		if(err != nil)
			return NO;

		// Setup the output device before mucking with settings
		NSDictionary *device = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];
		if(device) {
			BOOL ok = [self setOutputDeviceWithDeviceDict:device];
			if(!ok) {
				// Ruh roh.
				[self setOutputDeviceWithDeviceDict:nil];

				[[[NSUserDefaultsController sharedUserDefaultsController] defaults] removeObjectForKey:@"outputDevice"];
			}
		} else {
			[self setOutputDeviceWithDeviceDict:nil];
		}

		[self audioOutputBlock];

		[_au allocateRenderResourcesAndReturnError:&err];

		[self updateDeviceFormat];

		visController = [VisualizationController sharedController];

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:kOutputCoreAudioContext];

		observersapplied = YES;

		return (err == nil);
	}
}

- (void)updateLatency:(double)secondsPlayed {
	if(secondsPlayed > 0) {
		[outputController setAmountPlayed:streamTimestamp];
	}
	[visController postLatency:[outputController getPostVisLatency]];
}

- (double)volume {
	return volume * 100.0f;
}

- (void)setVolume:(double)v {
	volume = v * 0.01f;
}

- (double)latency {
	return 0.0;
}

- (void)start {
	[self threadEntry:nil];
}

- (void)stop {
	commandStop = YES;
	[self doStop];
}

- (void)doStop {
	if(stopInvoked) {
		return;
	}
	@synchronized(self) {
		stopInvoked = YES;
		if(observersapplied) {
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputDevice" context:kOutputCoreAudioContext];
			observersapplied = NO;
		}
		stopping = YES;
		paused = NO;
		if(defaultdevicelistenerapplied || currentdevicelistenerapplied || devicealivelistenerapplied) {
			AudioObjectPropertyAddress theAddress = {
				.mScope = kAudioObjectPropertyScopeGlobal,
				.mElement = kAudioObjectPropertyElementMaster
			};
			if(defaultdevicelistenerapplied) {
				theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
				AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &theAddress, default_device_changed, (__bridge void *_Nullable)(self));
				defaultdevicelistenerapplied = NO;
			}
			if(devicealivelistenerapplied) {
				theAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;
				AudioObjectRemovePropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
				devicealivelistenerapplied = NO;
			}
			if(currentdevicelistenerapplied) {
				theAddress.mSelector = kAudioDevicePropertyStreamFormat;
				AudioObjectRemovePropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
				theAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
				AudioObjectRemovePropertyListener(outputDeviceID, &theAddress, current_device_listener, (__bridge void *_Nullable)(self));
				currentdevicelistenerapplied = NO;
			}
		}
		if(_au) {
			if(shouldPlayOutBuffer && !commandStop) {
				int compareVal = 0;
				double secondsLatency = [outputController getTotalLatency];
				int compareMax = (((1000000 / 5000) * secondsLatency) + (10000 / 5000)); // latency plus 10ms, divide by sleep intervals
				do {
					compareVal = [outputController getTotalLatency];
					usleep(5000);
				} while(!commandStop && compareVal > 0 && compareMax-- > 0);
			}
			[_au stopHardware];
			_au = nil;
		}
		if(running) {
			while(!stopped) {
				stopping = YES;
				usleep(5000);
			}
		}
#ifdef OUTPUT_LOG
		if(_logFile) {
			fclose(_logFile);
			_logFile = NULL;
		}
#endif
		outputController = nil;
		if(visController) {
			[visController reset];
			visController = nil;
		}
		stopCompleted = YES;
	}
}

- (void)dealloc {
	[self stop];
	// In case stop called on another thread first
	while(!stopCompleted) {
		usleep(500);
	}
}

- (void)pause {
	paused = YES;
	if(started)
		[_au stopHardware];
}

- (void)resume {
	NSError *err;
	[_au startHardwareAndReturnError:&err];
	paused = NO;
	started = YES;
}

- (void)sustainHDCD {
	secondsHdcdSustained = 10.0;
}

- (void)setShouldPlayOutBuffer:(BOOL)s {
	shouldPlayOutBuffer = s;
}

- (AudioStreamBasicDescription)deviceFormat {
	return deviceFormat;
}

- (uint32_t)deviceChannelConfig {
	return deviceChannelConfig;
}

@end
