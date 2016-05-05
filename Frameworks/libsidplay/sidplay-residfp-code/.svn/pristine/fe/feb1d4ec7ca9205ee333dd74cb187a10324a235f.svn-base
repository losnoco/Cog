/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2010 Antti Lankila
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

#ifndef COLORRAMBANK_H
#define COLORRAMBANK_H

#include <stdint.h>
#include <cstring>

#include "Bank.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * Color RAM.
 *
 * 1K x 4-bit Static RAM that stores text screen color information.
 *
 * Located at $D800-$DBFF (last 24 bytes are unused)
 */
class ColorRAMBank final : public Bank
{
private:
    uint8_t ram[0x400];

public:
    void reset()
    {
         memset(ram, 0, sizeof(ram));
    }

    void poke(uint_least16_t address, uint8_t value) override
    {
        ram[address & 0x3ff] = value & 0xf;
    }

    uint8_t peek(uint_least16_t address) override
    {
        return ram[address & 0x3ff];
    }
};

}

#endif
