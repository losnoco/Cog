/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2013 Leandro Nini
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

#ifdef   HAVE_DIRECTX

#include <stdio.h>

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

Audio_DirectX::Audio_DirectX() :
    AudioBase("DIRECTX"),
    lpds(0),
    lpDsb(0),
    lpdsNotify(0),
    isOpen(false) {}

Audio_DirectX::~Audio_DirectX()
{
    close();
}

// Need this to setup DirectX
HWND Audio_DirectX::GetConsoleHwnd ()
{   // Taken from Microsoft Knowledge Base
    // Article ID: Q124103
    #define MY_bufSize 1024 // buffer size for console window totles

    TCHAR  pszNewWindowTitle[MY_bufSize]; // contains fabricated WindowTitle
    TCHAR  pszOldWindowTitle[MY_bufSize]; // contains original WindowTitle

    // fetch curent window title
    GetConsoleTitle (pszOldWindowTitle, MY_bufSize);

    // format a "unique" NewWindowTitle
    wsprintf (pszNewWindowTitle, TEXT("%d/%d"), GetTickCount (),
        GetCurrentProcessId ());

    // change the window title
    SetConsoleTitle (pszNewWindowTitle);

    // ensure window title has been updated
    Sleep (40);

    // look for NewWindowTitle
    HWND hwndFound = FindWindow (NULL, pszNewWindowTitle);

    // restore original window title
    SetConsoleTitle (pszOldWindowTitle);
    return hwndFound;
}

bool Audio_DirectX::open (AudioConfig &cfg)
{
    // Assume we have a console.  Use other other
    // if we have a non console Window
    HWND hwnd = GetConsoleHwnd ();
    return open (cfg, hwnd);
}

bool Audio_DirectX::open (AudioConfig &cfg, HWND hwnd)
{
    LPDIRECTSOUNDBUFFER lpDsbPrimary = 0;

    try
    {
        if (isOpen)
        {
            throw error("Audio device already open.");
        }

        lpvData = 0;
        isOpen  = true;

        for (int i = 0; i < AUDIO_DIRECTX_BUFFERS; i++)
            rghEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (FAILED (DirectSoundCreate (NULL, &lpds, NULL)))
        {
            throw error("Could not open audio device.");
        }
        if (FAILED (lpds->SetCooperativeLevel (hwnd, DSSCL_PRIORITY)))
        {
            throw error("Could not set cooperative level.");
        }

        // Primary Buffer Setup
        DSBUFFERDESC dsbdesc;
        memset (&dsbdesc, 0, sizeof(DSBUFFERDESC));
        dsbdesc.dwSize        = sizeof(DSBUFFERDESC);
        dsbdesc.dwFlags       = DSBCAPS_PRIMARYBUFFER;
        dsbdesc.dwBufferBytes = 0;
        dsbdesc.lpwfxFormat   = NULL;

        // Format
        WAVEFORMATEX wfm;
        memset (&wfm, 0, sizeof(WAVEFORMATEX));
        wfm.wFormatTag      = WAVE_FORMAT_PCM;
        wfm.nChannels       = cfg.channels;
        wfm.nSamplesPerSec  = cfg.frequency;
        wfm.wBitsPerSample  = 16;
        wfm.nBlockAlign     = wfm.wBitsPerSample / 8 * wfm.nChannels;
        wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;

        if (FAILED (lpds->CreateSoundBuffer(&dsbdesc, &lpDsbPrimary, NULL)))
        {
            throw error("Unable to create sound buffer.");
        }
        if (FAILED (lpDsbPrimary->SetFormat(&wfm)))
        {
            throw error("Unable to setup required sampling format.");
        }
        lpDsbPrimary->Release ();

        // Buffer size reduced to 2 blocks of 500ms
        bufSize = wfm.nSamplesPerSec / 2 * wfm.nBlockAlign;

        // Allocate secondary buffers
        memset (&dsbdesc, 0, sizeof(DSBUFFERDESC));
        dsbdesc.dwSize  = sizeof(DSBUFFERDESC);
        dsbdesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY |
            DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPAN;
        dsbdesc.dwBufferBytes = bufSize * AUDIO_DIRECTX_BUFFERS;
        dsbdesc.lpwfxFormat   = &wfm;

        if (FAILED (lpds->CreateSoundBuffer(&dsbdesc, &lpDsb, NULL)))
        {
            throw error("Could not create sound buffer.");
        }
        lpDsb->Stop();

        // Apparently this is used for timing ------------------------
        DSBPOSITIONNOTIFY rgdscbpn[AUDIO_DIRECTX_BUFFERS];
        // Buffer Start Notification
        // Rev 2.0.4 (saw) - On starting to play a buffer
        for (int i = 0; i < AUDIO_DIRECTX_BUFFERS; i++)
        {   // Track one buffer ahead
            rgdscbpn[i].dwOffset     = bufSize * ((i + 1) % AUDIO_DIRECTX_BUFFERS);
            rgdscbpn[i].hEventNotify = rghEvent[i];
        }

        if (FAILED (lpDsb->QueryInterface (IID_IDirectSoundNotify, (VOID **) &lpdsNotify)))
        {
            throw error("Sound interface query failed.");
        }
        if (FAILED (lpdsNotify->SetNotificationPositions(AUDIO_DIRECTX_BUFFERS, rgdscbpn)))
        {
            throw error("Unable to set up sound notification positions.");
        }
        // -----------------------------------------------------------

        lpDsb->Stop ();
        DWORD dwBytes;
        if (FAILED (lpDsb->Lock (0, bufSize, &lpvData, &dwBytes, NULL, NULL, 0)))
        {
            throw error("Unable to lock sound buffer.");
        }

        // Rev 1.7 (saw) - Set the play position back to the begining
        if (FAILED (lpDsb->SetCurrentPosition(0)))
        {
            throw error("Unable to set play position to start of buffer.");
        }

        // Update the users settings
        cfg.bufSize   = bufSize / 2;
        _settings     = cfg;
        isPlaying     = false;
        _sampleBuffer = (short*)lpvData;
        return true;
    }
    catch(error const &e)
    {
        setError(e.message());

        SAFE_RELEASE (lpDsbPrimary);
        close ();
        return false;
    }
}

