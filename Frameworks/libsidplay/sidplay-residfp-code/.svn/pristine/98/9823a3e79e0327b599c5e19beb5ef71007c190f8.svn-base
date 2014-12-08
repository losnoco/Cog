/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef FLAGS_H
#define FLAGS_H

#include <stdint.h>

namespace libsidplayfp
{

/**
 * Processor status flag.
 */
class Flags
{
public:
    bool C; ///< Carry
    bool Z; ///< Zero
    bool I; ///< Interrupt disabled
    bool D; ///< Decimal
    bool B; ///< Break
    bool V; ///< Overflow
    bool N; ///< Negative

public:
    void reset()
    {
        C = Z = I = D = V = N = false;
        B = true;
    }

    /**
     * Set N and Z flag values.
     *
     * @param value to set flags from
     */
    void setNZ(uint8_t value)
    {
        Z = value == 0;
        N = value & 0x80;
    }

    /**
     * Get status register value.
     */
    uint8_t get()
    {
        uint8_t sr = 0x20;

        if (C) sr |= 0x01;
        if (Z) sr |= 0x02;
        if (I) sr |= 0x04;
        if (D) sr |= 0x08;
        if (B) sr |= 0x10;
        if (V) sr |= 0x40;
        if (N) sr |= 0x80;

        return sr;
    }

    /**
     * Set status register value.
     */
    void set(uint8_t sr)
    {
        C = sr & 0x01;
        Z = sr & 0x02;
        I = sr & 0x04;
        D = sr & 0x08;
        B = sr & 0x10;
        V = sr & 0x40;
        N = sr & 0x80;
    }
};

}

#endif
