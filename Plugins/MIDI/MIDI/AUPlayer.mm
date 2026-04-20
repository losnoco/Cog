#import "AUPlayer.h"

#import <stdlib.h>

#import <Accelerate/Accelerate.h>

#define SF2PACK

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BLOCK_SIZE (512)

AUPlayer::AUPlayer()
: MIDIPlayer() {
	samplerUnit[0] = NULL;
	samplerUnit[1] = NULL;
	samplerUnit[2] = NULL;
	samplerUnit[3] = NULL;
	bufferList = NULL;
	audioBuffer = NULL;

	componentSubType = kAudioUnitSubType_DLSSynth;
	componentManufacturer = kAudioUnitManufacturer_Apple;
}

AUPlayer::~AUPlayer() {
	shutdown();
}

void AUPlayer::sendEventTime(uint32_t b, uint32_t time, unsigned port) {
	unsigned char event[3];
	event[0] = (unsigned char)b;
	event[1] = (unsigned char)(b >> 8);
	event[2] = (unsigned char)(b >> 16);
	if(port > 3) port = 0;
	if(samplerUnit[port])
		MusicDeviceMIDIEvent(samplerUnit[port], event[0], event[1], event[2], time);
}

void AUPlayer::sendSysexTime(const uint8_t *data, size_t size, unsigned port, uint32_t time) {
	if(port > 3) port = 0;
	if(samplerUnit[port])
		MusicDeviceSysEx(samplerUnit[port], data, (UInt32)size);
	if(port == 0) {
		if(samplerUnit[1])
			MusicDeviceSysEx(samplerUnit[1], data, (UInt32)size);
		if(samplerUnit[2])
			MusicDeviceSysEx(samplerUnit[2], data, (UInt32)size);
		if(samplerUnit[3])
			MusicDeviceSysEx(samplerUnit[3], data, (UInt32)size);
	}
}

void AUPlayer::dispatchMidi(const uint8_t *data, size_t length,
                            uint32_t sample_offset, unsigned port) {
	if(!length) return;
	uint8_t sb = data[0];

	if(sb == 0xF0) {
		sendSysexTime(data, length, port, sample_offset);
		return;
	}
	if(sb >= 0xF1) {
		/* System-common (e.g. tuning request) — route as sysex. */
		sendSysexTime(data, length, port, sample_offset);
		return;
	}

	uint32_t packed = sb;
	if(length >= 2) packed |= (uint32_t)data[1] << 8;
	if(length >= 3) packed |= (uint32_t)data[2] << 16;
	sendEventTime(packed, sample_offset, port);
}

void AUPlayer::renderChunk(float *out, uint32_t count) {
	const float *ptrL, *ptrR;
	bzero(out, count * sizeof(float) * 2);
	while(count) {
		UInt32 numberFrames = count > BLOCK_SIZE ? BLOCK_SIZE : (UInt32)count;

		for(unsigned long i = 0; i < 4; ++i) {
			if(!samplerUnit[i]) continue;

			AudioUnitRenderActionFlags ioActionFlags = 0;

			bufferList->mNumberBuffers = 2;
			for(unsigned long j = 0; j < 2; j++) {
				bufferList->mBuffers[j].mNumberChannels = 1;
				bufferList->mBuffers[j].mDataByteSize = (UInt32)(numberFrames * sizeof(float));
				bufferList->mBuffers[j].mData = audioBuffer + j * BLOCK_SIZE;
				bzero(bufferList->mBuffers[j].mData, numberFrames * sizeof(float));
			}

			AudioUnitRender(samplerUnit[i], &ioActionFlags, &mTimeStamp, 0, numberFrames, bufferList);

			ptrL = (const float *)bufferList->mBuffers[0].mData;
			ptrR = (const float *)bufferList->mBuffers[1].mData;
			size_t numBytesL = bufferList->mBuffers[0].mDataByteSize;
			size_t numBytesR = bufferList->mBuffers[1].mDataByteSize;
			size_t numBytes = MIN(numBytesL, numBytesR);
			size_t numFrames = numBytes / sizeof(float);
			numFrames = MIN(numFrames, numberFrames);
			vDSP_vadd(ptrL, 1, out, 2, out, 2, numFrames);
			vDSP_vadd(ptrR, 1, out + 1, 2, out + 1, 2, numFrames);
		}

		out += numberFrames * 2;
		count -= numberFrames;

		mTimeStamp.mSampleTime += (double)numberFrames;
	}
}

