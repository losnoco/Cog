/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2016 Leandro Nini <drfiemost@users.sourceforge.net>
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
 * Processor Status Register
 */
class Flags
{
private:
    bool C; ///< Carry
    bool Z; ///< Zero
    bool I; ///< Interrupt disabled
    bool D; ///< Decimal
    bool B; ///< Break
    bool V; ///< Overflow
    bool N; ///< Negative

public:
    inline void reset()
    {
        C = Z = I = D = V = N = false;
        B = true;
    }

    /**
     * Set N and Z flag values.
     *
     * @param value to set flags from
     */
    inline void setNZ(uint8_t value)
    {
        Z = value == 0;
        N = value & 0x80;
    }

    /**
     * Get status register value.
     */
    inline uint8_t get()
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
    inline void set(uint8_t sr)
    {
        C = sr & 0x01;
        Z = sr & 0x02;
        I = sr & 0x04;
        D = sr & 0x08;
        B = sr & 0x10;
        V = sr & 0x40;
        N = sr & 0x80;
    }

    inline bool getN() const { return N; }
    inline bool getC() const { return C; }
    inline bool getD() const { return D; }
    inline bool getZ() const { return Z; }
    inline bool getV() const { return V; }
    inline bool getI() const { return I; }
    inline bool getB() const { return B; }

    inline void setN(bool f) { N = f; }
    inline void setC(bool f) { C = f; }
    inline void setD(bool f) { D = f; }
    inline void setZ(bool f) { Z = f; }
    inline void setV(bool f) { V = f; }
    inline void setI(bool f) { I = f; }
    inline void setB(bool f) { B = f; }
};

}

#endif
