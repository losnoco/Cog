/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "interrupt.h"

namespace libsidplayfp
{

class SerialPort
{
private:
    InterruptSource *interruptSource;

    int count;

    bool buffered;

    uint8_t out;

public:
    explicit SerialPort(InterruptSource *intSource) :
        interruptSource(intSource)
    {}

    void reset()
    {
        out = 0;
        count = 0;
        buffered = false;
    }

    void setBuffered() { buffered = true; }

    void handle(uint8_t serialDataReg)
    {
        if (count && --count == 0)
        {
            interruptSource->trigger(InterruptSource::INTERRUPT_SP);
        }

        if (count == 0 && buffered)
        {
            out = serialDataReg;
            buffered = false;
            count = 16;
            // Output rate 8 bits at ta / 2
        }
    }
};

}

#endif // SERIALPORT_H
