/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2013 Leandro Nini
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

#ifndef IAUDIO_H
#define IAUDIO_H

class AudioConfig;

class IAudio
{
public:
    virtual ~IAudio() {}

    virtual bool open(AudioConfig &cfg) = 0;
    virtual void reset() = 0;
    virtual bool write() = 0;
    virtual void close() = 0;
    virtual void pause() = 0;
    virtual short *buffer() const = 0;
    virtual void getConfig(AudioConfig &cfg) const = 0;
    virtual const char *getErrorString() const = 0;
};

#endif // IAUDIO_H
