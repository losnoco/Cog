//
//  OutputAVFoundation.m
//  Cog
//
//  Created by Christopher Snowhill on 6/23/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

#import "OutputAVFoundation.h"
#import "OutputNode.h"

#ifdef _DEBUG
#import "BadSampleCleaner.h"
#endif

#import "Logging.h"

#import <Accelerate/Accelerate.h>

extern void scale_by_volume(float *buffer, size_t count, float volume);

static NSString *CogPlaybackDidBeginNotficiation = @"CogPlaybackDidBeginNotficiation";

@implementation OutputAVFoundation

static void *kOutputAVFoundationContext = &kOutputAVFoundationContext;

static void fillBuffers(AudioBufferList *ioData, const float *inbuffer, size_t count, size_t offset) {
	const size_t channels = ioData->mNumberBuffers;
	for(int i = 0; i < channels; ++i) {
		const size_t maxCount = (ioData->mBuffers[i].mDataByteSize / sizeof(float)) - offset;
		float *output = ((float *)ioData->mBuffers[i].mData) + offset;
		const float *input = inbuffer + i;
		cblas_scopy((int)((count > maxCount) ? maxCount : count), input, (int)channels, output, 1);
		ioData->mBuffers[i].mNumberChannels = 1;
	}
}

static void clearBuffers(AudioBufferList *ioData, size_t count, size_t offset) {
	for(int i = 0; i < ioData->mNumberBuffers; ++i) {
		memset(ioData->mBuffers[i].mData + offset * sizeof(float), 0, count * sizeof(float));
		ioData->mBuffers[i].mNumberChannels = 1;
	}
}

static OSStatus eqRenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
	if(inNumberFrames > 1024 || !inRefCon) {
		clearBuffers(ioData, inNumberFrames, 0);
		return 0;
	}

	OutputAVFoundation *_self = (__bridge OutputAVFoundation *)inRefCon;

	fillBuffers(ioData, &_self->inputBuffer[0], inNumberFrames, 0);

	return 0;
}

- (int)renderInput {
	int amountToRead, amountRead = 0;

	amountToRead = 1024;

	int visTabulated = 0;
	float visAudio[amountToRead]; // Chunk size

	if(stopping == YES || [outputController shouldContinue] == NO) {
		// Chain is dead, fill out the serial number pointer forever with silence
		stopping = YES;
		return 0;
	}

	AudioChunk *chunk = [outputController readChunk:1024];

	int frameCount = (int)[chunk frameCount];
	AudioStreamBasicDescription format = [chunk format];
	uint32_t config = [chunk channelConfig];
	double chunkDuration = 0;

	if(frameCount) {
		// XXX ERROR with AirPods - Can't go higher than CD*8 surround - 192k stereo
		// Emits to console: [AUScotty] Initialize: invalid FFT size 16384
		// DSD256 stereo emits: [AUScotty] Initialize: invalid FFT size 65536
		BOOL formatClipped = NO;
		BOOL isSurround = format.mChannelsPerFrame > 2;
		const double maxSampleRate = isSurround ? 352800.0 : 192000.0;
		if(format.mSampleRate > maxSampleRate) {
			format.mSampleRate = maxSampleRate;
			formatClipped = YES;
		}
		if(!streamFormatStarted || config != streamChannelConfig || memcmp(&streamFormat, &format, sizeof(format)) != 0) {
			if(formatClipped) {
				ALog(@"Sample rate clipped to no more than %f Hz!", maxSampleRate);
			}
			streamFormat = format;
			streamChannelConfig = config;
			streamFormatStarted = YES;
			[self updateStreamFormat];
			downmixerForVis = [[DownmixProcessor alloc] initWithInputFormat:format inputConfig:config andOutputFormat:visFormat outputConfig:AudioConfigMono];
		}

		chunkDuration = [chunk duration];

		NSData *samples = [chunk removeSamples:frameCount];
#ifdef _DEBUG
		[BadSampleCleaner cleanSamples:(float *)[samples bytes]
		                        amount:frameCount * format.mChannelsPerFrame
		                      location:@"pre downmix"];
#endif

		[downmixerForVis process:[samples bytes]
		              frameCount:frameCount
		                  output:visAudio];
		visTabulated += frameCount;

		cblas_scopy((int)(frameCount * streamFormat.mChannelsPerFrame), (const float *)[samples bytes], 1, &inputBuffer[0], 1);
		amountRead = frameCount;
	} else {
		return 0;
	}

	float volumeScale = 1.0;
	double sustained;
	@synchronized(self) {
		sustained = secondsHdcdSustained;
	}
	if(sustained > 0) {
		if(sustained < amountRead) {
			@synchronized(self) {
				secondsHdcdSustained = 0;
			}
		} else {
			@synchronized(self) {
				secondsHdcdSustained -= chunkDuration;
			}
			volumeScale = 0.5;
		}
	}

	if(eqEnabled) {
		volumeScale *= eqPreamp;
	}

	scale_by_volume(&inputBuffer[0], amountRead * streamFormat.mChannelsPerFrame, volumeScale);

	[visController postSampleRate:streamFormat.mSampleRate];
	[visController postVisPCM:visAudio amount:visTabulated];

	return amountRead;
}

