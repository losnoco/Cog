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


#include "MPAInfo.h"

CMPAInfo::CMPAInfo()
{
	m_nEncodedDataSize=-1;
	m_nEncodedDataOffset=0;
	m_nDuration=-1;
	m_bXingVBR=false;

	m_XingHeader.toc=m_aXingTOC;
}

CMPAInfo::~CMPAInfo()
{
}

bool CMPAInfo::InitInfo(CMPAFrameFinder* pFrameFinder,IFileAccess* pFileAccess)
{
	m_nEncodedDataSize=-1;
	m_nEncodedDataOffset=0;
	m_nDuration=-1;
	m_bXingVBR=false;

	while(!pFrameFinder->ReadNextFrame())
	{
		if(pFrameFinder->IsStreamInvalid())
			return false;

		if(!pFrameFinder->ReadInput(pFileAccess))
			return false;
	}

	m_nEncodedDataOffset=pFrameFinder->GetFirstFramePosition();

	m_nEncodedDataSize=pFileAccess->GetLength();
	if(m_nEncodedDataSize!=-1)
	{
		//ignore any leading non-MPEG-Audio data
		m_nEncodedDataSize-=m_nEncodedDataOffset;
	}

	Init(pFrameFinder);

	return true;
}

void CMPAInfo::Init(CMPAFrameFinder* pFrameFinder)
{
	double nAvgFrameSize;
	int nFrameSize;
	long nFrameCount=-1;
		
	nAvgFrameSize=pFrameFinder->GetAvgFrameSize();
	nFrameSize=pFrameFinder->GetFrameSize();
	if(nAvgFrameSize>0)
	{
		if(ReadXingHeader(pFrameFinder->GetFrameData(),nFrameSize))
		{
			m_bXingVBR=true;
			nFrameCount=m_XingHeader.frames;
		}
		else
		{
			if(m_nEncodedDataSize>=0)
				nFrameCount=(long)(m_nEncodedDataSize/nAvgFrameSize);
		}
	}

	if(nFrameCount!=-1)
	{
		MpegAudioHeader* pHeader=pFrameFinder->GetFrameHeader();
		int nDecodedSamplesPerFrame;
		int nFrequency;
		double nDecodedSamples;

		nDecodedSamplesPerFrame=pHeader->getpcmperframe();
		nFrequency=pHeader->getFrequencyHz();

		nDecodedSamples=((double)nFrameCount)*nDecodedSamplesPerFrame;

		if(nFrequency!=0)
			m_nDuration=(long)((nDecodedSamples*1000)/nFrequency);
	}
}

bool CMPAInfo::ReadXingHeader(void* pFrameData,int nFrameSize)
{
	if(nFrameSize<152)	//cannot have xing header
		return false;

	if(::GetXingHeader(&m_XingHeader,(unsigned char*)pFrameData)==0)
		return false;

	//we use floats so that we won't loose precision
	//with the un-compensation stuff below
	for(int i=0;i<100;i++)
		m_aTOC[i]=((float)m_XingHeader.toc[i])/256.0f;
	m_aTOC[100]=1.0f;

	if(m_aTOC[0]!=0)
	{
		//the Xing table of contents compensates for some
		//leading non-MP3 data (maybe an ID3 tag or something)
		//Since that data may never have been passed to the library
		//(or if it has been passed, it should have been skipped)
		//we undo this compensation
		float FullSizeValue;

		FullSizeValue=1.0f-m_aTOC[0];

		for(int i=100;i>=0;i--)
			m_aTOC[i]=(m_aTOC[i]-m_aTOC[0])/FullSizeValue;
	}

	return true;
}

long CMPAInfo::GetFilePositionFromTime(long Millis)
{
	long nPos=-1;
	double nTimeFraction;
	
	if(m_nDuration>0 && m_nEncodedDataSize>=0)
	{
		nTimeFraction=((double)Millis)/m_nDuration;

		if(m_bXingVBR)
		{
			nPos=GetEncodedDataPositionFromTOC((float)nTimeFraction);

			if(nPos>=m_nEncodedDataSize)
				nPos=m_nEncodedDataSize;
		}
		else
			nPos=(long)(nTimeFraction*m_nEncodedDataSize);
	}

	if(nPos==-1)
	{
		//ok, we couldn't find the correct position because
		//we don't know enough about the stream

		//however, ONE position we know: the beginning
		if(Millis==0)
			nPos=0;
	}

	nPos+=m_nEncodedDataOffset;

	return nPos;
}

long CMPAInfo::GetEncodedDataPositionFromTOC(float TimeFract)
{
	// interpolate in TOC to get file seek point in bytes
	int nTOCIndex;
	float LowerPosFract;
	float UpperPosFract;
	float PosFract;
	float Percent=TimeFract*100.0f;
	
	if(Percent<0.0f)
		Percent=0.0f;
	if(Percent>100.0f)
		Percent=100.0f;

	nTOCIndex=(int)Percent;
	if(nTOCIndex>99)
		nTOCIndex=99;
	LowerPosFract=m_aTOC[nTOCIndex];
	UpperPosFract=m_aTOC[nTOCIndex+1];
	
	PosFract=LowerPosFract + ((UpperPosFract-LowerPosFract)*(Percent-nTOCIndex));

	return (long)(PosFract*m_nEncodedDataSize);
}

