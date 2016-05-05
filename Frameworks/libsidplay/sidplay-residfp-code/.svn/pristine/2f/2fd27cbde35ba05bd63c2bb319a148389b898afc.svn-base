/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "PSID.h"

#include <cstring>
#include <string>
#include <memory>

#include "sidplayfp/SidTuneInfo.h"

#include "sidendian.h"
#include "sidmd5.h"

namespace libsidplayfp
{

const int PSID_MAXSTRLEN = 32;


// Header has been extended for 'RSID' format
// The following changes are present:
//     id = 'RSID'
//     version = 2, 3 or 4
//     play, load and speed reserved 0
//     psidspecific flag is called C64BASIC flag
//     init cannot be under ROMS/IO memory area
//     load address cannot be less than $07E8
//     info strings may be 32 characters long without trailing zero

struct psidHeader           // all values are big-endian
{
    uint32_t id;                   // 'PSID' or 'RSID' (ASCII)
    uint16_t version;              // 1, 2, 3 or 4
    uint16_t data;                 // 16-bit offset to binary data in file
    uint16_t load;                 // 16-bit C64 address to load file to
    uint16_t init;                 // 16-bit C64 address of init subroutine
    uint16_t play;                 // 16-bit C64 address of play subroutine
    uint16_t songs;                // number of songs
    uint16_t start;                // start song out of [1..256]
    uint32_t speed;                // 32-bit speed info
                                   // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[PSID_MAXSTRLEN];     // ASCII strings, 31 characters long and
    char author[PSID_MAXSTRLEN];   // terminated by a trailing zero
    char released[PSID_MAXSTRLEN]; //