- (id)initWithController:(OutputNode *)c {
	self = [super init];
	if(self) {
		outputController = c;
		volume = 1.0;
		outputDeviceID = -1;

		secondsHdcdSustained = 0;

#ifdef OUTPUT_LOG
		_logFile = fopen("/tmp/CogAudioLog.raw", "wb");
#endif
	}

	return self;
}

static OSStatus
default_device_changed(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inUserData) {
	OutputAVFoundation *this = (__bridge OutputAVFoundation *)inUserData;
	return [this setOutputDeviceByID:-1];
}

static OSStatus
current_device_listener(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inUserData) {
	OutputAVFoundation *this = (__bridge OutputAVFoundation *)inUserData;
	for(UInt32 i = 0; i < inNumberAddresses; ++i) {
		switch(inAddresses[i].mSelector) {
			case kAudioDevicePropertyDeviceIsAlive:
				return [this setOutputDeviceByID:-1];

			case kAudioDevicePropertyNominalSampleRate:
			case kAudioDevicePropertyStreamFormat:
				this->outputdevicechanged = YES;
				return noErr;
		}
	}
	return noErr;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kOutputAVFoundationContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.outputDevice"]) {
		NSDictionary *device = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"outputDevice"];

		[self setOutputDeviceWithDeviceDict:device];
	} else if([keyPath isEqualToString:@"values.GraphicEQenable"]) {
		BOOL enabled = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue];

		[self setEqualizerEnabled:enabled];
	} else if([keyPath isEqualToString:@"values.eqPreamp"]) {
		float preamp = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] floatForKey:@"eqPreamp"];
		eqPreamp = pow(10.0, preamp / 20.0);
	} else if([keyPath isEqualToString:@"values.dontRemix"]) {
		dontRemix = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"dontRemix"];
	}
}

- (BOOL)signalEndOfStream:(double)latency {
	BOOL ret = [outputController selectNextBuffer];
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC * latency)), dispatch_get_main_queue(), ^{
		[self->outputController endOfInputPlayed];
		[self->outputController resetAmountPlayed];
		self->lastCheckpointPts = self->trackPts;
		self->trackPts = kCMTimeZero;
	});
	return ret;
}

