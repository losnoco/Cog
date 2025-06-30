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

#ifdef OUTPUT_LOG
#import <NSFileHandle+CreateFile.h>
#endif

extern void scale_by_volume(float *buffer, size_t count, float volume);

static NSString *CogPlaybackDidBeginNotificiation = @"CogPlaybackDidBeginNotificiation";

static BOOL fadeAudio(const float *inSamples, float *outSamples, size_t channels, size_t count, float *fadeLevel, float fadeStep, float fadeTarget) {
	float _fadeLevel = *fadeLevel;
	BOOL towardZero = fadeStep < 0.0;
	BOOL stopping = NO;
	size_t maxCount = (size_t)floor(fabs(fadeTarget - _fadeLevel) / fabs(fadeStep));
	if(maxCount) {
		size_t countToDo = MIN(count, maxCount);
		for(size_t i = 0; i < channels; ++i) {
			_fadeLevel = *fadeLevel;
			vDSP_vrampmuladd(&inSamples[i], channels, &_fadeLevel, &fadeStep, &outSamples[i], channels, countToDo);
		}
	}
	if(maxCount <= count) {
		if(!towardZero && maxCount < count) {
			vDSP_vadd(&inSamples[maxCount * channels], 1, &outSamples[maxCount * channels], 1, &outSamples[maxCount * channels], 1, (count - maxCount) * channels);
		}
		stopping = YES;
	}
	*fadeLevel = _fadeLevel;
	return stopping;
}

@interface FadedBuffer : NSObject {
	float fadeLevel;
	float fadeStep;
	float fadeTarget;

	ChunkList *lastBuffer;
}

- (id)initWithBuffer:(ChunkList *)buffer fadeTarget:(float)fadeTarget sampleRate:(double)sampleRate;
- (BOOL)mix:(float *)outputBuffer sampleCount:(size_t)samples channelCount:(size_t)channels;

@end

@implementation FadedBuffer

- (id)initWithBuffer:(ChunkList *)buffer fadeTarget:(float)fadeTarget sampleRate:(double)sampleRate {
	self = [super init];
	if(self) {
		fadeLevel = 1.0;
		self->fadeTarget = fadeTarget;
		lastBuffer = buffer;
		const double maxFadeDurationMS = 1000.0 * [buffer listDuration];
		const double fadeDuration = MIN(125.0f, maxFadeDurationMS);
		fadeStep = ((fadeTarget - fadeLevel) / sampleRate) * (1000.0f / fadeDuration);
	}
	return self;
}

- (BOOL)mix:(float *)outputBuffer sampleCount:(size_t)samples channelCount:(size_t)channels {
	if(lastBuffer) {
		AudioChunk * chunk = [lastBuffer removeAndMergeSamples:samples callBlock:^BOOL{
			// Always interrupt if buffer runs empty, because it is not being refilled any more
			return true;
		}];
		if(chunk && [chunk frameCount]) {
			// Will always be input request size or less
			size_t samplesToMix = [chunk frameCount];
			NSData *sampleData = [chunk removeSamples:samplesToMix];
			return fadeAudio((const float *)[sampleData bytes], outputBuffer, channels, samplesToMix, &fadeLevel, fadeStep, fadeTarget);
		}
	}
	// No buffer or no chunk, stream ended
	return true;
}

@end

@implementation OutputCoreAudio {
	VisualizationController *visController;

	NSLock *fadedBuffersLock;
	NSMutableArray<FadedBuffer *> *fadedBuffers;
}

static void *kOutputCoreAudioContext = &kOutputCoreAudioContext;

- (AudioChunk *)renderInput:(int)amountToRead {
	if(stopping == YES || [outputController shouldContinue] == NO) {
		// Chain is dead, fill out the serial number pointer forever with silence
		stopping = YES;
		return [[AudioChunk alloc] init];
	}

	AudioStreamBasicDescription format;
	uint32_t config;
	if([outputController peekFormat:&format channelConfig:&config]) {
		if(!streamFormatStarted || config != realStreamChannelConfig || memcmp(&realStreamFormat, &format, sizeof(format)) != 0) {
			realStreamFormat = format;
			realStreamChannelConfig = config;
			streamFormatStarted = YES;
			streamFormatChanged = YES;
		}
	}

	if(streamFormatChanged) {
		return [[AudioChunk alloc] init];
	}

	return [outputController readChunk:amountToRead];
}

