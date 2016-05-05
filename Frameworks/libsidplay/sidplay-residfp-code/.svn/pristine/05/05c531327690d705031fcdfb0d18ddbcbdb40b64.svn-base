/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2000-2005 Simon White
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

#ifndef AUDIO_ALSA_H
#define AUDIO_ALSA_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_ALSA

#ifndef AudioDriver
#  define AudioDriver Audio_ALSA
#endif

#include <alsa/asoundlib.h>
#include "../AudioBase.h"


class Audio_ALSA: public AudioBase
{
private:  // ------------------------------------------------------- private
    snd_pcm_t *_audioHandle;
    int _alsa_to_frames_divisor;

private:
    void outOfOrder();
    static void checkResult(int err);

public:  // --------------------------------------------------------- public
    Audio_ALSA();
    ~Audio_ALSA();

    bool open  (AudioConfig &cfg) override;
    void close () override;
    void reset () override {}
    bool write () override;
    void pause () override {}
};

#endif // HAVE_ALSA
#endif // AUDIO_ALSA_H
