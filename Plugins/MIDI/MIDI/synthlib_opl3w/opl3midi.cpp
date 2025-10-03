//
// Copyright (C) 2015-2016 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#include "opl3midi.h"
#include <limits.h>
#include <math.h>
#include <string.h>

void OPL3MIDI::opl_writereg(uint32_t reg, byte data) {
	opl_chip->fm_writereg(reg, data);
}

uint32_t OPL3MIDI::opl_tofnum(double freq) {
	return (uint32_t)(((1 << 19) * freq) / opl_samplerate);
}

void OPL3MIDI::opl_buildfreqtable(void) {
	double opl_semitone = pow(2.0, 1.0 / 12.0);
	opl_freq[0] = opl_tofnum(opl_tune * pow(opl_semitone, -9));
	opl_freq[1] = opl_tofnum(opl_tune * pow(opl_semitone, -8));
	opl_freq[2] = opl_tofnum(opl_tune * pow(opl_semitone, -7));
	opl_freq[3] = opl_tofnum(opl_tune * pow(opl_semitone, -6));
	opl_freq[4] = opl_tofnum(opl_tune * pow(opl_semitone, -5));
	opl_freq[5] = opl_tofnum(opl_tune * pow(opl_semitone, -4));
	opl_freq[6] = opl_tofnum(opl_tune * pow(opl_semitone, -3));
	opl_freq[7] = opl_tofnum(opl_tune * pow(opl_semitone, -2));
	opl_freq[8] = opl_tofnum(opl_tune * pow(opl_semitone, -1));
	opl_freq[9] = opl_tofnum(opl_tune * pow(opl_semitone, 0));
	opl_freq[10] = opl_tofnum(opl_tune * pow(opl_semitone, 1));
	opl_freq[11] = opl_tofnum(opl_tune * pow(opl_semitone, 2));

	opl_uppitch = (uint32_t)((opl_semitone * opl_semitone - 1.0) * (1 << opl_pitchfrac));
	opl_downpitch = (uint32_t)((1.0 - 1.0 / (opl_semitone * opl_semitone)) * (1 << opl_pitchfrac));
}

uint32_t OPL3MIDI::opl_calcblock(uint32_t freq) {
	byte block = 1;
	while(freq > 0x3ff) {
		block++;
		freq /= 2;
	}
	if(block > 0x07) {
		block = 0x07;
	}

	return (block << 10) | freq;
}

uint32_t OPL3MIDI::opl_applypitch(uint32_t freq, int32_t pitch) {
	int32_t diff;

	if(pitch > 0) {
		diff = (pitch * opl_uppitch) >> opl_pitchfrac;
		freq += (diff * freq) >> 15;
	} else if(pitch < 0) {
		diff = (-pitch * opl_downpitch) >> opl_pitchfrac;
		freq -= (diff * freq) >> 15;
	}
	return freq;
}

opl_voice *OPL3MIDI::opl_allocvoice(opl_timbre *timbre) {
	uint32_t time;
	uint32_t i;
	int32_t id;

	for(i = 0; i < opl_voice_num; i++) {
		if(opl_voices[i].time == 0) {
			return &opl_voices[i];
		}
	}

	time = UINT32_MAX;
	id = -1;

	for(i = 0; i < opl_voice_num; i++) {
		if(!opl_voices[i].keyon && opl_voices[i].time < time) {
			id = i;
			time = opl_voices[i].time;
		}
	}
	if(id >= 0) {
		return &opl_voices[id];
	}

	for(i = 0; i < opl_voice_num; i++) {
		if(opl_voices[i].timbre == timbre && opl_voices[i].time < time) {
			id = i;
			time = opl_voices[i].time;
		}
	}
	if(id >= 0) {
		return &opl_voices[id];
	}

	for(i = 0; i < opl_voice_num; i++) {
		if(opl_voices[i].time < time) {
			id = i;
			time = opl_voices[i].time;
		}
	}

	return &opl_voices[id];
}

opl_voice *OPL3MIDI::opl_findvoice(opl_channel *channel, byte note) {
	uint32_t i;

	for(i = 0; i < opl_voice_num; i++) {
		if(opl_voices[i].keyon && opl_voices[i].channel == channel && opl_voices[i].note == note) {
			return &opl_voices[i];
		}
	}
	return NULL;
}

