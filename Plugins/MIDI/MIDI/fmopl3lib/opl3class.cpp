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

#include <string.h>
#include "opl3class.h"

const Bit64u lat = (50 * 49716) / 1000;

#define RSM_FRAC 10

int opl3class::fm_init(unsigned int rate) {
    OPL3_Reset(&chip, rate);

    memset(command,0,sizeof(command));
    memset(time, 0, sizeof(time));
    memset(oldsamples, 0, sizeof(oldsamples));
    memset(samples, 0, sizeof(samples));
    counter = 0;
    lastwrite = 0;
    strpos = 0;
    endpos = 0;
    samplecnt = 0;
    rateratio = (rate << RSM_FRAC) / 49716;
    
	return 1;
}

void opl3class::fm_writereg(unsigned short reg, unsigned char data) {
    command[endpos % 8192][0] = reg;
    command[endpos % 8192][1] = data;
    Bit64u t1 = lastwrite + 2;
    Bit64u t2 = counter + lat;
    if (t2 > t1)
    {
        t1 = t2;
    }
    time[endpos % 8192] = t1;
    lastwrite = t1;
    endpos = (endpos + 1) % 8192;
}



void opl3class::fm_generate_one(signed short *buffer) {
    while (strpos != endpos && time[strpos] < counter)
    {
        OPL3_WriteReg(&chip, command[strpos][0], command[strpos][1]);
        strpos = (strpos + 1) % 8192;
    }
    OPL3_Generate(&chip, (Bit16s*)buffer);
    buffer += 2;
    counter++;
}

void opl3class::fm_generate(signed short *buffer, unsigned int length) {
    for (; length--;)
    {
        while (samplecnt >= rateratio)
        {
            oldsamples[0] = samples[0];
            oldsamples[1] = samples[1];
            fm_generate_one(samples);
            samplecnt -= rateratio;
        }
        buffer[0] = (Bit16s)((oldsamples[0] * (rateratio - samplecnt)
                            + samples[0] * samplecnt) / rateratio);
        buffer[1] = (Bit16s)((oldsamples[1] * (rateratio - samplecnt)
                            + samples[1] * samplecnt) / rateratio);
        samplecnt += 1 << RSM_FRAC;
        
        buffer += 2;
    }
}

fm_chip *getchip() {
	opl3class *chip = new opl3class;
	return chip;
}
