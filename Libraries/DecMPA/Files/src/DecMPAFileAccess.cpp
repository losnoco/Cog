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

#include "DefInc.h"
#include "DecMPAFileAccess.h"

#include <memory.h>

CDecMPAFileAccess::CDecMPAFileAccess(const DecMPA_Callbacks& Callbacks,void* pContext)
{
	m_Callbacks=Callbacks;
	m_pCallbackContext=pContext;

	m_nPosition=Callbacks.GetPosition(pContext);
	if(m_nPosition<0)
		m_nPosition=0;

	m_bEOF=false;

	m_pUndoBuffer=NULL;
	m_nUndoBufferSize=0;

	m_nUndoBufferFilled=0;	
	m_nUndoBufferPos=0;

	m_nUndoRecordingCount=0;

	m_LastError=DECMPA_OK;

	m_pSkipBuffer=NULL;
	m_nSkipBufferSize=0;
}

CDecMPAFileAccess::~CDecMPAFileAccess()
{
	if(m_pUndoBuffer!=NULL)
		delete[] m_pUndoBuffer;

	if(m_pSkipBuffer!=NULL)
		delete[] m_pSkipBuffer;
}

int CDecMPAFileAccess::Read(void* pDest,int nBytes)
{
	long nResult=0;
	long nRedoBytes=0;

	nRedoBytes=m_nUndoBufferFilled-m_nUndoBufferPos;
	if(nRedoBytes>0)
	{
		if(nRedoBytes>nBytes)
			nRedoBytes=nBytes;
		memcpy(pDest,m_pUndoBuffer+m_nUndoBufferPos,nRedoBytes);
		m_nUndoBufferPos+=nRedoBytes;
		nBytes-=nRedoBytes;
		pDest=((char*)pDest)+nRedoBytes;

		m_nPosition+=nRedoBytes;

		if(m_nUndoBufferPos==m_nUndoBufferFilled && m_nUndoRecordingCount==0)
		{
			m_nUndoBufferFilled=0;
			m_nUndoBufferPos=0;
		}

		nResult+=nRedoBytes;
	}
	
	if(nBytes>0)
	{
		long nRead;

		nRead=m_Callbacks.Read(m_pCallbackContext,pDest,nBytes);
		if(nRead>=0)
		{
			if(nRead!=nBytes)
				m_bEOF=true;
			m_nPosition+=nRead;

			if(m_nUndoRecordingCount>0)
			{
				EnsureUndoBufferFree(nRead);
				memcpy(m_pUndoBuffer+m_nUndoBufferPos,pDest,nRead);
				m_nUndoBufferFilled+=nRead;
				m_nUndoBufferPos+=nRead;
			}

			nResult+=nRead;
		}
		else
		{
			m_LastError=DECMPA_ERROR_READ;
			nResult=-1;
		}
	}

	return nResult;
}

