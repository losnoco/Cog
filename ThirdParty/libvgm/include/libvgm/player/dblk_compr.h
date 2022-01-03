#ifndef __DBLK_COMPR_H__
#define __DBLK_COMPR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"
#include "../common_def.h"

typedef struct _pcm_compression_table
{
	UINT8 comprType;
	UINT8 cmpSubType;
	UINT8 bitsDec;
	UINT8 bitsCmp;
	UINT16 valueCount;
	union
	{
		UINT8* d8;
		UINT16* d16;	// note: stored in Native Endian
	} values;
} PCM_COMPR_TBL;

typedef struct _compression_parameters
{
	// Compression Types:
	//	00 - bit packing
	//	01 - Delta-PCM
	UINT8 comprType;
	UINT8 subType;	// compression sub-type
	UINT8 bitsDec;	// bits per value (decompressed)
	UINT8 bitsCmp;	// bits per value (compressed)
	UINT16 baseVal;
	const PCM_COMPR_TBL* comprTbl;
} PCM_CMP_INF;

typedef struct _pcm_compr_datablk_info
{
	// general data
	UINT32 hdrSize;		// number of bytes taken by the compression header in the VGM *1
	UINT32 decmpLen;	// size of decompressed data *2
	// *1 written by Read/WriteComprDataBlkHdr
	// *2 set by ReadComprDataBlkHdr, read by WriteComprDataBlkHdr
	
	// actual parameters required for (De-)CompressDataBlk
	PCM_CMP_INF cmprInfo;
} PCM_CDB_INF;

// small functions to help calculating data sizes
// Bit Packing/DPCM:
INLINE UINT32 BPACK_SIZE_CMP(UINT32 sizeDec, UINT32 bitsCmp, UINT32 bitsDec)
{
	UINT32 byteBits = (bitsDec + 7) & ~7;
	return (UINT32)(((UINT64)sizeDec * bitsCmp + 7) / byteBits);
}

INLINE UINT32 BPACK_SIZE_DEC(UINT32 sizeCmp, UINT32 bitsCmp, UINT32 bitsDec)
{
	UINT32 byteBits = (bitsDec + 7) & ~7;
	return (UINT32)((UINT64)sizeCmp * byteBits / bitsCmp);
}

UINT8 ReadComprDataBlkHdr(UINT32 inLen, const UINT8* inData, PCM_CDB_INF* retCdbInf);
UINT8 WriteComprDataBlkHdr(UINT32 outLen, UINT8* outData, PCM_CDB_INF* cdbInf);
UINT8 DecompressDataBlk(UINT32 outLen, UINT8* outData, UINT32 inLen, const UINT8* inData, const PCM_CMP_INF* cmprInfo);
UINT8 DecompressDataBlk_VGM(UINT32* outLen, UINT8** retOutData, UINT32 inLen, const UINT8* inData, const PCM_COMPR_TBL* comprTbl);
UINT8 CompressDataBlk(UINT32 outLen, UINT8* outData, UINT32 inLen, const UINT8* inData, const PCM_CMP_INF* cmprInfo);
void ReadPCMComprTable(UINT32 dataSize, const UINT8* data, PCM_COMPR_TBL* comprTbl);
UINT32 WriteCompressionTable(UINT32 dataSize, UINT8* data, PCM_COMPR_TBL* comprTbl);
void GenerateReverseLUT_8(UINT16 dstLen, UINT8* dstLUT, UINT16 srcLen, const UINT8* srcLUT);
void GenerateReverseLUT_16(UINT32 dstLen, UINT16* dstLUT, UINT32 srcLen, const UINT16* srcLUT);

#ifdef __cplusplus
}
#endif

#endif	// __DBLK_COMPR_H__
