#include "libatrac9.h"
#include "structures.h"
#include <stdlib.h>
#include <string.h>
#include "decinit.h"
#include "decoder.h"

void* Atrac9GetHandle()
{
	struct atrac9_handle* handle = malloc(sizeof(atrac9_handle));
	memset(handle, 0, sizeof(atrac9_handle));
	return handle;
}

void Atrac9ReleaseHandle(void* handle)
{
	free(handle);
}

int Atrac9InitDecoder(void* handle, unsigned char * pConfigData)
{
	return init_decoder(handle, pConfigData, 16);
}

int Atrac9Decode(void* handle, const unsigned char *pAtrac9Buffer, short *pPcmBuffer, int *pNBytesUsed)
{
	return Decode(handle, pAtrac9Buffer, (unsigned char*)pPcmBuffer, pNBytesUsed);
}

int Atrac9GetCodecInfo(void* handle, Atrac9CodecInfo * pCodecInfo)
{
	return GetCodecInfo(handle, (CodecInfo*)pCodecInfo);
}