void OPL3MIDI::opl_midikeyon(opl_channel *channel, byte note, opl_timbre *timbre, byte velocity) {
	opl_voice *voice;
	uint32_t freq;
	uint32_t freqpitched;
	uint32_t octave;
	uint32_t carvol;
	uint32_t modvol;
	byte fb;

	octave = note / 12;
	freq = opl_freq[note % 12];
	if(octave < 5) {
		freq >>= (5 - octave);
	} else if(octave > 5) {
		freq <<= (octave - 5);
	}

	if(timbre->octave < 4) {
		freq >>= (4 - timbre->octave);
	} else if(timbre->octave > 4) {
		freq >>= (timbre->octave - 4);
	}

	freqpitched = opl_calcblock(opl_applypitch(freq, channel->pitch));

	carvol = (timbre->tl[1] & 0x3f) + channel->volume + opl_volume_map[velocity >> 2];
	modvol = timbre->tl[0] & 0x3f;

	if(timbre->fb & 0x01) {
		modvol += channel->volume + opl_volume_map[velocity >> 2];
	}

	if(carvol > 0x3f) {
		carvol = 0x3f;
	}

	if(modvol > 0x3f) {
		modvol = 0x3f;
	}

	carvol |= (timbre->tl[1] & 0xc0);
	modvol |= (timbre->tl[0] & 0xc0);

	fb = timbre->fb & channel->pan;

	voice = opl_allocvoice(timbre);

	opl_writereg(OPL_BLOCK + voice->num, 0x00);

	opl_writereg(OPL_MULT + voice->mod, timbre->mult[0]);
	opl_writereg(OPL_TL + voice->mod, modvol);
	opl_writereg(OPL_AD + voice->mod, timbre->ad[0]);
	opl_writereg(OPL_SR + voice->mod, timbre->sr[0]);
	opl_writereg(OPL_WAVE + voice->mod, timbre->wf[0]);

	opl_writereg(OPL_MULT + voice->car, timbre->mult[1]);
	opl_writereg(OPL_TL + voice->car, carvol);
	opl_writereg(OPL_AD + voice->car, timbre->ad[1]);
	opl_writereg(OPL_SR + voice->car, timbre->sr[1]);
	opl_writereg(OPL_WAVE + voice->car, timbre->wf[1]);

	opl_writereg(OPL_FNUM + voice->num, freqpitched & 0xff);
	opl_writereg(OPL_FEEDBACK + voice->num, fb);
	opl_writereg(OPL_BLOCK + voice->num, (freqpitched >> 8) | 0x20);

	if(opl_extp) {
		opl_writereg(0x107, (voice->num & 0xFF) + ((voice->num / 256) * 9));
		opl_writereg(0x108, channel->panex * 2);
	}

	voice->freq = freq;
	voice->freqpitched = freqpitched;
	voice->note = note;
	voice->velocity = velocity;
	voice->timbre = timbre;
	voice->channel = channel;
	voice->time = opl_time++;
	voice->keyon = true;
	voice->sustained = false;
}

void OPL3MIDI::opl_midikeyoff(opl_channel *channel, byte note, opl_timbre *timbre, bool sustained) {
	opl_voice *voice;

	voice = opl_findvoice(channel, note);
	if(!voice) {
		return;
	}

	if(sustained) {
		voice->sustained = true;
		return;
	}

	opl_writereg(OPL_BLOCK + voice->num, voice->freqpitched >> 8);

	voice->keyon = false;
	voice->time = opl_time;
}

void OPL3MIDI::opl_midikeyoffall(opl_channel *channel) {
	uint32_t i;

	for(i = 0; i < opl_voice_num; i++) {
		if(opl_voices[i].channel == channel) {
			opl_midikeyoff(opl_voices[i].channel, opl_voices[i].note, opl_voices[i].timbre, false);
		}
	}
}

