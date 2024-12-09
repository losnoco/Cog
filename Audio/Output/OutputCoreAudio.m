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

#import <CoreMotion/CoreMotion.h>

#import "rsstate.h"

#import "FSurroundFilter.h"

#import <rubberband/rubberband-c.h>

#define OCTAVES 5

extern void scale_by_volume(float *buffer, size_t count, float volume);

static NSString *CogPlaybackDidBeginNotificiation = @"CogPlaybackDidBeginNotificiation";

#define tts ((RubberBandState)ts)

simd_float4x4 convertMatrix(CMRotationMatrix r) {
	simd_float4x4 matrix = {
		simd_make_float4(r.m33, -r.m31, r.m32, 0.0f),
		simd_make_float4(r.m13, -r.m11, r.m12, 0.0f),
		simd_make_float4(r.m23, -r.m21, r.m22, 0.0f),
		simd_make_float4(0.0f, 0.0f, 0.0f, 1.0f)
	};
	return matrix;
}

NSLock *motionManagerLock = nil;
API_AVAILABLE(macos(14.0)) CMHeadphoneMotionManager *motionManager = nil;
OutputCoreAudio *registeredMotionListener = nil;

@implementation OutputCoreAudio
+ (void)initialize {
	motionManagerLock = [[NSLock alloc] init];

	if(@available(macOS 14, *)) {
		CMAuthorizationStatus status = [CMHeadphoneMotionManager authorizationStatus];
		if(status == CMAuthorizationStatusDenied) {
			ALog(@"Headphone motion not authorized");
			return;
		} else if(status == CMAuthorizationStatusAuthorized) {
			ALog(@"Headphone motion authorized");
		} else if(status == CMAuthorizationStatusRestricted) {
			ALog(@"Headphone motion restricted");
		} else if(status == CMAuthorizationStatusNotDetermined) {
			ALog(@"Headphone motion status not determined; will prompt for access");
		}

		motionManager = [[CMHeadphoneMotionManager alloc] init];
	}
}

void registerMotionListener(OutputCoreAudio *listener) {
	if(@available(macOS 14, *)) {
		[motionManagerLock lock];
		if([motionManager isDeviceMotionActive]) {
			[motionManager stopDeviceMotionUpdates];
		}
		if([motionManager isDeviceMotionAvailable]) {
			registeredMotionListener = listener;
			[motionManager startDeviceMotionUpdatesToQueue:[NSOperationQueue mainQueue] withHandler:^(CMDeviceMotion * _Nullable motion, NSError * _Nullable error) {
				if(motion) {
					[motionManagerLock lock];
					[registeredMotionListener reportMotion:convertMatrix(motion.attitude.rotationMatrix)];
					[motionManagerLock unlock];
				}
			}];
		}
		[motionManagerLock unlock];
	}
}

void unregisterMotionListener(void) {
	if(@available(macOS 14, *)) {
		[motionManagerLock lock];
		if([motionManager isDeviceMotionActive]) {
			[motionManager stopDeviceMotionUpdates];
		}
		registeredMotionListener = nil;
		[motionManagerLock unlock];
	}
}

static void *kOutputCoreAudioContext = &kOutputCoreAudioContext;

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
		memset((uint8_t *)ioData->mBuffers[i].mData + offset * sizeof(float), 0, count * sizeof(float));
		ioData->mBuffers[i].mNumberChannels = 1;
	}
}

