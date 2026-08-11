#import "AUPlayer.h"

#import <stdlib.h>

#import <Accelerate/Accelerate.h>
#import <CoreMIDI/CoreMIDI.h>

#define SF2PACK

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BLOCK_SIZE (512)

/* How the port reaches the unit.
 *
 * There is no port-select message in a MIDI stream.  The port rides on each
 * message -- as the cable nibble of a USB-MIDI packet on hardware, as the
 * group nibble of a Universal MIDI Packet, and as `cable` in the Audio Unit
 * API -- and nothing latches it from one message to the next.  So a unit that
 * accepts several cables can serve several ports, and this opens one instance
 * per *group of cables* rather than one per port.
 *
 * Which call carries the cable is not a matter of taste:
 *
 *   MusicDeviceMIDIEvent          no cable argument at all.  This is what this
 *                                 file used to send everything with, and it is
 *                                 why it used to open four units.
 *   scheduleMIDIEventBlock        takes a cable, and delivers it verbatim,
 *                                 0-15, for channel voice and SysEx alike.
 *                                 Bridged to MusicDeviceMIDIEvent for a v2
 *                                 unit, so the cable goes nowhere there -- but
 *                                 a v2 unit reports one cable anyway, and one
 *                                 cable means one port per instance.
 *   scheduleMIDIEventListBlock    Apple's preferred call, and the one to use
 *                                 where it exists (macOS 12).  Takes a cable
 *                                 *and* Universal MIDI Packets whose group
 *                                 nibble is also a port, and the two do not
 *                                 translate into one another: a unit that has
 *                                 not adopted MIDIEventList is handed the
 *                                 block's cable and never sees the group,
 *                                 while a unit that has adopted it reads the
 *                                 group and ignores the cable.  Setting both
 *                                 to the port is what works in either case.
 *
 * The instance count follows `virtualMIDICableCount`, which is the only thing
 * that reports this and lives on the v3 side of the unit.  A unit that claims
 * one cable is given one port, exactly as before this file learned any of the
 * above; nothing regresses for Apple's DLS synth.
 */

AUPlayer::AUPlayer()
: MIDIPlayer() {
	instanceCount = 0;
	cablesPerInstance = 1;
	bufferList = NULL;
	audioBuffer = NULL;

	componentSubType = kAudioUnitSubType_DLSSynth;
	componentManufacturer = kAudioUnitManufacturer_Apple;
}

AUPlayer::~AUPlayer() {
	shutdown();
}

/* ── Sending ─────────────────────────────────────────────────────────────── */

void AUPlayer::sendToCable(Instance &instance, uint8_t cable, const uint8_t *data, size_t length,
                           uint32_t sample_offset) {
	if(!length) return;

	/* Sample-accurate placement within the block about to be rendered.  The
	 * old code could only do this for channel messages, because MusicDeviceSysEx
	 * has no offset argument; both blocks below take one. */
	if(@available(macOS 12.0, iOS 15.0, *)) {
		if(instance.scheduleEventList && length <= 3 && data[0] < 0xF0) {
			MIDIEventList list;
			MIDIEventPacket *packet = MIDIEventListInit(&list, kMIDIProtocol_1_0);
			uint32_t word = (UInt32)0x2 << 28 | (UInt32)(cable & 0x0F) << 24 |
			                (UInt32)data[0] << 16;
			if(length >= 2) word |= (UInt32)data[1] << 8;
			if(length >= 3) word |= (UInt32)data[2];
			packet = MIDIEventListAdd(&list, sizeof(list), packet, 0, 1, &word);
			/* Cable *and* group, because a unit reads one or the other. */
			instance.scheduleEventList(AUEventSampleTimeImmediate + sample_offset, cable, &list);
			return;
		}
	}

	/* Everything else, SysEx included: this block takes a whole message of any
	 * length and delivers the cable with it.
	 *
	 * SysEx does not go the MIDIEventList route above because a Universal MIDI
	 * Packet carries it six bytes at a time, in start/continue/end pieces that
	 * would have to be assembled here and taken apart again by anything that
	 * has not adopted the protocol.  The cost is that a unit which *has*
	 * adopted it reads the group rather than the cable, so its SysEx lands on
	 * port A -- worth fixing the day such a unit turns up, and not before. */
	if(instance.scheduleEvent) {
		instance.scheduleEvent(AUEventSampleTimeImmediate + sample_offset, cable,
		                       (NSInteger)length, data);
		return;
	}

	/* No block: a unit reachable only through the v2 API, which has nowhere to
	 * put a cable.  It reported one cable, so this instance is this port. */
	if(!instance.unit) return;
	if(data[0] >= 0xF0) {
		MusicDeviceSysEx(instance.unit, data, (UInt32)length);
	} else {
		MusicDeviceMIDIEvent(instance.unit, data[0], length >= 2 ? data[1] : 0,
		                     length >= 3 ? data[2] : 0, sample_offset);
	}
}

