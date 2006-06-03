/*
	Millennium Sound System
	©1999, Subband Software
	
	Description:	GP Ring Buffer
	Released:		9/15/99

	Version history:
	
	Date		Who		Changes
	--------+---------+------------------------------------------------------
	09-15-99	DAB		Initial release.

	DAB = Dmitry Boldyrev
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

#ifndef	__RINGBUFFER_H__
#define	__RINGBUFFER_H__

class RingBuffer
{
	public:
					RingBuffer();
				   ~RingBuffer();

	int				Init(long inSize);

	long			WriteData(char *data, long len);
	long			ReadData(char *data, long len);

	long			FreeSpace(bool inTrueSpace = true);
	long			UsedSpace(bool inTrueSpace = true);
	long			BufSize()
					{
						return mBufSize;
					}
	
	void			Empty();
	
	void			SaveRead();
	void			RestoreRead();
	
	bool			IsReadMode()
					{
						return mSaveFreeSpace == -1;
					}

	protected:
	
	char*			mBuffer;
	long			mBufSize;
	long			mBufWxIdx;
	
	long			mBufRdIdx;
	long			mFreeSpace;
	
	long			mSaveFreeSpace;
	long			mSaveReadPos;
};

#endif //__RINGBUFFER_H__
