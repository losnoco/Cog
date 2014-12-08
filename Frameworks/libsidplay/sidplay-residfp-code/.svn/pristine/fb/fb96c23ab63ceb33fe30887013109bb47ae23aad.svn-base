/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#include "SidTuneBase.h"

#include <cstring>
#include <climits>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <fstream>

#include "SmartPtr.h"
#include "SidTuneTools.h"
#include "SidTuneInfoImpl.h"
#include "sidendian.h"
#include "sidmemory.h"
#include "stringutils.h"

#include "sidcxx11.h"

#include "MUS.h"
#include "p00.h"
#include "prg.h"
#include "PSID.h"

namespace libsidplayfp
{

// Error and status message strings.
const char ERR_EMPTY[]               = "SIDTUNE ERROR: No data to load";
const char ERR_UNRECOGNIZED_FORMAT[] = "SIDTUNE ERROR: Could not determine file format";
const char ERR_NOT_ENOUGH_MEMORY[]   = "SIDTUNE ERROR: Not enough free memory";
const char ERR_CANT_LOAD_FILE[]      = "SIDTUNE ERROR: Could not load input file";
const char ERR_CANT_OPEN_FILE[]      = "SIDTUNE ERROR: Could not open file for binary input";
const char ERR_FILE_TOO_LONG[]       = "SIDTUNE ERROR: Input data too long";
const char ERR_DATA_TOO_LONG[]       = "SIDTUNE ERROR: Size of music data exceeds C64 memory";
const char ERR_BAD_ADDR[]            = "SIDTUNE ERROR: Bad address data";
const char ERR_BAD_RELOC[]           = "SIDTUNE ERROR: Bad reloc data";
const char ERR_CORRUPT[]             = "SIDTUNE ERROR: File is incomplete or corrupt";

const char SidTuneBase::ERR_TRUNCATED[] = "SIDTUNE ERROR: File is most likely truncated";
const char SidTuneBase::ERR_INVALID[]   = "SIDTUNE ERROR: File contains invalid data";

/**
 * Petscii to Ascii conversion table.
 *
 * CHR$ conversion table (0x01 = no output)
 */
static const char CHR_tab[256] =
{
  0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 0xd,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x20,0x21,0x01,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x24,0x5d,0x20,0x20,
  // alternative: CHR$(92=0x5c) => ISO Latin-1(0xa3)
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  // 0x80-0xFF
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23,
  0x2d,0x23,0x7c,0x2d,0x2d,0x2d,0x2d,0x7c,0x7c,0x5c,0x5c,0x2f,0x5c,0x5c,0x2f,0x2f,
  0x5c,0x23,0x5f,0x23,0x7c,0x2f,0x58,0x4f,0x23,0x7c,0x23,0x2b,0x7c,0x7c,0x26,0x5c,
  0x20,0x7c,0x23,0x2d,0x2d,0x7c,0x23,0x7c,0x23,0x2f,0x7c,0x7c,0x2f,0x5c,0x5c,0x2d,
  0x2f,0x2d,0x2d,0x7c,0x7c,0x7c,0x7c,0x2d,0x2d,0x2d,0x2f,0x5c,0x5c,0x2f,0x2f,0x23
};

/// C64KB + LOAD + PSID
const uint_least32_t MAX_FILELEN = 65536 + 2 + 0x7C;

const uint_least32_t MAX_MEMORY = 65536;

SidTuneBase* SidTuneBase::load(const char* fileName, const char **fileNameExt,
                 bool separatorIsSlash)
{
    if (!fileName)
        return nullptr;

#if !defined(SIDTUNE_NO_STDIN_LOADER)
    // Filename "-" is used as a synonym for standard input.
    if (strcmp(fileName, "-") == 0)
        return getFromStdIn();
#endif
    return getFromFiles(fileName, fileNameExt, separatorIsSlash);
}

SidTuneBase* SidTuneBase::read(const uint_least8_t* sourceBuffer, uint_least32_t bufferLen)
{
    return getFromBuffer(sourceBuffer, bufferLen);
}

const SidTuneInfo* SidTuneBase::getInfo() const
{
    return info.get();
}

const SidTuneInfo* SidTuneBase::getInfo(unsigned int songNum)
{
    selectSong(songNum);
    return info.get();
}

unsigned int SidTuneBase::selectSong(unsigned int selectedSong)
{
    // First, check whether selected song is valid.
    if (selectedSong > info->m_songs || selectedSong > MAX_SONGS)
    {
        return info->m_currentSong;
    }

    // Determine and set starting song number.
    const unsigned int song = (selectedSong == 0) ? info->m_startSong : selectedSong;

    // Copy any song-specific variable information
    // such a speed/clock setting to the info structure.
    info->m_currentSong = song;

    // Retrieve song speed definition.
    switch (info->m_compatibility)
    {
    case SidTuneInfo::COMPATIBILITY_R64:
        info->m_songSpeed = SidTuneInfo::SPEED_CIA_1A;
        break;
    case SidTuneInfo::COMPATIBILITY_PSID:
        // This does not take into account the PlaySID bug upon evaluating the
        // SPEED field. It would most likely break compatibility to lots of
        // sidtunes, which have been converted from .SID format and vice versa.
        // The .SID format does the bit-wise/song-wise evaluation of the SPEED
        // value correctly, like it is described in the PlaySID documentation.
        info->m_songSpeed = songSpeed[(song-1)&31];
        break;
    default:
        info->m_songSpeed = songSpeed[song-1];
        break;
    }

    info->m_clockSpeed = clockSpeed[song-1];

    return info->m_currentSong;
}

// ------------------------------------------------- private member functions

bool SidTuneBase::placeSidTuneInC64mem(sidmemory* mem)
{
    if (mem != nullptr)
    {
        // The Basic ROM sets these values on loading a file.
        // Program end address
        const uint_least16_t start = info->m_loadAddr;
        const uint_least16_t end   = start + info->m_c64dataLen;
        mem->writeMemWord(0x2d, end); // Variables start
        mem->writeMemWord(0x2f, end); // Arrays start
        mem->writeMemWord(0x31, end); // Strings start
        mem->writeMemWord(0xac, start);
        mem->writeMemWord(0xae, end);

        // Copy data from cache to the correct destination.
        mem->fillRam(info->m_loadAddr, &cache[fileOffset], info->m_c64dataLen);

        return true;
    }
    return false;
}

void SidTuneBase::loadFile(const char* fileName, buffer_t& bufferRef)
{
    std::ifstream inFile(fileName, std::ifstream::binary);

    if (!inFile.is_open())
    {
        throw loadError(ERR_CANT_OPEN_FILE);
    }

    inFile.seekg(0, inFile.end);
    const int fileLen = inFile.tellg();

    if (fileLen <= 0)
    {
        throw loadError(ERR_EMPTY);
    }

    inFile.seekg(0, inFile.beg);

    buffer_t fileBuf;
    fileBuf.reserve(fileLen);

    try
    {
        fileBuf.assign(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>());
    }
    catch (std::exception &ex)
    {
        throw loadError(ex.what());
    }

    if (inFile.bad())
    {
        throw loadError(ERR_CANT_LOAD_FILE);
    }

    inFile.close();

    bufferRef.swap(fileBuf);
}

SidTuneBase::SidTuneBase() :
    info(new SidTuneInfoImpl()),
    fileOffset(0)
{
    // Initialize the object with some safe defaults.
    for (unsigned int si = 0; si < MAX_SONGS; si++)
    {
        songSpeed[si] = info->m_songSpeed;
        clockSpeed[si] = info->m_clockSpeed;
    }
}

#if !defined(SIDTUNE_NO_STDIN_LOADER)

SidTuneBase* SidTuneBase::getFromStdIn()
{
    buffer_t fileBuf;

    // We only read as much as fits in the buffer.
    // This way we avoid choking on huge data.
    char datb;
    while (std::cin.get(datb) && fileBuf.size() < MAX_FILELEN)
    {
        fileBuf.push_back((uint_least8_t)datb);
    }

    return getFromBuffer(&fileBuf.front(), fileBuf.size());
}

#endif

SidTuneBase* SidTuneBase::getFromBuffer(const uint_least8_t* const buffer, uint_least32_t bufferLen)
{
    if (buffer == nullptr || bufferLen == 0)
    {
        throw loadError(ERR_EMPTY);
    }

    if (bufferLen > MAX_FILELEN)
    {
        throw loadError(ERR_FILE_TOO_LONG);
    }

    buffer_t buf1(buffer, buffer+bufferLen);

    // Here test for the possible single file formats.
    std::unique_ptr<SidTuneBase> s(PSID::load(buf1));
    if (s.get() == nullptr)
    {
        buffer_t buf2;  // empty
        s.reset(MUS::load(buf1, buf2, 0, true));
    }

    if (s.get() != nullptr)
    {
        s->acceptSidTune("-", "-", buf1, false);
        return s.release();

    }

    throw loadError(ERR_UNRECOGNIZED_FORMAT);
}

void SidTuneBase::acceptSidTune(const char* dataFileName, const char* infoFileName,
                            buffer_t& buf, bool isSlashedFileName)
{
    // Make a copy of the data file name and path, if available.
    if (dataFileName != nullptr)
    {
        const size_t fileNamePos = isSlashedFileName ?
            SidTuneTools::slashedFileNameWithoutPath(dataFileName) :
            SidTuneTools::fileNameWithoutPath(dataFileName);
        info->m_path = std::string(dataFileName, fileNamePos);
        info->m_dataFileName = std::string(dataFileName + fileNamePos);
    }

    // Make a copy of the info file name, if available.
    if (infoFileName != nullptr)
    {
        const size_t fileNamePos = isSlashedFileName ?
            SidTuneTools::slashedFileNameWithoutPath(infoFileName) :
            SidTuneTools::fileNameWithoutPath(infoFileName);
        info->m_infoFileName = std::string(infoFileName + fileNamePos);
    }

    // Fix bad sidtune set up.
    if (info->m_songs > MAX_SONGS)
    {
        info->m_songs = MAX_SONGS;
    }
    else if (info->m_songs == 0)
    {
        info->m_songs++;
    }

    if (info->m_startSong > info->m_songs)
    {
        info->m_startSong = 1;
    }
    else if (info->m_startSong == 0)
    {
        info->m_startSong++;
    }

    info->m_dataFileLen = buf.size();
    info->m_c64dataLen = buf.size() - fileOffset;

    // Calculate any remaining addresses and then
    // confirm all the file details are correct
    resolveAddrs(&buf[fileOffset]);

    if (checkRelocInfo() == false)
    {
        throw loadError(ERR_BAD_RELOC);
    }
    if (checkCompatibility() == false)
    {
         throw loadError(ERR_BAD_ADDR);
    }

    if (info->m_dataFileLen >= 2)
    {
        // We only detect an offset of two. Some position independent
        // sidtunes contain a load address of 0xE000, but are loaded
        // to 0x0FFE and call player at 0x1000.
        info->m_fixLoad = (endian_little16(&buf[fileOffset])==(info->m_loadAddr+2));
    }

    // Check the size of the data.
    if (info->m_c64dataLen > MAX_MEMORY)
    {
        throw loadError(ERR_DATA_TOO_LONG);
    }
    else if (info->m_c64dataLen == 0)
    {
        throw loadError(ERR_EMPTY);
    }

    cache.swap(buf);
}

void SidTuneBase::createNewFileName(std::string& destString,
                                const char* sourceName,
                                const char* sourceExt)
{
    destString.assign(sourceName);
    destString.erase(destString.find_last_of('.'));
    destString.append(sourceExt);
}

// Initializing the object based upon what we find in the specified file.

SidTuneBase* SidTuneBase::getFromFiles(const char* fileName, const char **fileNameExtensions, bool separatorIsSlash)
{
    buffer_t fileBuf1;

    loadFile(fileName, fileBuf1);

    // File loaded. Now check if it is in a valid single-file-format.
    std::unique_ptr<SidTuneBase> s(PSID::load(fileBuf1));
    if (s.get() == nullptr)
    {
        buffer_t fileBuf2;

        // Try some native C64 file formats
        s.reset(MUS::load(fileBuf1, fileBuf2, 0, true));
        if (s.get() != nullptr)
        {
            // Try to find second file.
            std::string fileName2;
            int n = 0;
            while (fileNameExtensions[n] != nullptr)
            {
                createNewFileName(fileName2, fileName, fileNameExtensions[n]);
                // 1st data file was loaded into "fileBuf1",
                // so we load the 2nd one into "fileBuf2".
                // Do not load the first file again if names are equal.
                if (!stringutils::equal(fileName, fileName2.data(), fileName2.size()))
                {
                    try
                    {
                        loadFile(fileName2.c_str(), fileBuf2);
                        // Check if tunes in wrong order and therefore swap them here
                        if (stringutils::equal(fileNameExtensions[n], ".mus"))
                        {
                            std::unique_ptr<SidTuneBase> s2(MUS::load(fileBuf2, fileBuf1, 0, true));
                            if (s2.get())
                            {
                                s2->acceptSidTune(fileName2.c_str(), fileName, fileBuf2, separatorIsSlash);
                                return s2.release();
                            }
                        }
                        else
                        {
                            std::unique_ptr<SidTuneBase> s2(MUS::load(fileBuf1, fileBuf2, 0, true));
                            if (s2.get() != nullptr)
                            {
                                s2->acceptSidTune(fileName, fileName2.c_str(), fileBuf1, separatorIsSlash);
                                return s2.release();
                            }
                        }
                    // The first tune loaded ok, so ignore errors on the
                    // second tune, may find an ok one later
                    }
                    catch (loadError const &) {}
                }
                n++;
            }

            s->acceptSidTune(fileName, 0, fileBuf1, separatorIsSlash);
            return s.release();
        }
    }
    if (s.get() == nullptr) s.reset(p00::load(fileName, fileBuf1));
    if (s.get() == nullptr) s.reset(prg::load(fileName, fileBuf1));

    if (s.get() != nullptr)
    {
        s->acceptSidTune(fileName, 0, fileBuf1, separatorIsSlash);
        return s.release();
    }

    throw loadError(ERR_UNRECOGNIZED_FORMAT);
}

void SidTuneBase::convertOldStyleSpeedToTables(uint_least32_t speed, SidTuneInfo::clock_t clock)
{
    // Create the speed/clock setting tables.
    //
    // This routine implements the PSIDv2NG compliant speed conversion.  All tunes
    // above 32 use the same song speed as tune 32
    const unsigned int toDo = std::min(info->m_songs, MAX_SONGS);
    for (unsigned int s = 0; s < toDo; s++)
    {
        clockSpeed[s] = clock;
        songSpeed[s] = (speed & 1) ? SidTuneInfo::SPEED_CIA_1A : SidTuneInfo::SPEED_VBI;

        if (s < 31)
        {
            speed >>= 1;
        }
    }
}

bool SidTuneBase::checkRelocInfo()
{
    // Fix relocation information
    if (info->m_relocStartPage == 0xFF)
    {
        info->m_relocPages = 0;
        return true;
    }
    else if (info->m_relocPages == 0)
    {
        info->m_relocStartPage = 0;
        return true;
    }

    // Calculate start/end page
    const uint_least8_t startp = info->m_relocStartPage;
    const uint_least8_t endp   = (startp + info->m_relocPages - 1) & 0xff;
    if (endp < startp)
    {
        return false;
    }

    {    // Check against load range
        const uint_least8_t startlp = (uint_least8_t) (info->m_loadAddr >> 8);
        const uint_least8_t endlp   = startlp + (uint_least8_t) ((info->m_c64dataLen - 1) >> 8);

        if (((startp <= startlp) && (endp >= startlp))
            || ((startp <= endlp)   && (endp >= endlp)))
        {
            return false;
        }
    }

    // Check that the relocation information does not use the following
    // memory areas: 0x0000-0x03FF, 0xA000-0xBFFF and 0xD000-0xFFFF
    if ((startp < 0x04)
        || ((0xa0 <= startp) && (startp <= 0xbf))
        || (startp >= 0xd0)
        || ((0xa0 <= endp) && (endp <= 0xbf))
        || (endp >= 0xd0))
    {
        return false;
    }

    return true;
}

void SidTuneBase::resolveAddrs(const uint_least8_t *c64data)
{
    // Originally used as a first attempt at an RSID
    // style format. Now reserved for future use
    if (info->m_playAddr == 0xffff)
    {
        info->m_playAddr = 0;
    }

    // loadAddr = 0 means, the address is stored in front of the C64 data.
    if (info->m_loadAddr == 0)
    {
        if (info->m_c64dataLen < 2)
        {
            throw loadError(ERR_CORRUPT);
        }

        info->m_loadAddr = endian_16(*(c64data+1), *c64data);
        fileOffset += 2;
        info->m_c64dataLen -= 2;
    }

    if (info->m_compatibility == SidTuneInfo::COMPATIBILITY_BASIC)
    {
        if (info->m_initAddr != 0)
        {
            throw loadError(ERR_BAD_ADDR);
        }
    }
    else if (info->m_initAddr == 0)
    {
        info->m_initAddr = info->m_loadAddr;
    }
}

bool SidTuneBase::checkCompatibility()
{
    if  (info->m_compatibility == SidTuneInfo::COMPATIBILITY_R64)
    {
        // Check valid init address
        switch (info->m_initAddr >> 12)
        {
        case 0x0F:
        case 0x0E:
        case 0x0D:
        case 0x0B:
        case 0x0A:
            return false;
        default:
            if ((info->m_initAddr < info->m_loadAddr)
                || (info->m_initAddr > (info->m_loadAddr + info->m_c64dataLen - 1)))
            {
                return false;
            }
        }

        // Check tune is loadable on a real C64
        if (info->m_loadAddr < SIDTUNE_R64_MIN_LOAD_ADDR)
        {
            return false;
        }
    }

    return true;
}

const char* SidTuneBase::PetsciiToAscii::convert(SmartPtr_sidtt<const uint8_t>& spPet)
{
    char c;
    do
    {
        c = CHR_tab[*spPet];  // ASCII CHR$ conversion
        if ((c >= 0x20) && (buffer.length() <= 31))
            buffer.push_back(c);  // copy to info string

        // If character is 0x9d (left arrow key) then move back.
        if ((*spPet == 0x9d) && (!buffer.empty()))
        {
            buffer.resize(buffer.size() - 1);
        }

        spPet++;
    }
    while (!((c == 0x0D) || (c == 0x00) || spPet.fail()));

    return buffer.c_str();
}

}
