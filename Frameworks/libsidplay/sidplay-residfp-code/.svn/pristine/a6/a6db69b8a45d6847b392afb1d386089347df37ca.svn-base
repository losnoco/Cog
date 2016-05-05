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

#include <cctype>
#include <cstdlib>

#include "SidDatabase.h"

#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"

#include "iniParser.h"

#include "sidcxx11.h"

const char ERR_DATABASE_CORRUPT[]        = "SID DATABASE ERROR: Database seems to be corrupt.";
const char ERR_NO_DATABASE_LOADED[]      = "SID DATABASE ERROR: Songlength database not loaded.";
const char ERR_NO_SELECTED_SONG[]        = "SID DATABASE ERROR: No song selected for retrieving song length.";
const char ERR_MEM_ALLOC[]               = "SID DATABASE ERROR: Memory Allocation Failure.";
const char ERR_UNABLE_TO_LOAD_DATABASE[] = "SID DATABASE ERROR: Unable to load the songlegnth database.";

class parseError {};

SidDatabase::SidDatabase() :
    m_parser(0),
    errorString(ERR_NO_DATABASE_LOADED)
{}

SidDatabase::~SidDatabase()
{
    // Needed to delete auto_ptr with complete type
}

const char *parseTime(const char *str, long &result)
{
    char *end;
    const long minutes = strtol(str, &end, 10);

    if (*end != ':')
    {
        throw parseError();
    }

    end++;
    const long seconds = strtol(end, &end, 10);
    result = (minutes * 60) + seconds;

    while (!isspace(*end))
    {
        end++;
    }

    return end;
}


bool SidDatabase::open(const char *filename)
{
    m_parser.reset(new libsidplayfp::iniParser());

    if (!m_parser->open(filename))
    {
        close();
        errorString = ERR_UNABLE_TO_LOAD_DATABASE;
        return false;
    }

    return true;
}

void SidDatabase::close()
{
    m_parser.reset(nullptr);
}

int_least32_t SidDatabase::length(SidTune &tune)
{
    const unsigned int song = tune.getInfo()->currentSong();

    if (!song)
    {
        errorString = ERR_NO_SELECTED_SONG;
        return -1;
    }

    char md5[SidTune::MD5_LENGTH + 1];
    tune.createMD5(md5);
    return length(md5, song);
}

int_least32_t SidDatabase::length(const char *md5, unsigned int song)
{
    if (m_parser.get() == nullptr)
    {
        errorString = ERR_NO_DATABASE_LOADED;
        return -1;
    }

    // Read Time (and check times before hand)
    if (!m_parser->setSection("Database"))
    {
        errorString = ERR_DATABASE_CORRUPT;
        return -1;
    }

    const char *timeStamp = m_parser->getValue(md5);

    // If return is null then no entry found in database
    if (!timeStamp)
    {
        errorString = ERR_DATABASE_CORRUPT;
        return -1;
    }

    const char *str = timeStamp;
    long  time = 0;

    for (unsigned int i = 0; i < song; i++)
    {
        // Validate Time
        try
        {
            str = parseTime(str, time);
        }
        catch (parseError const &)
        {
            errorString = ERR_DATABASE_CORRUPT;
            return -1;
        }
    }

    return time;
}
