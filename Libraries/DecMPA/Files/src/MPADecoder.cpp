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

//This file is a heavily modified version of SPlayPlugin.cpp from the original
//mpeglib. See Readme.txt for details.

#include "DefInc.h"
#include "MPADecoder.h"

#include "DecodeEngine.h"

#include <exception>
#include <memory>

#include <math.h>


CMPADecoder::CMPADecoder(const DecMPA_Callbacks& Callbacks,void* pCallbackContext)
{
	pow(6.0,3.0);            // fixes bug in __math.h

	m_Callbacks=Callbacks;
	m_pCallbackContext=pCallbackContext;

	m_pFileAccess=new CDecMPAFileAccess(Callbacks,pCallbackContext);

	m_nResyncCounter=0;

	m_nBufferBytes=0;
	m_nUsedBufferBytes=0;
	m_pBuffer=NULL;

	m_OutputFormat.nType=0;
	m_OutputFormat.nFrequency=0;
	m_OutputFormat.nChannels=0;
	m_bOutputFormatChanged=false;

	memset(&m_MPEGHeader,0,sizeof(m_MPEGHeader));

	m_bDoFloat=false;
	m_pPCMFrame=new PCMFrame(MP3FRAMESIZE);
	m_pFloatFrame=new FloatFrame(MP3FRAMESIZE);
	m_pAudioFrame=NULL;
	
	m_pFormatFrame=new AudioFrame;

	m_nReadTime=0;
	m_nBufferStartTime=0;
	m_nDecodeTime=0;

	m_bPrepared=false;
	m_bHasFrame=false;

	m_bFirstDecode=true;
	
	m_pDecodeEngine=DecodeEngine_Create();

	m_bStoreID3v2=false;
	m_pID3v2Data=(unsigned char*)0;
	m_nID3v2DataSize=0;

	m_pDestroyNotify=(void (*)(void*))0;
	m_pDestroyNotifyContext=(void*)0;

	m_aParams[DECMPA_PARAM_OUTPUT]=DECMPA_OUTPUT_INT16;
	m_aParams[DECMPA_PARAM_PROVIDEID3V2]=DECMPA_NO;

	m_bNotMPEG=false;

	m_bDecoderNeedsFlush=false;
}

CMPADecoder::~CMPADecoder()
{
	if(m_pDestroyNotify!=(void (*)(void*))0)
		m_pDestroyNotify(m_pDestroyNotifyContext);

	delete m_pPCMFrame;
	delete m_pFloatFrame;

	delete m_pFileAccess;
	delete m_pFormatFrame;

	DecodeEngine_Destroy(m_pDecodeEngine);
}

int CMPADecoder::DecodeNoData(long& nDecodedBytes)
{
	return Decode(NULL,-1,nDecodedBytes);
}

int CMPADecoder::Decode(void* pBuffer,long nBufferBytes,long& nBytesDecoded)
{
	int Result=DECMPA_OK;

	if(pBuffer==NULL)
		nBufferBytes=-1;

	nBytesDecoded=0;
	m_bOutputFormatChanged=false;	

	if((Result=EnsurePrepared())!=DECMPA_OK)
		return Result;

	try
	{
		while(!ReadDecodedData(pBuffer,nBufferBytes,nBytesDecoded))
		{
			Result=DecodeNextFrame(pBuffer==NULL);
			if(Result!=DECMPA_OK)
				break;
		}
	}
	catch(std::bad_alloc)
	{
		return DECMPA_ERROR_MEMORY;
	}
	catch(std::exception)
	{
		return DECMPA_ERROR_INTERNAL;
	}

	return Result;
}