    uint16_t flags;                // only version >= 2
    uint8_t relocStartPage;        // only version >= 2ng
    uint8_t relocPages;            // only version >= 2ng
    uint8_t sidChipBase2;          // only version >= 3
    uint8_t sidChipBase3;          // only version >= 4
};

enum
{
    PSID_MUS       = 1 << 0,
    PSID_SPECIFIC  = 1 << 1, // These two are mutally exclusive
    PSID_BASIC     = 1 << 1,
    PSID_CLOCK     = 3 << 2,
    PSID_SIDMODEL  = 3 << 4
};

enum
{
    PSID_CLOCK_UNKNOWN = 0,
    PSID_CLOCK_PAL     = 1 << 2,
    PSID_CLOCK_NTSC    = 1 << 3,
    PSID_CLOCK_ANY     = PSID_CLOCK_PAL | PSID_CLOCK_NTSC
};

enum
{
    PSID_SIDMODEL_UNKNOWN = 0,
    PSID_SIDMODEL_6581    = 1,
    PSID_SIDMODEL_8580    = 2,
    PSID_SIDMODEL_ANY     = PSID_SIDMODEL_6581 | PSID_SIDMODEL_8580
};

// Format strings
const char TXT_FORMAT_PSID[]  = "PlaySID one-file format (PSID)";
const char TXT_FORMAT_RSID[]  = "Real C64 one-file format (RSID)";
const char TXT_UNKNOWN_PSID[] = "Unsupported PSID version";
const char TXT_UNKNOWN_RSID[] = "Unsupported RSID version";

const int psid_headerSize = 118;
const int psidv2_headerSize = psid_headerSize + 6;

// Magic fields
const uint32_t PSID_ID = 0x50534944;
const uint32_t RSID_ID = 0x52534944;

/**
 * Decode SID model flags.
 */
SidTuneInfo::model_t getSidModel(uint_least16_t modelFlag)
{
    if ((modelFlag & PSID_SIDMODEL_ANY) == PSID_SIDMODEL_ANY)
        return SidTuneInfo::SIDMODEL_ANY;

    if (modelFlag & PSID_SIDMODEL_6581)
        return SidTuneInfo::SIDMODEL_6581;

    if (modelFlag & PSID_SIDMODEL_8580)
        return SidTuneInfo::SIDMODEL_8580;

    return SidTuneInfo::SIDMODEL_UNKNOWN;
}

/**
 * Check if extra SID addres is valid for PSID specs.
 */
bool validateAddress(uint_least8_t address)
{
    // Only even values are valid.
    if (address & 1)
        return false;

    // Ranges $00-$41 ($D000-$D410) and $80-$DF ($D800-$DDF0) are invalid.
    // Any invalid value means that no second SID is used, like $00.
    if (address <= 0x41
            || (address >= 0x80 && address <= 0xdf))
        return false;

    return true;
}

SidTuneBase* PSID::load(buffer_t& dataBuf)
{
    // File format check
    if (dataBuf.size() < 4)
    {
        return nullptr;
    }

    const uint32_t magic = endian_big32(&dataBuf[0]);
    if ((magic != PSID_ID)
        && (magic != RSID_ID))
    {
        return nullptr;
    }

    psidHeader pHeader;
    readHeader(dataBuf, pHeader);

    std::unique_ptr<PSID> tune(new PSID());
    tune->tryLoad(pHeader);

    return tune.release();
}

void PSID::readHeader(const buffer_t &dataBuf, psidHeader &hdr)
{
    // Due to security concerns, input must be at least as long as version 1
    // header plus 16-bit C64 load address. That is the area which will be
    // accessed.
    if (dataBuf.size() < (psid_headerSize + 2))
    {
        throw loadError(ERR_TRUNCATED);
    }

    // Read v1 fields
    hdr.id               = endian_big32(&dataBuf[0]);
    hdr.version          = endian_big16(&dataBuf[4]);
    hdr.data             = endian_big16(&dataBuf[6]);
    hdr.load             = endian_big16(&dataBuf[8]);
    hdr.init             = endian_big16(&dataBuf[10]);
    hdr.play             = endian_big16(&dataBuf[12]);
    hdr.songs            = endian_big16(&dataBuf[14]);
    hdr.start            = endian_big16(&dataBuf[16]);
    hdr.speed            = endian_big32(&dataBuf[18]);
    memcpy(hdr.name,     &dataBuf[22], PSID_MAXSTRLEN);
    memcpy(hdr.author,   &dataBuf[54], PSID_MAXSTRLEN);
    memcpy(hdr.released, &dataBuf[86], PSID_MAXSTRLEN);

    if (hdr.version >= 2)
    {
        if (dataBuf.size() < (psidv2_headerSize + 2))
        {
            throw loadError(ERR_TRUNCATED);
        }

        // Read v2/3/4 fields
        hdr.flags            = endian_big16(&dataBuf[118]);
        hdr.relocStartPage   = dataBuf[120];
        hdr.relocPages       = dataBuf[121];
        hdr.sidChipBase2     = dataBuf[122];
        hdr.sidChipBase3     = dataBuf[123];
    }
}

void PSID::tryLoad(const psidHeader &pHeader)
{
    SidTuneInfo::compatibility_t compatibility = SidTuneInfo::COMPATIBILITY_C64;

    // Require a valid ID and version number.
    if (pHeader.id == PSID_ID)
    {
       switch (pHeader.version)
       {
       case 1:
           compatibility = SidTuneInfo::COMPATIBILITY_PSID;
           break;
       case 2:
       case 3:
       case 4:
           break;
       default:
           throw loadError(TXT_UNKNOWN_PSID);
       }
       info->m_formatString = TXT_FORMAT_PSID;
    }
    else if (pHeader.id == RSID_ID)
    {
       switch (pHeader.version)
       {
       case 2:
       case 3:
       case 4:
           break;
       default:
           throw loadError(TXT_UNKNOWN_RSID);
       }
       info->m_formatString = TXT_FORMAT_RSID;
       compatibility = SidTuneInfo::COMPATIBILITY_R64;
    }

    fileOffset             = pHeader.data;
    info->m_loadAddr       = pHeader.load;
    info->m_initAddr       = pHeader.init;
    info->m_playAddr       = pHeader.play;
    info->m_songs          = pHeader.songs;
    info->m_startSong      = pHeader.start;
    info->m_compatibility  = compatibility;
    info->m_relocPages     = 0;
    info->m_relocStartPage = 0;

    uint_least32_t speed = pHeader.speed;
    SidTuneInfo::clock_t clock = SidTuneInfo::CLOCK_UNKNOWN;

    bool musPlayer = false;

    if (pHeader.version >= 2)
    {
        const uint_least16_t flags = pHeader.flags;

        // Check clock
        if (flags & PSID_MUS)
        {   // MUS tunes run at any speed
            clock = SidTuneInfo::CLOCK_ANY;
            musPlayer = true;
        }
        else
        {
            switch (flags & PSID_CLOCK)
            {
            case PSID_CLOCK_ANY:
                clock = SidTuneInfo::CLOCK_ANY;
                break;
            case PSID_CLOCK_PAL:
                clock = SidTuneInfo::CLOCK_PAL;
                break;
            case PSID_CLOCK_NTSC:
                clock = SidTuneInfo::CLOCK_NTSC;
                break;
            default:
                break;
            }
        }

        // These flags are only available for the appropriate
        // file formats
        switch (compatibility)
        {
        case SidTuneInfo::COMPATIBILITY_C64:
            if (flags & PSID_SPECIFIC)
                info->m_compatibility = SidTuneInfo::COMPATIBILITY_PSID;
            break;
        case SidTuneInfo::COMPATIBILITY_R64:
            if (flags & PSID_BASIC)
                info->m_compatibility = SidTuneInfo::COMPATIBILITY_BASIC;
            break;
        default:
            break;
        }

        info->m_clockSpeed = clock;

        info->m_sidModels[0] = getSidModel(flags >> 4);

        info->m_relocStartPage = pHeader.relocStartPage;
        info->m_relocPages     = pHeader.relocPages;

        if (pHeader.version >= 3)
        {
            if (validateAddress(pHeader.sidChipBase2))
            {
                info->m_sidChipAddresses.push_back(0xd000 | (pHeader.sidChipBase2 << 4));

                info->m_sidModels.push_back(getSidModel(flags >> 6));
            }

            if (pHeader.version >= 4)
            {
                if (pHeader.sidChipBase3 != pHeader.sidChipBase2
                    && validateAddress(pHeader.sidChipBase3))
                {
                    info->m_sidChipAddresses.push_back(0xd000 | (pHeader.sidChipBase3 << 4));

                    info->m_sidModels.push_back(getSidModel(flags >> 8));
                }
            }
        }
    }

    // Check reserved fields to force real c64 compliance
    // as required by the RSID specification
    if (compatibility == SidTuneInfo::COMPATIBILITY_R64)
    {
        if ((info->m_loadAddr != 0)
            || (info->m_playAddr != 0)
            || (speed != 0))
        {
            throw loadError(ERR_INVALID);
        }

        // Real C64 tunes appear as CIA
        speed = ~0;
    }

    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(speed, clock);

    // Copy info strings.
    info->m_infoString.push_back(std::string(pHeader.name, PSID_MAXSTRLEN));
    info->m_infoString.push_back(std::string(pHeader.author, PSID_MAXSTRLEN));
    info->m_infoString.push_back(std::string(pHeader.released, PSID_MAXSTRLEN));

    if (musPlayer)
        throw loadError("Compute!'s Sidplayer MUS data is not supported yet"); // TODO
}

const char *PSID::createMD5(char *md5)
{
    if (md5 == nullptr)
        md5 = m_md5;

    *md5 = '\0';

    try
    {
        // Include C64 data.
        sidmd5 myMD5;
        myMD5.append(&cache[fileOffset], info->m_c64dataLen);

        uint8_t tmp[2];
        // Include INIT and PLAY address.
        endian_little16(tmp, info->m_initAddr);
        myMD5.append(tmp, sizeof(tmp));
        endian_little16(tmp, info->m_playAddr);
        myMD5.append(tmp, sizeof(tmp));

        // Include number of songs.
        endian_little16(tmp, info->m_songs);
        myMD5.append(tmp, sizeof(tmp));

        {
            // Include song speed for each song.
            const unsigned int currentSong = info->m_currentSong;
            for (unsigned int s = 1; s <= info->m_songs; s++)
            {
                selectSong(s);
                const uint8_t songSpeed = static_cast<uint8_t>(info->m_songSpeed);
                myMD5.append(&songSpeed, sizeof(songSpeed));
            }
            // Restore old song
            selectSong(currentSong);
        }

        // Deal with PSID v2NG clock speed flags: Let only NTSC
        // clock speed change the MD5 fingerprint. That way the
        // fingerprint of a PAL-speed sidtune in PSID v1, v2, and
        // PSID v2NG format is the same.
        if (info->m_clockSpeed == SidTuneInfo::CLOCK_NTSC)
        {
            const uint8_t ntsc_val = 2;
            myMD5.append(&ntsc_val, sizeof(ntsc_val));
        }

        // NB! If the fingerprint is used as an index into a
        // song-lengths database or cache, modify above code to
        // allow for PSID v2NG files which have clock speed set to
        // SIDTUNE_CLOCK_ANY. If the SID player program fully
        // supports the SIDTUNE_CLOCK_ANY setting, a sidtune could
        // either create two different fingerprints depending on
        // the clock speed chosen by the player, or there could be
        // two different values stored in the database/cache.

        myMD5.finish();

        // Get fingerprint.
        myMD5.getDigest().copy(md5, SidTune::MD5_LENGTH);
        md5[SidTune::MD5_LENGTH] = '\0';
    }
    catch (md5Error const &)
    {
        return nullptr;
    }

    return md5;
}

}
