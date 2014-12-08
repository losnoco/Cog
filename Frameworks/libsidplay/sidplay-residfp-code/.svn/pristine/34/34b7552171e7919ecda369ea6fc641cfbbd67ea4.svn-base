/*
 * This file is part of sidplayfp, a console SID player.
 *
 * Copyright 2013 Leandro Nini
 * Copyright 2001-2002 Simon White
 * Copyright 2000 Jarno Paananen
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

#ifdef HAVE_MMSYSTEM

#include <stdio.h>
#include <mmreg.h>

Audio_MMSystem::Audio_MMSystem() :
    AudioBase("MMSYSTEM"),
    waveHandle(0),
    isOpen(false)
{
    for ( int i = 0; i < MAXBUFBLOCKS; i++ )
    {
        blockHeaderHandles[i] = 0;
        blockHandles[i]       = 0;
        blockHeaders[i]       = NULL;
        blocks[i]             = NULL;
    }
}

Audio_MMSystem::~Audio_MMSystem()
{
    close();
}

const char* Audio_MMSystem::getErrorMessage(MMRESULT err)
{
    switch (err)
    {
        default:
        case MMSYSERR_ERROR:            return "Unspecified error";
        case MMSYSERR_BADDEVICEID:      return "Device ID out of range";
        case MMSYSERR_NOTENABLED:       return "Driver failed enable";
        case MMSYSERR_ALLOCATED:        return "Device already allocated";
        case MMSYSERR_INVALHANDLE:      return "Device handle is invalid";
        case MMSYSERR_NODRIVER:         return "No device driver present";
        case MMSYSERR_NOMEM:            return "Memory allocation error";
        case MMSYSERR_NOTSUPPORTED:     return "Function isn't supported";
        case MMSYSERR_BADERRNUM:        return "Error value out of range";
        case MMSYSERR_INVALFLAG:        return "Invalid flag passed";
        case MMSYSERR_INVALPARAM:       return "Invalid parameter passed";
        case MMSYSERR_HANDLEBUSY:       return "Handle being used simultaneously on another thread (eg callback)";
        case MMSYSERR_INVALIDALIAS:     return "Specified alias not found";
        case MMSYSERR_BADDB:            return "Bad registry database";
        case MMSYSERR_KEYNOTFOUND:      return "Registry key not found";
        case MMSYSERR_READERROR:        return "Registry read error";
        case MMSYSERR_WRITEERROR:       return "Registry write error";
        case MMSYSERR_DELETEERROR:      return "Registry delete error";
        case MMSYSERR_VALNOTFOUND:      return "Registry value not found";
        case MMSYSERR_NODRIVERCB:       return "Driver does not call DriverCallback";
    }
/*
    TCHAR buffer[MAXERRORLENGTH];
    waveOutGetErrorText(err, buffer, MAXERRORLENGTH);
*/
}

void Audio_MMSystem::checkResult(MMRESULT err)
{
    if (err != MMSYSERR_NOERROR)
    {
        throw error(getErrorMessage(err));
    }
}