static OSStatus eqRenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
	if(inNumberFrames > 4096 || !inRefCon) {
		clearBuffers(ioData, inNumberFrames, 0);
		return 0;
	}

	OutputCoreAudio *_self = (__bridge OutputCoreAudio *)inRefCon;

	fillBuffers(ioData, _self->samplePtr, inNumberFrames, 0);

	return 0;
}

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

		UInt32 srcChannels = format.mChannelsPerFrame;
		uint32_t dmConfig = config;
		uint32_t dmChannels = srcChannels;
		AudioStreamBasicDescription dmFormat;
		dmFormat = format;
		[outputLock lock];
		if(fsurround) {
			dmChannels = [fsurround channelCount];
			dmConfig = [fsurround channelConfig];
		}
		if(hrtf) {
			dmChannels = 2;
			dmConfig = AudioChannelFrontLeft | AudioChannelFrontRight;
		}
		[outputLock unlock];
		if(dmChannels != srcChannels) {
			dmFormat.mChannelsPerFrame = dmChannels;
			dmFormat.mBytesPerFrame = ((dmFormat.mBitsPerChannel + 7) / 8) * dmChannels;
			dmFormat.mBytesPerPacket = dmFormat.mBytesPerFrame * dmFormat.mFramesPerPacket;
		}
		UInt32 dstChannels = deviceFormat.mChannelsPerFrame;
		if(dmChannels != dstChannels) {
			format.mChannelsPerFrame = dstChannels;
			format.mBytesPerFrame = ((format.mBitsPerChannel + 7) / 8) * dstChannels;
			format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
			downmixer = [[DownmixProcessor alloc] initWithInputFormat:dmFormat inputConfig:dmConfig andOutputFormat:format outputConfig:deviceChannelConfig];
			format = origFormat;
		} else {
			downmixer = nil;
		}
		if(!streamFormatStarted || config != realStreamChannelConfig || memcmp(&realStreamFormat, &format, sizeof(format)) != 0) {
			realStreamFormat = format;
			realStreamChannelConfig = config;
			streamFormatStarted = YES;
			streamFormatChanged = YES;

			visFormat = format;
			visFormat.mChannelsPerFrame = 1;
			visFormat.mBytesPerFrame = visFormat.mChannelsPerFrame * (visFormat.mBitsPerChannel / 8);
			visFormat.mBytesPerPacket = visFormat.mBytesPerFrame * visFormat.mFramesPerPacket;

			downmixerForVis = [[DownmixProcessor alloc] initWithInputFormat:origFormat inputConfig:origConfig andOutputFormat:visFormat outputConfig:AudioConfigMono];
		}
	}

	if(streamFormatChanged) {
		return 0;
	}

	AudioChunk *chunk;
	
	if(!chunkRemain) {
		chunk = [outputController readChunk:amountToRead];
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
		[downmixerForVis process:outputPtr
					  frameCount:frameCount
						  output:&visAudio[0]];

		[visController postSampleRate:44100.0];

		[outputLock lock];
		if(fabs(realStreamFormat.mSampleRate - 44100.0) > 1e-5) {
			if(fabs(realStreamFormat.mSampleRate - lastVisRate) > 1e-5) {
				if(rsvis) {
					for(;;) {
						if(stopping) {
							break;
						}
						int samplesFlushed;
						samplesFlushed = (int)rsstate_flush(rsvis, &visTemp[0], 8192);
						if(samplesFlushed > 1) {
							[visController postVisPCM:visTemp amount:samplesFlushed];
							visPushed += (double)samplesFlushed / 44100.0;
						} else {
							break;
						}
					}
					rsstate_delete(rsvis);
					rsvis = NULL;
				}
				lastVisRate = realStreamFormat.mSampleRate;
				rsvis = rsstate_new(1, lastVisRate, 44100.0);
			}
			if(rsvis) {
				int samplesProcessed;
				size_t totalDone = 0;
				size_t inDone = 0;
				size_t visFrameCount = frameCount;
				do {
					if(stopping) {
						break;
					}
					int visTodo = (int)MIN(visFrameCount, visResamplerRemain + visFrameCount - 8192);
					if(visTodo) {
						cblas_scopy(visTodo, &visAudio[0], 1, &visResamplerInput[visResamplerRemain], 1);
					}
					visTodo += visResamplerRemain;
					visResamplerRemain = 0;
					samplesProcessed = (int)rsstate_resample(rsvis, &visResamplerInput[0], visTodo, &inDone, &visTemp[0], 8192);
					visResamplerRemain = (int)(visTodo - inDone);
					if(visResamplerRemain && inDone) {
						memmove(&visResamplerInput[0], &visResamplerInput[inDone], visResamplerRemain * sizeof(float));
					}
					if(samplesProcessed) {
						[visController postVisPCM:&visTemp[0] amount:samplesProcessed];
						visPushed += (double)samplesProcessed / 44100.0;
					}
					totalDone += inDone;
					visFrameCount -= inDone;
				} while(samplesProcessed && visFrameCount);
			}
		} else if(rsvis) {
			for(;;) {
				if(stopping) {
					break;
				}
				int samplesFlushed;
				samplesFlushed = (int)rsstate_flush(rsvis, &visTemp[0], 8192);
				if(samplesFlushed > 1) {
					[visController postVisPCM:visTemp amount:samplesFlushed];
					visPushed += (double)samplesFlushed / 44100.0;
				} else {
					break;
				}
			}
			rsstate_delete(rsvis);
			rsvis = NULL;
			[visController postVisPCM:&visAudio[0] amount:frameCount];
			visPushed += (double)frameCount / 44100.0;
		} else if(!stopping) {
			[visController postVisPCM:&visAudio[0] amount:frameCount];
			visPushed += (double)frameCount / 44100.0;
		}
		[outputLock unlock];

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

	if(eqEnabled) {
		volumeScale *= eqPreamp;
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

		pitch = 1.0; tempo = 1.0;
		lastPitch = 1.0; lastTempo = 1.0;

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
	} else if([keyPath isEqualToString:@"values.GraphicEQenable"]) {
		BOOL enabled = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue];

		[self setEqualizerEnabled:enabled];
	} else if([keyPath isEqualToString:@"values.eqPreamp"]) {
		float preamp = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] floatForKey:@"eqPreamp"];
		eqPreamp = pow(10.0, preamp / 20.0);
	} else if([keyPath isEqualToString:@"values.enableHrtf"]) {
		enableHrtf = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"enableHrtf"];
		if(streamFormatStarted)
			resetStreamFormat = YES;
	} else if([keyPath isEqualToString:@"values.enableFSurround"]) {
		enableFSurround = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"enableFSurround"];
		if(streamFormatStarted)
			resetStreamFormat = YES;
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
	if(stopping || ([outputController endOfStream] == YES && [self signalEndOfStream:secondsLatency])) {
		stopping = YES;
		return YES;
	}
	return NO;
}