- (void)threadEntry:(id)arg {
	running = YES;
	started = NO;
	secondsLatency = 1.0;

	while(!stopping) {
		if([outputController shouldReset]) {
			[outputController setShouldReset:NO];
			[self removeSynchronizerBlock];
			[renderSynchronizer setRate:0];
			[audioRenderer stopRequestingMediaData];
			[audioRenderer flush];
			currentPts = kCMTimeZero;
			lastPts = kCMTimeZero;
			outputPts = kCMTimeZero;
			trackPts = kCMTimeZero;
			lastCheckpointPts = kCMTimeZero;
			secondsLatency = 1.0;
			started = NO;
			[self synchronizerBlock];
		}

		if(stopping)
			break;

		while(!stopping && [audioRenderer isReadyForMoreMediaData]) {
			CMSampleBufferRef bufferRef = [self makeSampleBuffer];

			if(bufferRef) {
				CMTime chunkDuration = CMSampleBufferGetDuration(bufferRef);

				outputPts = CMTimeAdd(outputPts, chunkDuration);
				trackPts = CMTimeAdd(trackPts, chunkDuration);

				[audioRenderer enqueueSampleBuffer:bufferRef];
			} else {
				if([outputController endOfStream] == YES && ![self signalEndOfStream:secondsLatency]) {
					// Wait for output to catch up
					CMTime currentTime;
					[currentPtsLock lock];
					currentTime = currentPts;
					[currentPtsLock unlock];
					CMTime timeToWait = CMTimeSubtract(outputPts, currentTime);
					double secondsToWait = CMTimeGetSeconds(timeToWait);
					if(secondsToWait > 0) {
						usleep(secondsToWait * 1000000.0);
					}
					stopping = YES;
				}
				break;
			}
		}

		if(!paused && !started) {
			[self resume];
		}

		if([outputController shouldContinue] == NO) {
			break;
		}

		usleep(5000);
	}

	stopped = YES;
	[self stop];
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

	if(audioRenderer) {
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

			CFStringRef deviceUID = NULL;
			theAddress.mSelector = kAudioDevicePropertyDeviceUID;
			UInt32 size = sizeof(deviceUID);

			err = AudioObjectGetPropertyData(outputDeviceID, &theAddress, 0, NULL, &size, &deviceUID);
			if(err != noErr) {
				DLog(@"Unable to get UID of device");
				return err;
			}

			[audioRenderer setAudioOutputDeviceUniqueID:(__bridge NSString *)deviceUID];
			CFRelease(deviceUID);

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
	AudioDeviceID *devids = malloc(propsize);
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

- (void)updateStreamFormat {
	visFormat = streamFormat;
	visFormat.mChannelsPerFrame = 1;
	visFormat.mBytesPerFrame = visFormat.mChannelsPerFrame * (visFormat.mBitsPerChannel / 8);
	visFormat.mBytesPerPacket = visFormat.mBytesPerFrame * visFormat.mFramesPerPacket;

	/* Set the channel layout for the audio queue */
	AudioChannelLayoutTag tag = 0;
	AudioChannelLayout layout = { 0 };
	switch(streamFormat.mChannelsPerFrame) {
		case 1:
			tag = kAudioChannelLayoutTag_Mono;
			deviceChannelConfig = AudioConfigMono;
			break;
		case 2:
			tag = kAudioChannelLayoutTag_Stereo;
			deviceChannelConfig = AudioConfigStereo;
			break;
		case 3:
			tag = kAudioChannelLayoutTag_WAVE_3_0;
			deviceChannelConfig = AudioConfig3Point0;
			break;
		case 4:
			tag = kAudioChannelLayoutTag_WAVE_4_0_A;
			deviceChannelConfig = AudioConfig4Point0;
			break;
		case 5:
			tag = kAudioChannelLayoutTag_WAVE_5_0_A;
			deviceChannelConfig = AudioConfig5Point0;
			break;
		case 6:
			tag = kAudioChannelLayoutTag_WAVE_5_1_A;
			deviceChannelConfig = AudioConfig5Point1;
			break;
		case 7:
			tag = kAudioChannelLayoutTag_WAVE_6_1;
			deviceChannelConfig = AudioConfig6Point1;
			break;
		case 8:
			tag = kAudioChannelLayoutTag_WAVE_7_1;
			deviceChannelConfig = AudioConfig7Point1;
			break;

		default:
			tag = 0;
			break;
	}

	streamTag = tag;

	if(tag) {
		layout.mChannelLayoutTag = tag;
	}

	if(CMAudioFormatDescriptionCreate(kCFAllocatorDefault, &streamFormat, sizeof(layout), &layout, 0, NULL, NULL, &audioFormatDescription) != noErr) {
		return;
	}

	AudioStreamBasicDescription asbd = streamFormat;

	asbd.mFormatFlags &= ~kAudioFormatFlagIsPacked;

	UInt32 maximumFrames = 1024;
	AudioUnitSetProperty(_eq, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maximumFrames, sizeof(maximumFrames));

	AudioUnitSetProperty(_eq, kAudioUnitProperty_StreamFormat,
	                     kAudioUnitScope_Input, 0, &asbd, sizeof(asbd));

	AudioUnitSetProperty(_eq, kAudioUnitProperty_StreamFormat,
	                     kAudioUnitScope_Output, 0, &asbd, sizeof(asbd));
	AudioUnitReset(_eq, kAudioUnitScope_Input, 0);
	AudioUnitReset(_eq, kAudioUnitScope_Output, 0);

	AudioUnitReset(_eq, kAudioUnitScope_Global, 0);

	eqEnabled = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue];
}

- (CMSampleBufferRef)makeSampleBuffer {
	CMBlockBufferRef blockBuffer = nil;

	int samplesRendered = [self makeBlockBuffer:&blockBuffer];
	if(!samplesRendered) return nil;

	CMSampleBufferRef sampleBuffer = nil;

	OSStatus err = CMAudioSampleBufferCreateReadyWithPacketDescriptions(kCFAllocatorDefault, blockBuffer, audioFormatDescription, samplesRendered, outputPts, nil, &sampleBuffer);
	if(err != noErr) {
		return nil;
	}

	return sampleBuffer;
}

- (int)makeBlockBuffer:(CMBlockBufferRef *)blockBufferOut {
	OSStatus status;
	CMBlockBufferRef blockListBuffer = nil;

	status = CMBlockBufferCreateEmpty(kCFAllocatorDefault, 0, 0, &blockListBuffer);
	if(status != noErr || !blockListBuffer) return 0;

	int samplesRendered = [self renderInput];

	if(samplesRendered) {
		if(eqEnabled) {
			const int channels = streamFormat.mChannelsPerFrame;
			if(channels > 0) {
				const size_t channelsminusone = channels - 1;
				uint8_t tempBuffer[sizeof(AudioBufferList) + sizeof(AudioBuffer) * channelsminusone];
				AudioBufferList *ioData = (AudioBufferList *)&tempBuffer[0];

				ioData->mNumberBuffers = channels;
				for(size_t i = 0; i < channels; ++i) {
					ioData->mBuffers[i].mData = &eqBuffer[1024 * i];
					ioData->mBuffers[i].mDataByteSize = 1024 * sizeof(float);
					ioData->mBuffers[i].mNumberChannels = 1;
				}

				status = AudioUnitRender(_eq, NULL, &timeStamp, 0, samplesRendered, ioData);

				if(status != noErr) {
					return 0;
				}

				timeStamp.mSampleTime += ((double)samplesRendered) / streamFormat.mSampleRate;

				for(int i = 0; i < channels; ++i) {
					cblas_scopy(samplesRendered, &eqBuffer[1024 * i], 1, &inputBuffer[i], channels);
				}
			}
		}

		CMBlockBufferRef blockBuffer = nil;
		size_t dataByteSize = samplesRendered * sizeof(float) * streamFormat.mChannelsPerFrame;

		status = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, nil, dataByteSize, kCFAllocatorDefault, nil, 0, dataByteSize, kCMBlockBufferAssureMemoryNowFlag, &blockBuffer);

		if(status != noErr || !blockBuffer) {
			return 0;
		}

		status = CMBlockBufferReplaceDataBytes(&inputBuffer[0], blockBuffer, 0, dataByteSize);

		if(status != noErr) {
			return 0;
		}

		status = CMBlockBufferAppendBufferReference(blockListBuffer, blockBuffer, 0, CMBlockBufferGetDataLength(blockBuffer), 0);

		if(status != noErr) {
			return 0;
		}
	}

	*blockBufferOut = blockListBuffer;

	return samplesRendered;
}