int CMPADecoder::DecodeNextFrame(bool bNoData)
{
	int Result=DECMPA_OK;

	//find and read the next frame
	//we may still have one frame buffered that was read to find out
	//the file information. In that case don't read another one
	if(!m_bHasFrame)
		m_bHasFrame=ReadNextFrame();
	if(m_bHasFrame)
	{
		bool bDecodeOK;

		//don't use this frame again
		m_bHasFrame=false;

		//if bNoData==true we don't decode any audio data but only
		//parse the frame headers
		//this allows the user to do a quick pass over the file and
		//get an accurate estimate of the files properties
		//(actual duration, decoded size, etc...).
		if(!bNoData)
		{
			if(m_bDecoderNeedsFlush)
			{
				DecodeEngine_Flush(m_pDecodeEngine);
				m_bDecoderNeedsFlush=false;
			}

			bDecodeOK=DecodeEngine_Decode(m_pDecodeEngine,m_pAudioFrame,m_FrameFinder.GetFrameData(),m_FrameFinder.GetFrameSize());
		}
		else
		{
			MpegAudioHeader* pHeader=m_FrameFinder.GetFrameHeader();
			long nDecodedFrameSize;

			m_bDecoderNeedsFlush=true;

			nDecodedFrameSize=pHeader->getpcmperframe();
			if(pHeader->getInputstereo()==1)
				nDecodedFrameSize*=2;

			//pretend that we have decoded something
			m_pAudioFrame->setLen(nDecodedFrameSize);
			m_pAudioFrame->setFrameFormat(pHeader->getInputstereo(),pHeader->getFrequencyHz());

			//note that this "ghost data" will never be retrieved because the call
			//to ReadDecodedData that follows will always remove all of it
			//(see Decode)

			bDecodeOK=true;
		}
		
		if(bDecodeOK)
		{
			//make the decoded data available to the "user"
			HandleDecodedData(m_pAudioFrame);			
		}
		//ignore if the decoding of the frame failed. Just continue with
		//the next frame
	}
	else
	{
		if(m_FrameFinder.IsStreamInvalid())
			Result=DECMPA_ERROR_DECODE;
		else
		{
			if(m_pFileAccess->IsEndOfFile())
			{
				//end of file reached
				if(!m_FrameFinder.KnowsCharacteristics())
				{
					//the end was reached without being able to find out
					//essential stream characteristics
					//=> not an MPEG Audio file (or one that has less than 4 frames)
					Result=DECMPA_ERROR_DECODE;
				}
				else
					Result=DECMPA_END;
			}
			else
			{
				if(m_pFileAccess->GetLastError()!=DECMPA_OK)
					Result=m_pFileAccess->GetLastError();
				else
				{
					//should never happen - an invalid stream, end of file
					//or a read error are the only things that could cause
					//ReadNextFrame to fail
					Result=DECMPA_ERROR_INTERNAL;
				}
			}
		}
	}

	return Result;
}

void CMPADecoder::Flush()
{
	m_FrameFinder.Flush();
	DecodeEngine_Flush(m_pDecodeEngine);

	m_nBufferBytes=0;
	m_nUsedBufferBytes=0;
}

bool CMPADecoder::ReadDecodedData(void* pDest,long nBytes,long& nRead)
{
	long nAvailBytes=m_nBufferBytes-m_nUsedBufferBytes;

	if(nAvailBytes>0)
	{
		if(nBytes>nAvailBytes || nBytes==-1)
			nBytes=nAvailBytes;
		nBytes&=~m_nOutputBlockSize;	//only full samples

		if(pDest!=NULL)
			memcpy(pDest,m_pBuffer+m_nUsedBufferBytes,nBytes);
		m_nUsedBufferBytes+=nBytes;

		nRead=nBytes;

		m_nReadTime=m_nBufferStartTime+(((double)(m_nUsedBufferBytes/m_nOutputBlockSize))*1000)/m_pFormatFrame->getFrequenceHZ();

		return true;
	}
	else	
		return false;
}

long CMPADecoder::GetFilePositionFromTime(long Millis)
{
	int nResult;

	if((nResult=EnsurePrepared())!=DECMPA_OK)
		return nResult;

	return m_Info.GetFilePositionFromTime(Millis);
}

int CMPADecoder::SeekToTime(long Millis)
{
	int Result=DECMPA_OK;

	if(!m_pFileAccess->CanSeek())
		Result=DECMPA_ERROR_UNSUPPORTED;
	else
	{
		long nFilePos;

		if((Result=EnsurePrepared())==DECMPA_OK)
		{
			nFilePos=m_Info.GetFilePositionFromTime(Millis);
			if(nFilePos==-1)
				Result=DECMPA_ERROR_UNSUPPORTED;
			else
			{
				if(!m_pFileAccess->Seek(nFilePos))
					Result=DECMPA_ERROR_SEEK;
				else
				{
					//throw away buffered data
					Flush();

					if(Millis!=0)
					{
						//initiate resync (skip next 5 frames until back
						//references are ok again)
						m_nResyncCounter=5;
					}

					m_nDecodeTime=Millis;
					m_nBufferStartTime=Millis;				
					m_nReadTime=Millis;
				}
			}
		}
	}

	return Result;
}

int CMPADecoder::GetTime(long& Time)
{
	Time=(long)m_nReadTime;

	return DECMPA_OK;
}

int CMPADecoder::GetDuration(long& Duration)
{
	int nResult;

	try
	{
		if((nResult=EnsurePrepared())!=DECMPA_OK)
			return nResult;

		Duration=m_Info.GetDuration();

		return DECMPA_OK;
	}
	catch(std::bad_alloc)
	{
		return DECMPA_ERROR_MEMORY;
	}
	catch(std::exception)
	{
		return DECMPA_ERROR_INTERNAL;
	}
}

