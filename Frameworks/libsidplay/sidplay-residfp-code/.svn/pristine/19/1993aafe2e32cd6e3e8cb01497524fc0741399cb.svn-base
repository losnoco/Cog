/*
 * This file is part of sidplayfp, a console SID player.
 *
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

#ifndef AUDIODRV_H
#define AUDIODRV_H

#include "IAudio.h"

#include <memory>

#include "AudioBase.h"

class audioDrv : public IAudio
{
private:
    std::unique_ptr<AudioBase> audio;

public:
    virtual ~audioDrv() {}

    bool open(AudioConfig &cfg);
    void reset() { audio->reset(); }
    bool write() { return audio->write(); }
    void close() { audio->close(); }
    void pause() { audio->pause(); }
    short *buffer() const { return audio->buffer(); }
    void getConfig(AudioConfig &cfg) const { audio->getConfig(cfg); }
    const char *getErrorString() const { return audio->getErrorString(); }
};

#endif // AUDIODRV_H
