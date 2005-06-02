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


#include "splayDecoder.h"
#include "../DecodeEngine.h"

#include <new>

void* DecodeEngine_Create() 
{
	return new SplayDecoder();
}

void DecodeEngine_Destroy(void* pEng)
{
	delete (SplayDecoder*)pEng;
}

bool DecodeEngine_Decode(void* pEng,AudioFrame* pDest,void* pData,long nDataLength)
{
	SplayDecoder* pDecoder=(SplayDecoder*)pEng;

	return pDecoder->decode((unsigned char*)pData,nDataLength,pDest)!=0;
}

void DecodeEngine_Flush(void* pEngine)
{
	//splay doesn't store old data, so we don't have to do anything
}
