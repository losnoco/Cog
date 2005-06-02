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


#include "MPAFrameFinder.h"

#include <memory.h>

CMPAFrameFinder::CMPAFrameFinder()
{
	// max size of buffer is:
	// header:          4
	// max bitrate:   448
	// min freq:    22050
	// padding:         1
	// ------------------
	// maxsize:    4+144000*max(bitrate)/min(freq)+1 ca: 2931 byte
	// then we add a "sentinel" at the end these are 4 byte.
	// so we should be ok, with a 4KB buffer.
	m_OutBuffer.Alloc(4096);
	m_OutBuffer.SetSize(4096);

	m_MyInBuffer.Alloc(65536);

	m_pFirstHeader=(HeaderRecord*)0;
	m_pLastHeader=(HeaderRecord*)0;
	m_pNewHeaderRecord=(HeaderRecord*)0;

	m_bStreamInvalid=false;
	m_nLayer=-1;

	m_bAllowFrequencyChanges=false;
	
	m_nLookAheadFrames=3; //try to find 4 consistent frames by default

	m_nReadStartPosition=0;
	
	Restart();
}

CMPAFrameFinder::~CMPAFrameFinder()
{
	DeleteHeaderRecords();
}

bool CMPAFrameFinder::ReadNextFrame()
{
	bool bNeedMoreThanAvailable;

	if(m_State==STATE_HAVEFRAME)
		ResetState();

	while(true)
	{
		bNeedMoreThanAvailable=false;
		while(m_State!=STATE_HAVEFRAME && m_pInBuffer->HasMoreData() && !bNeedMoreThanAvailable)
		{
			switch(m_State)
			{
			case STATE_FINDCHARACTERISTICS:	if(!FindCharacteristics())
											{
												//need more data, but we may still have 
												//up to 3 bytes buffered
												//so, break manually here
												bNeedMoreThanAvailable=true;
											}
				break;
			case STATE_FINDSYNC:	FindNextSync();
				break;
			case STATE_READHEADER:	ReadHeader();
				break;
			case STATE_CHECKHEADER:	CheckHeader();
				break;
			case STATE_READFRAME:	ReadFrame();
				break;
			}
		}

		if(m_State!=STATE_HAVEFRAME && m_State>=STATE_FINDSYNC && m_pInBuffer==&m_MyInBuffer)
		{
			//we need more data, but were still reading
			//from our own preread data that was used to find out the
			//stream characteristics

			//switch to user buffer (it may already contain some data)
			m_pInBuffer=&m_UserInBuffer;
		}
		else
			break;
	}

	if(m_State==STATE_FINDCHARACTERISTICS && m_MyInBuffer.GetSize()==m_MyInBuffer.GetCapacity())
	{
		//we have read the maximum of data and not found m_nLookAheadFrames+1 consistent
		//frames

		//the stream does probably not contain valid MPEG Audio data
		m_bStreamInvalid=true;
	}

	return (m_State==STATE_HAVEFRAME);	//otherwise we need more data
}

bool CMPAFrameFinder::FindCharacteristics()
{
	unsigned char* pData;

	//we read up to 64k data and try to find at least
	//m_nLookAheadFrames+1 frame headers that are consistent with one
	//another

	while(m_MyInBuffer.GetBytesLeft()>=4)
	{
		pData=m_MyInBuffer.GetPosPtr();

		if(pData[0]==0xff && (pData[1] & 0xe0)==0xe0)
		{
			//found sync
			//parse the header
			if(m_pNewHeaderRecord==(HeaderRecord*)0)
				m_pNewHeaderRecord=new HeaderRecord;

			if(m_pNewHeaderRecord->Header.parseHeader(pData))
			{
				if(IsValidHeader(&m_pNewHeaderRecord->Header))
				{
					//seems to be a valid header.
					//add the a header record to the list

					m_pNewHeaderRecord->nPosition=m_MyInBuffer.GetPos();
					m_pNewHeaderRecord->pNext=(HeaderRecord*)0;

					if(m_pLastHeader==(HeaderRecord*)0)
						m_pFirstHeader=m_pLastHeader=m_pNewHeaderRecord;
					else
					{
						m_pLastHeader->pNext=m_pNewHeaderRecord;
						m_pLastHeader=m_pNewHeaderRecord;
					}
					m_pNewHeaderRecord=(HeaderRecord*)0;

					//check wether we have found m_nLookAheadFrames+1 consistent headers yet
					if(FindConsistentHeaders())
						break;
				}
			}
		}

		m_MyInBuffer.AddToPos(1);
	}

	return (m_State>=STATE_FINDSYNC);
}