void OPL3MIDI::opl_updatevolpan(opl_channel *channel) {
	uint32_t i;
	uint32_t modvol;
	uint32_t carvol;

	for(i = 0; i < opl_voice_num; i++) {
		if(opl_voices[i].channel == channel) {
			carvol = (opl_voices[i].timbre->tl[1] & 0x3f) + channel->volume + opl_volume_map[opl_voices[i].velocity >> 2];
			modvol = opl_voices[i].timbre->tl[0] & 0x3f;

			if(opl_voices[i].timbre->fb & 0x01) {
				modvol += channel->volume + opl_volume_map[opl_voices[i].velocity >> 2];
			}

			if(carvol > 0x3f) {
				carvol = 0x3f;
			}

			if(modvol > 0x3f) {
				modvol = 0x3f;
			}

			carvol |= (opl_voices[i].timbre->tl[1] & 0xc0);
			modvol |= (opl_voices[i].timbre->tl[0] & 0xc0);

			opl_writereg(OPL_TL + opl_voices[i].mod, modvol);
			opl_writereg(OPL_TL + opl_voices[i].car, carvol);

			opl_writereg(OPL_FEEDBACK + opl_voices[i].num, opl_voices[i].timbre->fb & channel->pan);

			if(opl_extp) {
				opl_writereg(0x107, (opl_voices[i].num & 0xFF) + ((opl_voices[i].num & 0x100) * 9 / 256));
				opl_writereg(0x108, channel->panex * 2);
			}
		}
	}
}

void OPL3MIDI::opl_updatevol(opl_channel *channel, byte vol) {
	channel->volume = opl_volume_map[vol >> 2];
	opl_updatevolpan(channel);
}

void OPL3MIDI::opl_updatepan(opl_channel *channel, byte pan) {
	if(opl_extp) {
		channel->panex = pan;
	} else {
		if(pan < 48) {
			channel->pan = 0xdf;
		} else if(pan > 80) {
			channel->pan = 0xef;
		} else {
			channel->pan = 0xff;
		}
	}
	opl_updatevolpan(channel);
}

void OPL3MIDI::opl_updatesustain(opl_channel *channel, byte sustain) {
	uint32_t i;

	if(sustain >= 64) {
		channel->sustained = true;
	} else {
		channel->sustained = false;

		for(i = 0; i < opl_voice_num; i++) {
			if(opl_voices[i].channel == channel && opl_voices[i].sustained) {
				opl_midikeyoff(channel, opl_voices[i].note, opl_voices[i].timbre, false);
			}
		}
	}
}

void OPL3MIDI::opl_updatepitch(opl_channel *channel) {
	uint32_t i;
	uint32_t freqpitch;

	for(i = 0; i < opl_voice_num; i++) {
		if(opl_voices[i].channel == channel) {
			freqpitch = opl_calcblock(opl_applypitch(opl_voices[i].freq, channel->pitch));
			opl_voices[i].freqpitched = freqpitch;

			opl_writereg(OPL_BLOCK + opl_voices[i].num, (freqpitch >> 8) | ((!!opl_voices[i].keyon) << 5));
			opl_writereg(OPL_FNUM + opl_voices[i].num, freqpitch & 0xff);
		}
	}
}

void OPL3MIDI::opl_midicontrol(opl_channel *channel, byte type, byte data) {
	switch(type) {
		case MIDI_CONTROL_VOL:
			opl_updatevol(channel, data);
			break;
		case MIDI_CONTROL_BAL:
		case MIDI_CONTROL_PAN:
			opl_updatepan(channel, data);
			break;
		case MIDI_CONTROL_SUS:
			opl_updatesustain(channel, data);
			break;
		default:
			if(type >= MIDI_CONTROL_ALLOFF) {
				opl_midikeyoffall(channel);
			}
	}
}

void OPL3MIDI::opl_midiprogram(opl_channel *channel, byte program) {
	if(channel != &opl_channels[MIDI_DRUMCHANNEL]) {
		channel->timbre = &opl_timbres[program];
	}
}

void OPL3MIDI::opl_midipitchbend(opl_channel *channel, byte parm1, byte parm2) {
	int16_t pitch;

	pitch = (parm2 << 9) | (parm1 << 2);
	pitch += 0x7fff;
	channel->pitch = pitch;

	opl_updatepitch(channel);
}

