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
#include "../include/decmpa.h"
#include "MPADecoder.h"

#include <exception>

#include <stdio.h>


#if defined(__GNUC__) && (defined(_WINDOWS) || defined(_WIN32))
	#define DLLEXPORT __declspec(dllexport)
#else
	#define DLLEXPORT
#endif

long DECMPA_CC DecMPA_StdioRead(void* pContext,void* pBuffer,long nBytes)
{
	long nBytesRead;

	nBytesRead=fread(pBuffer,1,nBytes,(FILE*)pContext);
	if(nBytesRead!=nBytes)
	{
		if(ferror((FILE*)pContext))
			return -1;
	}

	return nBytesRead;
}


int DECMPA_CC DecMPA_StdioSeek(void* pContext,long DestPos)
{
	if(fseek((FILE*)pContext,DestPos,SEEK_SET)!=0)
		return 0;	//error
	else
		return 1;	//no error
}

long DECMPA_CC DecMPA_StdioGetLength(void* pContext)
{
	long nOldPos;
	long nLength=-1;
	
	nOldPos=ftell((FILE*)pContext);
	if(fseek((FILE*)pContext,0,SEEK_END)==0)
	{
		nLength=ftell((FILE*)pContext);
		fseek((FILE*)pContext,nOldPos,SEEK_SET);
	}

	return nLength;
}

long DECMPA_CC DecMPA_StdioGetPosition(void* pContext)
{
	return ftell((FILE*)pContext);	
}

void DecMPA_StdioDestroyNotify(void* pContext)
{
	fclose((FILE*)pContext);
}

DLLEXPORT int DECMPA_CC DecMPA_CreateUsingFile(void** ppDecoder,const char* sFilePath,int Version)
{
	FILE* pFile;
	DecMPA_Callbacks Callbacks;
	int nResult;

	if(Version>DECMPA_VERSION)
		return DECMPA_ERROR_INCOMPATIBLEVERSION;

	pFile=fopen(sFilePath,"rb");
	if(pFile==NULL)
		return DECMPA_ERROR_OPENFILE;

	Callbacks.Read=DecMPA_StdioRead;
	Callbacks.Seek=DecMPA_StdioSeek;
	Callbacks.GetLength=DecMPA_StdioGetLength;
	Callbacks.GetPosition=DecMPA_StdioGetPosition;

	nResult=DecMPA_CreateUsingCallbacks(ppDecoder,&Callbacks,pFile,Version);
	if(nResult!=DECMPA_OK)
		fclose(pFile);

	((CMPADecoder*)(*ppDecoder))->SetDestroyNotify(DecMPA_StdioDestroyNotify,pFile);

	return nResult;
}


DLLEXPORT int DECMPA_CC DecMPA_CreateUsingCallbacks(void** ppDecoder,const DecMPA_Callbacks* pCallbacks,void* pContext,int Version)
{
	DecMPA_Callbacks Callbacks=*pCallbacks;

	if(Callbacks.Read==NULL)
	{
		Callbacks.Read=DecMPA_StdioRead;
		Callbacks.Seek=NULL;
		Callbacks.GetLength=NULL;
		Callbacks.GetPosition=DecMPA_StdioGetPosition;

		pContext=stdin;
	}

	if(Version>DECMPA_VERSION)
		return DECMPA_ERROR_INCOMPATIBLEVERSION;

	if(ppDecoder==NULL || Callbacks.GetPosition==NULL)
		return DECMPA_ERROR_PARAM;

	try
	{
		*ppDecoder=new CMPADecoder(Callbacks,pContext);

		return DECMPA_OK;
	}
	catch(std::bad_alloc e)
	{
		return DECMPA_ERROR_MEMORY;
	}
	catch(std::exception e)
	{
		return DECMPA_ERROR_INTERNAL;
	}
}

DLLEXPORT int DECMPA_CC DecMPA_SetParam(void* pDecoder,int ID,long Value)
{
	if(pDecoder==NULL)
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->SetParam(ID,Value);
}

DLLEXPORT long DECMPA_CC DecMPA_GetParam(void* pDecoder,int ID)
{
	if(pDecoder==NULL)
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->GetParam(ID);
}

DLLEXPORT int DECMPA_CC DecMPA_Decode(void* pDecoder,void* pBuffer,long nBufferBytes,long* pBytesDecoded)
{
	if(pDecoder==NULL || pBytesDecoded==NULL || (pBuffer==NULL && nBufferBytes!=0))
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->Decode(pBuffer,nBufferBytes,*pBytesDecoded);
}

DLLEXPORT int DECMPA_CC DecMPA_DecodeNoData(void* pDecoder,long* pDecodedBytes)
{
	if(pDecoder==NULL || pDecodedBytes==NULL)
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->DecodeNoData(*pDecodedBytes);
}

DLLEXPORT int DECMPA_CC DecMPA_SeekToTime(void* pDecoder,long Millis)
{
	if(pDecoder==NULL)
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->SeekToTime(Millis);
}

DLLEXPORT int DECMPA_CC DecMPA_GetTime(void* pDecoder,long* pTime)
{
	if(pDecoder==NULL || pTime==NULL)
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->GetTime(*pTime);
}

DLLEXPORT int DECMPA_CC DecMPA_GetDuration(void* pDecoder,long* pDuration)
{
	if(pDecoder==NULL || pDuration==NULL)
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->GetDuration(*pDuration);
}

DLLEXPORT int DECMPA_CC DecMPA_GetOutputFormat(void* pDecoder,DecMPA_OutputFormat* pFormat)
{
	if(pDecoder==NULL || pFormat==NULL)
		return DECMPA_ERROR_PARAM;

	((CMPADecoder*)pDecoder)->GetOutputFormat(*pFormat);

	return DECMPA_OK;
}

DLLEXPORT int DECMPA_CC DecMPA_OutputFormatChanged(void* pDecoder)
{
	if(pDecoder==NULL)
		return 0;

	return ((CMPADecoder*)pDecoder)->OutputFormatChanged() ? 1 : 0;
}

DLLEXPORT int DECMPA_CC DecMPA_GetMPEGHeader(void* pDecoder,DecMPA_MPEGHeader* pHeader)
{
	if(pDecoder==NULL || pHeader==NULL)
		return DECMPA_ERROR_PARAM;

	((CMPADecoder*)pDecoder)->GetMPEGHeader(*pHeader);

	return DECMPA_OK;
}

DLLEXPORT int DECMPA_CC DecMPA_GetID3v2Data(void* pDecoder,unsigned char** ppData,long* pDataSize)
{
	if(pDecoder==NULL || ppData==NULL || pDataSize==NULL)
		return DECMPA_ERROR_PARAM;

	return ((CMPADecoder*)pDecoder)->GetID3v2Data(*ppData,*pDataSize);
}

DLLEXPORT void DECMPA_CC DecMPA_Destroy(void* pDecoder)
{
	if(pDecoder!=NULL)
	{
		try
		{
			delete (CMPADecoder*)pDecoder;
		}
		catch(std::exception)
		{
		}
	}
}

DLLEXPORT int DECMPA_CC DecMPA_GetVersion(void)
{
	return DECMPA_VERSION;
}


