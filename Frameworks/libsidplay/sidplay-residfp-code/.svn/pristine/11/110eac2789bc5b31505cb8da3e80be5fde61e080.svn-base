/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2011-2015 Leandro Nini
 *  Copyright 2007-2010 Antti Lankila
 *  Copyright 2000 Simon White
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SIDTUNEINFOIMPL_H
#define SIDTUNEINFOIMPL_H

#include <stdint.h>
#include <vector>
#include <string>

#include "sidplayfp/SidTuneInfo.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * The implementation of the SidTuneInfo interface.
 */
class SidTuneInfoImpl final : public SidTuneInfo
{
public:
    const char* m_formatString;

    unsigned int m_songs;
    unsigned int m_startSong;
    unsigned int m_currentSong;

    int m_songSpeed;

    clock_t m_clockSpeed;

    compatibility_t m_compatibility;

    uint_least32_t m_dataFileLen;

    uint_least32_t m_c64dataLen;

    uint_least16_t m_loadAddr;
    uint_least16_t m_initAddr;
    uint_least16_t m_playAddr;

    uint_least8_t m_relocStartPage;

    uint_least8_t m_relocPages;

    std::string m_path;

    std::string m_dataFileName;

    std::string m_infoFileName;

    std::vector<model_t> m_sidModels;

    std::vector<uint_least16_t> m_sidChipAddresses;

    std::vector<std::string> m_infoString;

    std::vector<std::string> m_commentString;

    bool m_fixLoad;

private:    // prevent copying
    SidTuneInfoImpl(const SidTuneInfoImpl&);
    SidTuneInfoImpl& operator=(SidTuneInfoImpl&);

public:
    SidTuneInfoImpl() :
        m_formatString("N/A"),
        m_songs(0),
        m_startSong(0),
        m_currentSong(0),
        m_songSpeed(SPEED_VBI),
        m_clockSpeed(CLOCK_UNKNOWN),
        m_compatibility(COMPATIBILITY_C64),
        m_dataFileLen(0),
        m_c64dataLen(0),
        m_loadAddr(0),
        m_initAddr(0),
        m_playAddr(0),
        m_relocStartPage(0),
        m_relocPages(0),
        m_fixLoad(false)
    {
        m_sidModels.push_back(SIDMODEL_UNKNOWN);
        m_sidChipAddresses.push_back(0xd400);
    }

    uint_least16_t getLoadAddr() const override { return m_loadAddr; }

    uint_least16_t getInitAddr() const override { return m_initAddr; }

    uint_least16_t getPlayAddr() const override { return m_playAddr; }

    unsigned int getSongs() const override { return m_songs; }

    unsigned int getStartSong() const override { return m_startSong; }

    unsigned int getCurrentSong() const override { return m_currentSong; }

    uint_least16_t getSidChipBase(unsigned int i) const override
    {
        return i < m_sidChipAddresses.size() ? m_sidChipAddresses[i] : 0;
    }

    int getSidChips() const override { return m_sidChipAddresses.size(); }

    int getSongSpeed() const override { return m_songSpeed; }

    uint_least8_t getRelocStartPage() const override { return m_relocStartPage; }

    uint_least8_t getRelocPages() const override { return m_relocPages; }

    model_t getSidModel(unsigned int i) const override
    {
        return i < m_sidModels.size() ? m_sidModels[i] : SIDMODEL_UNKNOWN;
    }

    compatibility_t getCompatibility() const override { return m_compatibility; }

    unsigned int getNumberOfInfoStrings() const override { return m_infoString.size(); }
    const char* getInfoString(unsigned int i) const override { return i<getNumberOfInfoStrings() ? m_infoString[i].c_str() : ""; }

    unsigned int getNumberOfCommentStrings() const override { return m_commentString.size(); }
    const char* getCommentString(unsigned int i) const override { return i<getNumberOfCommentStrings() ? m_commentString[i].c_str() : ""; }

    uint_least32_t getDataFileLen() const override { return m_dataFileLen; }

    uint_least32_t getC64dataLen() const override { return m_c64dataLen; }

    clock_t getClockSpeed() const override { return m_clockSpeed; }

    const char* getFormatString() const override { return m_formatString; }

    bool getFixLoad() const override { return m_fixLoad; }

    const char* getPath() const override { return m_path.c_str(); }

    const char* getDataFileName() const override { return m_dataFileName.c_str(); }

    const char* getInfoFileName() const override { return !m_infoFileName.empty() ? m_infoFileName.c_str() : nullptr; }
};

}

#endif  /* SIDTUNEINFOIMPL_H */
