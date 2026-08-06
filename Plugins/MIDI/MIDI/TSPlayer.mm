//
//  TSPlayer.mm
//  MIDI
//
//  Created by Christopher Snowhill on 8/2/26.
//

#import "TSPlayer.h"

#import <stdlib.h>

#import <Accelerate/Accelerate.h>

#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BLOCK_SIZE (320)

TSPlayer::TSPlayer()
: MIDIPlayer() {
	rom.reset();
	notes = NULL;
	tonegen = NULL;
	audioBuffer = NULL;
}

TSPlayer::~TSPlayer() {
	shutdown();
}

void TSPlayer::sendEventTime(uint32_t b, uint32_t time, unsigned port) {
	unsigned char event[3];
	event[0] = (unsigned char)b;
	event[1] = (unsigned char)(b >> 8);
	event[2] = (unsigned char)(b >> 16);
	if(tonegen && port < 4)
		tonegen->send_channel_at(time, port, event[0], event[1], event[2]);
}

void TSPlayer::sendSysexTime(const uint8_t *data, size_t size, unsigned port, uint32_t time) {
	if(tonegen && port < 4)
		tonegen->send_sysex_at(time, port, std::span<const uint8_t>(data, size));
}

void TSPlayer::dispatchMidi(const uint8_t *data, size_t length,
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

void TSPlayer::renderChunk(float *out, uint32_t count) {
	float *ptrL, *ptrR;
	while(count) {
		UInt32 numberFrames = count > BLOCK_SIZE ? BLOCK_SIZE : (UInt32)count;
		ptrL = audioBuffer;
		ptrR = audioBuffer + BLOCK_SIZE;

		bzero(ptrL, numberFrames * sizeof(float));
		bzero(ptrR, numberFrames * sizeof(float));

		tonegen->render(std::span<float>(ptrL, numberFrames), std::span<float>(ptrR, numberFrames));

		for(unsigned long j = 0; j < numberFrames; ++j) {
			out[j * 2] = ptrL[j];
			out[j * 2 + 1] = ptrR[j];
		}

		out += numberFrames * 2;
		count -= numberFrames;
	}
}

void TSPlayer::setSCCore(const char *in) {
	romPath = in;
	shutdown();
}

void TSPlayer::shutdown() {
	delete tonegen;
	delete notes;
	rom.reset();
	delete[] audioBuffer;

	tonegen = NULL;
	notes = NULL;
	audioBuffer = NULL;

	initialized = false;
}

bool TSPlayer::startup() {
	if(initialized) return true;

	if(!(port_mask & 15)) return false;

	rom = std::make_unique<const ts::RomImage>(ts::RomImage::open(romPath, ts::RomVerification::quick));

	audioBuffer = new float[BLOCK_SIZE * 2];

	ts::ToneGeneratorOptions options;

	options.ports = 4;
	options.polyphony = 256;

	NSString *flavor = [[NSUserDefaults standardUserDefaults] stringForKey:@"midi.flavor"];
	if([flavor isEqualToString:@"sc55"])
		options.map = ts::ToneMap::sc55;
	else if([flavor isEqualToString:@"sc88"])
		options.map = ts::ToneMap::sc88;
	else if([flavor isEqualToString:@"sc88pro"])
		options.map = ts::ToneMap::sc88pro;
	else if([flavor isEqualToString:@"xg"])
		options.map = ts::ToneMap::xg;

	notes = new ts::NoteRenderer(*rom.get());
	tonegen = new ts::ToneGenerator(*notes, options);

	initialized = true;

	return true;
}

double TSPlayer::sampleRate() {
	return 32000;
}