- (id)initWithController:(OutputNode *)c {
	self = [super init];
	if(self) {
		buffer = [[ChunkList alloc] initWithMaximumDuration:0.5];
		writeSemaphore = [[Semaphore alloc] init];
		readSemaphore = [[Semaphore alloc] init];

		outputController = c;
		volume = 1.0;
		outputDeviceID = -1;

		secondsHdcdSustained = 0;

		outputLock = [[NSLock alloc] init];

		fadedBuffersLock = [[NSLock alloc] init];
		fadedBuffers = [[NSMutableArray alloc] init];

#ifdef OUTPUT_LOG
		NSString *logName = [NSTemporaryDirectory() stringByAppendingPathComponent:@"CogAudioLog.raw"];
		_logFile = [NSFileHandle fileHandleForWritingAtPath:logName createFile:YES];
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

- (NSArray *)DSPs {
	if(DSPsLaunched) {
		return @[hrtfNode, downmixNode];
	} else {
		return @[];
	}
}

- (DSPDownmixNode *)downmix {
	return downmixNode;
}

- (void)launchDSPs {
	NSArray *DSPs = [self DSPs];

	for (Node *node in DSPs) {
		[node launchThread];
	}
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
	BOOL rendered = NO;

	while(!stopping) {
		@autoreleasepool {
			if(outputdevicechanged) {
				if([self updateDeviceFormat]) {
					outputdevicechanged = NO;
				} else {
					usleep(2000);
					continue;
				}
			}

			if([outputController shouldReset]) {
				[outputController setShouldReset:NO];
				[outputLock lock];
				started = NO;
				restarted = NO;
				[buffer reset];
				[self setShouldReset:YES];
				[outputLock unlock];
			}

			if(stopping)
				break;

			if(!cutOffInput && ![buffer isFull]) {
				[self renderAndConvert];
				rendered = YES;
			} else {
				rendered = NO;
			}

			if(!started && !paused) {
				// Prevent this call from hanging when used in this thread, when buffer may be empty
				// and waiting for this very thread to fill it
				resetting = YES;
				[self resume];
				resetting = NO;
			}

			if([outputController shouldContinue] == NO) {
				break;
			}
		}

		if(!rendered) {
			usleep(5000);
		}
	}

	stopped = YES;
	if(!stopInvoked) {
		[self doStop];
	}
}

- (OSStatus)setOutputDeviceByID:(int)deviceIDIn {
	OSStatus err;
	BOOL defaultDevice = NO;
	AudioObjectPropertyAddress theAddress = {
		.mSelector = kAudioHardwarePropertyDefaultOutputDevice,
		.mScope = kAudioObjectPropertyScopeGlobal,
		.mElement = kAudioObjectPropertyElementMaster
	};

	AudioDeviceID deviceID = (AudioDeviceID)deviceIDIn;

	if(deviceIDIn == -1) {
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
	}

	return noErr;
}

- (BOOL)setOutputDeviceWithDeviceDict:(NSDictionary *)deviceDict {
	NSNumber *deviceIDNum = deviceDict ? [deviceDict objectForKey:@"deviceID"] : @(-1);
	int outputDeviceID = deviceIDNum ? [deviceIDNum intValue] : -1;

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

	OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize);
	if(status != noErr) return;
	UInt32 nDevices = propsize / (UInt32)sizeof(AudioDeviceID);
	AudioDeviceID *devids = (AudioDeviceID *)malloc(propsize);
	status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, devids);
	if(status != noErr) return;

	theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
	AudioDeviceID systemDefault;
	propsize = sizeof(systemDefault);
	status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &propsize, &systemDefault);
	if(status != noErr) return;

	theAddress.mScope = kAudioDevicePropertyScopeOutput;

	for(UInt32 i = 0; i < nDevices; ++i) {
		UInt32 isAlive = 0;
		propsize = sizeof(isAlive);
		theAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;
		status = AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &isAlive);
		if(status != noErr) return;
		if(!isAlive) continue;

		CFStringRef name = NULL;
		propsize = sizeof(name);
		theAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
		status = AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, &name);
		if(status != noErr) return;

		propsize = 0;
		theAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
		status = AudioObjectGetPropertyDataSize(devids[i], &theAddress, 0, NULL, &propsize);
		if(status != noErr) {
			if(name) CFRelease(name);
			return;
		}

		if(propsize < sizeof(UInt32)) {
			if(name) CFRelease(name);
			continue;
		}

		AudioBufferList *bufferList = (AudioBufferList *)malloc(propsize);
		if(!bufferList) {
			if(name) CFRelease(name);
			return;
		}
		status = AudioObjectGetPropertyData(devids[i], &theAddress, 0, NULL, &propsize, bufferList);
		if(status != noErr) {
			if(name) CFRelease(name);
			return;
		}
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

		if(name) CFRelease(name);

		if(stop) {
			break;
		}
	}

	free(devids);
}