void AUPlayer::sendToPort(unsigned port, const uint8_t *data, size_t length,
                          uint32_t sample_offset) {
	if(!cablesPerInstance || !instanceCount) return;

	unsigned index = port / cablesPerInstance;
	uint8_t cable = (uint8_t)(port % cablesPerInstance);

	/* A port past what was opened -- the file addresses one the unit cannot
	 * back.  Fold onto an instance that exists rather than dropping it, which
	 * is what a module does with the cables it advertises and cannot honour. */
	if(index >= instanceCount) index = 0;

	sendToCable(instances[index], cable, data, length, sample_offset);
}

void AUPlayer::sendEventTime(uint32_t b, uint32_t time, unsigned port) {
	uint8_t event[3];
	event[0] = (uint8_t)b;
	event[1] = (uint8_t)(b >> 8);
	event[2] = (uint8_t)(b >> 16);

	/* Program change and channel pressure are two bytes; everything else in
	 * 0x80-0xEF is three.  This did not matter when every message went to
	 * MusicDeviceMIDIEvent, which takes the two data bytes as arguments and
	 * ignores the second where the status has no use for it.  The scheduling
	 * blocks are given a length instead, and a three-byte program change is
	 * not a message -- the byte after it would be read as the start of one. */
	uint8_t status = event[0] & 0xF0;
	size_t length = (status == 0xC0 || status == 0xD0) ? 2 : 3;

	if(port >= max_ports) port = 0;
	sendToPort(port, event, length, time);
}

