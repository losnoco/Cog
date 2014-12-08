/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2000-2002 Simon White
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

#include "null.h"

Audio_Null::Audio_Null() :
    AudioBase("NULL"),
    isOpen(false) {}

Audio_Null::~Audio_Null()
{
    close();
}

bool Audio_Null::open(AudioConfig &cfg)
{
    if (isOpen)
    {
        setError("Audio device already open.");
        return false;
    }

    isOpen    = true;
    _settings = cfg;
    return true;
}

bool Audio_Null::write()
{
    if (!isOpen)
    {
        setError("Audio device not open.");
        return false;
    }
    return true;
}

void Audio_Null::close(void)
{
    if (!isOpen)
        return;

    isOpen = false;
}
