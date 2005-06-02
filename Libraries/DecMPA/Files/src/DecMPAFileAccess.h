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

#ifndef _DECMPAFILEACCESS_H_
#define _DECMPAFILEACCESS_H_

#include "../include/decmpa.h"

#include "IFileAccess.h"

class CDecMPAFileAccess : public IFileAccess
{
public:
	CDecMPAFileAccess(const DecMPA_Callbacks& Callbacks,void* pContext);
	~CDecMPAFileAccess();

	int Read(void* pDest,int nBytes);
	long Skip(long nBytes);

	bool IsEndOfFile();

	bool CanSeek();
	bool Seek(long pos);

	long GetPosition();
	long GetLength();	

	long StartUndoRecording();
	void EndUndoRecording(long ID,bool bUndo);

	int GetLastError();

protected:
	void EnsureUndoBufferFree(long nBytes);

	DecMPA_Callbacks m_Callbacks;
	void* m_pCallbackContext;

	long m_nPosition;
	bool m_bEOF;

	unsigned char* m_pUndoBuffer;
	long m_nUndoBufferSize;
	long m_nUndoBufferFilled;
	long m_nUndoBufferPos;

	int m_nUndoRecordingCount;	

	unsigned char* m_pSkipBuffer;
	long m_nSkipBufferSize;

	int m_LastError;
};

#endif