- (BOOL)updateDeviceFormat {
	AVAudioFormat *format = _au.outputBusses[0].format;
	if(!format) {
		return NO;
	}

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
		resetting = YES;
		[_au stopHardware];
		[_au.inputBusses[0] setFormat:renderFormat error:&err];
		if(err != nil)
			return NO;

		[outputController setFormat:&deviceFormat channelConfig:deviceChannelConfig];
		
		[outputLock lock];
		[buffer reset];
		[self setShouldReset:YES];
		[outputLock unlock];

		if(started) {
			[_au startHardwareAndReturnError:&err];
			if(err != nil)
				return NO;
		}

		resetting = NO;
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

- (void)renderAndConvert {
	if(resetStreamFormat) {
		[self updateStreamFormat];
		if([self processEndOfStream]) {
			return;
		}
	}

	AudioChunk *chunk = [self renderInput:512];
	size_t frameCount = 0;
	if(chunk && (frameCount = [chunk frameCount])) {
		[outputLock lock];
		[buffer addChunk:chunk];
		[outputLock unlock];
		[readSemaphore signal];
	}
	
	if(streamFormatChanged) {
		streamFormatChanged = NO;
		if(frameCount) {
			resetStreamFormat = YES;
		} else {
			[self updateStreamFormat];
		}
	}
	[self processEndOfStream];
}

- (void)audioOutputBlock {
	__block AudioStreamBasicDescription *format = &deviceFormat;
	__block void *refCon = (__bridge void *)self;
	__block NSLock *refLock = self->outputLock;
	__block NSLock *fadersLock = self->fadedBuffersLock;
	__block NSMutableArray *faders = self->fadedBuffers;

#ifdef OUTPUT_LOG
	__block NSFileHandle *logFile = _logFile;
#endif

	_au.outputProvider = ^AUAudioUnitStatus(AudioUnitRenderActionFlags *_Nonnull actionFlags, const AudioTimeStamp *_Nonnull timestamp, AUAudioFrameCount frameCount, NSInteger inputBusNumber, AudioBufferList *_Nonnull inputData) {
		if(!frameCount) return 0;

		const int channels = format->mChannelsPerFrame;
		if(!channels) return 0;

		if(!inputData->mNumberBuffers || !inputData->mBuffers[0].mData) return 0;

		OutputCoreAudio *_self = (__bridge OutputCoreAudio *)refCon;
		int renderedSamples = 0;

		inputData->mBuffers[0].mDataByteSize = frameCount * format->mBytesPerPacket;
		bzero(inputData->mBuffers[0].mData, inputData->mBuffers[0].mDataByteSize);
		inputData->mBuffers[0].mNumberChannels = channels;
		
		if(_self->resetting) {
			return 0;
		}
		
		float *outSamples = (float*)inputData->mBuffers[0].mData;

		@autoreleasepool {
			if(!_self->faded) {
				while(renderedSamples < frameCount) {
					[refLock lock];
					AudioChunk *chunk = nil;
					if(![_self->downmixNode.buffer isEmpty]) {
						chunk = [self->downmixNode.buffer removeSamples:frameCount - renderedSamples];
					}
					[refLock unlock];

					size_t _frameCount = 0;

					if(chunk && [chunk frameCount]) {
						_self->streamTimestamp = [chunk streamTimestamp];

						_frameCount = [chunk frameCount];
						NSData *sampleData = [chunk removeSamples:_frameCount];
						float *samplePtr = (float *)[sampleData bytes];
						size_t inputTodo = MIN(_frameCount, frameCount - renderedSamples);

						if(!_self->fading) {
							cblas_scopy((int)(inputTodo * channels), samplePtr, 1, outSamples + renderedSamples * channels, 1);
						} else {
							BOOL faded = fadeAudio(samplePtr, outSamples + renderedSamples * channels, channels, inputTodo, &_self->fadeLevel, _self->fadeStep, _self->fadeTarget);
							if(faded) {
								if(_self->fadeStep < 0.0) {
									_self->faded = YES;
								}
								_self->fading = NO;
								_self->fadeStep = 0.0f;
							}
						}

						renderedSamples += inputTodo;
					}

					if(_self->stopping || _self->resetting || _self->faded || !chunk || !_frameCount) {
						break;
					}
				}
			}

			double secondsRendered = (double)renderedSamples / format->mSampleRate;

			[fadersLock lock];
			for(size_t i = 0; i < [faders count];) {
				FadedBuffer *buffer = faders[i];
				BOOL stopping = [buffer mix:outSamples sampleCount:frameCount channelCount:channels];
				if(stopping) {
					[faders removeObjectAtIndex:i];
				} else {
					++i;
				}
			}
			[fadersLock unlock];

			scale_by_volume(outSamples, frameCount * channels, _self->volume);

			[_self updateLatency:secondsRendered];

#ifdef OUTPUT_LOG
			NSData *outData = [NSData dataWithBytes:outSamples length:frameCount * format->mBytesPerPacket];
			[logFile writeData:outData];
#endif
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

		cutOffInput = NO;
		fadeTarget = 1.0f;
		fadeLevel = 1.0f;
		fadeStep = 0.0f;
		fading = NO;
		faded = NO;

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

		if(![self updateDeviceFormat]) {
			return NO;
		}

		visController = [VisualizationController sharedController];

		hrtfNode = [[DSPHRTFNode alloc] initWithController:self previous:self latency:0.03];
		downmixNode = [[DSPDownmixNode alloc] initWithController:self previous:hrtfNode latency:0.03];

		[self setShouldContinue:YES];
		[self setEndOfStream:NO];

		DSPsLaunched = YES;
		[self launchDSPs];

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputDevice" options:0 context:kOutputCoreAudioContext];

		observersapplied = YES;
		
		return (err == nil);
	}
}

- (void)updateLatency:(double)secondsPlayed {
	double visLatency = [outputController getVisLatency];
	if(secondsPlayed > 0) {
		[outputController setAmountPlayed:streamTimestamp];
	}
	[visController postLatency:visLatency];
}

- (double)volume {
	return volume * 100.0f;
}

- (void)setVolume:(double)v {
	volume = v * 0.01f;
}

- (double)latency {
	return [buffer listDuration] + [[hrtfNode buffer] listDuration] + [[downmixNode buffer] listDuration];
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
				double compareVal = 0;
				double secondsLatency = [outputController getTotalLatency];
				int compareMax = (((1000000 / 5000) * secondsLatency) + (10000 / 5000)); // latency plus 10ms, divide by sleep intervals
				do {
					compareVal = [outputController getTotalLatency];
					usleep(5000);
				} while(!commandStop && compareVal > 0 && compareMax-- > 0);
			} else {
				[self fadeOut];
				[fadedBuffersLock lock];
				while([fadedBuffers count]) {
					[fadedBuffersLock unlock];
					usleep(10000);
					[fadedBuffersLock lock];
				}
				[fadedBuffersLock unlock];
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
		if(DSPsLaunched) {
			[self setShouldContinue:NO];
			[hrtfNode setShouldContinue:NO];
			[downmixNode setShouldContinue:NO];
			hrtfNode = nil;
			downmixNode = nil;
			DSPsLaunched = NO;
		}
#ifdef OUTPUT_LOG
		if(_logFile) {
			[_logFile closeFile];
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

// 125 milliseconds
- (void)fadeOut {
	fadeTarget = 0.0f;
	fadeStep = ((fadeTarget - fadeLevel) / deviceFormat.mSampleRate) * (1000.0f / 125.0f);
	fading = YES;
}

- (void)fadeOutBackground {
	cutOffInput = YES;
	[outputLock lock];
	[fadedBuffersLock lock];
	FadedBuffer *fbuffer = [[FadedBuffer alloc] initWithBuffer:buffer fadeTarget:0.0 sampleRate:deviceFormat.mSampleRate];
	buffer = [[ChunkList alloc] initWithMaximumDuration:0.5];
	[fadedBuffers addObject:fbuffer];
	[fadedBuffersLock unlock];
	[outputLock unlock];
	cutOffInput = NO;
}

- (void)fadeIn {
	fadeLevel = 0.0f;
	fadeTarget = 1.0f;
	fadeStep = ((fadeTarget - fadeLevel) / deviceFormat.mSampleRate) * (1000.0f / 125.0f);
	fading = YES;
	faded = NO;
}

@end
