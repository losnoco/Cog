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


#ifndef _DECODEENGINE_H_
#define _DECODEENGINE_H_

#include "frame/audioFrame.h"

void* DecodeEngine_Create();

bool DecodeEngine_Decode(void* pEngine,AudioFrame* pDest,void* pData,long nDataLength);
void DecodeEngine_Flush(void* pEngine);

void DecodeEngine_Destroy(void* pEngine);

#endif