- (BOOL)setup {
	if(audioRenderer || renderSynchronizer)
		[self stop];

	@synchronized(self) {
		stopInvoked = NO;

		running = NO;
		stopping = NO;
		stopped = NO;
		paused = NO;
		outputDeviceID = -1;
		restarted = NO;

		downmixerForVis = nil;

		AudioComponentDescription desc;
		NSError *err;

		desc.componentType = kAudioUnitType_Output;
		desc.componentSubType = kAudioUnitSubType_HALOutput;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;

		audioRenderer = [[AVSampleBufferAudioRenderer alloc] init];
		renderSynchronizer = [[AVSampleBufferRenderSynchronizer alloc] init];

		if(audioRenderer == nil || renderSynchronizer == nil)
			return NO;

		if(@available(macOS 12.0, *)) {
			[audioRenderer setAllowedAudioSpatializationFormats:AVAudioSpatializationFormatMonoStereoAndMultichannel];
		}

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

		AudioComponent comp = NULL;

		desc.componentType = kAudioUnitType_Effect;
		desc.componentSubType = kAudioUnitSubType_GraphicEQ;

		comp = AudioComponentFindNext(comp, &desc);
		if(!comp)
			return NO;

		OSStatus _err = AudioComponentInstanceNew(comp, &_eq);
		if(err)
			return NO;

		UInt32 value;
		UInt32 size = sizeof(value);

		value = CHUNK_SIZE;
		AudioUnitSetProperty(_eq, kAudioUnitProperty_MaximumFramesPerSlice,
		                     kAudioUnitScope_Global, 0, &value, size);

		value = 127;
		AudioUnitSetProperty(_eq, kAudioUnitProperty_RenderQuality,
		                     kAudioUnitScope_Global, 0, &value, size);

		AURenderCallbackStruct callbackStruct;
		callbackStruct.inputProcRefCon = (__bridge void *)self;
		callbackStruct.inputProc = eqRenderCallback;
		AudioUnitSetProperty(_eq, kAudioUnitProperty_SetRenderCallback,
		                     kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct));

		AudioUnitReset(_eq, kAudioUnitScope_Input, 0);
		AudioUnitReset(_eq, kAudioUnitScope_Output, 0);

		AudioUnitReset(_eq, kAudioUnitScope_Global, 0);

		_err = AudioUnitInitialize(_eq);
		if(_err)
			return NO;

		eqInitialized = YES;

		[self setEqualizerEnabled:[[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue]];

		[outputController beginEqualizer:_eq];

		visController = [VisualizationController sharedController];

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:kOutputAVFoundationContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.GraphicEQenable" options:0 context:kOutputAVFoundationContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.eqPreamp" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kOutputAVFoundationContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.dontRemix" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kOutputAVFoundationContext];
		observersapplied = YES;

		[renderSynchronizer addRenderer:audioRenderer];

		currentPts = kCMTimeZero;
		lastPts = kCMTimeZero;
		outputPts = kCMTimeZero;
		trackPts = kCMTimeZero;
		lastCheckpointPts = kCMTimeZero;
		bzero(&timeStamp, sizeof(timeStamp));
		timeStamp.mFlags = kAudioTimeStampSampleTimeValid;

		[self synchronizerBlock];

		[audioRenderer setVolume:volume];

		return (err == nil);
	}
}

