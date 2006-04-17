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

//This file is a heavily modified version of SPlayPlugin.h from the original
//mpeglib. See Readme.txt for details.


#ifndef _MPADECODER_H_
#define _MPADECODER_H_

#include "../include/decmpa.h"

#include "DecMPAFileAccess.h"

#include "frame/pcmFrame.h"
#include "frame/floatFrame.h"

#include "MPAFrameFinder.h"
#include "MPAInfo.h"

class CMPADecoder
{
public:
	CMPADecoder(const DecMPA_Callbacks& Callbacks,void* pCallbackContext);
	~CMPADecoder();

	int Decode(void* pBuffer,long nBufferBytes,long& nBytesDecoded);
	int DecodeNoData(long& nDecodedBytes);

	int SeekToTime(long Millis);
	
	int GetTime(long& Time);
	int GetDuration(long& Duration);

	bool OutputFormatChanged();
	void GetOutputFormat(DecMPA_OutputFormat& Format);

	void GetMPEGHeader(DecMPA_MPEGHeader& Header);

	int GetID3v2Data(unsigned char*& pData,long& nDataSize);

	inline void SetDestroyNotify(void (*pNotify)(void*),void* pContext);

	long GetFilePositionFromTime(long Millis);

	int SetParam(int ID,long Value);
	long GetParam(int ID);

protected:
	int DecodeNextFrame(bool bNoData);
	bool ReadNextFrame();	

	void HandleDecodedData(AudioFrame* playFrame);
	void SetOutputFormat(AudioFrame* pNewFormatFrame);	
	
	bool ReadDecodedData(void* pDest,long nBytes,long& nRead);

	void Flush();
	
	int EnsurePrepared();
	void HandleID3Tag();

	void UpdateMPEGHeader(MpegAudioHeader* pHeader);

	DecMPA_Callbacks m_Callbacks;
	void* m_pCallbackContext;

	bool m_bDoFloat;

	AudioFrame* m_pAudioFrame;
	FloatFrame* m_pFloatFrame;
	PCMFrame* m_pPCMFrame;

	AudioFrame* m_pFormatFrame;
	int m_nOutputBlockSize;

	CDecMPAFileAccess* m_pFileAccess; 

	long m_nBufferBytes;
	long m_nUsedBufferBytes;
	unsigned char* m_pBuffer;	

	double m_nReadTime;
	double m_nBufferStartTime;
	double m_nDecodeTime;

	DecMPA_OutputFormat m_OutputFormat;
	bool m_bOutputFormatChanged;

	DecMPA_MPEGHeader m_MPEGHeader;

	bool m_bFirstDecode;
	bool m_bPrepared;
	bool m_bHasFrame;

	int m_nResyncCounter;

	void* m_pDecodeEngine;

	CMPAFrameFinder m_FrameFinder;
	CMPAInfo m_Info;

	bool m_bStoreID3v2;
	unsigned char* m_pID3v2Data;
	long m_nID3v2DataSize;

	void (*m_pDestroyNotify)(void*);
	void* m_pDestroyNotifyContext;

	long m_aParams[DECMPA_PARAMCOUNT];

	bool m_bNotMPEG;

	bool m_bDecoderNeedsFlush;
};

inline void CMPADecoder::SetDestroyNotify(void (*pNotify)(void*),void* pContext)
{
	m_pDestroyNotify=pNotify;
	m_pDestroyNotifyContext=pContext;
}

#endif

