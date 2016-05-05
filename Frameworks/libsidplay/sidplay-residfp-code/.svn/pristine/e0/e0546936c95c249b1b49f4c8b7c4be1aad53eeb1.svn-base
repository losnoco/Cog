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

#ifndef SIDTUNEINFO_H
#define SIDTUNEINFO_H

#include <stdint.h>

#include "sidplayfp/siddefs.h"

/**
 * This interface is used to get values from SidTune objects.
 *
 * You must read (i.e. activate) sub-song specific information
 * via:
 *        const SidTuneInfo* tuneInfo = SidTune.getInfo();
 *        const SidTuneInfo* tuneInfo = SidTune.getInfo(songNumber);
 */
class SID_EXTERN SidTuneInfo
{
public:
    typedef enum {
        CLOCK_UNKNOWN,
        CLOCK_PAL,
        CLOCK_NTSC,
        CLOCK_ANY
    } clock_t;

    typedef enum {
        SIDMODEL_UNKNOWN,
        SIDMODEL_6581,
        SIDMODEL_8580,
        SIDMODEL_ANY
    } model_t;

    typedef enum {
        COMPATIBILITY_C64,   ///< File is C64 compatible
        COMPATIBILITY_PSID,  ///< File is PSID specific
        COMPATIBILITY_R64,   ///< File is Real C64 only
        COMPATIBILITY_BASIC  ///< File requires C64 Basic
    } compatibility_t;

public:
    /// Vertical-Blanking-Interrupt
    static const int SPEED_VBI = 0;

    /// CIA 1 Timer A
    static const int SPEED_CIA_1A = 60;

public:
    /**
     * Load Address.
     */
    uint_least16_t loadAddr() const;

    /**
     * Init Address.
     */
    uint_least16_t initAddr() const;

    /**
     * Play Address.
     */
    uint_least16_t playAddr() const;

    /**
     * The number of songs.
     */
    unsigned int songs() const;

    /**
     * The default starting song.
     */
    unsigned int startSong() const;

    /**
     * The tune that has been initialized.
     */
    unsigned int currentSong() const;

    /**
     * @name Base addresses
     * The SID chip base address(es) used by the sidtune.
     * - 0xD400 for the 1st SID
     * - 0 if the nth SID is not required
     */
    //@{
    SID_DEPRECATED uint_least16_t sidChipBase1() const;
    SID_DEPRECATED uint_least16_t sidChipBase2() const;
    uint_least16_t sidChipBase(unsigned int i) const;
    //@}

    /**
     * \deprecated
     * Whether sidtune uses two SID chips.
     */
    SID_DEPRECATED bool isStereo() const;

    /**
     * The number of SID chips required by the tune.
     */
    int sidChips() const;

    /**
     * Intended speed.
     */
    int songSpeed() const;

    /**
     * First available page for relocation.
     */
    uint_least8_t relocStartPage() const;

    /**
     * Number of pages available for relocation.
     */
    uint_least8_t relocPages() const;

    /**
     * @name SID model
     * The SID chip model(s) requested by the sidtune.
     */
    //@{
    SID_DEPRECATED model_t sidModel1() const;
    SID_DEPRECATED model_t sidModel2() const;
    model_t sidModel(unsigned int i) const;
    //@}

    /**
     * Compatibility requirements.
     */
    compatibility_t compatibility() const;

    /**
     * @name Tune infos
     * Song title, credits, ...
     * - 0 = Title
     * - 1 = Author
     * - 2 = Released
     */
    //@{
    unsigned int numberOfInfoStrings() const;     ///< The number of available text info lines
    const char* infoString(unsigned int i) const; ///< Text info from the format headers etc.
    //@}

    /**
     * @name Tune comments
     * MUS comments.
     */
    //@{
    unsigned int numberOfCommentStrings() const;     ///< Number of comments
    const char* commentString(unsigned int i) const; ///< Used to stash the MUS comment somewhere
    //@}

    /**
     * Length of single-file sidtune file.
     */
    uint_least32_t dataFileLen() const;

    /**
     * Length of raw C64 data without load address.
     */
    uint_least32_t c64dataLen() const;

    /**
     * The tune clock speed.
     */
    clock_t clockSpeed() const;

    /**
     * The name of the identified file format.
     */
    const char* formatString() const;

    /**
     * Whether load address might be duplicate.
     */
    bool fixLoad() const;

    /**
     * Path to sidtune files.
     */
    const char* path() const;

    /**
     * A first file: e.g. "foo.sid" or "foo.mus".
     */
    const char* dataFileName() const;

    /**
     * A second file: e.g. "foo.str".
     * Returns 0 if none.
     */
    const char* infoFileName() const;

private:
    virtual uint_least16_t getLoadAddr() const =0;

    virtual uint_least16_t getInitAddr() const =0;

    virtual uint_least16_t getPlayAddr() const =0;

    virtual unsigned int getSongs() const =0;

    virtual unsigned int getStartSong() const =0;

    virtual unsigned int getCurrentSong() const =0;

    virtual uint_least16_t getSidChipBase(unsigned int i) const =0;

    virtual int getSidChips() const =0;

    virtual int getSongSpeed() const =0;

    virtual uint_least8_t getRelocStartPage() const =0;

    virtual uint_least8_t getRelocPages() const =0;

    virtual model_t getSidModel(unsigned int i) const =0;

    virtual compatibility_t getCompatibility() const =0;

    virtual unsigned int getNumberOfInfoStrings() const =0;
    virtual const char* getInfoString(unsigned int i) const =0;

    virtual unsigned int getNumberOfCommentStrings() const =0;
    virtual const char* getCommentString(unsigned int i) const =0;

    virtual uint_least32_t getDataFileLen() const =0;

    virtual uint_least32_t getC64dataLen() const =0;

    virtual clock_t getClockSpeed() const =0;

    virtual const char* getFormatString() const =0;

    virtual bool getFixLoad() const =0;

    virtual const char* getPath() const =0;

    virtual const char* getDataFileName() const =0;

    virtual const char* getInfoFileName() const =0;

protected:
    ~SidTuneInfo() {}
};

#endif  /* SIDTUNEINFO_H */