bool CMPADecoder::ReadNextFrame()
{
	while(!m_FrameFinder.ReadNextFrame())
	{
		if(m_FrameFinder.IsStreamInvalid())
			return false;

		if(!m_FrameFinder.ReadInput(m_pFileAccess))
			return false;
	}

	return true;
}

void CMPADecoder::SetOutputFormat(AudioFrame* pNewFormatFrame)
{
	pNewFormatFrame->copyFormat(m_pFormatFrame);

	m_OutputFormat.nFrequency=m_pFormatFrame->getFrequenceHZ();
	m_OutputFormat.nChannels=m_pFormatFrame->getStereo() ? 2 : 1;	

	m_bOutputFormatChanged=true;

	m_nOutputBlockSize=m_pFormatFrame->getSampleSize()/8;
	m_nOutputBlockSize*=m_OutputFormat.nChannels;
}

void CMPADecoder::UpdateMPEGHeader(MpegAudioHeader* pHeader)
{
	memcpy(m_MPEGHeader.aRawData,pHeader->getHeader(),4);
	m_MPEGHeader.bProtection=pHeader->getProtection()!=0;

	m_MPEGHeader.nLayer=pHeader->getLayer();
	m_MPEGHeader.nVersion=pHeader->getVersion();
	m_MPEGHeader.bPadding=pHeader->getPadding()!=0;
	m_MPEGHeader.nFrequencyIndex=pHeader->getFrequency();
	m_MPEGHeader.nFrequency=pHeader->getFrequencyHz();
	m_MPEGHeader.nBitRateIndex=pHeader->getBitrateindex();
	m_MPEGHeader.nExtendedMode=pHeader->getExtendedmode();
	m_MPEGHeader.nMode=pHeader->getMode();
	m_MPEGHeader.bInputStereo=pHeader->getInputstereo()!=0;
	m_MPEGHeader.bMPEG25=pHeader->getLayer25()!=0;

	m_MPEGHeader.nFrameSize=pHeader->getFramesize();
	m_MPEGHeader.nDecodedSamplesPerFrame=pHeader->getpcmperframe();
	m_MPEGHeader.nBitRateKbps=pHeader->GetBitRateKbps();
}

bool CMPADecoder::OutputFormatChanged()
{
	return m_bOutputFormatChanged;
}

void CMPADecoder::GetOutputFormat(DecMPA_OutputFormat& Format)
{
	Format=m_OutputFormat;
}

void CMPADecoder::GetMPEGHeader(DecMPA_MPEGHeader& Header)
{
	Header=m_MPEGHeader;
}


void CMPADecoder::HandleDecodedData(AudioFrame* pDecodedFrame)
{
	int nDecodedSamples;

	if(m_bFirstDecode)
	{
		SetOutputFormat(pDecodedFrame);
		m_bFirstDecode=false;
	}

	nDecodedSamples=pDecodedFrame->getLen();
	if(pDecodedFrame->getStereo()==1)
		nDecodedSamples/=2;

	m_nBufferStartTime=m_nDecodeTime;
	m_nDecodeTime+=(((double)nDecodedSamples)*1000)/pDecodedFrame->getFrequenceHZ();

	if(m_nResyncCounter>0)
	{
		//we need to resync = read a few frames to make sure
		//that the back references are ok again
		//do not output any of the decoded data
		m_nResyncCounter--;
	}
	else
	{
		if(!m_pFormatFrame->isFormatEqual(pDecodedFrame))
			SetOutputFormat(pDecodedFrame);

		UpdateMPEGHeader(m_FrameFinder.GetFrameHeader());

		if(m_bDoFloat)
		{
			m_nBufferBytes=m_pFloatFrame->getLen()*sizeof(float);
			m_pBuffer=(unsigned char*)m_pFloatFrame->getData();
		}
		else
		{
			m_nBufferBytes=m_pPCMFrame->getLen()*sizeof(short);
			m_pBuffer=(unsigned char*)m_pPCMFrame->getData();
		}
		m_nUsedBufferBytes=0;
	}
		
}

