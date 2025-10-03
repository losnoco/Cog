//
// Copyright (C) 2015 Alexey Khokholov (Nuke.YKT)
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

class fm_chip {
	public:
	virtual ~fm_chip() {
	}
	virtual int fm_init(unsigned int rate) = 0;
	virtual void fm_writereg(unsigned short reg, unsigned char data) = 0;
	virtual void fm_generate(signed short* buffer, unsigned int length) = 0;
};

class midisynth {
	public:
	virtual ~midisynth() {
	}
	virtual const char* midi_synth_name(void) = 0;
	virtual unsigned int midi_bank_count(void) = 0;
	virtual const char* midi_bank_name(unsigned int bank) = 0;
	virtual int midi_init(unsigned int rate, unsigned int bank, unsigned int extp) = 0;
	virtual void midi_write(unsigned int data) = 0;
	virtual void midi_generate(signed short* buffer, unsigned int length) = 0;
};

midisynth* getsynth_doom();
midisynth* getsynth_opl3w();
fm_chip* getchip();