- (void)synchronizerBlock {
	__block NSLock *lock = [[NSLock alloc] init];
	currentPtsLock = lock;
	CMTime interval = CMTimeMakeWithSeconds(1.0 / 60.0, 1000000000);
	CMTime *lastPts = &self->lastPts;
	CMTime *outputPts = &self->outputPts;
	OutputNode *outputController = self->outputController;
	double *latencySecondsOut = &self->secondsLatency;
	VisualizationController *visController = self->visController;
	currentPtsObserver = [renderSynchronizer addPeriodicTimeObserverForInterval:interval
	                                                                      queue:NULL
	                                                                 usingBlock:^(CMTime time) {
		                                                                 [lock lock];
		                                                                 self->currentPts = time;
		                                                                 [lock unlock];
		                                                                 CMTime timeToAdd = CMTimeSubtract(time, *lastPts);
		                                                                 self->lastPts = time;
		                                                                 double timeAdded = CMTimeGetSeconds(timeToAdd);
		                                                                 if(timeAdded > 0) {
			                                                                 [outputController incrementAmountPlayed:timeAdded];
		                                                                 }

		                                                                 CMTime latencyTime = CMTimeSubtract(*outputPts, time);
		                                                                 double latencySeconds = CMTimeGetSeconds(latencyTime);
		                                                                 if(latencySeconds < 0)
			                                                                 latencySeconds = 0;
		                                                                 *latencySecondsOut = latencySeconds;
		                                                                 [visController postLatency:latencySeconds];
	                                                                 }];
}