bool CMPAFrameFinder::FindConsistentHeaders()
{
	HeaderRecord* pRecord;

	//ok, we have found some headers in the data stream
	//now we try to find m_nLookAheadFrames+1 that are consistent
	//and assume that they correspond to valid frames in the stream

	pRecord=m_pFirstHeader;
	while(pRecord!=(HeaderRecord*)0)
	{
		//be prejudiced for early headers
		if(!CanHaveConsistentHeaders(pRecord,m_nLookAheadFrames))
		{
			//if we haven't yet read enough data to find
			//consistent headers for the first headers, stop
			//here
			pRecord=(HeaderRecord*)0;
			break;
		}

		//find two headers that are consistent with this one
		if(HasConsistentHeaders(pRecord,pRecord->pNext,m_nLookAheadFrames))
			break;

		pRecord=pRecord->pNext;
	}

	if(pRecord!=(HeaderRecord*)0)
	{
		//yay! we found m_nLookAheadFrames+1 consistent headers.
		//The first of them is pRecord

		//store stream characteristics
		m_nLayer=pRecord->Header.getLayer();
		m_nInputStereo=pRecord->Header.getInputstereo();
		m_nFrequency=pRecord->Header.getFrequencyHz();
		m_nLayer25=pRecord->Header.getLayer25();
		m_nVersion=pRecord->Header.getVersion();

		//set input buffer to start of first frame
		m_MyInBuffer.SetPos(pRecord->nPosition);

		m_nFirstFramePosition=pRecord->nPosition+m_nReadStartPosition;

		//throw away the header records
		DeleteHeaderRecords();

		//got to next state (start extracting frame data)
		m_State++;

		return true;
	}

	return false;
}

bool CMPAFrameFinder::CanHaveConsistentHeaders(CMPAFrameFinder::HeaderRecord* pRecord,int nHeaderCount)
{
	long nFirstFrameBegin;
	int nFrameSize;
	long nLastFrameHeaderEnd;

	nFirstFrameBegin=pRecord->nPosition;
	nFrameSize=pRecord->Header.getFramesize();

	//assume that the frames have a size difference of at most 512 bytes
	//(or at most 512 bytes of data in between)
	nLastFrameHeaderEnd=nFirstFrameBegin+((nFrameSize+512)*nHeaderCount)+4;

	if(m_MyInBuffer.GetPos()<nLastFrameHeaderEnd)
	{
		//abort because we have not yet read parsed enough data
		return false;
	}

	return true;
}

bool CMPAFrameFinder::HasConsistentHeaders(CMPAFrameFinder::HeaderRecord* pCheckRecord,HeaderRecord* pRecord,int nHeadersNeeded)
{
	while(pRecord!=(HeaderRecord*)0)
	{
		if(AreHeadersConsistent(pCheckRecord,pRecord))
		{
			nHeadersNeeded--;
			if(nHeadersNeeded==0)
				return true;

			pCheckRecord=pRecord;
		}

		pRecord=pRecord->pNext;
	}

	return false;
}

