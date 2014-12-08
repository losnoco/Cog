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

#ifndef BANK_H
#define BANK_H

#include <stdint.h>

#include "sidplayfp/siddefs.h"

namespace libsidplayfp
{

/**
 * Base interface for memory and I/O banks.
 */
class Bank
{
public:
    /**
     * Bank write.
     *
     * Override this method if you expect write operations on your bank.
     * Leave unimplemented if it's logically/operationally impossible for
     * writes to ever arrive to bank.
     *
     * @param address address to write to
     * @param value value to write
     */
    virtual void poke(uint_least16_t address, uint8_t value) =0;

    /**
     * Bank read. You probably
     * should override this method, except if the Bank is only used in
     * write context.
     *
     * @param address value to read from
     * @return value at address
     */
    virtual uint8_t peek(uint_least16_t address) =0;

protected:
    ~Bank() {}
};

}

#endif