int CMPADecoder::EnsurePrepared()
{
	int Result=DECMPA_OK;

	if(!m_bPrepared)
	{
		if(m_bNotMPEG)
			Result=DECMPA_ERROR_DECODE;
		else
		{
			m_bStoreID3v2=(m_aParams[DECMPA_PARAM_PROVIDEID3V2]==DECMPA_YES);

			m_bDoFloat=(m_aParams[DECMPA_PARAM_OUTPUT]==DECMPA_OUTPUT_FLOAT);
			m_nOutputBlockSize=1;
			if(m_bDoFloat)
				m_pAudioFrame=m_pFloatFrame;
			else
				m_pAudioFrame=m_pPCMFrame;

			m_OutputFormat.nType=m_aParams[DECMPA_PARAM_OUTPUT];

			//Skip or store any leading ID3 tags - some tags are not
			//correctly escaped and contain "sync" codes that can confuse
			//the decoder
			HandleID3Tag();

			if(!m_Info.InitInfo(&m_FrameFinder,m_pFileAccess))
			{
				m_bNotMPEG=true;
				Result=DECMPA_ERROR_DECODE;
			}
			else
			{	
				//info reads up to one frame.
				//we can use this in decoding, we only have to remember not
				//to read a fresh one in the first iteration of Decode
				m_bHasFrame=m_FrameFinder.HasFrame();

				m_bPrepared=true;
			}
		}
	}

	return Result;
}

void CMPADecoder::HandleID3Tag()
{
	unsigned char aHeaderData[10];
	struct ID3v2Header
	{
		char sID[3];
		unsigned char Version;
		unsigned char Revision;
		unsigned char Flags;
		unsigned long Size;
	} Header;

	int nBytesRead;
	long UndoID;

	UndoID=m_pFileAccess->StartUndoRecording();

	if((nBytesRead=m_pFileAccess->Read((char*)&aHeaderData,10))==10)
	{
		//we do not read directly into the header because the fields may
		//have been padded by the compiler
		memcpy(Header.sID,aHeaderData,3);
		Header.Version=aHeaderData[3];
		Header.Revision=aHeaderData[4];
		Header.Flags=aHeaderData[5];
		Header.Size=aHeaderData[6];
		Header.Size<<=8;
		Header.Size|=aHeaderData[7];
		Header.Size<<=8;
		Header.Size|=aHeaderData[8];
		Header.Size<<=8;
		Header.Size|=aHeaderData[9];

		//make sure its an ID3 header
		if(strncmp(Header.sID,"ID3",3)==0 && Header.Version!=0xff && Header.Revision!=0xff
			&& (Header.Size & 0x80808080)==0)
		{
			unsigned long TagSize=Header.Size;

			//ok, we have an ID3 tag - no need for undo
			m_pFileAccess->EndUndoRecording(UndoID,false);
			UndoID=-1;
			
			//correct "unsyncing" of tag size
			TagSize=(TagSize & 0x7f) | ((TagSize & 0x7f00)>>1)
						| ((TagSize & 0x7f0000)>>2) | ((TagSize & 0x7f000000)>>3);

			if(m_bStoreID3v2)
			{
				//read ID3 tag

				m_pID3v2Data=new unsigned char[10+TagSize];
				memcpy(m_pID3v2Data,aHeaderData,10);
				
				if(m_pFileAccess->Read(m_pID3v2Data+10,TagSize)==(long)TagSize)
					m_nID3v2DataSize=TagSize+10;
				else
				{
					delete[] m_pID3v2Data;
					m_pID3v2Data=(unsigned char*)0;
				}
			}
			else
			{
				//skip the ID3 tag
				m_pFileAccess->Skip(TagSize);
			}
		}
	}

	if(UndoID!=-1)
	{
		//no ID3 tag found
		//=> undo read operations
		m_pFileAccess->EndUndoRecording(UndoID,true);
	}
}

int CMPADecoder::GetID3v2Data(unsigned char*& pData,long& nDataSize)
{
	int nResult;

	if((nResult=EnsurePrepared())!=DECMPA_OK)
		return nResult;

	if(m_pID3v2Data==(unsigned char*)0)
		return DECMPA_ERROR_NOTAVAILABLE;
	
	pData=m_pID3v2Data;
	nDataSize=m_nID3v2DataSize;

	return DECMPA_OK;
}


int CMPADecoder::SetParam(int ID,long Value)
{
	bool bOK=false;

	if(m_bPrepared)
		return DECMPA_ERROR_WRONGSTATE;
	
	switch(ID)
	{
	case DECMPA_PARAM_OUTPUT:		bOK=(Value==DECMPA_OUTPUT_INT16 || Value==DECMPA_OUTPUT_FLOAT);
		break;
	case DECMPA_PARAM_PROVIDEID3V2:	bOK=(Value==DECMPA_YES || Value==DECMPA_NO);
		break;
	}
	if(!bOK)
		return DECMPA_ERROR_PARAM;

	m_aParams[ID]=Value;
		
	return DECMPA_OK;

}

long CMPADecoder::GetParam(int ID)
{
	if(ID<0 || ID>=DECMPA_PARAMCOUNT)
		return 0;

	return m_aParams[ID];
}
