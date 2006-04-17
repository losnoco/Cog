/*  DecMPA - simple MPEG Audio decoding library.
    Copyright (C) 2002  Hauke Duden

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	For more information look at the file License.txt in this package.

  
	email: hazard_hd@users.sourceforge.net
*/


#include <math.h>
#include "interface.h"

#include "../DecodeEngine.h"

#include "../frame/pcmFrame.h"

#include <new>

extern const long freqs[9];

struct DecodeEngine
{
	MPSTR Decoder;
	short aTempBuffer[MP3FRAMESIZE*2];
};

void* DecodeEngine_Create() 
{
	DecodeEngine* pEngine=new DecodeEngine;

	if(!InitMP3(&pEngine->Decoder))
	{
		delete pEngine;
		pEngine=(DecodeEngine*)0;
	}

	return pEngine;
}

void DecodeEngine_Destroy(void* pEng)
{
	DecodeEngine* pEngine=(DecodeEngine*)pEng;

	ExitMP3(&pEngine->Decoder);

	delete pEngine;
}

bool DecodeEngine_Decode(void* pEng,AudioFrame* pDest,void* pData,long nDataLength)
{
	DecodeEngine* pEngine=(DecodeEngine*)pEng;
	int nDone;
	int nResult;
	char* pOut;
	int nOutSize;
	int nChannels;

	if(pDest->getFrameType()==_FRAME_AUDIO_PCM)
	{
		pOut=(char*)((PCMFrame*)pDest)->getData();
		nOutSize=pDest->getSize();
	}
	else
	{
		pOut=(char*)pEngine->aTempBuffer;
		nOutSize=sizeof(pEngine->aTempBuffer)/sizeof(short);
	}

	nResult=decodeMP3(&pEngine->Decoder,(unsigned char*)pData,nDataLength,pOut,nOutSize,&nDone);	
	if(nResult==MP3_OK)
	{
		nDone/=sizeof(short);
		nChannels=pEngine->Decoder.fr.stereo;
		
		if(pDest->getFrameType()!=_FRAME_AUDIO_PCM)
			pDest->putInt16Data((short*)pOut,nDone);
		else
			pDest->setLen(nDone);

		pDest->setFrameFormat((nChannels==2) ? 1 : 0,freqs[pEngine->Decoder.fr.sampling_frequency]);

		return true;
	}
	else if(nResult==MP3_NEED_MORE)
	{
		//should never happen otherwise because only whole frames
		//are passed to HIP and the modified version of HIP we use
		//here does not buffer data from the beginning for later use

		//not critical, though

		pDest->setLen(0);
		pDest->setFrameFormat(1,44100);

		return true;
	}
	else
	{
		//error
		return false;
	}
}

void DecodeEngine_Flush(void* pEng)
{
	DecodeEngine* pEngine=(DecodeEngine*)pEng;

	//complete reinit
	ExitMP3(&pEngine->Decoder);
	InitMP3(&pEngine->Decoder);
	
	/*while(pEngine->Decoder.tail!=NULL)
		remove_buf(&pEngine->Decoder);
	pEngine->Decoder.bsize=0;

	pEngine->Decoder.header_parsed=0;
	pEngine->Decoder.side_parsed=0;
	pEngine->Decoder.data_parsed=0;
	pEngine->Decoder.sync_bitstream=1;*/
}
