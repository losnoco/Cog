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


#ifndef _MPAFRAMEFINDER_H_
#define _MPAFRAMEFINDER_H_

#include "IFileAccess.h"
#include "mpegAudioFrame/mpegAudioHeader.h"

#include "MemBuffer.h"

class CMPAFrameFinder
{
public:
	CMPAFrameFinder();
	~CMPAFrameFinder();

	inline void SetLookAheadFrames(int nFrames);
	inline void SetAllowFrequencyChanges(bool bAllow);	

	inline int GetLookAheadFrames();
	inline bool GetAllowFrequencyChanges();	

	//if false, the finder needs more input data or the stream
	//invalid
	bool ReadNextFrame();

	inline bool IsStreamInvalid();
	inline bool KnowsCharacteristics();

	inline bool HasFrame();
	inline void* GetFrameData();
	inline int GetFrameSize();
	inline double GetAvgFrameSize();	
	inline MpegAudioHeader* GetFrameHeader();

	void SetInput(void* pBuffer,int nBytes,long nStreamPosition);
	bool ReadInput(IFileAccess* pAccess);

	//throw away buffered data and find next frame
	void Flush();

	//start with a completely blank slate for a new data stream
	void Restart();

	inline long GetFirstFramePosition();
	
protected:
	struct HeaderRecord
	{
		MpegAudioHeader Header;
		long nPosition;

		HeaderRecord* pNext;
	};

	bool FindCharacteristics();
	bool FindConsistentHeaders();
	bool CanHaveConsistentHeaders(HeaderRecord* pRecord,int nHeaderCount);
	bool HasConsistentHeaders(HeaderRecord* pCheckRecord,HeaderRecord* pRecord,int nHeadersNeeded);
	bool AreHeadersConsistent(HeaderRecord* pFirst,HeaderRecord* pSecond);

	bool IsConformingHeader(MpegAudioHeader* pHeader);

	void FindNextSync();
	void ReadHeader();
	void CheckHeader();
	void ReadFrame();
	
	void ResetState();

	void DeleteHeaderRecords();

	static bool IsValidHeader(MpegAudioHeader* pHeader);

	CMemBuffer m_OutBuffer;

	CMemBuffer* m_pInBuffer;
	CMemBuffer m_MyInBuffer;
	CMemBuffer m_UserInBuffer;

	int m_State;

	int m_nLookAheadFrames;
	bool m_bAllowFrequencyChanges;

	int m_nLayer;
	int m_nInputStereo;
	int m_nFrequency;
	int m_nLayer25;
	int m_nVersion;
	
	MpegAudioHeader m_Header;
	int m_nFrameSize;

	long m_nReadStartPosition;
	long m_nFirstFramePosition;

	enum
	{
		STATE_FINDCHARACTERISTICS=0,
		STATE_FINDSYNC,
		STATE_READHEADER,
		STATE_CHECKHEADER,
		STATE_READFRAME,

		STATE_HAVEFRAME
	};

	HeaderRecord* m_pFirstHeader;
	HeaderRecord* m_pLastHeader;

	HeaderRecord* m_pNewHeaderRecord;

	bool m_bStreamInvalid;
};

inline void CMPAFrameFinder::SetLookAheadFrames(int nFrames)
{
	if(nFrames<0)
		nFrames=0;

	m_nLookAheadFrames=nFrames;
}

inline void CMPAFrameFinder::SetAllowFrequencyChanges(bool bAllow)
{
	m_bAllowFrequencyChanges=bAllow;
}

inline int CMPAFrameFinder::GetLookAheadFrames()
{
	return m_nLookAheadFrames;
}

inline bool CMPAFrameFinder::GetAllowFrequencyChanges()
{
	return m_bAllowFrequencyChanges;
}

inline bool CMPAFrameFinder::HasFrame()
{
	return (m_State==STATE_HAVEFRAME);
}

inline void* CMPAFrameFinder::GetFrameData()
{
	return m_OutBuffer.GetPtr();
}

inline int CMPAFrameFinder::GetFrameSize()
{
	return m_OutBuffer.GetPos();
}

inline double CMPAFrameFinder::GetAvgFrameSize()
{
	return m_Header.GetAvgFrameSize();
}

inline MpegAudioHeader* CMPAFrameFinder::GetFrameHeader()
{
	return &m_Header;
}

inline bool CMPAFrameFinder::IsStreamInvalid()
{
	return m_bStreamInvalid;
}

inline bool CMPAFrameFinder::KnowsCharacteristics()
{
	return (m_State>STATE_FINDCHARACTERISTICS);
}

inline long CMPAFrameFinder::GetFirstFramePosition()
{
	return m_nFirstFramePosition;
}

#endif