- (void)removeSynchronizerBlock {
	[renderSynchronizer removeTimeObserver:currentPtsObserver];
}

- (void)setVolume:(double)v {
	volume = v * 0.01f;
	if(audioRenderer) {
		[audioRenderer setVolume:volume];
	}
}

- (void)setEqualizerEnabled:(BOOL)enabled {
	if(enabled && !eqEnabled) {
		if(_eq) {
			AudioUnitReset(_eq, kAudioUnitScope_Input, 0);
			AudioUnitReset(_eq, kAudioUnitScope_Output, 0);
			AudioUnitReset(_eq, kAudioUnitScope_Global, 0);
		}
	}

	eqEnabled = enabled;
}

- (void)start {
	[self threadEntry:nil];
}

- (void)stop {
	@synchronized(self) {
		if(stopInvoked) return;
		stopInvoked = YES;
		if(observersapplied) {
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputDevice" context:kOutputAVFoundationContext];
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.GraphicEQenable" context:kOutputAVFoundationContext];
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.eqPreamp" context:kOutputAVFoundationContext];
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.dontRemix" context:kOutputAVFoundationContext];
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
		if(renderSynchronizer || audioRenderer) {
			if(renderSynchronizer) {
				[self removeSynchronizerBlock];
			}
			if(audioRenderer) {
				[audioRenderer stopRequestingMediaData];
				[audioRenderer flush];
			}
			renderSynchronizer = nil;
			audioRenderer = nil;
		}
		if(running) {
			while(!stopped) {
				stopping = YES;
				usleep(5000);
			}
		}
		if(_eq) {
			[outputController endEqualizer:_eq];
			if(eqInitialized) {
				AudioUnitUninitialize(_eq);
				eqInitialized = NO;
			}
			AudioComponentInstanceDispose(_eq);
			_eq = NULL;
		}
		if(downmixerForVis) {
			downmixerForVis = nil;
		}
#ifdef OUTPUT_LOG
		if(_logFile) {
			fclose(_logFile);
			_logFile = NULL;
		}
#endif
		outputController = nil;
		visController = nil;
	}
}

- (void)dealloc {
	[self stop];
}

- (void)pause {
	paused = YES;
	if(started)
		[renderSynchronizer setRate:0];
}

- (void)resume {
	[renderSynchronizer setRate:1.0 time:currentPts];
	paused = NO;
	started = YES;
}

- (void)sustainHDCD {
	@synchronized(self) {
		secondsHdcdSustained = 10.0;
	}
}

@end
