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

#ifndef IOBANK_H
#define IOBANK_H

#include <stdint.h>

#include "Bank.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * IO region handler. 4k region, 16 chips, 256b banks.
 *
 * Located at $D000-$DFFF
 *
 * @author Antti Lankila
 */
class IOBank final : public Bank
{
private:
    Bank* map[16];

public:
    void setBank(int num, Bank* bank)
    {
        map[num] = bank;
    }

    Bank *getBank(int num) const
    {
        return map[num];
    }

    uint8_t peek(uint_least16_t addr) override
    {
        return map[addr >> 8 & 0xf]->peek(addr);
    }

    void poke(uint_least16_t addr, uint8_t data) override
    {
        map[addr >> 8 & 0xf]->poke(addr, data);
    }
};

}

#endif
