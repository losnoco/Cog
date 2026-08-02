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
	notes[0] = NULL;
	notes[1] = NULL;
	tonegen[0] = NULL;
	tonegen[1] = NULL;
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
	if(port > 3) port = 0;
	if(tonegen[port / 2])
		tonegen[port / 2]->send_channel(port & 1, event[0], event[1], event[2]);
}

void TSPlayer::sendSysexTime(const uint8_t *data, size_t size, unsigned port, uint32_t time) {
	if(port > 3) port = 0;
	if(tonegen[port / 2])
		tonegen[port / 2]->send_sysex(port & 1, std::span<const std::uint8_t>(data, size));
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
	bzero(out, count * sizeof(float) * 2);
	while(count) {
		UInt32 numberFrames = count > BLOCK_SIZE ? BLOCK_SIZE : (UInt32)count;
		ptrL = audioBuffer;
		ptrR = audioBuffer + BLOCK_SIZE;

		for(unsigned long i = 0; i < 2; ++i) {
			if(!tonegen[i]) continue;

			bzero(ptrL, numberFrames * sizeof(float));
			bzero(ptrR, numberFrames * sizeof(float));

			tonegen[i]->render(std::span<float>(ptrL, numberFrames), std::span<float>(ptrR, numberFrames));

			for(unsigned long j = 0; j < numberFrames; ++j) {
				out[j * 2] += ptrL[j];
				out[j * 2 + 1] += ptrR[j];
			}
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
	delete tonegen[0];
	delete notes[0];
	delete tonegen[1];
	delete notes[1];
	rom.reset();
	delete[] audioBuffer;

	tonegen[0] = NULL;
	notes[0] = NULL;
	tonegen[1] = NULL;
	notes[1] = NULL;
	audioBuffer = NULL;

	initialized = false;
}

bool TSPlayer::startup() {
	if(initialized) return true;

	rom = std::make_unique<const ts::RomImage>(ts::RomImage::open(romPath, ts::RomVerification::quick));

	audioBuffer = new float[BLOCK_SIZE * 2];

	ts::ToneGeneratorOptions options;
	NSString *flavor = [[NSUserDefaults standardUserDefaults] stringForKey:@"midi.flavor"];
	if([flavor isEqualToString:@"sc55"])
		options.map = ts::ToneMap::sc55;
	else if([flavor isEqualToString:@"sc88"])
		options.map = ts::ToneMap::sc88;
	else if([flavor isEqualToString:@"sc88pro"])
		options.map = ts::ToneMap::sc88pro;

	for(int i = 0; i < 4; i += 2) {
		if(!(port_mask & (3u << i))) continue;

		notes[i / 2] = new ts::NoteRenderer(*rom.get());
		tonegen[i / 2] = new ts::ToneGenerator(*notes[i / 2], options);
	}

	initialized = true;

	return true;
}

double TSPlayer::sampleRate() {
	return 32000;
}