- (void)threadEntry:(id)arg {
	running = YES;
	started = NO;
	shouldPlayOutBuffer = NO;
	secondsLatency = 1.0;

	while(!stopping) {
		@autoreleasepool {
			if(outputdevicechanged) {
				[self updateDeviceFormat];
				outputdevicechanged = NO;
			}

			if([outputController shouldReset]) {
				[outputController setShouldReset:NO];
				[outputLock lock];
				secondsLatency = 0.0;
				visPushed = 0.0;
				started = NO;
				restarted = NO;
				if(rsvis) {
					rsstate_delete(rsvis);
					rsvis = NULL;
				}
				lastClippedSampleRate = 0.0;
				lastVisRate = 0.0;
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

		// Force format for HRTF
		deviceFormat.mSampleRate = 96000.0;

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

		visFormat = deviceFormat;
		visFormat.mChannelsPerFrame = 1;
		visFormat.mBytesPerFrame = visFormat.mChannelsPerFrame * (visFormat.mBitsPerChannel / 8);
		visFormat.mBytesPerPacket = visFormat.mBytesPerFrame * visFormat.mFramesPerPacket;

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

		AudioStreamBasicDescription asbd = deviceFormat;

		asbd.mFormatFlags &= ~kAudioFormatFlagIsPacked;

		AudioUnitSetProperty(_eq, kAudioUnitProperty_StreamFormat,
							 kAudioUnitScope_Input, 0, &asbd, sizeof(asbd));

		AudioUnitSetProperty(_eq, kAudioUnitProperty_StreamFormat,
							 kAudioUnitScope_Output, 0, &asbd, sizeof(asbd));
		AudioUnitReset(_eq, kAudioUnitScope_Input, 0);
		AudioUnitReset(_eq, kAudioUnitScope_Output, 0);

		AudioUnitReset(_eq, kAudioUnitScope_Global, 0);

		eqEnabled = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue];
	}

	return YES;
}

- (void)updateStreamFormat {
	/* Set the channel layout for the audio queue */
	resetStreamFormat = NO;

	uint32_t channels = realStreamFormat.mChannelsPerFrame;
	uint32_t channelConfig = realStreamChannelConfig;

	if(enableFSurround && channels == 2 && channelConfig == AudioConfigStereo) {
		[outputLock lock];
		fsurround = [[FSurroundFilter alloc] initWithSampleRate:realStreamFormat.mSampleRate];
		[outputLock unlock];
		channels = [fsurround channelCount];
		channelConfig = [fsurround channelConfig];
		FSurroundDelayRemoved = NO;
	} else {
		[outputLock lock];
		fsurround = nil;
		[outputLock unlock];
	}

	if(enableHrtf) {
		NSURL *presetUrl = [[NSBundle mainBundle] URLForResource:@"SADIE_D02-96000" withExtension:@"mhr"];

		rotationMatrixUpdated = NO;

		simd_float4x4 matrix;
		if(!referenceMatrixSet) {
			matrix = matrix_identity_float4x4;
			self->referenceMatrix = matrix;
			registerMotionListener(self);
		} else {
			matrix = simd_mul(rotationMatrix, referenceMatrix);
		}

		[outputLock lock];
		hrtf = [[HeadphoneFilter alloc] initWithImpulseFile:presetUrl forSampleRate:realStreamFormat.mSampleRate withInputChannels:channels withConfig:channelConfig withMatrix:matrix];
		[outputLock unlock];

		channels = 2;
		channelConfig = AudioChannelSideLeft | AudioChannelSideRight;
	} else {
		unregisterMotionListener();
		referenceMatrixSet = NO;

		[outputLock lock];
		hrtf = nil;
		[outputLock unlock];
	}

	if(ts) {
		rubberband_delete(tts);
		ts = NULL;
	}

	RubberBandOptions options = RubberBandOptionProcessRealTime;
	ts = rubberband_new(realStreamFormat.mSampleRate, realStreamFormat.mChannelsPerFrame, options, 1.0 / tempo, pitch);

	blockSize = 1024;
	toDrop = rubberband_get_start_delay(tts);
	samplesBuffered = 0;
	rubberband_set_max_process_size(tts, (int)blockSize);

	for(size_t i = 0; i < 32; ++i) {
		rsPtrs[i] = &rsInBuffer[1024 * i];
	}

	size_t toPad = rubberband_get_preferred_start_pad(tts);
	if(toPad > 0) {
		for(size_t i = 0; i < realStreamFormat.mChannelsPerFrame; ++i) {
			memset(rsPtrs[i], 0, 1024 * sizeof(float));
		}
		while(toPad > 0) {
			size_t p = toPad;
			if(p > blockSize) p = blockSize;
			rubberband_process(tts, (const float * const *)rsPtrs, (int)p, false);
			toPad -= p;
		}
	}

	ssRenderedIn = 0.0;
	ssLastRenderedIn = 0.0;
	ssRenderedOut = 0.0;

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

	if(eqInitialized) {
		AudioUnitUninitialize(_eq);
		eqInitialized = NO;
	}

	AudioStreamBasicDescription asbd = streamFormat;

	// Of course, non-interleaved has only one sample per frame/packet, per buffer
	asbd.mFormatFlags |= kAudioFormatFlagIsNonInterleaved;
	asbd.mBytesPerFrame = sizeof(float);
	asbd.mBytesPerPacket = sizeof(float);
	asbd.mFramesPerPacket = 1;

	UInt32 maximumFrames = 4096;
	AudioUnitSetProperty(_eq, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maximumFrames, sizeof(maximumFrames));

	AudioUnitSetProperty(_eq, kAudioUnitProperty_StreamFormat,
						 kAudioUnitScope_Input, 0, &asbd, sizeof(asbd));

	AudioUnitSetProperty(_eq, kAudioUnitProperty_StreamFormat,
						 kAudioUnitScope_Output, 0, &asbd, sizeof(asbd));
	AudioUnitReset(_eq, kAudioUnitScope_Input, 0);
	AudioUnitReset(_eq, kAudioUnitScope_Output, 0);

	AudioUnitReset(_eq, kAudioUnitScope_Global, 0);

	if(AudioUnitInitialize(_eq) != noErr) {
		eqEnabled = NO;
		return;
	}
	eqInitialized = YES;

	eqEnabled = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"GraphicEQenable"] boolValue];
}

