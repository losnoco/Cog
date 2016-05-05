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

#ifndef AUDIO_DIRECTX_H
#define AUDIO_DIRECTX_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_DIRECTX_H

#define HAVE_DIRECTX

#ifndef AudioDriver
#  define AudioDriver Audio_DirectX
#endif

#if DIRECTSOUND_VERSION < 0x0500
#   undef  DIRECTSOUND_VERSION
#   define DIRECTSOUND_VERSION 0x0500       /* version 5.0 */
#endif

#include <dsound.h>
#include <mmsystem.h>

#include "../AudioBase.h"
#define AUDIO_DIRECTX_BUFFERS 2

class Audio_DirectX: public AudioBase
{
private:  // ------------------------------------------------------- private
    HWND   hwnd;

    // DirectSound Support
    LPDIRECTSOUND       lpds;
    LPDIRECTSOUNDBUFFER lpDsb;
    LPDIRECTSOUNDNOTIFY lpdsNotify;
    void               *lpvData;
    // DirectSound Notify
    HANDLE rghEvent[AUDIO_DIRECTX_BUFFERS];
    DWORD  bufSize;

    bool isOpen;
    bool isPlaying;

private:
    HWND GetConsoleHwnd ();

public:  // --------------------------------------------------------- public
    Audio_DirectX();
    ~Audio_DirectX();

    // This first one assumes progrm is built as a
    // console application
    bool open  (AudioConfig &cfg) override;
    bool open  (AudioConfig &cfg, HWND hwnd);
    void close () override;
    void reset () override;
    bool write () override;
    void pause () override;
};

#endif // HAVE_DIRECTX_H
#endif // AUDIO_DIRECTX_H
