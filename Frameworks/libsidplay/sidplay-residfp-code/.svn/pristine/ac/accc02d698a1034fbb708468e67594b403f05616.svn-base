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

#include "audiodrv.h"

#ifdef HAVE_OSS

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <new>

#if defined(HAVE_NETBSD)
const char Audio_OSS::AUDIODEVICE[] = "/dev/audio";
#else
const char Audio_OSS::AUDIODEVICE[] = "/dev/dsp";
#endif

Audio_OSS::Audio_OSS() :
    AudioBase("OSS")
{
    // Reset everything.
    outOfOrder();
}

Audio_OSS::~Audio_OSS ()
{
    close ();
}

void Audio_OSS::outOfOrder ()
{
    // Reset everything.
    clearError();
    _audiofd = -1;
}

bool Audio_OSS::open (AudioConfig &cfg)
{
    if (_audiofd != -1)
    {
        setError("Device already in use");
        return false;
    }

    try
    {
        if (access (AUDIODEVICE, W_OK) == -1)
        {
            throw error("Could not locate an audio device.");
        }

        if ((_audiofd = ::open (AUDIODEVICE, O_WRONLY, 0)) == (-1))
        {
            throw error("Could not open audio device.");
        }

        int format = AFMT_S16_LE;
        if (ioctl (_audiofd, SNDCTL_DSP_SETFMT, &format) == (-1))
        {
            throw error("Could not set sample format.");
        }

        // Set mono/stereo.
        if (ioctl (_audiofd, SNDCTL_DSP_CHANNELS, &cfg.channels) == (-1))
        {
            throw error("Could not set mono/stereo.");
        }

        // Verify and accept the number of channels the driver accepted.
        switch (cfg.channels)
        {
        case 1:
        case 2:
        break;
        default:
            throw error("Could not set mono/stereo.");
        break;
        }

        // Set frequency.
        if (ioctl (_audiofd, SNDCTL_DSP_SPEED, &cfg.frequency) == (-1))
        {
            throw error("Could not set frequency.");
        }

        int temp = 0;
        ioctl (_audiofd, SNDCTL_DSP_GETBLKSIZE, &temp);
        cfg.bufSize = (uint_least32_t) temp;

        try
        {
            _sampleBuffer = new short[cfg.bufSize];
        }
        catch (std::bad_alloc const &ba)
        {
            throw error("Unable to allocate memory for sample buffers.");
        }

        // Setup internal Config
        _settings = cfg;
        return true;
    }
    catch(error const &e)
    {
        setError(e.message());

        if (_audiofd != -1)
        {
            close ();
            _audiofd = -1;
        }

        perror (AUDIODEVICE);
        return false;
    }
}

// Close an opened audio device, free any allocated buffers and
// reset any variables that reflect the current state.
void Audio_OSS::close ()
{
    if (_audiofd != -1)
    {
        ::close (_audiofd);
        delete [] _sampleBuffer;
        outOfOrder ();
    }
}

void Audio_OSS::reset ()
{
    if (_audiofd != -1)
    {
        ioctl (_audiofd, SNDCTL_DSP_RESET, 0);
    }
}

bool Audio_OSS::write ()
{
    if (_audiofd == -1)
    {
        setError("Device not open.");
        return false;
    }

    ::write (_audiofd, _sampleBuffer, 2 * _settings.bufSize);
    return true;
}

#endif // HAVE_OSS
