/*
 * This file is part of sidplayfp, a console SID player.
 *
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

#ifndef AUDIO_OSS_H
#define AUDIO_OSS_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if defined(HAVE_SYS_SOUNDCARD_H) \
    || defined(HAVE_LINUX_SOUNDCARD_H) \
    || defined(HAVE_MACHINE_SOUNDCARD_H) \
    || defined(HAVE_SOUNDCARD_H) \

#define HAVE_OSS

#ifndef AudioDriver
#  define AudioDriver Audio_OSS
#endif

#if defined(HAVE_SYS_SOUNDCARD_H)
#  include <sys/soundcard.h>
#elif defined(HAVE_LINUX_SOUNDCARD_H)
#  include <linux/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
#  include <machine/soundcard.h>
#elif defined(HAVE_SOUNDCARD_H)
#  include <soundcard.h>
#else
#  error Audio driver not supported.
#endif

#include "../AudioBase.h"

/*
 * Open Sound System (OSS) specific audio driver interface.
 */
class Audio_OSS: public AudioBase
{
private:  // ------------------------------------------------------- private
    static   const char AUDIODEVICE[];
    volatile int   _audiofd;

    void outOfOrder ();

public:  // --------------------------------------------------------- public
    Audio_OSS();
    ~Audio_OSS();

    bool open  (AudioConfig &cfg) override;
    void close () override;
    void reset () override;
    bool write () override;
    void pause () override {}
};

#endif // HAVE_*_SOUNDCARD_H
#endif // AUDIO_OSS_H