bool Audio_MMSystem::open(AudioConfig &cfg)
{
    WAVEFORMATEX  wfm;

    if (isOpen)
    {
        setError("Audio device already open.");
        return false;
    }
    isOpen = true;

    /* Initialise blocks */
    memset (blockHandles, 0, sizeof (blockHandles));
    memset (blockHeaders, 0, sizeof (blockHeaders));
    memset (blockHeaderHandles, 0, sizeof (blockHeaderHandles));

    // Format
    memset (&wfm, 0, sizeof(WAVEFORMATEX));
    wfm.wFormatTag      = WAVE_FORMAT_PCM;
    wfm.nChannels       = cfg.channels;
    wfm.nSamplesPerSec  = cfg.frequency;
    wfm.wBitsPerSample  = 16;
    wfm.nBlockAlign     = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;
    wfm.cbSize          = 0;

    // Rev 1.3 (saw) - Calculate buffer to hold 250ms of data
    bufSize = wfm.nSamplesPerSec / 4 * wfm.nBlockAlign;

    try
    {
        cfg.bufSize = bufSize / 2;
        checkResult(waveOutOpen(&waveHandle, WAVE_MAPPER, &wfm, 0, 0, 0));

        _settings = cfg;

        /* Allocate and lock memory for all mixing blocks: */
        for (int i = 0; i < MAXBUFBLOCKS; i++ )
        {
            /* Allocate global memory for mixing block: */
            if ( (blockHandles[i] = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
                                                bufSize)) == NULL )
            {
                throw error("Can't allocate global memory.");
            }

            /* Lock mixing block memory: */
            if ( (blocks[i] = (short *)GlobalLock(blockHandles[i])) == NULL )
            {
                throw error("Can't lock global memory.");
            }

            /* Allocate global memory for mixing block header: */
            if ( (blockHeaderHandles[i] = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
                                                    sizeof(WAVEHDR))) == NULL )
            {
                throw error("Can't allocate global memory.");
            }

            /* Lock mixing block header memory: */
            WAVEHDR *header;
            if ( (header = blockHeaders[i] =
                (WAVEHDR*)GlobalLock(blockHeaderHandles[i])) == NULL )
            {
                throw error("Can't lock global memory.");
            }

            /* Reset wave header fields: */
            memset (header, 0, sizeof (WAVEHDR));
            header->lpData         = (char*)blocks[i];
            header->dwBufferLength = bufSize;
            header->dwFlags        = WHDR_DONE; /* mark the block is done */
        }

        blockNum = 0;
        _sampleBuffer = blocks[blockNum];
        return true;
    }
    catch(error const &e)
    {
        setError(e.message());

        close ();
        return false;
    }
}

bool Audio_MMSystem::write()
{
    if (!isOpen)
    {
        setError("Device not open.");
        return false;
    }

    /* Reset wave header fields: */
    blockHeaders[blockNum]->dwFlags = 0;

    /* Prepare block header: */
    checkResult(waveOutPrepareHeader(waveHandle, blockHeaders[blockNum], sizeof(WAVEHDR)));

    checkResult(waveOutWrite(waveHandle, blockHeaders[blockNum], sizeof(WAVEHDR)));

    /* Next block, circular buffer style, and I don't like modulo. */
    blockNum++;
    blockNum %= MAXBUFBLOCKS;

    /* Wait for the next block to become free */
    while ( !(blockHeaders[blockNum]->dwFlags & WHDR_DONE) )
        Sleep(20);

    checkResult(waveOutUnprepareHeader(waveHandle, blockHeaders[blockNum], sizeof(WAVEHDR)));

    _sampleBuffer = blocks[blockNum];
    return true;
}

// Rev 1.2 (saw) - Changed, see AudioBase.h
void Audio_MMSystem::reset()
{
    if (!isOpen)
        return;

    // Stop play and kill the current music.
    // Start new music data being added at the begining of
    // the first buffer
    checkResult(waveOutReset(waveHandle));
    blockNum = 0;
    _sampleBuffer = blocks[blockNum];
}

void Audio_MMSystem::close()
{
    if ( !isOpen )
        return;

    isOpen        = false;
    _sampleBuffer = NULL;

    /* Reset wave output device, stop playback, and mark all blocks done: */
    if ( waveHandle )
    {
        waveOutReset(waveHandle);

        /* Make sure all blocks are indeed done: */
        int doneTimeout = 500;

        for (;;) {
            bool allDone = true;
            for ( int i = 0; i < MAXBUFBLOCKS; i++ ) {
                if ( blockHeaders[i] && (blockHeaders[i]->dwFlags & WHDR_DONE) == 0 )
                    allDone = false;
            }

            if ( allDone || (doneTimeout == 0) )
                break;
            doneTimeout--;
            Sleep(20);
        }

        /* Unprepare all mixing blocks, unlock and deallocate
           all mixing blocks and mixing block headers: */
        for ( int i = 0; i < MAXBUFBLOCKS; i++ )
        {
            if ( blockHeaders[i] )
                waveOutUnprepareHeader(waveHandle, blockHeaders[i], sizeof(WAVEHDR));

            if ( blockHeaderHandles[i] )
            {
                GlobalUnlock(blockHeaderHandles[i]);
                GlobalFree(blockHeaderHandles[i]);
            }
            if ( blockHandles[i] )
            {
                GlobalUnlock(blockHandles[i]);
                GlobalFree(blockHandles[i]);
            }
        }

        /* Close wave output device: */
        waveOutClose(waveHandle);
    }
}

#endif // HAVE_MMSYSTEM
