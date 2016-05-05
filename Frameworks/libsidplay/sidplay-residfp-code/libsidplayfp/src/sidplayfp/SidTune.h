/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef SIDTUNE_H
#define SIDTUNE_H

#include <stdint.h>
#include <memory>
#include <vector>

#include "sidplayfp/siddefs.h"

class SidTuneInfo;

namespace libsidplayfp
{
class SidTuneBase;
class sidmemory;
}

/**
 * SidTune
 */
class SID_EXTERN SidTune
{
public:
    static const int MD5_LENGTH = 32;

private:
    /// Filename extensions to append for various file types.
    static const char** fileNameExtensions;

private:  // -------------------------------------------------------------
    std::auto_ptr<libsidplayfp::SidTuneBase> tune;

    const char* m_statusString;

    bool m_status;

public:  // ----------------------------------------------------------------

    /**
     * Load a sidtune from a file.
     *
     * To retrieve data from standard input pass in filename "-".
     * If you want to override the default filename extensions use this
     * contructor. Please note, that if the specified "fileName"
     * does exist and the loader is able to determine its file format,
     * this function does not try to append any file name extension.
     * See "SidTune.cpp" for the default list of file name extensions.
     * You can specify "fileName = 0", if you do not want to
     * load a sidtune. You can later load one with open().
     *
     * @param fileName
     * @param fileNameExt
     * @param separatorIsSlash
     */
    typedef void (*SidTuneLoaderFunc)(const char* fileName,std::vector<uint_least8_t>& bufferRef);
    SidTune(const char* fileName, const char **fileNameExt = 0,
            bool separatorIsSlash = false, SidTuneLoaderFunc loaderFunc = 0);

    /**
     * Load a single-file sidtune from a memory buffer.
     * Currently supported: PSID and MUS formats.
     *
     * @param oneFileFormatSidtune the buffer that contains song data
     * @param sidtuneLength length of the buffer
     */
    SidTune(const uint_least8_t* oneFileFormatSidtune, uint_least32_t sidtuneLength);

    ~SidTune();

    /**
     * The SidTune class does not copy the list of file name extensions,
     * so make sure you keep it. If the provided pointer is 0, the
     * default list will be activated. This is a static list which
     * is used by all SidTune objects.
     *
     * @param fileNameExt
     */
    void setFileNameExtensions(const char **fileNameExt);

    /**
     * Load a sidtune into an existing object from a file.
     *
     * @param fileName
     * @param separatorIsSlash
     */
    void load(const char* fileName, bool separatorIsSlash = false, SidTuneLoaderFunc loaderFunc = 0);

    /**
     * Load a sidtune into an existing object from a buffer.
     *
     * @param sourceBuffer the buffer that contains song data
     * @param bufferLen length of the buffer
     */
    void read(const uint_least8_t* sourceBuffer, uint_least32_t bufferLen);

    /**
     * Select sub-song.
     *
     * @param songNum the selected song (0 = default starting song)
     * @return active song number, 0 if no tune is loaded.
     */
    unsigned int selectSong(unsigned int songNum);

    /**
     * Retrieve current active sub-song specific information.
     *
     * @return a pointer to #SidTuneInfo, 0 if no tune is loaded. The pointer must not be deleted.
     */
    const SidTuneInfo* getInfo() const;

    /**
     * Select sub-song and retrieve information.
     *
     * @param songNum the selected song (0 = default starting song)
     * @return a pointer to #SidTuneInfo, 0 if no tune is loaded. The pointer must not be deleted.
     */
    const SidTuneInfo* getInfo(unsigned int songNum);

    /**
     * Determine current state of object.
     * Upon error condition use #statusString to get a descriptive
     * text string.
     *
     * @return current state (true = okay, false = error)
     */
    bool getStatus() const;

    /**
     * Error/status message of last operation.
     */
    const char* statusString() const;

    /**
     * Copy sidtune into C64 memory (64 KB).
     */
    bool placeSidTuneInC64mem(libsidplayfp::sidmemory& mem);

    /**
     * Calculates the MD5 hash of the tune.
     * Not providing an md5 buffer will cause the internal one to be used.
     * If provided, buffer must be MD5_LENGTH + 1
     *
     * @return a pointer to the buffer containing the md5 string, 0 if no tune is loaded.
     */
    const char *createMD5(char *md5 = 0);

    const uint_least8_t* c64Data() const;

private:    // prevent copying
    SidTune(const SidTune&);
    SidTune& operator=(SidTune&);
};

#endif  /* SIDTUNE_H */
