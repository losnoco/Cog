/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2015 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#ifndef SIDDATABASE_H
#define SIDDATABASE_H

#include <stdint.h>
#include <memory>

#include "sidplayfp/siddefs.h"

class SidTune;

namespace libsidplayfp
{
class iniParser;
}

/**
 * SidDatabase
 * An utility class to deal with the songlength DataBase.
 */
class SID_EXTERN SidDatabase
{
private:
    std::auto_ptr<libsidplayfp::iniParser> m_parser;

    const char *errorString;

public:
    SidDatabase();
    ~SidDatabase();

    /**
     * Open the songlength DataBase.
     *
     * @param filename songlengthDB file name with full path.
     * @return false in case of errors, true otherwise.
     */
    bool open(const char *filename);

    /**
     * Close the songlength DataBase.
     */
    void close();

    /**
     * Get the length of the current subtune.
     *
     * @param tune
     * @return tune length in seconds, -1 in case of errors.
     */
    int_least32_t length(SidTune &tune);

    /**
     * Get the length of the selected subtune.
     *
     * @param md5 the md5 hash of the tune.
     * @param song the subtune.
     * @return tune length in seconds, -1 in case of errors.
     */
    int_least32_t length(const char *md5, unsigned int song);

    /**
     * Get descriptive error message.
     */
    const char *error() const { return errorString; }
};

#endif // SIDDATABASE_H
