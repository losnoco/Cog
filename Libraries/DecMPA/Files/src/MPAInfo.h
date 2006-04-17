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


#ifndef _MPAINFO_H_
#define _MPAINFO_H_

#include "IFileAccess.h"
#include "MPAFrameFinder.h"
#include "mpegAudioFrame/dxHead.h"

class CMPAInfo
{
public:
	CMPAInfo();
	~CMPAInfo();

	bool InitInfo(CMPAFrameFinder* pFrameFinder,IFileAccess* pFileAccess);

	//-1 if unknown
	inline long GetDuration();

	//-1 if unknown
	long GetFilePositionFromTime(long Millis);

	inline bool IsXingVBR();
	inline XHEADDATA* GetXingHeader();
	
protected:
	void Init(CMPAFrameFinder* pFrameFinder);
	bool ReadXingHeader(void* pFrameData,int nFrameSize);
	long GetEncodedDataPositionFromTOC(float TimeFract);

	long m_nEncodedDataSize;
	long m_nEncodedDataOffset;

	long m_nDuration;

	bool m_bXingVBR;
	XHEADDATA m_XingHeader;
	unsigned char m_aXingTOC[100];

	float m_aTOC[101];
};

inline long CMPAInfo::GetDuration()
{
	return m_nDuration;
}

inline bool CMPAInfo::IsXingVBR()
{
	return m_bXingVBR;
}

inline XHEADDATA* CMPAInfo::GetXingHeader()
{
	return &m_XingHeader;
}


#endif
