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


#ifndef _MEMBUFFER_H_
#define _MEMBUFFER_H_

class CMemBuffer
{
public:
	inline CMemBuffer()
	{
		m_pBuffer=(unsigned char*)0;
		m_nSize=0;
		m_nCapacity=0;
		m_bAllocated=false;
		m_nPos=0;
	}

	inline ~CMemBuffer()
	{
		Free();
	}

	inline void Alloc(int nBytes)
	{
		Free();
		if(nBytes>0)
		{
			m_pBuffer=new unsigned char[nBytes];
			m_bAllocated=true;
		}
		m_nCapacity=nBytes;		
	}

	inline void Free()
	{
		if(m_bAllocated)
			delete[] m_pBuffer;
		Detach();
	}

	inline void Attach(void* pMem,int nBytes)
	{
		Free();
		m_pBuffer=(unsigned char*)pMem;
		m_nSize=nBytes;
		m_nCapacity=nBytes;
	}

	inline void Detach()
	{		
		m_pBuffer=(unsigned char*)0;
		m_nSize=0;
		m_nCapacity=0;
		m_bAllocated=false;
		m_nPos=0;
	}

	inline int GetPos()
	{
		return m_nPos;
	}

	inline void SetPos(int Pos)
	{
		m_nPos=Pos;
	}

	inline void AddToPos(int Add)
	{
		m_nPos+=Add;
	}

	inline unsigned char ReadByte()
	{
		return m_pBuffer[m_nPos++];
	}

	inline void WriteByte(unsigned char Value)
	{
		m_pBuffer[m_nPos++]=Value;
	}

	inline int GetSize()
	{
		return m_nSize;
	}

	inline int GetCapacity()
	{
		return m_nCapacity;
	}

	inline int GetBytesLeft()
	{
		return m_nSize-m_nPos;
	}

	inline void SetSize(int nSize)
	{
		if(nSize>m_nCapacity)
			nSize=m_nCapacity;
		m_nSize=nSize;
		if(m_nPos>m_nSize)
			m_nPos=m_nSize;
	}

	inline unsigned char* GetPosPtr()
	{
		return &m_pBuffer[m_nPos];
	}

	inline unsigned char* GetPtr()
	{
		return m_pBuffer;
	}

	inline bool IsAttached()
	{
		return !m_bAllocated;
	}

	inline bool HasMoreData()
	{
		return m_nPos<m_nSize;
	}

protected:
	unsigned char* m_pBuffer;
	int m_nSize;
	int m_nCapacity;
	bool m_bAllocated;

	int m_nPos;
};

#endif