void AUPlayer::sendSysexTime(const uint8_t *data, size_t size, unsigned port, uint32_t time) {
	if(port >= max_ports) port = 0;
	sendToPort(port, data, size, time);

	/* A reset arriving on port A is meant for the whole module, and a file that
	 * addresses four ports very often sends exactly one.  Repeating it on the
	 * others is what this did when each port was its own unit, and it has to
	 * keep happening now that they may share one. */
	if(port == 0) {
		unsigned ports = instanceCount * cablesPerInstance;
		for(unsigned p = 1; p < ports && p < max_ports; ++p)
			sendToPort(p, data, size, time);
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

/* ── Rendering ───────────────────────────────────────────────────────────── */

void AUPlayer::renderChunk(float *out, uint32_t count) {
	const float *ptrL, *ptrR;
	bzero(out, count * sizeof(float) * 2);
	while(count) {
		UInt32 numberFrames = count > BLOCK_SIZE ? BLOCK_SIZE : (UInt32)count;

		for(unsigned long i = 0; i < instanceCount; ++i) {
			if(!instances[i].unit) continue;

			AudioUnitRenderActionFlags ioActionFlags = 0;

			bufferList->mNumberBuffers = 2;
			for(unsigned long j = 0; j < 2; j++) {
				bufferList->mBuffers[j].mNumberChannels = 1;
				bufferList->mBuffers[j].mDataByteSize = (UInt32)(numberFrames * sizeof(float));
				bufferList->mBuffers[j].mData = audioBuffer + j * BLOCK_SIZE;
				bzero(bufferList->mBuffers[j].mData, numberFrames * sizeof(float));
			}

			AudioUnitRender(instances[i].unit, &ioActionFlags, &mTimeStamp, 0, numberFrames, bufferList);

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

/* ── Lifecycle ───────────────────────────────────────────────────────────── */

void AUPlayer::closeInstance(Instance &instance) {
	if(instance.unit) {
		AudioUnitUninitialize(instance.unit);
		instance.unit = NULL;
	}
	/* The AVAudioUnit owns the instance; dropping it disposes of the component.
	 * The blocks are released with it and must not outlive it. */
	instance.scheduleEvent = nil;
	instance.scheduleEventList = nil;
	instance.auUnit = nil;
	instance.node = nil;
	instance.needsInput = false;
}

void AUPlayer::shutdown() {
	for(int i = (int)max_ports - 1; i >= 0; --i)
		closeInstance(instances[i]);
	instanceCount = 0;
	cablesPerInstance = 1;

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

/* Creates one instance, and gets hold of both faces of it.
 *
 * Through AVAudioUnit rather than AudioComponentInstanceNew, because that call
 * cannot create an AUv3 at all: an app extension lives in another process and
 * there is nothing to return synchronously.  AVAudioUnit's asynchronous
 * instantiation can, works for v2 units just the same, and is the only way to
 * hold the v2 handle and the AUAudioUnit for one instance -- there is no
 * property that turns one into the other. */
bool AUPlayer::openInstance(const AudioComponentDescription &cd, Instance &into) {
	__block AVAudioUnit *created = nil;
	dispatch_semaphore_t ready = dispatch_semaphore_create(0);

	/* In process where the unit allows it.  Nothing here is realtime -- Cog
	 * decodes as fast as it can -- so a render block that has to cross to
	 * another process for every 512 frames is pure overhead.  A v2 unit is
	 * in-process regardless and the default suits it; only a v3 unit that
	 * advertises the flag may be asked. */
	AudioComponentInstantiationOptions options = 0;
	AudioComponentDescription found = cd;
	AudioComponent comp = AudioComponentFindNext(NULL, &found);
	if(comp) {
		UInt32 flags = 0;
		if(AudioComponentGetDescription(comp, &found) == noErr)
			flags = found.componentFlags;
		if(flags & kAudioComponentFlag_CanLoadInProcess)
			options = kAudioComponentInstantiation_LoadInProcess;
	}

	/* The completion is delivered on a private queue, not the main one, so
	 * waiting here cannot deadlock against our own caller. */
	[AVAudioUnit instantiateWithComponentDescription:cd
	                                         options:options
	                               completionHandler:^(AVAudioUnit *unit, NSError *error) {
		                               created = unit;
		                               dispatch_semaphore_signal(ready);
	                               }];

	/* Bounded, because a broken extension that never answers must not hang the
	 * decoder thread for the rest of the session. */
	if(dispatch_semaphore_wait(ready,
	                           dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC)) != 0)
		return false;

	if(!created || !created.audioUnit) return false;

	into.node = created;
	into.unit = created.audioUnit;
	into.auUnit = created.AUAudioUnit;

	/* Before the unit is initialized, which is when the API says to take them. */
	into.scheduleEvent = into.auUnit.scheduleMIDIEventBlock;
	if(@available(macOS 12.0, iOS 15.0, *))
		into.scheduleEventList = into.auUnit.scheduleMIDIEventListBlock;

	return true;
}

bool AUPlayer::configureInstance(Instance &instance) {
	AudioUnit unit = instance.unit;
	OSStatus error;
	UInt32 value;

	instance.needsInput = false;

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
		UInt32 size = 0;
		error = AudioUnitGetPropertyInfo(unit, kAudioUnitProperty_SupportedNumChannels, kAudioUnitScope_Global, 0, &size, NULL);
		if(error == noErr) {
			size = sizeof(channelInfo);
			error = AudioUnitGetProperty(unit, kAudioUnitProperty_SupportedNumChannels, kAudioUnitScope_Global, 0, &channelInfo, &size);
			/* Parenthesised: && binds tighter than ||, so without these the
			 * error check only guarded the first of the three cases. */
			if(error == noErr && (channelInfo.inChannels == -1 || channelInfo.inChannels <= -2 || channelInfo.inChannels >= 2)) {
				instance.needsInput = true;
			}
		} else {
			UInt32 channelCount = 0;
			size = sizeof(channelCount);
			error = AudioUnitGetProperty(unit, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &channelCount, &size);
			if(error == noErr && channelCount >= 2) {
				instance.needsInput = true;
			}
		}

		if(instance.needsInput) {
			AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
			                     kAudioUnitScope_Input, 0, &stream, sizeof(stream));
		}

		AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
		                     kAudioUnitScope_Output, 0, &stream, sizeof(stream));
	}

	/* sizeof(value), not whatever size the queries above happened to leave
	 * behind -- these are all UInt32 properties. */
	value = BLOCK_SIZE;
	AudioUnitSetProperty(unit, kAudioUnitProperty_MaximumFramesPerSlice,
	                     kAudioUnitScope_Global, 0, &value, sizeof(value));

	value = 127;
	AudioUnitSetProperty(unit, kAudioUnitProperty_RenderQuality,
	                     kAudioUnitScope_Global, 0, &value, sizeof(value));

	if(instance.needsInput) {
		AURenderCallbackStruct callbackStruct;
		callbackStruct.inputProc = renderCallback;
		callbackStruct.inputProcRefCon = 0;
		AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback,
		                     kAudioUnitScope_Input, 0, &callbackStruct, sizeof(callbackStruct));

		AudioUnitReset(unit, kAudioUnitScope_Input, 0);
	}
	AudioUnitReset(unit, kAudioUnitScope_Output, 0);

	AudioUnitReset(unit, kAudioUnitScope_Global, 0);

	value = 1;
	AudioUnitSetProperty(unit, kMusicDeviceProperty_StreamFromDisk, kAudioUnitScope_Global, 0, &value, sizeof(value));

	if(preset) {
		CFDictionaryRef cdict = (__bridge CFDictionaryRef)preset;
		AudioUnitSetProperty(unit, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &cdict, sizeof(cdict));
	}

	return AudioUnitInitialize(unit) == noErr;
}

bool AUPlayer::startup() {
	if(initialized) return true;

	AudioComponentDescription cd = { 0 };
	cd.componentType = kAudioUnitType_MusicDevice;
	cd.componentSubType = componentSubType;
	cd.componentManufacturer = componentManufacturer;

	if(!AudioComponentFindNext(NULL, &cd))
		return false;

	/* How many ports the file actually addresses, so that a file touching only
	 * port A opens one unit however many the unit could have served.
	 *
	 * The four low bits and no more: sixty-four channels is the ceiling either
	 * side of this, so a fifth port has nothing to play it.  Taking the highest
	 * bit rather than counting them costs nothing and does not assume the mask
	 * is contiguous, which in practice it always is. */
	unsigned portsNeeded = 1;
	for(unsigned p = 0; p < max_ports; ++p) {
		if(port_mask & (1u << p)) portsNeeded = p + 1;
	}

	/* One unit first, to ask it how many cables it takes.  Nothing else can
	 * answer that: virtualMIDICableCount lives only on the v3 side, and a unit
	 * has to exist before it can be asked. */
	if(!openInstance(cd, instances[0])) {
		shutdown();
		return false;
	}
	instanceCount = 1;

	NSInteger cables = instances[0].auUnit ? instances[0].auUnit.virtualMIDICableCount : 1;
	if(cables < 1) cables = 1;
	if(cables > (NSInteger)max_ports) cables = max_ports;

	/* A cable is only worth counting if there is a call that can carry one.
	 * Without a block we are back to MusicDeviceMIDIEvent, which has no cable
	 * argument, so the unit gets one port however many it claims. */
	if(!instances[0].scheduleEvent && !instances[0].scheduleEventList) cables = 1;

	cablesPerInstance = (unsigned)cables;

	unsigned wanted = (portsNeeded + cablesPerInstance - 1) / cablesPerInstance;
	if(wanted > max_ports) wanted = max_ports;

	for(unsigned i = 1; i < wanted; ++i) {
		if(!openInstance(cd, instances[i])) {
			shutdown();
			return false;
		}
		instanceCount = i + 1;
	}

	for(unsigned i = 0; i < instanceCount; ++i) {
		if(!configureInstance(instances[i])) {
			shutdown();
			return false;
		}
	}

	if(sSoundFontName.length()) {
		loadSoundFont(sSoundFontName.c_str());
	}

	bufferList = (AudioBufferList *)calloc(1, sizeof(AudioBufferList) + sizeof(AudioBuffer));
	if(!bufferList) {
		shutdown();
		return false;
	}

	audioBuffer = (float *)malloc(BLOCK_SIZE * 2 * sizeof(float));
	if(!audioBuffer) {
		shutdown();
		return false;
	}

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
		/* Only the units that exist.  This used to run to four regardless and
		 * hand AudioUnitSetProperty a null instance for the ones that did not. */
		for(unsigned i = 0; i < instanceCount; i++) {
			if(!instances[i].unit) continue;
			AudioUnitSetProperty(instances[i].unit,
			                     kMusicDeviceProperty_SoundBankURL, kAudioUnitScope_Global,
			                     0,
			                     &url, sizeof(url));
		}

		CFRelease(url);
	}
}