bool CMPAFrameFinder::AreHeadersConsistent(HeaderRecord* pFirst,HeaderRecord* pSecond)
{
	//do the corresponding frames overlap?
	if(pFirst->nPosition+pFirst->Header.getFramesize()>pSecond->nPosition)
		return false;

	//different layers?
	if(pFirst->Header.getLayer()!=pSecond->Header.getLayer())
		return false;

	//one stereo, the other not?
	if(pFirst->Header.getInputstereo()!=pSecond->Header.getInputstereo())
		return false;

	//mpeg 2.5?
	if(pFirst->Header.getLayer25()!=pSecond->Header.getLayer25())
		return false;

	//version?
	if(pFirst->Header.getVersion()!=pSecond->Header.getVersion())
		return false;

	if(!m_bAllowFrequencyChanges)
	{
		if(pFirst->Header.getFrequencyHz()!=pSecond->Header.getFrequencyHz())
			return false;
	}

	return true;
}

bool CMPAFrameFinder::IsConformingHeader(MpegAudioHeader* pHeader)
{
	//check wether the header conforms with the detected stream characteristics
	
	if(pHeader->getInputstereo()==m_nInputStereo
		&& pHeader->getLayer()==m_nLayer
		&& pHeader->getLayer25()==m_nLayer25
		&& pHeader->getVersion()==m_nVersion
		&& (pHeader->getFrequencyHz()==m_nFrequency || m_bAllowFrequencyChanges))
	{
		return true;
	}

	return false;
}

void CMPAFrameFinder::FindNextSync()
{
	unsigned char* pOut=m_OutBuffer.GetPosPtr();

	while(m_pInBuffer->HasMoreData())
	{
		// shift
		pOut[0]=pOut[1];
		pOut[1]=m_pInBuffer->ReadByte();
		
		if (pOut[0] == 0xff)
		{
			// upper 4 bit are syncword, except bit one
			// which is layer 2.5 indicator.
			if ( (pOut[1] & 0xe0) == 0xe0)
			{
				m_OutBuffer.SetPos(2);
				m_State++;
				break;
			}
		}
	}
}

void CMPAFrameFinder::ReadHeader()
{
	while(m_pInBuffer->HasMoreData())
	{
		if(m_OutBuffer.GetPos()>=4)
		{
			m_State++;
			break;
		}

		m_OutBuffer.WriteByte(m_pInBuffer->ReadByte());		
	}
}

void CMPAFrameFinder::CheckHeader()
{
	bool bHeaderOK=false;

	if(m_Header.parseHeader(m_OutBuffer.GetPtr()))
	{
		if(IsValidHeader(&m_Header))
		{
			//make sure the header conforms to the established stream
			//characteristics
			if(IsConformingHeader(&m_Header))
			{
				m_nFrameSize=m_Header.getFramesize();
				bHeaderOK=true;
			}
		}
	}

	if(bHeaderOK)
		m_State++;
	else
		ResetState();
}

void CMPAFrameFinder::ReadFrame()
{
	int nBytesNeeded;
	int nBytesAvailable;
	int nCopyBytes;

	do
	{
		nBytesNeeded=m_nFrameSize-m_OutBuffer.GetPos();		
		nBytesAvailable=m_pInBuffer->GetSize()-m_pInBuffer->GetPos();
		
		nCopyBytes=(nBytesAvailable<nBytesNeeded) ? nBytesAvailable : nBytesNeeded;

		memcpy(m_OutBuffer.GetPosPtr(),m_pInBuffer->GetPosPtr(),nCopyBytes);
		m_OutBuffer.AddToPos(nCopyBytes);
		m_pInBuffer->AddToPos(nCopyBytes);

		if(nBytesNeeded==nCopyBytes)
		{
			m_State++;
			break;
		}
	}
	while(m_pInBuffer->HasMoreData());
}