void AUPlayer::shutdown() {
	for(int i = 3; i >= 0; --i) {
		if(samplerUnit[i]) {
			AudioUnitUninitialize(samplerUnit[i]);
			AudioComponentInstanceDispose(samplerUnit[i]);
			samplerUnit[i] = NULL;
		}
	}
	if(audioBuffer) {
		free(audioBuffer);
		audioBuffer = NULL;
	}
	if(bufferList) {
		free(bufferList);
		bufferList = NULL;
	}
	initialized = false;
}

void AUPlayer::enumComponents(callback cbEnum) {
	AudioComponentDescription cd = { 0 };
	cd.componentType = kAudioUnitType_MusicDevice;

	AudioComponent comp = NULL;

	const char *bytes;
	char bytesBuffer[512];

	comp = AudioComponentFindNext(comp, &cd);

	while(comp != NULL) {
		CFStringRef cfName;
		AudioComponentCopyName(comp, &cfName);
		bytes = CFStringGetCStringPtr(cfName, kCFStringEncodingUTF8);
		if(!bytes) {
			CFStringGetCString(cfName, bytesBuffer, sizeof(bytesBuffer) - 1, kCFStringEncodingUTF8);
			bytes = bytesBuffer;
		}
		AudioComponentGetDescription(comp, &cd);
		cbEnum(cd.componentSubType, cd.componentManufacturer, bytes);
		CFRelease(cfName);
		comp = AudioComponentFindNext(comp, &cd);
	}
}

void AUPlayer::setComponent(OSType uSubType, OSType uManufacturer) {
	componentSubType = uSubType;
	componentManufacturer = uManufacturer;
	shutdown();
}

void AUPlayer::setSoundFont(const char *in) {
	const char *ext = strrchr(in, '.');
	if(ext && *ext && ((strncasecmp(ext + 1, "sf2", 3) == 0) || (strncasecmp(ext + 1, "dls", 3) == 0))) {
		sSoundFontName = in;
		shutdown();
	}
}

void AUPlayer::setPreset(NSDictionary *preset) {
	this->preset = preset;
}

static OSStatus renderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
	if(inNumberFrames && ioData) {
		for(int i = 0, j = ioData->mNumberBuffers; i < j; ++i) {
			int k = inNumberFrames * sizeof(float);
			if(k > ioData->mBuffers[i].mDataByteSize)
				k = ioData->mBuffers[i].mDataByteSize;
			bzero(ioData->mBuffers[i].mData, k);
		}
	}

	return noErr;
}

