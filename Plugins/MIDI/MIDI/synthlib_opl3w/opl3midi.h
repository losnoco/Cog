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

#pragma once

#include "../interface.h"
#include "gmtimbre.h"
#include "opl3type.h"
#include <stdint.h>

#define OPL_LSI 0x00
#define OPL_TIMER 0x04
#define OPL_4OP 0x104
#define OPL_NEW 0x105
#define OPL_NTS 0x08
#define OPL_MULT 0x20
#define OPL_TL 0x40
#define OPL_AD 0x60
#define OPL_SR 0x80
#define OPL_WAVE 0xe0
#define OPL_FNUM 0xa0
#define OPL_BLOCK 0xb0
#define OPL_RHYTHM 0xbd
#define OPL_FEEDBACK 0xc0

#define MIDI_DRUMCHANNEL 9
#define MIDI_NOTEOFF 0x80
#define MIDI_NOTEON 0x90
#define MIDI_CONTROL 0xb0
#define MIDI_CONTROL_VOL 0x07
#define MIDI_CONTROL_BAL 0x08
#define MIDI_CONTROL_PAN 0x0a
#define MIDI_CONTROL_SUS 0x40
#define MIDI_CONTROL_ALLOFF 0x78
#define MIDI_PROGRAM 0xc0
#define MIDI_PITCHBEND 0xe0

const double opl_samplerate = 50000.0;
const double opl_tune = 440.0;
const byte opl_pitchfrac = 8;

const byte opl_volume_map[32] = {
	80, 63, 40, 36, 32, 28, 23, 21,
	19, 17, 15, 14, 13, 12, 11, 10,
	9, 8, 7, 6, 5, 5, 4, 4,
	3, 3, 2, 2, 1, 1, 0, 0
};

const byte opl_voice_map[9]{
	0, 1, 2, 8, 9, 10, 16, 17, 18
};

typedef struct
{
	opl_timbre *timbre;
	int32_t pitch;
	uint32_t volume;
	uint32_t pan, panex;
	bool sustained;
} opl_channel;

typedef struct
{
	uint32_t num;
	uint32_t mod, car;
	uint32_t freq;
	uint32_t freqpitched;
	uint32_t time;
	byte note;
	byte velocity;
	bool keyon;
	bool sustained;
	opl_timbre *timbre;
	opl_channel *channel;
} opl_voice;

class OPL3MIDI : public midisynth {
	private:
	fm_chip *opl_chip;
	bool opl_opl3mode;
	bool opl_extp;

	uint32_t opl_voice_num;

	opl_channel opl_channels[16];
	opl_voice opl_voices[18];

	uint32_t opl_freq[12];
	uint32_t opl_time;
	int32_t opl_uppitch;
	int32_t opl_downpitch;

	void opl_writereg(uint32_t reg, byte data);

	uint32_t opl_tofnum(double freq);
	void opl_buildfreqtable(void);
	uint32_t opl_calcblock(uint32_t fnum);
	uint32_t opl_applypitch(uint32_t freq, int32_t pitch);
	opl_voice *opl_allocvoice(opl_timbre *timbre);
	opl_voice *opl_findvoice(opl_channel *channel, byte note);
	void opl_midikeyon(opl_channel *channel, byte note, opl_timbre *timbre, byte velocity);
	void opl_midikeyoff(opl_channel *channel, byte note, opl_timbre *timbre, bool sustained);
	void opl_midikeyoffall(opl_channel *channel);
	void opl_updatevolpan(opl_channel *channel);
	void opl_updatevol(opl_channel *channel, byte vol);
	void opl_updatepan(opl_channel *channel, byte pan);
	void opl_updatesustain(opl_channel *channel, byte sustain);
	void opl_updatepitch(opl_channel *channel);
	void opl_midicontrol(opl_channel *channel, byte type, byte data);
	void opl_midiprogram(opl_channel *channel, byte program);
	void opl_midipitchbend(opl_channel *channel, byte parm1, byte parm2);

	public:
	virtual ~OPL3MIDI();

	const char *midi_synth_name(void);
	unsigned int midi_bank_count(void);
	const char *midi_bank_name(unsigned int bank);
	int midi_init(unsigned int rate, unsigned int bank, unsigned int extp);
	void midi_write(unsigned int data);
	void midi_generate(signed short *buffer, unsigned int length);
};