bool Audio_DirectX::write ()
{
    if (!isOpen)
    {
        setError("Device not open.");
        return false;
    }
    // Unlock the current buffer for playing
    lpDsb->Unlock (lpvData, bufSize, NULL, 0);

    // Check to see of the buffer is playing
    // and if not start it off
    if (!isPlaying)
    {
        isPlaying = true;
        if (FAILED (lpDsb->Play (0,0,DSBPLAY_LOOPING)))
        {
            setError("Unable to start playback.");
            return false;
        }
    }

    // Check the incoming event to make sure it's one of our event messages and
    // not something else
    DWORD dwEvt;
    do
    {
        dwEvt  = MsgWaitForMultipleObjects (AUDIO_DIRECTX_BUFFERS, rghEvent, FALSE, INFINITE, QS_ALLINPUT);
        dwEvt -= WAIT_OBJECT_0;
    } while (dwEvt >= AUDIO_DIRECTX_BUFFERS);

//    printf ("Event - %lu\n", dwEvt);

    // Lock the next buffer for filling
    DWORD dwBytes;
    if (FAILED (lpDsb->Lock (bufSize * dwEvt, bufSize, &lpvData, &dwBytes, NULL, NULL, 0)))
    {
        setError("Unable to lock sound buffer.");
        return false;
    }
    _sampleBuffer = (short*)lpvData;
    return true;
}

void Audio_DirectX::reset (void)
{
    DWORD dwBytes;
    if (!isOpen)
         return;

    // Stop play and kill the current music.
    // Start new music data being added at the begining of
    // the first buffer
    lpDsb->Stop ();
    // Rev 1.7 (saw) - Prevents output going silent after reset
    isPlaying = false;
    lpDsb->Unlock (lpvData, bufSize, NULL, 0);

    // Rev 1.4 (saw) - Added as lock can fail.
    if (FAILED (lpDsb->Lock (0, bufSize, &lpvData, &dwBytes, NULL, NULL, 0)))
    {
        setError("Unable to lock sound buffer.");
        return;
    }
    _sampleBuffer = (short*)lpvData;
}

// Rev 1.8 (saw) - Alias fix
void Audio_DirectX::close (void)
{
    if (!isOpen)
        return;

    isOpen        = false;
    _sampleBuffer = NULL;

    if (lpDsb)
    {
        lpDsb->Stop();
        isPlaying = false;
        if (lpvData)
        {
            // Rev 1.4 (iv) - Unlock before we release buffer.
            lpDsb->Unlock (lpvData, bufSize, NULL, 0);
        }
    }

    SAFE_RELEASE (lpdsNotify);
    SAFE_RELEASE (lpDsb);
    SAFE_RELEASE (lpds);

    // Rev 1.3 (Ingve Vormestrand) - Changed "<=" to "<"
    // as closing invalid handle.
    for (int i=0;i < AUDIO_DIRECTX_BUFFERS; i++)
        CloseHandle (rghEvent[i]);
}

void Audio_DirectX::pause (void)
{
    lpDsb->Stop ();
    isPlaying = false;
}

#endif // HAVE_DIRECTX