void CMPAFrameFinder::SetInput(void* pBuffer,int nBufferBytes,long nStreamPosition)
{
	if(m_State==STATE_FINDCHARACTERISTICS)
	{
		int nCopyBytes=m_MyInBuffer.GetCapacity()-m_MyInBuffer.GetSize();

		//copy it in into our own buffer
		if(nCopyBytes>nBufferBytes)
			nCopyBytes=nBufferBytes;

		if(m_MyInBuffer.GetSize()==0)
			m_nReadStartPosition=nStreamPosition;

		memcpy(m_MyInBuffer.GetPtr()+m_MyInBuffer.GetSize(),pBuffer,nCopyBytes);
		m_MyInBuffer.SetSize(m_MyInBuffer.GetSize()+nCopyBytes);

		pBuffer=((unsigned char*)pBuffer)+nCopyBytes;
		nBufferBytes-=nCopyBytes;

		//attach the rest to the user buffer
	}

	m_UserInBuffer.Attach(pBuffer,nBufferBytes);
}

bool CMPAFrameFinder::ReadInput(IFileAccess* pFileAccess)
{
	int nResult;

	if(m_State==STATE_FINDCHARACTERISTICS)
	{
		int nFree=m_MyInBuffer.GetCapacity()-m_MyInBuffer.GetSize();

		//read data into our own buffer
		if(nFree>0)
		{
			//read in small chunks
			if(nFree>4096)
				nFree=4096;

			if(m_MyInBuffer.GetSize()==0)	//first input
				m_nReadStartPosition=pFileAccess->GetPosition();

			nResult=pFileAccess->Read((char*)m_MyInBuffer.GetPtr()+m_MyInBuffer.GetSize(),nFree);
			if(nResult<=0)
				return false;

			m_MyInBuffer.SetSize(m_MyInBuffer.GetSize()+nResult);
		}				
	}
	else
	{
		if(m_UserInBuffer.IsAttached())
			m_UserInBuffer.Alloc(4096);
		
		nResult=pFileAccess->Read((char*)m_UserInBuffer.GetPtr(),m_UserInBuffer.GetCapacity());
		if(nResult<=0)
			return false;

		m_UserInBuffer.SetSize(nResult);
		m_UserInBuffer.SetPos(0);
	}

	return true;
}

void CMPAFrameFinder::Flush()
{
	ResetState();

	m_UserInBuffer.SetSize(0);
	m_MyInBuffer.SetSize(0);

	DeleteHeaderRecords();
}

void CMPAFrameFinder::ResetState()
{
	m_OutBuffer.SetPos(0);

	//make sure that we do not accidentally treat
	//"leftover" data in the buffer as a sync mark
	m_OutBuffer.GetPtr()[0]=0;
	m_OutBuffer.GetPtr()[1]=0;

	if(m_State>=STATE_FINDSYNC)
		m_State=STATE_FINDSYNC;
}

void CMPAFrameFinder::Restart()
{
	Flush();

	m_State=STATE_FINDCHARACTERISTICS;
	m_pInBuffer=&m_MyInBuffer;

	m_bStreamInvalid=false;

	m_nFirstFramePosition=0;
}

void CMPAFrameFinder::DeleteHeaderRecords()
{
	HeaderRecord* pRecord;

	while(m_pFirstHeader!=(HeaderRecord*)0)
	{
		pRecord=m_pFirstHeader;
		m_pFirstHeader=m_pFirstHeader->pNext;

		delete pRecord;
	}

	m_pLastHeader=(HeaderRecord*)0;

	if(m_pNewHeaderRecord!=(HeaderRecord*)0)
	{
		delete m_pNewHeaderRecord;
		m_pNewHeaderRecord=(HeaderRecord*)0;
	}
}

bool CMPAFrameFinder::IsValidHeader(MpegAudioHeader* pHeader)
{
	int nFrameSize;

	nFrameSize=pHeader->getFramesize();
		
	// don't allow stupid framesizes:
	// if framesize <4 or > max mepg framsize its an error
	if(nFrameSize<4 || nFrameSize+4>4096)
		return false;

	if(pHeader->GetBitRateKbps()==0)
	{
		//skip frames with bitrate 0 (empty)
		return false;
	}
	
	return true;
}



