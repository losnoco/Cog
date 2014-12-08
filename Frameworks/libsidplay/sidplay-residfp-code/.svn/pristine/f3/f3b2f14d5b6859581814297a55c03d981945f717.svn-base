/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "MUS.h"

#include <memory>

#include "sidplayfp/SidTuneInfo.h"

#include "SmartPtr.h"
#include "sidendian.h"
#include "sidmemory.h"

#include "sidcxx11.h"

#include "sidplayer.bin"

namespace libsidplayfp
{

// Format strings
const char TXT_FORMAT_MUS[]        = "C64 Sidplayer format (MUS)";
const char TXT_FORMAT_STR[]        = "C64 Stereo Sidplayer format (MUS+STR)";

// Error strings
const char ERR_2ND_INVALID[]       = "SIDTUNE ERROR: 2nd file contains invalid data";
const char ERR_SIZE_EXCEEDED[]     = "SIDTUNE ERROR: Total file size too large";

static const uint_least16_t SIDTUNE_MUS_HLT_CMD = 0x14F;

static const uint_least16_t SIDTUNE_MUS_DATA_ADDR  = 0x0900;
static const uint_least16_t SIDTUNE_SID1_BASE_ADDR = 0xd400;
static const uint_least16_t SIDTUNE_SID2_BASE_ADDR = 0xd500;

bool detect(const uint_least8_t* buffer, uint_least32_t bufLen,
                         uint_least32_t& voice3Index)
{
    SmartPtr_sidtt<const uint8_t> spMus(static_cast<const uint8_t*>(buffer), bufLen);
    // Skip load address and 3x length entry.
    uint_least32_t voice1Index = (2+3*2);
    // Add length of voice 1 data.
    voice1Index += endian_16(spMus[3], spMus[2]);
    // Add length of voice 2 data.
    uint_least32_t voice2Index = voice1Index + endian_16(spMus[5], spMus[4]);
    // Add length of voice 3 data.
    voice3Index = voice2Index + endian_16(spMus[7], spMus[6]);
    return ((endian_16(spMus[voice1Index-2], spMus[voice1Index+1-2]) == SIDTUNE_MUS_HLT_CMD)
            && (endian_16(spMus[voice2Index-2], spMus[voice2Index+1-2]) == SIDTUNE_MUS_HLT_CMD)
            && (endian_16(spMus[voice3Index-2], spMus[voice3Index+1-2]) == SIDTUNE_MUS_HLT_CMD)
            && spMus);
}

void MUS::setPlayerAddress()
{
    if (info->m_sidChipBase2 == 0)
    {
        // Player #1.
        info->m_initAddr = 0xec60;
        info->m_playAddr = 0xec80;
    }
    else
    {
        // Player #1 + #2.
        info->m_initAddr = 0xfc90;
        info->m_playAddr = 0xfc96;
    }
}

void MUS::acceptSidTune(const char* dataFileName, const char* infoFileName,
                            buffer_t& buf, bool isSlashedFileName)
{
    setPlayerAddress();
    SidTuneBase::acceptSidTune(dataFileName, infoFileName, buf, isSlashedFileName);
}

bool MUS::placeSidTuneInC64mem(sidmemory* mem)
{
    if (SidTuneBase::placeSidTuneInC64mem(mem))
    {
        installPlayer(mem);
        return true;
    }
    return false;
}

bool MUS::mergeParts(buffer_t& musBuf, buffer_t& strBuf)
{
    const uint_least32_t mergeLen = musBuf.size() + strBuf.size();

    // Sanity check. I do not trust those MUS/STR files around.
    const uint_least32_t freeSpace = endian_16(sidplayer1[1], sidplayer1[0]) - SIDTUNE_MUS_DATA_ADDR;
    if ((mergeLen - 4) > freeSpace)
    {
        throw loadError(ERR_SIZE_EXCEEDED);
    }

    if (!strBuf.empty() && info->m_sidChipBase2 != 0)
    {
        // Install MUS data #2 _NOT_ including load address.
        musBuf.insert(musBuf.end(), strBuf.begin(), strBuf.end());
    }

    strBuf.clear();

    return true;
}

void MUS::installPlayer(sidmemory *mem)
{
    if (mem != nullptr)
    {
        // Install MUS player #1.
        uint_least16_t dest = endian_16(sidplayer1[1], sidplayer1[0]);

        mem->fillRam(dest, sidplayer1 + 2, sizeof(sidplayer1) - 2);
        // Point player #1 to data #1.
        mem->writeMemByte(dest+0xc6e, (SIDTUNE_MUS_DATA_ADDR+2)&0xFF);
        mem->writeMemByte(dest+0xc70, (SIDTUNE_MUS_DATA_ADDR+2)>>8);

        if (info->m_sidChipBase2 != 0)
        {
            // Install MUS player #2.
            dest = endian_16(sidplayer2[1], sidplayer2[0]);
            mem->fillRam(dest, sidplayer2 + 2, sizeof(sidplayer2) - 2);
            // Point player #2 to data #2.
            mem->writeMemByte(dest + 0xc6e, (SIDTUNE_MUS_DATA_ADDR + musDataLen + 2) & 0xFF);
            mem->writeMemByte(dest + 0xc70, (SIDTUNE_MUS_DATA_ADDR + musDataLen + 2) >> 8);
        }
    }
}

SidTuneBase* MUS::load(buffer_t& musBuf, bool init)
{
    buffer_t empty;
    return load(musBuf, empty, 0, init);
}

SidTuneBase* MUS::load(buffer_t& musBuf,
                            buffer_t& strBuf,
                            uint_least32_t fileOffset,
                            bool init)
{
    uint_least32_t voice3Index;
    SmartPtr_sidtt<const uint8_t> spPet(&musBuf[fileOffset], musBuf.size()-fileOffset);
    if (!detect(&spPet[0], spPet.tellLength(), voice3Index))
        return nullptr;

    std::unique_ptr<MUS> tune(new MUS());
    tune->tryLoad(musBuf, strBuf, spPet, voice3Index, init);
    tune->mergeParts(musBuf, strBuf);

    return tune.release();
}

void MUS::tryLoad(buffer_t& musBuf,
                    buffer_t& strBuf,
                    SmartPtr_sidtt<const uint8_t> &spPet,
                    uint_least32_t voice3Index,
                    bool init)
{
    if (init)
    {
        info->m_songs = (info->m_startSong = 1);

        songSpeed[0]  = SidTuneInfo::SPEED_CIA_1A;
        clockSpeed[0] = SidTuneInfo::CLOCK_ANY;
    }

    // Check setting compatibility for MUS playback
    if ((info->m_compatibility != SidTuneInfo::COMPATIBILITY_C64)
        || (info->m_relocStartPage != 0) || (info->m_relocPages != 0))
    {
        throw loadError(ERR_INVALID);
    }

    {   // All subtunes should be CIA
        for (uint_least16_t i = 0; i < info->m_songs; i++)
        {
            if (songSpeed[i] != SidTuneInfo::SPEED_CIA_1A)
            {
                throw loadError(ERR_INVALID);
            }
        }
    }

    musDataLen = musBuf.size();
    info->m_loadAddr = SIDTUNE_MUS_DATA_ADDR;
    info->m_sidChipBase1 = SIDTUNE_SID1_BASE_ADDR;

    // Voice3Index now is offset to text lines (uppercase Pet-strings).
    spPet += voice3Index;

    // Extract credits
    while (spPet[0])
    {
        PetsciiToAscii converter;
        info->m_commentString.push_back(converter.convert(spPet));
    }

    spPet++;

    // If we appear to have additional data at the end, check is it's
    // another mus file (but only if a second file isn't supplied)
    bool stereo = false;
    if (!strBuf.empty())
    {
        if (!detect(&strBuf[0], strBuf.size(), voice3Index))
            throw loadError(ERR_2ND_INVALID);
        spPet.setBuffer(&strBuf[0], strBuf.size());
        stereo = true;
    }
    else
    {
        // For MUS + STR via stdin the files come combined
        if (spPet.good())
        {
            const uint_least16_t pos = (uint_least16_t)spPet.tellPos();
            if ( detect(&spPet[0], spPet.tellLength()-pos, voice3Index) )
            {
                musDataLen = pos;
                stereo = true;
            }
        }
    }

    if (stereo)
    {
        // Voice3Index now is offset to text lines (uppercase Pet-strings).
        spPet += voice3Index;

        // Extract credits
        while (spPet[0])
        {
            PetsciiToAscii converter;
            info->m_commentString.push_back(converter.convert(spPet));
        }

        info->m_sidChipBase2 = SIDTUNE_SID2_BASE_ADDR;
        info->m_formatString = TXT_FORMAT_STR;
    }
    else
    {
        info->m_sidChipBase2 = 0;
        info->m_formatString = TXT_FORMAT_MUS;
    }

    setPlayerAddress();

    // Remove trailing empty lines.
    const int lines = info->m_commentString.size();
    {
        for (int line = lines-1; line >= 0; line--)
        {
            if (info->m_commentString[line].length() == 0)
                info->m_commentString.pop_back();
            else
                break;
        }
    }
}

}