int OPL3MIDI::midi_init(unsigned int rate, unsigned int bank, unsigned int extp) {
	uint32_t i;

	opl_chip = getchip();
	if(!opl_chip || !opl_chip->fm_init(rate)) {
		return 0;
	}

	opl_opl3mode = true;

	opl_extp = !!extp;

	if(opl_extp)
		opl_writereg(0x106, 0x17);

	opl_writereg(OPL_LSI, 0x00);
	opl_writereg(OPL_TIMER, 0x60);
	opl_writereg(OPL_NTS, 0x00);
	if(opl_opl3mode) {
		opl_writereg(OPL_NEW, 0x01);
		opl_writereg(OPL_4OP, 0x00);
	}
	opl_writereg(OPL_RHYTHM, 0xc0);

	for(i = 0; i <= 0x15; i++) {
		opl_writereg(OPL_TL + i, 0x3f);
		if(opl_opl3mode) {
			opl_writereg(OPL_TL + 0x100 + i, 0x3f);
		}
	}

	for(i = 0; i < 9; i++) {
		opl_writereg(OPL_BLOCK + i, 0x00);
		if(opl_opl3mode) {
			opl_writereg(OPL_BLOCK + 0x100 + i, 0x00);
		}
	}

	opl_voice_num = 9;

	if(opl_opl3mode) {
		opl_voice_num = 18;
	}

	for(i = 0; i < opl_voice_num; i++) {
		opl_voices[i].num = i % 9;
		opl_voices[i].mod = opl_voice_map[i % 9];
		opl_voices[i].car = opl_voice_map[i % 9] + 3;
		if(i >= 9) {
			opl_voices[i].num += 0x100;
			opl_voices[i].mod += 0x100;
			opl_voices[i].car += 0x100;
		}
		opl_voices[i].freq = 0;
		opl_voices[i].freqpitched = 0;
		opl_voices[i].time = 0;
		opl_voices[i].note = 0;
		opl_voices[i].velocity = 0;
		opl_voices[i].keyon = false;
		opl_voices[i].sustained = false;
		opl_voices[i].timbre = &opl_timbres[0];
		opl_voices[i].channel = &opl_channels[0];
	}

	for(i = 0; i < 16; i++) {
		opl_channels[i].timbre = &opl_timbres[0];
		opl_channels[i].pitch = 0;
		opl_channels[i].volume = 0;
		opl_channels[i].pan = 0xff;
		opl_channels[i].panex = 64;
		opl_channels[i].sustained = false;
	}

	opl_buildfreqtable();

	opl_time = 1;

	return 1;
}

OPL3MIDI::~OPL3MIDI() {
	delete opl_chip;
}

void OPL3MIDI::midi_write(unsigned int data) {
	byte event_type = data & 0xf0;
	byte channel = data & 0x0f;
	byte parm1 = (data >> 8) & 0x7f;
	byte parm2 = (data >> 16) & 0x7f;
	opl_channel *channelp = &opl_channels[channel];

	switch(event_type) {
		case MIDI_NOTEON:
			if(parm2 > 0) {
				if(channel == MIDI_DRUMCHANNEL) {
					if(opl_drum_maps[parm1].base != 0xff) {
						opl_midikeyon(channelp, opl_drum_maps[parm1].note, &opl_timbres[opl_drum_maps[parm1].base + 128], parm2);
					}
				} else {
					opl_midikeyon(channelp, parm1, channelp->timbre, parm2);
				}
				break;
			}
		case MIDI_NOTEOFF:
			if(channel == MIDI_DRUMCHANNEL) {
				if(opl_drum_maps[parm1].base != 0xff) {
					opl_midikeyoff(channelp, opl_drum_maps[parm1].note, &opl_timbres[opl_drum_maps[parm1].base + 128], false);
				}
			} else {
				opl_midikeyoff(channelp, parm1, channelp->timbre, channelp->sustained);
			}
			break;
		case MIDI_CONTROL:
			opl_midicontrol(channelp, parm1, parm2);
			break;
		case MIDI_PROGRAM:
			opl_midiprogram(channelp, parm1);
			break;
		case MIDI_PITCHBEND:
			opl_midipitchbend(channelp, parm1, parm2);
			break;
	}
}

void OPL3MIDI::midi_generate(signed short *buffer, unsigned int length) {
	opl_chip->fm_generate(buffer, length);
}

const char *OPL3MIDI::midi_synth_name(void) {
	return "OPL3W";
}

unsigned int OPL3MIDI::midi_bank_count(void) {
	return 1;
}

const char *OPL3MIDI::midi_bank_name(unsigned int bank) {
	return "Default";
}

midisynth *getsynth_opl3w() {
	OPL3MIDI *synth = new OPL3MIDI;
	return synth;
}