- (int)renderAndConvert {
	OSStatus status;
	int inputRendered = inputBufferLastTime;
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

	inputBufferLastTime = inputRendered;

	int samplesRendered = inputRendered;

	samplePtr = &inputBuffer[0];

	if(samplesRendered || fsurround) {
		{
			int simpleSpeedInput = samplesRendered;
			int simpleSpeedRendered = 0;
			int channels = realStreamFormat.mChannelsPerFrame;
			size_t max_block_len = blockSize;

			if (fabs(pitch - lastPitch) > 1e-5 ||
				fabs(tempo - lastTempo) > 1e-5) {
				lastPitch = pitch;
				lastTempo = tempo;
				rubberband_set_pitch_scale(tts, pitch);
				rubberband_set_time_ratio(tts, 1.0 / tempo);
			}

			const double inputRatio = 1.0 / realStreamFormat.mSampleRate;
			const double outputRatio = inputRatio * tempo;

			while (simpleSpeedInput > 0) {
				float *ibuf = samplePtr;
				size_t len = simpleSpeedInput;
				if(len > blockSize) len = blockSize;

				for(size_t i = 0; i < channels; ++i) {
					cblas_scopy((int)len, ibuf + i, channels, rsPtrs[i], 1);
				}

				rubberband_process(tts, (const float * const *)rsPtrs, (int)len, false);

				simpleSpeedInput -= len;
				ibuf += len * channels;
				ssRenderedIn += len * inputRatio;

				size_t samplesAvailable;
				while ((samplesAvailable = rubberband_available(tts)) > 0) {
					if(toDrop > 0) {
						size_t blockDrop = toDrop;
						if(blockDrop > blockSize) blockDrop = blockSize;
						rubberband_retrieve(tts, (float * const *)rsPtrs, (int)blockDrop);
						toDrop -= blockDrop;
						continue;
					}
					size_t maxAvailable = 65536 - samplesBuffered;
					if(samplesAvailable > maxAvailable) {
						samplesAvailable = maxAvailable;
						if(!samplesAvailable) {
							break;
						}
					}
					size_t samplesOut = samplesAvailable;
					if(samplesOut > blockSize) samplesOut = blockSize;
					rubberband_retrieve(tts, (float * const *)rsPtrs, (int)samplesOut);
					for(size_t i = 0; i < channels; ++i) {
						cblas_scopy((int)samplesOut, rsPtrs[i], 1, &rsOutBuffer[samplesBuffered * channels + i], channels);
					}
					samplesBuffered += samplesOut;
					ssRenderedOut += samplesOut * outputRatio;
					simpleSpeedRendered += samplesOut;
				}
				samplePtr = ibuf;
			}
			samplePtr = &rsOutBuffer[0];
			samplesRendered = simpleSpeedRendered;
			samplesBuffered = 0;
		}
		[outputLock lock];
		if(fsurround) {
			int countToProcess = samplesRendered;
			if(countToProcess < 4096) {
				bzero(samplePtr + countToProcess * 2, (4096 - countToProcess) * 2 * sizeof(float));
				countToProcess = 4096;
			}
			[fsurround process:samplePtr output:&fsurroundBuffer[0] count:countToProcess];
			samplePtr = &fsurroundBuffer[0];
			if(resetStreamFormat || samplesRendered < 4096) {
				bzero(&fsurroundBuffer[4096 * 6], 4096 * 2 * sizeof(float));
				[fsurround process:&fsurroundBuffer[4096 * 6] output:&fsurroundBuffer[4096 * 6] count:4096];
				samplesRendered += 2048;
			}
			if(!FSurroundDelayRemoved) {
				FSurroundDelayRemoved = YES;
				if(samplesRendered > 2048) {
					samplePtr += 2048 * 6;
					samplesRendered -= 2048;
				}
			}
		}
		[outputLock unlock];

		if(!samplesRendered) {
			return 0;
		}

		[outputLock lock];
		if(hrtf) {
			if(rotationMatrixUpdated) {
				rotationMatrixUpdated = NO;
				simd_float4x4 mirrorTransform = {
					simd_make_float4(-1.0, 0.0, 0.0, 0.0),
					simd_make_float4(0.0, 1.0, 0.0, 0.0),
					simd_make_float4(0.0, 0.0, 1.0, 0.0),
					simd_make_float4(0.0, 0.0, 0.0, 1.0)
				};

				simd_float4x4 matrix = simd_mul(mirrorTransform, rotationMatrix);
				matrix = simd_mul(matrix, referenceMatrix);

				[hrtf reloadWithMatrix:matrix];
			}
			[hrtf process:samplePtr sampleCount:samplesRendered toBuffer:&hrtfBuffer[0]];
			samplePtr = &hrtfBuffer[0];
		}
		[outputLock unlock];

		if(eqEnabled && eqInitialized) {
			const int channels = streamFormat.mChannelsPerFrame;
			if(channels > 0) {
				const size_t channelsminusone = channels - 1;
				uint8_t tempBuffer[sizeof(AudioBufferList) + sizeof(AudioBuffer) * channelsminusone];
				AudioBufferList *ioData = (AudioBufferList *)&tempBuffer[0];

				ioData->mNumberBuffers = channels;
				for(size_t i = 0; i < channels; ++i) {
					ioData->mBuffers[i].mData = &eqBuffer[4096 * i];
					ioData->mBuffers[i].mDataByteSize = samplesRendered * sizeof(float);
					ioData->mBuffers[i].mNumberChannels = 1;
				}

				status = AudioUnitRender(_eq, NULL, &timeStamp, 0, samplesRendered, ioData);

				if(status != noErr) {
					return 0;
				}

				timeStamp.mSampleTime += ((double)samplesRendered) / streamFormat.mSampleRate;

				for(int i = 0; i < channels; ++i) {
					cblas_scopy(samplesRendered, &eqBuffer[4096 * i], 1, samplePtr + i, channels);
				}
			}
		}
			
		if(downmixer) {
			[downmixer process:samplePtr frameCount:samplesRendered output:&downmixBuffer[0]];
			samplePtr = &downmixBuffer[0];
		}

#ifdef OUTPUT_LOG
		size_t dataByteSize = samplesRendered * sizeof(float) * deviceFormat.mChannelsPerFrame;

		fwrite(samplePtr, 1, dataByteSize, _logFile);
#endif
	}

	inputBufferLastTime = 0;

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
					if(_self->stopping)
						return 0;
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

		inputBufferLastTime = 0;

		running = NO;
		stopping = NO;
		stopped = NO;
		paused = NO;
		outputDeviceID = -1;
		restarted = NO;

		downmixer = nil;
		downmixerForVis = nil;

		lastClippedSampleRate = 0.0;

		rsvis = NULL;
		lastVisRate = 44100.0;
		
		inputRemain = 0;

		chunkRemain = nil;

		visResamplerRemain = 0;
		
		secondsLatency = 0;
		visPushed = 0;

		referenceMatrixSet = NO;
		rotationMatrix = matrix_identity_float4x4;

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
		
		[self audioOutputBlock];

		[_au allocateRenderResourcesAndReturnError:&err];

		[self updateDeviceFormat];

		visController = [VisualizationController sharedController];

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:kOutputCoreAudioContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.GraphicEQenable" options:0 context:kOutputCoreAudioContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.eqPreamp" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kOutputCoreAudioContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableHrtf" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kOutputCoreAudioContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableFSurround" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kOutputCoreAudioContext];
		observersapplied = YES;

		bzero(&timeStamp, sizeof(timeStamp));
		timeStamp.mFlags = kAudioTimeStampSampleTimeValid;

		return (err == nil);
	}
}

