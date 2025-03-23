//---------------------------------------------------------------------------\ 
//
//               (C) copyright Fraunhofer - IIS (2000)
//                        All Rights Reserved
//
//   filename: CVbriHeader.c (nee CVbriHeader.cpp)
//             MPEG Layer-3 Audio Decoder
//   author  : Martin Weishart martin.weishart@iis.fhg.de
//   modified: Christopher Snowhill kode54@gmail.com
//   date    : 2000-02-11
//   new date: 2022-06-19
//   contents/description: provides functions to read a VBRI header
//                         of a MPEG Layer 3 bitstream encoded
//                         with variable bitrate using Fraunhofer
//                         variable bitrate format
//
//--------------------------------------------------------------------------/

#include <stdlib.h>

#include "CVbriHeader.h"

enum offset {
	BYTE = 1,
	WORD = 2,
	DWORD = 4
};

//---------------------------------------------------------------------------\ 
//
//   Constructor: set position in buffer to parse and create a
//                VbriHeaderTable
//
//---------------------------------------------------------------------------/

struct VbriHeaderTable {
	int SampleRate;
	char VbriSignature[5];
	unsigned int VbriVersion;
	unsigned int VbriDelay;
	unsigned int VbriQuality;
	unsigned int VbriStreamBytes;
	unsigned int VbriStreamFrames;
	unsigned int VbriTableSize;
	unsigned int VbriTableScale;
	unsigned int VbriEntryBytes;
	unsigned int VbriEntryFrames;
	int *VbriTable;
};

struct VbriHeader {
	struct VbriHeaderTable VBHeader;
	int position;
};

void initVbriHeaderTable(struct VbriHeaderTable *pthis) {
	pthis->VbriTable = 0;
}

void freeVbriHeaderTable(struct VbriHeaderTable *pthis) {
	if(pthis->VbriTable) free(pthis->VbriTable);
}

void initVbriHeader(struct VbriHeader *pthis) {
	pthis->position = 0;
	initVbriHeaderTable(&pthis->VBHeader);
}

void freeVbriHeader(struct VbriHeader *pthis) {
	freeVbriHeaderTable(&pthis->VBHeader);
}

//---------------------------------------------------------------------------\
//
//   Method:   checkheader
//             Reads the header to a struct that has to be stored and is
//             used in other functions to determine file offsets
//   Input:    buffer containing the first frame
//   Output:   fills struct VbriHeader
//   Return:   0 on success; 1 on error
//
//---------------------------------------------------------------------------/

static int VbriGetSampleRate(const unsigned char *buffer);
static int VbriReadFromBuffer(struct VbriHeader *pthis, const unsigned char *HBuffer, int length);

int readVbriHeader(struct VbriHeader **outHeader, const unsigned char *Hbuffer, size_t length) {
	if(!outHeader) return 1;
	if(!*outHeader) {
		*outHeader = malloc(sizeof(**outHeader));
		if(!*outHeader) return 1;
	}

	struct VbriHeader *pthis = *outHeader;

	initVbriHeader(pthis);

	unsigned int i, TableLength;

	// My code will have the VBRI right at position 0

	/*if ( length >= 4 &&
	 *(Hbuffer+pthis->position  ) == 'V' &&
	 *(Hbuffer+pthis->position+1) == 'B' &&
	 *(Hbuffer+pthis->position+2) == 'R' &&
	 *(Hbuffer+pthis->position+3) == 'I')*/
	{
		/*pthis->position += DWORD ;
		  length -= DWORD ;*/

		// Caller already read and verified the 'VBRI' signature

		if(length < 22) return 1;

		pthis->VBHeader.VbriVersion = VbriReadFromBuffer(pthis, Hbuffer, WORD);
		pthis->VBHeader.VbriDelay = VbriReadFromBuffer(pthis, Hbuffer, WORD);
		pthis->VBHeader.VbriQuality = VbriReadFromBuffer(pthis, Hbuffer, WORD);
		pthis->VBHeader.VbriStreamBytes = VbriReadFromBuffer(pthis, Hbuffer, DWORD);
		pthis->VBHeader.VbriStreamFrames = VbriReadFromBuffer(pthis, Hbuffer, DWORD);
		pthis->VBHeader.VbriTableSize = VbriReadFromBuffer(pthis, Hbuffer, WORD);
		pthis->VBHeader.VbriTableScale = VbriReadFromBuffer(pthis, Hbuffer, WORD);
		pthis->VBHeader.VbriEntryBytes = VbriReadFromBuffer(pthis, Hbuffer, WORD);
		pthis->VBHeader.VbriEntryFrames = VbriReadFromBuffer(pthis, Hbuffer, WORD);

		length -= 22;

		TableLength = pthis->VBHeader.VbriTableSize * pthis->VBHeader.VbriEntryBytes;

		if(length < TableLength) return 1;

		pthis->VBHeader.VbriTable = calloc(sizeof(int), pthis->VBHeader.VbriTableSize + 1);
		if(!pthis->VBHeader.VbriTable) return 1;

		for(i = 0; i <= pthis->VBHeader.VbriTableSize; i++) {
			pthis->VBHeader.VbriTable[i] = VbriReadFromBuffer(pthis, Hbuffer, pthis->VBHeader.VbriEntryBytes * BYTE) * pthis->VBHeader.VbriTableScale;
		}
	}
	/*else{
	  return 1;
	}*/
	return 0;
}