long CDecMPAFileAccess::Skip(long nBytes)
{
	long nResult=0;
	long nRedoBytes=0;
	bool bSeekFailed=false;

	nRedoBytes=m_nUndoBufferFilled-m_nUndoBufferPos;
	if(nRedoBytes>0)
	{
		if(nRedoBytes>nBytes)
			nRedoBytes=nBytes;
		m_nUndoBufferPos+=nRedoBytes;
		nBytes-=nRedoBytes;

		m_nPosition+=nRedoBytes;

		if(m_nUndoBufferPos==m_nUndoBufferFilled && m_nUndoRecordingCount==0)
		{
			m_nUndoBufferFilled=0;
			m_nUndoBufferPos=0;
		}

		nResult+=nRedoBytes;
	}	

	if(nBytes>0)
	{
		//first try to skip using seek

		if(nBytes>1024 && m_Callbacks.Seek!=NULL && m_nUndoRecordingCount==0)
		{
			long nLength=GetLength();

			if(nLength!=-1)
			{
				long nToSkip=nBytes;

				if(m_nPosition+nToSkip>nLength)
					nToSkip=nLength-m_nPosition;

				if(m_Callbacks.Seek(m_pCallbackContext,m_nPosition+nToSkip)!=0)
				{
					m_nPosition+=nToSkip;
					nResult+=nToSkip;
					nBytes-=nToSkip;
				}
				else
				{
					//seek failed - not critical, we can still try to skip
					//using read.
					bSeekFailed=true;
				}
			}
		}
	}

	if(nBytes>0)
	{
		int nToRead;
		int nRead;

		if(m_pSkipBuffer==NULL)
		{
			m_nSkipBufferSize=8192;
			m_pSkipBuffer=new unsigned char[m_nSkipBufferSize];
		}

		while(nBytes>0 && !m_bEOF)
		{
			nToRead=(nBytes<=m_nSkipBufferSize) ? nBytes : m_nSkipBufferSize;

			nRead=m_Callbacks.Read(m_pCallbackContext,m_pSkipBuffer,nToRead);
			if(nRead>=0)
			{
				if(nRead!=nToRead)
					m_bEOF=true;
				m_nPosition+=nRead;

				if(m_nUndoRecordingCount>0)
				{
					EnsureUndoBufferFree(nRead);
					memcpy(m_pUndoBuffer+m_nUndoBufferPos,m_pSkipBuffer,nRead);
					m_nUndoBufferFilled+=nRead;
					m_nUndoBufferPos+=nRead;
				}

				nResult+=nRead;
				nBytes-=nRead;
			}
			else
			{
				if(bSeekFailed)
				{
					//if seek also failed before we report a seek error
					m_LastError=DECMPA_ERROR_SEEK;
				}
				else
					m_LastError=DECMPA_ERROR_READ;

				nResult=-1;
				break;
			}
		}
	}

	return nResult;
}

bool CDecMPAFileAccess::IsEndOfFile()
{
	return m_bEOF;
}

bool CDecMPAFileAccess::Seek(long pos)
{
	bool bResult=false;

	if(pos==m_nPosition)
		bResult=true;
	else
	{
		if(m_nUndoRecordingCount==0)
		{			
			if(m_Callbacks.Seek!=NULL)
			{
				if(m_Callbacks.Seek(m_pCallbackContext,pos)!=0)
				{
					m_nPosition=pos;
					bResult=true;
				}
				else
					m_LastError=DECMPA_ERROR_SEEK;
			}
		}
	}

	return bResult;
}

long CDecMPAFileAccess::GetPosition()
{
	return m_nPosition;
}

long CDecMPAFileAccess::GetLength()
{
	long nLength=-1;
	
	if(m_Callbacks.GetLength!=NULL)
		nLength=m_Callbacks.GetLength(m_pCallbackContext);

	return nLength;
}

long CDecMPAFileAccess::StartUndoRecording()
{
	m_nUndoRecordingCount++;

	return m_nUndoBufferPos;
}
	
void CDecMPAFileAccess::EndUndoRecording(long ID,bool bUndo)
{
	if(m_nUndoRecordingCount>0)
	{
		if(bUndo)
		{
			m_nPosition-=m_nUndoBufferPos-ID;
			if(m_nUndoBufferPos!=ID)
				m_bEOF=false;

			m_nUndoBufferPos=ID;
		}

		m_nUndoRecordingCount--;
	}
}

void CDecMPAFileAccess::EnsureUndoBufferFree(long nBytes)
{
	if(m_nUndoBufferPos+nBytes>m_nUndoBufferSize)
	{
		unsigned char* pNew;
		long nAdd=m_nUndoBufferPos+nBytes-m_nUndoBufferSize;

		if((nAdd & 8191)!=0)
			nAdd+=8192-(nAdd & 8191);

		pNew=new unsigned char[m_nUndoBufferSize+nAdd];
		memcpy(pNew,m_pUndoBuffer,m_nUndoBufferFilled);
		m_nUndoBufferSize+=nAdd;

		if(m_pUndoBuffer!=NULL)
			delete[] m_pUndoBuffer;
		m_pUndoBuffer=pNew;
	}
}

int CDecMPAFileAccess::GetLastError()
{
	return m_LastError;
}

bool CDecMPAFileAccess::CanSeek()
{
	return (m_Callbacks.Seek!=NULL);
}

