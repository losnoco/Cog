/*

	Audio Overload SDK

	Copyright (c) 2007, R. Belmont and Richard Bannister.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	* Neither the names of R. Belmont and Richard Bannister nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
	LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <windows.h>
#include <dsound.h>

#include "cpuintrf.h"
#include "oss.h"
#include "ao.h"

static INT16 samples[44100*4];	// make sure we reserve enough for worst-case scenario

void (*m1sdr_Callback)(unsigned long dwSamples, short *samples);
unsigned long cbUserData;

static int hw_present, playtime;

LPDIRECTSOUND lpDS;			// DirectSound COM object
LPDIRECTSOUNDBUFFER lpPDSB;	// Primary DirectSound buffer
LPDIRECTSOUNDBUFFER lpSecB;	// Secondary DirectSound buffer

int nDSoundSamRate=44100;        // sample rate
int nDSoundSegCount=16;    // Segs in the pdsbLoop buffer
static int cbLoopLen=0;          // Loop length (in bytes) calculated

int nDSoundFps=600;              // Application fps * 10
int nDSoundSegLen=0;             // Seg length in samples (calculated from Rate/Fps)
short *DSoundNextSound=NULL;     // The next sound seg we will add to the sample loop
unsigned char bDSoundOkay=0;     // True if DSound was initted okay
unsigned char bDSoundPlaying=0;  // True if the Loop buffer is playing

#define WRAP_INC(x) { x++; if (x>=nDSoundSegCount) x=0; }

static int nDSoundNextSeg=0; // We have filled the sound in the loop up to the beginning of 'nNextSeg'

void m1sdr_SetSamplesPerTick(UINT32 spf)
{
	if (spf != (nDSoundFps/10))
	{
		m1sdr_Exit();
		nDSoundFps = spf * 10;
		m1sdr_Init(nDSoundSamRate);
	}
}

void m1sdr_TimeCheck(void)
{
	int nPlaySeg=0, nFollowingSeg=0;
	DWORD nPlay=0, nWrite=0;
	int nRet=0;

	if (!lpDS) return;

	// We should do nothing until nPlay has left nDSoundNextSeg
	IDirectSoundBuffer_GetCurrentPosition(lpSecB, &nPlay, &nWrite);

	nPlaySeg=nPlay/(nDSoundSegLen<<2);

	if (nPlaySeg>nDSoundSegCount-1) nPlaySeg=nDSoundSegCount-1;
	if (nPlaySeg<0) nPlaySeg=0; // important to ensure nPlaySeg clipped for below

	if (nDSoundNextSeg == nPlaySeg)
	{
		Sleep(200); // Don't need to do anything for a bit
		goto End;
	}

	// work out which seg we will fill next
	nFollowingSeg = nDSoundNextSeg; 
	WRAP_INC(nFollowingSeg)

	while (nDSoundNextSeg != nPlaySeg)
	{
		void *pData=NULL,*pData2=NULL; DWORD cbLen=0,cbLen2=0;

		// fill nNextSeg
		// Lock the relevant seg of the loop buffer
		nRet = IDirectSoundBuffer_Lock(lpSecB, nDSoundNextSeg*(nDSoundSegLen<<2), nDSoundSegLen<<2, &pData, &cbLen, &pData2, &cbLen2, 0);

		if (nRet>=0 && pData!=NULL)
		{
		  // Locked the seg okay - write the sound we calculated last time
			memcpy(pData, samples, nDSoundSegLen<<2);
		}
		// Unlock (2nd 0 is because we wrote nothing to second part)
		if (nRet>=0) IDirectSoundBuffer_Unlock(lpSecB, pData, cbLen, pData2, 0); 

		// generate more samples
		if (m1sdr_Callback)
		{
			m1sdr_Callback(nDSoundSegLen, samples);
			playtime++;
		}
		else
		{
			memset(samples, 0, nDSoundSegLen*4);
		}

		nDSoundNextSeg = nFollowingSeg;
		WRAP_INC(nFollowingSeg)
	}

End:
	return;
}

INT16 m1sdr_Init(int sample_rate)
{
	DSBUFFERDESC	dsbuf;
	WAVEFORMATEX	format;

	nDSoundSamRate = sample_rate;

	lpDS = NULL;
	lpPDSB = NULL;
	lpSecB = NULL;

	// Calculate the Seg Length and Loop length
	// (round to nearest sample)
	nDSoundSegLen=(nDSoundSamRate*10+(nDSoundFps>>1))/nDSoundFps;
	cbLoopLen=(nDSoundSegLen*nDSoundSegCount)<<2;

	// create an IDirectSound COM object

	if (DS_OK != DirectSoundCreate(NULL, &lpDS, NULL))
	{
	    	printf("Unable to create DirectSound object!\n");
		return(0);
	}

	// set cooperative level where we need it

	if (DS_OK != IDirectSound_SetCooperativeLevel(lpDS, GetForegroundWindow(), DSSCL_PRIORITY))
	{
	    	printf("Unable to set cooperative level!\n");
		return(0);
	}

	// now create a primary sound buffer
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.wBitsPerSample = 16;
	format.nSamplesPerSec = nDSoundSamRate;
	format.nBlockAlign = 4;	// stereo 16-bit
	format.cbSize = 0;
  	format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;

	memset(&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	if (DS_OK != IDirectSound_CreateSoundBuffer(lpDS, &dsbuf, &lpPDSB, NULL))
	{
	    	printf("Unable to create primary buffer!");
		return(0);		
	}

	// and set it's format how we want
	
	if (DS_OK != IDirectSoundBuffer_SetFormat(lpPDSB, &format))		
	{
	    	printf("Unable to set primary buffer format!\n");
		return(0);
	}

	// start the primary buffer playing now so we get
	// minimal lag when we trigger our secondary buffer

	IDirectSoundBuffer_Play(lpPDSB, 0, 0, DSBPLAY_LOOPING);

	// that's done, now let's create our secondary buffer

	memset(&dsbuf, 0, sizeof(DSBUFFERDESC));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);

	// we'll take default controls for this one
	dsbuf.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY; 
	dsbuf.dwBufferBytes = cbLoopLen;
	dsbuf.lpwfxFormat = (LPWAVEFORMATEX)&format;
	
	if (DS_OK != IDirectSound_CreateSoundBuffer(lpDS, &dsbuf, &lpSecB, NULL))
	{
	    	printf("Unable to create secondary buffer\n");
		return(0);
	}

	// ok, cool, we're ready to go!
	// blank out the entire sound buffer
	{
		LPVOID ptr; DWORD len;

		IDirectSoundBuffer_Lock(lpSecB, 0, 0, &ptr, &len, NULL, NULL, DSBLOCK_ENTIREBUFFER);
		ZeroMemory(ptr, len);
		IDirectSoundBuffer_Unlock(lpSecB, ptr, len, 0, 0);
	}

	ZeroMemory(samples, nDSoundSegLen<<2);

	bDSoundOkay=1; // This module was initted okay

	return(1);
}

void m1sdr_Exit(void)
{
	if (lpSecB)
	{
		IDirectSoundBuffer_Stop(lpSecB);
		IDirectSoundBuffer_Release(lpSecB);
		lpSecB = NULL;
	}
	if (lpPDSB)
	{
		IDirectSoundBuffer_Stop(lpPDSB);
		IDirectSoundBuffer_Release(lpPDSB);
		lpPDSB = NULL;
	}
	if (lpDS) 
	{
		IDirectSound_Release(lpDS);
		lpDS = NULL;
	}
}



void m1sdr_PlayStart(void)
{
	IDirectSound_SetCooperativeLevel(lpDS, GetForegroundWindow(), DSSCL_PRIORITY);
	
	IDirectSoundBuffer_SetCurrentPosition(lpSecB, 0);
	IDirectSoundBuffer_Play(lpSecB, 0, 0, DSBPLAY_LOOPING);

	playtime = 0;
}

void m1sdr_PlayStop(void)
{
	DSBUFFERDESC	dsbuf;
	WAVEFORMATEX	format;

	IDirectSoundBuffer_Stop(lpSecB);
	// this is a bit cheezity-hacky
	IDirectSoundBuffer_Release(lpSecB);

	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.wBitsPerSample = 16;
	format.nSamplesPerSec = nDSoundSamRate;
	format.nBlockAlign = 4;	// stereo 16-bit
	format.cbSize = 0;
  	format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;

        memset(&dsbuf, 0, sizeof(DSBUFFERDESC));
        dsbuf.dwSize = sizeof(DSBUFFERDESC);
	// we'll take default controls for this one
        dsbuf.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY; 
        dsbuf.dwBufferBytes = cbLoopLen;
	dsbuf.lpwfxFormat = (LPWAVEFORMATEX)&format;
	
	if (DS_OK != IDirectSound_CreateSoundBuffer(lpDS, &dsbuf, &lpSecB, NULL))
	{
	    	printf("Unable to create secondary buffer\n");
		return;
	}

	// zero out the buffer
	{
		LPVOID ptr; DWORD len;

		IDirectSoundBuffer_Lock(lpSecB, 0, 0, &ptr, &len, NULL, NULL, DSBLOCK_ENTIREBUFFER);
		ZeroMemory(ptr, len);
		IDirectSoundBuffer_Unlock(lpSecB, ptr, len, 0, 0);
	}
}


INT32 m1sdr_HwPresent(void)
{
	return hw_present;
}

INT16 m1sdr_IsThere(void)
{
    if(DS_OK == DirectSoundCreate(NULL, &lpDS, NULL)) 
    {
	IDirectSound_Release(lpDS);
	hw_present = 1;
	return(1);
    } 
    else
    {
	hw_present = 0;
	return(0);
    }
}

void m1sdr_SetCallback(void *fn)
{
	if (fn == (void *)NULL)
	{
		printf("ERROR: NULL CALLBACK!\n");
	}

	m1sdr_Callback = (void (*)(unsigned long, signed short *))fn;
}

void m1sdr_FlushAudio(void)
{
	memset(samples, 0, nDSoundSegLen * 4);
	m1sdr_TimeCheck();
	m1sdr_TimeCheck();
	m1sdr_TimeCheck();
	m1sdr_TimeCheck();
	m1sdr_TimeCheck();
}

short *m1sdr_GetSamples(void)
{
	return samples;
}

int m1sdr_GetPlayTime(void)
{
	int rv;
	int fps = nDSoundFps / 10;

	rv = playtime / fps;

	return rv;		// total seconds
}

int m1sdr_GetPlayTimeTicks(void)
{
	return playtime;
}