bool AUPlayer::startup() {
	if(initialized) return true;

	AudioComponentDescription cd = { 0 };
	cd.componentType = kAudioUnitType_MusicDevice;
	cd.componentSubType = componentSubType;
	cd.componentManufacturer = componentManufacturer;

	AudioComponent comp = NULL;

	comp = AudioComponentFindNext(comp, &cd);

	if(!comp)
		return false;

	OSStatus error;

	for(int i = 0; i < 4; i++) {
		if(!(port_mask & (1u << i))) continue;

		UInt32 value = 1;
		UInt32 size = sizeof(value);

		error = AudioComponentInstanceNew(comp, &samplerUnit[i]);

		if(error != noErr)
			return false;

		needsInput = NO;

		{
			AudioStreamBasicDescription stream = { 0 };
			stream.mSampleRate = dSampleRate;
			stream.mFormatID = kAudioFormatLinearPCM;
			stream.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
			stream.mBytesPerPacket = 4;
			stream.mFramesPerPacket = 1;
			stream.mBytesPerFrame = 4;
			stream.mChannelsPerFrame = 2;
			stream.mBitsPerChannel = 32;

			AUChannelInfo channelInfo = { 0 };
			Boolean *isWritable = 0;
			size = 0;
			error = AudioUnitGetPropertyInfo(samplerUnit[i], kAudioUnitProperty_SupportedNumChannels, kAudioUnitScope_Global, 0, &size, isWritable);
			if(error == noErr) {
				size = sizeof(channelInfo);
				error = AudioUnitGetProperty(samplerUnit[i], kAudioUnitProperty_SupportedNumChannels, kAudioUnitScope_Global, 0, &channelInfo, &size);
				if(error == noErr && channelInfo.inChannels == -1 || channelInfo.inChannels <= -2 || channelInfo.inChannels >= 2) {
					needsInput = YES;
				}
			} else {
				UInt32 channelCount = 0;
				size = sizeof(channelCount);
				error = AudioUnitGetProperty(samplerUnit[i], kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &channelCount, &size);
				if(error == noErr && channelCount >= 2) {
					needsInput = YES;
				}
			}

			if(needsInput) {
				AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_StreamFormat,
				                     kAudioUnitScope_Input, 0, &stream, sizeof(stream));
			}

			AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_StreamFormat,
			                     kAudioUnitScope_Output, 0, &stream, sizeof(stream));
		}

		value = BLOCK_SIZE;
		AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_MaximumFramesPerSlice,
		                     kAudioUnitScope_Global, 0, &value, size);

		value = 127;
		AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_RenderQuality,
		                     kAudioUnitScope_Global, 0, &value, size);

		if(needsInput) {
			AURenderCallbackStruct callbackStruct;
			callbackStruct.inputProc = renderCallback;
			callbackStruct.inputProcRefCon = 0;
			AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_SetRenderCallback,
			                     kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct));
		}

		if(needsInput) {
			AudioUnitReset(samplerUnit[i], kAudioUnitScope_Input, 0);
		}
		AudioUnitReset(samplerUnit[i], kAudioUnitScope_Output, 0);

		AudioUnitReset(samplerUnit[i], kAudioUnitScope_Global, 0);

		value = 1;
		AudioUnitSetProperty(samplerUnit[i], kMusicDeviceProperty_StreamFromDisk, kAudioUnitScope_Global, 0, &value, size);

		if(preset) {
			CFDictionaryRef cdict = (__bridge CFDictionaryRef)preset;
			AudioUnitSetProperty(samplerUnit[i], kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &cdict, sizeof(cdict));
		}

		error = AudioUnitInitialize(samplerUnit[i]);

		if(error != noErr)
			return false;
	}

	if(sSoundFontName.length()) {
		loadSoundFont(sSoundFontName.c_str());
	}

	bufferList = (AudioBufferList *)calloc(1, sizeof(AudioBufferList) + sizeof(AudioBuffer));
	if(!bufferList)
		return false;

	audioBuffer = (float *)malloc(BLOCK_SIZE * 2 * sizeof(float));
	if(!audioBuffer)
		return false;

	bufferList->mNumberBuffers = 2;

	memset(&mTimeStamp, 0, sizeof(mTimeStamp));
	mTimeStamp.mFlags = kAudioTimeStampSampleTimeValid;

	initialized = true;

	/* Warm up */
	float *temp = (float *)malloc(sizeof(float) * BLOCK_SIZE * 2);
	if(temp) {
		size_t count = (size_t)round(dSampleRate / BLOCK_SIZE) + 1;
		for(size_t i = 0; i < count; ++i) {
			renderChunk(temp, BLOCK_SIZE);
		}
		free(temp);
	}

	return true;
}

void AUPlayer::loadSoundFont(const char *name) {
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)name, strlen(name), false);

	if(url) {
		for(int i = 0; i < 4; i++)
			AudioUnitSetProperty(samplerUnit[i],
			                     kMusicDeviceProperty_SoundBankURL, kAudioUnitScope_Global,
			                     0,
			                     &url, sizeof(url));

		CFRelease(url);
	}
}
