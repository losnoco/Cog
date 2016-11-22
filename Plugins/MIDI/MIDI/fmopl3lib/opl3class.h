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

#include "../interface.h"
#include "opl3.h"


class opl3class : public fm_chip {
private:
    opl3_chip chip;
    Bit64u counter;
    Bit64u lastwrite;
    Bit16u command[8192][2];
    Bit64u time[8192];
    Bit16u strpos;
    Bit16s endpos;
	void *resampler;
    Bit16s samples[2];
    void fm_generate_one(signed short *buffer);
public:
	int fm_init(unsigned int rate);
	void fm_writereg(unsigned short reg, unsigned char data);
	void fm_generate(signed short *buffer, unsigned int length);
};