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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "AudioDrv.h"

// Unix Sound Drivers
#include "pulse/audiodrv.h"
#include "alsa/audiodrv.h"
#include "oss/audiodrv.h"

// Windows Sound Drivers
#include "directx/audiodrv.h"
#include "mmsystem/audiodrv.h"

// Make sure that a sound driver was used
#ifndef AudioDriver
#  warning Audio hardware not recognised, please check configuration files.
#endif

bool audioDrv::open(AudioConfig &cfg)
{
    bool res = false;
#ifdef HAVE_PULSE
    if(!res)
    {
        audio.reset(new Audio_Pulse());
        res = audio->open(cfg);
    }
#endif
#ifdef HAVE_ALSA
    if(!res)
    {
        audio.reset(new Audio_ALSA());
        res = audio->open(cfg);
    }
#endif
#ifdef HAVE_OSS
    if(!res)
    {
        audio.reset(new Audio_OSS());
        res = audio->open(cfg);
    }
#endif
#ifdef HAVE_DIRECTX
    if(!res)
    {
        audio.reset(new Audio_DirectX());
        res = audio->open(cfg);
    }
#endif
#ifdef HAVE_MMSYSTEM
    if(!res)
    {
        audio.reset(new Audio_MMSystem());
        res = audio->open(cfg);
    }
#endif
    return res;
}
