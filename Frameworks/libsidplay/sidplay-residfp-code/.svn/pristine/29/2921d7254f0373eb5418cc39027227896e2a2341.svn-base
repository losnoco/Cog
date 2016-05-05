/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef NULLSID_H
#define NULLSID_H

#include "c64/c64sid.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * SID chip placeholder which does nothing and returns 0xff on reading.
 */
class NullSid final : public c64sid
{
private:
    NullSid() {}
    virtual ~NullSid() {}

public:
    /**
     * Returns singleton instance.
     */
    static NullSid *getInstance()
    {
        static NullSid nullsid;
        return &nullsid;
    }

    void reset(uint8_t) override {}

    void write(uint_least8_t, uint8_t) override {}
    uint8_t read(uint_least8_t) override { return 0xff; }
};

}

#endif // NULLSID_H
