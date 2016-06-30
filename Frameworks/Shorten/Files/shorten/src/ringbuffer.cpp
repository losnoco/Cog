/*
	Millennium Sound System
	©1999-2000 Subband Software, Inc.
	
	Description:	GP Ring Buffer class
	Released:		9/15/99

	Version history:
	
	Date		Who		Changes
	--------+---------+------------------------------------------------------
	06-19-00	AVL		parameter types changes, conflicting function names fixed,
						IsFreeSpace() changed to IsReadMode(), ReadDataCR() removed.
	09-15-99	DAB		Initial release.

	DAB = Dmitry Boldyrev
	AVL	= Alex Lagutin
*/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include "ringbuffer.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

RingBuffer::RingBuffer()
{
	mFreeSpace=0;
	mBuffer=NULL;

	mBufSize=0;
	mBufRdIdx=0;
	mBufWxIdx=0;
	
	mSaveFreeSpace=-1;
}

int RingBuffer::Init(long inSize)
{
	mBufSize=inSize;
	if (mBuffer) {
		delete[] mBuffer;
	}
	if ((mBuffer = new char[mBufSize]) == NULL)
		return -1;
		
	Empty();
	
	return 0;
}

void 
RingBuffer::Empty()
{
	mBufRdIdx=0;
	mBufWxIdx=0;
	mFreeSpace=mBufSize;
	mSaveFreeSpace=-1;
	
//	if (mBuffer) {
//		memset(mBuffer, 0, mBufSize);
//	}
}
	
RingBuffer::~RingBuffer()
{
	if (mBuffer) {
		delete[] mBuffer;
	}
	mBuffer=NULL;
}


long RingBuffer::WriteData(char *data, long len)
{
	int written, before;
	
	if (!mBufSize || data==NULL || !mBuffer)
		return 0;
	
	written=0; 
	if (!(len = MIN(FreeSpace(false), len))) 
		return 0;

	while (1)
	{
		if (len + mBufWxIdx < mBufSize)
		{
			::memcpy(&mBuffer[mBufWxIdx], &data[written], (size_t) len);		
			mBufWxIdx  += len;
			written    += len;
			mFreeSpace -= len;
			if (mSaveFreeSpace != -1)
				mSaveFreeSpace -= len;

			return written;
		} else
		{
			before = (int)(mBufSize - mBufWxIdx);
			::memcpy(&mBuffer[mBufWxIdx], &data[written], (size_t) before);
			written    += before;
			len        -= before;
			mFreeSpace -= before;
			if (mSaveFreeSpace != -1)
				mSaveFreeSpace -= before;
			
			mBufWxIdx = 0;
		}
	}
}

void RingBuffer::SaveRead()
{
	if (mSaveFreeSpace!=-1)
		return;
	mSaveReadPos=mBufRdIdx;
	mSaveFreeSpace=mFreeSpace;
}

void RingBuffer::RestoreRead()
{
	if (mSaveFreeSpace==-1)
		return;
	mBufRdIdx=mSaveReadPos;
	mFreeSpace=mSaveFreeSpace;
	mSaveFreeSpace=-1;
}

long RingBuffer::ReadData(char *data, long len)
{
	int read, before;
	
	if (!mBufSize) return 0;
	
	read = 0;
	if (!(len  = MIN(UsedSpace(true), len))) 
		return 0;
	
	while (1)
	{	
		if (len + mBufRdIdx < mBufSize)
		{
			if (data) {
				::memcpy(&data[read], &mBuffer[mBufRdIdx], (size_t) len);
			}
			mBufRdIdx  += len;
			read       += len;
			mFreeSpace += len;
			
			return read;
		} else
		{
			before = (int)(mBufSize - mBufRdIdx);
			if (data) {
				::memcpy(&data[read], &mBuffer[mBufRdIdx], (size_t) before);		
			}
			read       += before;
			len  	   -= before;
			mFreeSpace += before;
			
			mBufRdIdx = 0;
		}
	}
}

long RingBuffer::FreeSpace(bool inTrueSpace)
{ 
	if (inTrueSpace)
		return mFreeSpace;
	else
		return (mSaveFreeSpace==-1) ? mFreeSpace : mSaveFreeSpace;
}

long RingBuffer::UsedSpace(bool inTrueSpace)
{ 
	if (inTrueSpace)
		return (mBufSize - mFreeSpace);
	else 
		return (mSaveFreeSpace==-1) ? (mBufSize - mFreeSpace) : 
			(mBufSize - mSaveFreeSpace);
}
