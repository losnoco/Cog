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
#include <stdlib.h>
#include <string.h>

#include "ao.h"

// local variables

void  (*m1sdr_Callback)(unsigned long dwNumSamples, signed short *data);

int nDSoundSegLen = 0;

static INT16 samples[44100*2];
static FILE *fout = NULL;

// set # of samples per update

void m1sdr_SetSamplesPerTick(UINT32 spf)
{
}

// m1sdr_Update - timer callback routine: runs sequencer and mixes sound

void m1sdr_Update(void)
{	
	if (m1sdr_Callback)
	{
		printf("Requesting %i\n", nDSoundSegLen);
		m1sdr_Callback(nDSoundSegLen, (INT16 *)samples);
	}
}
// checks the play position to see if we should trigger another update

void m1sdr_TimeCheck(void)
{
	m1sdr_Update();
	fwrite(samples, nDSoundSegLen*4, 1, fout);
}

// m1sdr_Init - inits the output device and our global state

INT16 m1sdr_Init(int sample_rate)
{
	nDSoundSegLen = 350; // ;sample_rate/60;
	memset(samples, 0, 44100*4);	// zero out samples
	m1sdr_Callback = NULL;

        fout = fopen("output.raw", "wb");
}

void m1sdr_Exit(void)
{
        fclose(fout);
}

void m1sdr_SetCallback(void *fn)
{
	if (fn == (void *)NULL)
	{
		printf("ERROR: NULL CALLBACK!\n");
	}

//	printf("m1sdr_SetCallback: aok!\n");
	m1sdr_Callback = (void (*)(unsigned long, signed short *))fn;
}

void m1sdr_PlayStart(void)
{
}


