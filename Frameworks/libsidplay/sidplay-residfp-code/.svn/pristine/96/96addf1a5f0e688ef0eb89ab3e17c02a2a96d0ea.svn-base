/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MUS_H
#define MUS_H

#include <stdint.h>

#include "SidTuneBase.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

class MUS final : public SidTuneBase
{
private:
    /// Needed for MUS/STR player installation.
    uint_least16_t musDataLen;

private:
    bool mergeParts(buffer_t& musBuf, buffer_t& strBuf);

    void tryLoad(buffer_t& musBuf,
                    buffer_t& strBuf,
                    SmartPtr_sidtt<const uint8_t> &spPet,
                    uint_least32_t voice3Index,
                    bool init);

protected:
    MUS() {}

    void installPlayer(sidmemory& mem);

    void setPlayerAddress();

    virtual void acceptSidTune(const char* dataFileName, const char* infoFileName,
                                buffer_t& buf, bool isSlashedFileName) override;

public:
    virtual ~MUS() {}

    static SidTuneBase* load(buffer_t& dataBuf, bool init = false);
    static SidTuneBase* load(buffer_t& musBuf,
                                buffer_t& strBuf,
                                uint_least32_t fileOffset,
                                bool init = false);

    virtual void placeSidTuneInC64mem(sidmemory& mem) override;

private:
    // prevent copying
    MUS(const MUS&);
    MUS& operator=(MUS&);
};

}

#endif // MUS_H