//---------------------------------------------------------------------------\ 
//
//   Method:   seekPointByTime
//             Returns a point in the file to decode in bytes that is nearest
//             to a given time in seconds
//   Input:    time in seconds
//   Output:   None
//   Returns:  point belonging to the given time value in bytes
//
//---------------------------------------------------------------------------/

#if 0
int CVbriHeader::seekPointByTime(float EntryTimeInMilliSeconds){

  unsigned int SamplesPerFrame, i=0, SeekPoint = 0 , fraction = 0;

  float TotalDuration ;
  float DurationPerVbriFrames ;
  float AccumulatedTime = 0.0f ;
 
  (VBHeader.SampleRate >= 32000) ? (SamplesPerFrame = 1152) : (SamplesPerFrame = 576) ;

  TotalDuration		= ((float)VBHeader.VbriStreamFrames * (float)SamplesPerFrame) 
						  / (float)VBHeader.SampleRate * 1000.0f ;
  DurationPerVbriFrames = (float)TotalDuration / (float)(VBHeader.VbriTableSize+1) ;
 
  if ( EntryTimeInMilliSeconds > TotalDuration ) EntryTimeInMilliSeconds = TotalDuration; 
 
  while ( AccumulatedTime <= EntryTimeInMilliSeconds ){
    
    SeekPoint	      += VBHeader.VbriTable[i] ;
    AccumulatedTime += DurationPerVbriFrames;
    i++;
    
  }
  
  // Searched too far; correct result
  fraction = ( (int)(((( AccumulatedTime - EntryTimeInMilliSeconds ) / DurationPerVbriFrames ) 
			 + (1.0f/(2.0f*(float)VBHeader.VbriEntryFrames))) * (float)VBHeader.VbriEntryFrames));

  
  SeekPoint -= (int)((float)VBHeader.VbriTable[i-1] * (float)(fraction) 
				 / (float)VBHeader.VbriEntryFrames) ;

  return SeekPoint ;

}
#endif

//---------------------------------------------------------------------------\ 
//
//   Method:   seekTimeByPoint
//             Returns a time in the file to decode in seconds that is
//             nearest to a given point in bytes
//   Input:    time in seconds
//   Output:   None
//   Returns:  point belonging to the given time value in bytes
//
//---------------------------------------------------------------------------/

