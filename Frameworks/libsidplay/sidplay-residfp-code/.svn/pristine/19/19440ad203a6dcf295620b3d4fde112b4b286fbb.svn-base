/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef MD5_INTERNAL_H
#define MD5_INTERNAL_H

#include "iMd5.h"

#include "MD5/MD5.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

class md5Internal final : public iMd5
{
private:
    MD5 hd;

public:
    void append(const void* data, int nbytes) override { hd.append(data, nbytes); }

    void finish() override { hd.finish(); }

    const unsigned char* getDigest() override { return hd.getDigest(); }

    void reset() override { hd.reset(); }
};

}

#endif
