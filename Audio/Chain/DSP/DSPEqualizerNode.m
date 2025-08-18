//
//  DSPEqualizerNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/11/25.
//

#import <Foundation/Foundation.h>

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import <Accelerate/Accelerate.h>

#import "DSPEqualizerNode.h"

#import "OutputNode.h"

#import "Logging.h"

#import "AudioPlayer.h"

extern void scale_by_volume(float *buffer, size_t count, float volume);

static void * kDSPEqualizerNodeContext = &kDSPEqualizerNodeContext;

@implementation DSPEqualizerNode {
	BOOL enableEqualizer;
	BOOL equalizerInitialized;

	double equalizerPreamp;

	__weak AudioPlayer *audioPlayer;

	AudioUnit _eq;

	AudioTimeStamp timeStamp;

	BOOL stopping, paused;
	NSRecursiveLock *mutex;

	BOOL observersapplied;

	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;

	uint32_t lastInputChannelConfig, inputChannelConfig;
	uint32_t outputChannelConfig;

	float inBuffer[4096 * 32];
	float eqBuffer[4096 * 32];
	float outBuffer[4096 * 32];
}

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

	DSPEqualizerNode *_self = (__bridge DSPEqualizerNode *)inRefCon;

	fillBuffers(ioData, _self->samplePtr, inNumberFrames, 0);

	return 0;
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableEqualizer = [defaults boolForKey:@"GraphicEQenable"];

		float preamp = [defaults floatForKey:@"eqPreamp"];
		equalizerPreamp = pow(10.0, preamp / 20.0);

		OutputNode *outputNode = c;
		audioPlayer = [outputNode controller];

		mutex = [[NSRecursiveLock alloc] init];

		[self addObservers];
	}
	return self;
}

- (void)dealloc {
	DLog(@"Equalizer dealloc");
	[self setShouldContinue:NO];
	[self cleanUp];
	[self removeObservers];
	[super cleanUp];
}

- (void)addObservers {
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.GraphicEQenable" options:0 context:kDSPEqualizerNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.eqPreamp" options:0 context:kDSPEqualizerNodeContext];
	
	observersapplied = YES;
}

- (void)removeObservers {
	if(observersapplied) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.GraphicEQenable" context:kDSPEqualizerNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.eqPreamp" context:kDSPEqualizerNodeContext];
		observersapplied = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kDSPEqualizerNodeContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.GraphicEQenable"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableEqualizer = [defaults boolForKey:@"GraphicEQenable"];
	} else if([keyPath isEqualToString:@"values.eqPreamp"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		float preamp = [defaults floatForKey:@"eqPreamp"];
		equalizerPreamp = pow(10.0, preamp / 20.0);
	}
}

- (AudioPlayer *)audioPlayer {
	return audioPlayer;
}

- (BOOL)fullInit {
	[mutex lock];
	if(enableEqualizer) {
		AudioComponentDescription desc;

		desc.componentType = kAudioUnitType_Effect;
		desc.componentSubType = kAudioUnitSubType_GraphicEQ;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;

		AudioComponent comp = NULL;

		comp = AudioComponentFindNext(comp, &desc);
		if(!comp) {
			[mutex unlock];
			return NO;
		}

		OSStatus status = AudioComponentInstanceNew(comp, &_eq);
		if(status != noErr) {
			[mutex unlock];
			return NO;
		}

		UInt32 value;
		UInt32 size = sizeof(value);

		value = 4096;
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

		AudioStreamBasicDescription asbd = inputFormat;

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

		status = AudioUnitInitialize(_eq);
		if(status != noErr) {
			[mutex unlock];
			return NO;
		}

		bzero(&timeStamp, sizeof(timeStamp));
		timeStamp.mFlags = kAudioTimeStampSampleTimeValid;

		equalizerInitialized = YES;

		[[self audioPlayer] beginEqualizer:_eq];
	}

	[mutex unlock];

	return YES;
}

- (void)fullShutdown {
	[mutex lock];
	if(_eq) {
		if(equalizerInitialized) {
			[[self audioPlayer] endEqualizer:_eq];
			AudioUnitUninitialize(_eq);
			equalizerInitialized = NO;
		}
		AudioComponentInstanceDispose(_eq);
		_eq = NULL;
	}
	[mutex unlock];
}