#if 0
float CVbriHeader::seekTimeByPoint(unsigned int EntryPointInBytes){

  unsigned int SamplesPerFrame, i=0, AccumulatedBytes = 0, fraction = 0;

  float SeekTime = 0.0f;
  float TotalDuration ;
  float DurationPerVbriFrames ;

  (VBHeader.SampleRate >= 32000) ? (SamplesPerFrame = 1152) : (SamplesPerFrame = 576) ;

  TotalDuration		= ((float)VBHeader.VbriStreamFrames * (float)SamplesPerFrame) 
						  / (float)VBHeader.SampleRate;
  DurationPerVbriFrames = (float)TotalDuration / (float)(VBHeader.VbriTableSize+1) ;
 
  while (AccumulatedBytes <= EntryPointInBytes){
    
    AccumulatedBytes += VBHeader.VbriTable[i] ;
    SeekTime	       += DurationPerVbriFrames;
    i++;
    
  }
  
  // Searched too far; correct result
  fraction = (int)(((( AccumulatedBytes - EntryPointInBytes ) /  (float)VBHeader.VbriTable[i-1]) 
                    + (1/(2*(float)VBHeader.VbriEntryFrames))) * (float)VBHeader.VbriEntryFrames);
  
  SeekTime -= (DurationPerVbriFrames * (float) ((float)(fraction) / (float)VBHeader.VbriEntryFrames)) ;
 
  return SeekTime ;

}
#endif

//---------------------------------------------------------------------------\ 
//
//   Method:   seekPointByPercent
//             Returns a point in the file to decode in bytes that is
//             nearest to a given percentage of the time of the stream
//   Input:    percent of time
//   Output:   None
//   Returns:  point belonging to the given time percentage value in bytes
//
//---------------------------------------------------------------------------/

#if 0
int CVbriHeader::seekPointByPercent(float percent){

  int SamplesPerFrame;

  float TotalDuration ;
  
  if (percent >= 100.0f) percent = 100.0f;
  if (percent <= 0.0f)   percent = 0.0f;

  (VBHeader.SampleRate >= 32000) ? (SamplesPerFrame = 1152) : (SamplesPerFrame = 576) ;

  TotalDuration = ((float)VBHeader.VbriStreamFrames * (float)SamplesPerFrame) 
				  / (float)VBHeader.SampleRate;
  
  return seekPointByTime( (percent/100.0f) * TotalDuration * 1000.0f );
  
}
#endif

//---------------------------------------------------------------------------\ 
//
//   Method:   GetSampleRate
//             Returns the sampling rate of the file to decode
//   Input:    Buffer containing the part of the first frame after the
//             syncword
//   Output:   None
//   Return:   sampling rate of the file to decode
//
//---------------------------------------------------------------------------/

int VbriGetSampleRate(const unsigned char *buffer) {
	unsigned char id, idx, mpeg;

	id = (0xC0 & (buffer[1] << 3)) >> 4;
	idx = (0xC0 & (buffer[2] << 4)) >> 6;

	mpeg = id | idx;

	switch((int)mpeg) {
		case 0:
			return 11025;
		case 1:
			return 12000;
		case 2:
			return 8000;
		case 8:
			return 22050;
		case 9:
			return 24000;
		case 10:
			return 16000;
		case 12:
			return 44100;
		case 13:
			return 48000;
		case 14:
			return 32000;
		default:
			return 0;
	}
}

//---------------------------------------------------------------------------\ 
//
//   Method:   readFromBuffer
//             reads from a buffer a segment to an int value
//   Input:    Buffer containig the first frame
//   Output:   none
//   Return:   number containing int value of buffer segmenet
//             length
//
//---------------------------------------------------------------------------/

int VbriReadFromBuffer(struct VbriHeader *pthis, const unsigned char *HBuffer, int length) {
	int i, b, number = 0;

	int position = pthis->position;

	if(HBuffer) {
		for(i = 0; i < length; i++) {
			b = length - 1 - i;
			number = number | (unsigned int)(HBuffer[position + i] & 0xff) << (8 * b);
		}
		pthis->position += length;
		return number;

	} else {
		return 0;
	}
}

#if 0
float CVbriHeader::totalDuration()
{
	int SamplesPerFrame;
	(VBHeader.SampleRate >= 32000) ? (SamplesPerFrame = 1152) : (SamplesPerFrame = 576) ;
	return ((float)VBHeader.VbriStreamFrames * (float)SamplesPerFrame) / (float)VBHeader.SampleRate;
}
#endif

int VbriTotalFrames(struct VbriHeader *pthis) {
	if(pthis)
		return pthis->VBHeader.VbriStreamFrames;
	else
		return 0;
}
