#include <cmath>
#include <stdlib.h>
#include <string.h>

#include "MSPlayer.h"

MSPlayer::MSPlayer() {
	synth = 0;
	synth_id = 0;
	bank_id = 0;
	extp = 0;
}

MSPlayer::~MSPlayer() {
	shutdown();
}

void MSPlayer::set_synth(unsigned int synth_id) {
	shutdown();
	this->synth_id = synth_id;
}

void MSPlayer::set_bank(unsigned int bank_id) {
	shutdown();
	this->bank_id = bank_id;
}

void MSPlayer::set_extp(unsigned int extp) {
	shutdown();
	this->extp = extp;
}

void MSPlayer::dispatchMidi(const uint8_t *data, size_t length,
                            uint32_t sample_offset, unsigned port) {
	if(!synth || !length || port != 0) return;
	(void)sample_offset;

	uint8_t sb = data[0];
	if(sb >= 0xF0) {
		/* Legacy midisynth engines have no SysEx path; drop. */
		return;
	}

	uint32_t packed = sb;
	if(length >= 2) packed |= (uint32_t)data[1] << 8;
	if(length >= 3) packed |= (uint32_t)data[2] << 16;
	synth->midi_write(packed);
}

void MSPlayer::renderChunk(float *out, uint32_t count) {
	if(!synth) {
		bzero(out, sizeof(float) * count * 2);
		return;
	}
	float const scaler = 1.0f / 8192.0f;
	short buffer[512];
	while(count) {
		uint32_t todo = count > 256 ? 256 : count;
		synth->midi_generate(buffer, todo);
		for(uint32_t i = 0; i < todo; ++i) {
			*out++ = buffer[i * 2 + 0] * scaler;
			*out++ = buffer[i * 2 + 1] * scaler;
		}
		count -= todo;
	}
}

void MSPlayer::shutdown() {
	delete synth;
	synth = 0;
	initialized = false;
}

bool MSPlayer::startup() {
	if(synth) return true;

	switch(synth_id) {
		default:
		case 0:
			synth = getsynth_doom();
			break;

		case 1:
			synth = getsynth_opl3w();
			break;
	}

	if(!synth) return false;

	if(!synth->midi_init((unsigned int)std::lround(dSampleRate), bank_id, extp))
		return false;

	initialized = true;
	return true;
}

void MSPlayer::enum_synthesizers(enum_callback callback) {
	char buffer[512];
	const char *synth_name;

	midisynth *synth = getsynth_doom();

	synth_name = synth->midi_synth_name();

	unsigned int count = synth->midi_bank_count();

	unsigned int i;

	if(count > 1) {
		for(i = 0; i < count; ++i) {
			strcpy(buffer, synth_name);
			strcat(buffer, " ");
			strcat(buffer, synth->midi_bank_name(i));
			callback(0, i, buffer);
		}
	} else {
		callback(0, 0, synth_name);
	}

	delete synth;

	synth = getsynth_opl3w();

	synth_name = synth->midi_synth_name();

	count = synth->midi_bank_count();

	if(count > 1) {
		for(i = 0; i < count; ++i) {
			strcpy(buffer, synth_name);
			strcat(buffer, " ");
			strcat(buffer, synth->midi_bank_name(i));
			callback(0, i, buffer);
		}
	} else {
		callback(0, 0, synth_name);
	}

	delete synth;
}