- (BOOL)setup {
	if(stopping)
		return NO;
	[self fullShutdown];
	return [self fullInit];
}

- (void)cleanUp {
	stopping = YES;
	[self fullShutdown];
}

- (void)resetBuffer {
	paused = YES;
	[mutex lock];
	[buffer reset];
	[self fullShutdown];
	paused = NO;
	[mutex unlock];
}

- (BOOL)paused {
	return paused;
}

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused || endOfStream) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk || ![chunk frameCount]) {
				if([previousNode endOfStream] == YES) {
					usleep(500);
					endOfStream = YES;
					continue;
				}
				if(paused) {
					continue;
				}
				usleep(500);
			} else {
				[self writeChunk:chunk];
				chunk = nil;
			}
			if(!enableEqualizer && equalizerInitialized) {
				[self fullShutdown];
			}
		}
	}
}

- (AudioChunk *)convert {
	if(stopping)
		return nil;

	[mutex lock];

	if(stopping || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) || [self shouldContinue] == NO) {
		[mutex unlock];
		return nil;
	}

	if(![self peekFormat:&inputFormat channelConfig:&inputChannelConfig]) {
		[mutex unlock];
		return nil;
	}

	if(!inputFormat.mSampleRate ||
	   !inputFormat.mBitsPerChannel ||
	   !inputFormat.mChannelsPerFrame ||
	   !inputFormat.mBytesPerFrame ||
	   !inputFormat.mFramesPerPacket ||
	   !inputFormat.mBytesPerPacket) {
		[mutex unlock];
		return nil;
	}

	if((enableEqualizer && !equalizerInitialized) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(enableEqualizer && ![self setup]) {
			[mutex unlock];
			return nil;
		}
	}

	if(!equalizerInitialized) {
		[mutex unlock];
		return [self readChunk:4096];
	}

	AudioChunk *chunk = [self readChunkAsFloat32:4096];
	if(!chunk || ![chunk frameCount]) {
		[mutex unlock];
		return nil;
	}

	double streamTimestamp = [chunk streamTimestamp];

	samplePtr = &inBuffer[0];
	size_t channels = inputFormat.mChannelsPerFrame;

	size_t frameCount = [chunk frameCount];
	NSData *sampleData = [chunk removeSamples:frameCount];

	cblas_scopy((int)(frameCount * channels), [sampleData bytes], 1, &inBuffer[0], 1);

	const size_t channelsminusone = channels - 1;
	uint8_t tempBuffer[sizeof(AudioBufferList) + sizeof(AudioBuffer) * channelsminusone];
	AudioBufferList *ioData = (AudioBufferList *)&tempBuffer[0];

	ioData->mNumberBuffers = (UInt32)channels;
	for(size_t i = 0; i < channels; ++i) {
		ioData->mBuffers[i].mData = &eqBuffer[4096 * i];
		ioData->mBuffers[i].mDataByteSize = (UInt32)(frameCount * sizeof(float));
		ioData->mBuffers[i].mNumberChannels = 1;
	}

	OSStatus status = AudioUnitRender(_eq, NULL, &timeStamp, 0, (UInt32)frameCount, ioData);

	if(status != noErr) {
		[mutex unlock];
		return nil;
	}

	timeStamp.mSampleTime += ((double)frameCount) / inputFormat.mSampleRate;

	for(int i = 0; i < channels; ++i) {
		cblas_scopy((int)frameCount, &eqBuffer[4096 * i], 1, &outBuffer[i], (int)channels);
	}

	AudioChunk *outputChunk = nil;
	if(frameCount) {
		scale_by_volume(&outBuffer[0], frameCount * channels, equalizerPreamp);

		outputChunk = [[AudioChunk alloc] init];
		[outputChunk setFormat:inputFormat];
		if(outputChannelConfig) {
			[outputChunk setChannelConfig:inputChannelConfig];
		}
		if([chunk isHDCD]) [outputChunk setHDCD];
		if(chunk.resetForward) outputChunk.resetForward = YES;
		[outputChunk setStreamTimestamp:streamTimestamp];
		[outputChunk setStreamTimeRatio:[chunk streamTimeRatio]];
		[outputChunk assignSamples:&outBuffer[0] frameCount:frameCount];
	}

	[mutex unlock];
	return outputChunk;
}

@end
