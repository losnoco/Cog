/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#include "prg.h"

#include <memory>

#include "sidplayfp/SidTuneInfo.h"

#include "SidTuneTools.h"
#include "stringutils.h"

namespace libsidplayfp
{

// Format strings
const char TXT_FORMAT_PRG[] = "Tape image file (PRG)";

SidTuneBase* prg::load(const char *fileName, buffer_t& dataBuf)
{
    const char *ext = SidTuneTools::fileExtOfPath(fileName);
    if ((!stringutils::equal(ext, ".prg"))
        && (!stringutils::equal(ext, ".c64")))
    {
        return nullptr;
    }

    if (dataBuf.size() < 2)
    {
        throw loadError(ERR_TRUNCATED);
    }

    std::unique_ptr<prg> tune(new prg());
    tune->load();

    return tune.release();
}

void prg::load()
{
    info->m_formatString = TXT_FORMAT_PRG;

    // Automatic settings
    info->m_songs         = 1;
    info->m_startSong     = 1;
    info->m_compatibility = SidTuneInfo::COMPATIBILITY_BASIC;

    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(~0, info->m_clockSpeed);
}

}
