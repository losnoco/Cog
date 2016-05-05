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

#ifndef SYSTEMRAMBANK_H
#define SYSTEMRAMBANK_H

#include <stdint.h>
#include <cstring>

#include "Bank.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * Area backed by RAM.
 *
 * @author Antti Lankila
 */
class SystemRAMBank final : public Bank
{
    friend class MMU;

private:
    /// C64 RAM area
    uint8_t ram[0x10000];

public:
    /**
     * Initialize RAM with powerup pattern.
     */
    void reset()
    {
        memset(ram, 0, sizeof(ram));
        for (int i = 0x40; i < 0x10000; i += 0x80)
        {
            memset(ram+i, 0xff, 0x40);
        }
    }

    uint8_t peek(uint_least16_t address) override
    {
        return ram[address];
    }

    void poke(uint_least16_t address, uint8_t value) override
    {
        ram[address] = value;
    }
};

}

#endif