- (void)updateLatency:(double)secondsPlayed {
	if(secondsPlayed > 0) {
		double rendered = ssRenderedIn - ssLastRenderedIn;
		secondsPlayed = rendered;
		ssLastRenderedIn = ssRenderedIn;
		[outputController incrementAmountPlayed:rendered];
	}
	double simpleSpeedLatency = ssRenderedIn - ssRenderedOut;
	double visLatency = visPushed + simpleSpeedLatency;
	visPushed -= secondsPlayed;
	if(visLatency < secondsPlayed || visLatency > 30.0) {
		visLatency = secondsPlayed;
		visPushed = secondsPlayed;
	}
	secondsLatency = visLatency;
	[visController postLatency:visLatency];
}

- (void)setVolume:(double)v {
	volume = v * 0.01f;
}

- (void)setPitch:(double)p {
	pitch = p;
}

- (void)setTempo:(double)t {
	tempo = t;
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

- (double)latency {
	if(secondsLatency > 0) return secondsLatency;
	else return 0;
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
		if(hrtf) {
			unregisterMotionListener();
		}
		if(observersapplied) {
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputDevice" context:kOutputCoreAudioContext];
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.GraphicEQenable" context:kOutputCoreAudioContext];
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.eqPreamp" context:kOutputCoreAudioContext];
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.enableHrtf" context:kOutputCoreAudioContext];
			[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.enableFSurround" context:kOutputCoreAudioContext];
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
				double secondsLatency = self->secondsLatency >= 1e-5 ? self->secondsLatency : 0;
				int compareMax = (((1000000 / 5000) * secondsLatency) + (10000 / 5000)); // latency plus 10ms, divide by sleep intervals
				do {
					compareVal = self->secondsLatency >= 1e-5 ? self->secondsLatency : 0;
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
		if(visController) {
			[visController reset];
			visController = nil;
		}
		if(rsvis) {
			rsstate_delete(rsvis);
			rsvis = NULL;
		}
		if(ts) {
			rubberband_delete(tts);
			ts = NULL;
		}
		stopCompleted = YES;
	}
}

- (void)dealloc {
	[self stop];
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

- (void)reportMotion:(simd_float4x4)matrix {
	rotationMatrix = matrix;
	if(!referenceMatrixSet) {
		referenceMatrix = simd_inverse(matrix);
		referenceMatrixSet = YES;
	}
	rotationMatrixUpdated = YES;
}

@end
