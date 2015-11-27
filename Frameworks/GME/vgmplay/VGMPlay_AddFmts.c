// VGMPlay_AddFmts.c: C Source File for playback of additional non-VGM formats

#ifdef ADDITIONAL_FORMATS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "stdbool.h"
#include <math.h>

#ifdef WIN32
//#include <windows.h>
void __stdcall Sleep(unsigned int dwMilliseconds);
#else
#define	Sleep(msec)		usleep(msec * 1000)
#endif

#include <zlib.h>

#include "chips/mamedef.h"

#include "VGMPlay.h"

#include "ChipMapper.h"

// Structures for DRO and CMF files
typedef struct _cmf_file_header
{
	UINT32 fccCMF;
	UINT16 shtVersion;
	UINT16 shtOffsetInsData;
	UINT16 shtOffsetMusData;
	UINT16 shtTickspQuarter;
	UINT16 shtTickspSecond;
	UINT16 shtOffsetTitle;
	UINT16 shtOffsetAuthor;
	UINT16 shtOffsetComments;
	UINT8 bytChnUsed[0x10];
	UINT16 shtInstrumentCount;
	UINT16 shtTempo;
} CMF_HEADER;
typedef struct _cmf_instrument_table
{
	UINT8 Character[0x02];
	UINT8 ScaleLevel[0x02];
	UINT8 AttackDelay[0x02];
	UINT8 SustnRelease[0x02];
	UINT8 WaveSelect[0x02];
	UINT8 FeedbConnect;
	UINT8 Reserved[0x5];
} CMF_INSTRUMENT;

typedef struct _dro_file_header
{
	char cSignature[0x08];
	UINT16 iVersionMajor;
	UINT16 iVersionMinor;
} DRO_HEADER;
typedef struct _dro_version_header_1
{
	UINT32 iLengthMS;
	UINT32 iLengthBytes;
	UINT32 iHardwareType;
} DRO_VER_HEADER_1;
typedef struct _dro_version_header_2
{
	UINT32 iLengthPairs;
	UINT32 iLengthMS;
	UINT8 iHardwareType;
	UINT8 iFormat;
	UINT8 iCompression;
	UINT8 iShortDelayCode;
	UINT8 iLongDelayCode;
	UINT8 iCodemapLength;
} DRO_VER_HEADER_2;

#define FCC_CMF		0x464D5443	// 'CTMF'
#define FCC_DRO1	0x41524244	// 'DBRA'
#define FCC_DRO2	0x4C504F57	// 'WOPL'

extern UINT32 GetGZFileLength(const char* FileName);
//bool OpenOtherFile(const char* FileName)

INLINE UINT16 ReadLE16(const UINT8* Data);
INLINE UINT32 ReadLE32(const UINT8* Data);
INLINE int gzgetLE32(gzFile hFile, UINT32* RetValue);

static UINT32 GetMIDIDelay(UINT32* DelayLen);
static UINT16 MIDINote2FNum(UINT8 Note, INT8 Pitch);
static void SendMIDIVolume(UINT8 ChipID, UINT8 Channel, UINT8 Command,
						   UINT8 ChnIns, UINT8 Volume);
//void InterpretOther(UINT32 SampleCount);

INLINE INT32 SampleVGM2Playback(INT32 SampleVal);
INLINE INT32 SamplePlayback2VGM(INT32 SampleVal);


extern UINT32 SampleRate;	// Note: also used by some sound cores to determinate the chip sample rate

extern UINT8 FileMode;
extern VGM_HEADER VGMHead;
extern UINT32 VGMDataLen;
extern UINT8* VGMData;
extern GD3_TAG VGMTag;

CMF_HEADER CMFHead;
UINT16 CMFInsCount;
CMF_INSTRUMENT* CMFIns;

DRO_HEADER DROHead;
DRO_VER_HEADER_2 DROInf;
UINT8* DROCodemap;


extern UINT32 VGMPos;
extern INT32 VGMSmplPos;
extern INT32 VGMSmplPlayed;
extern INT32 VGMSampleRate;
extern UINT32 BlocksSent;
extern UINT32 BlocksPlayed;
bool VGMEnd;
extern bool EndPlay;
extern bool PausePlay;
extern bool FadePlay;
extern bool ForceVGMExec;

extern UINT32 VGMMaxLoop;
UINT32 CMFMaxLoop;
extern UINT32 VGMMaxLoopM;
extern UINT32 VGMCurLoop;

extern UINT32 FadeTime;
extern UINT32 VGMMaxLoop;
extern bool ErrorHappened;

extern UINT8 CmdList[0x100];

bool OpenOtherFile(const char* FileName)
{
	gzFile hFile;
	UINT32 FileSize;
	UINT32 fccHeader;
	UINT32 CurPos;
	UINT32 TempLng;
	UINT16 FileVer;
	const char* TempStr;
	DRO_VER_HEADER_1 DRO_V1;
	
	FileSize = GetGZFileLength(FileName);
	
	FileMode = 0x00;
	hFile = gzopen(FileName, "rb");
	if (hFile == NULL)
		return false;
	
	gzseek(hFile, 0x00, SEEK_SET);
	gzgetLE32(hFile, &fccHeader);
	switch(fccHeader)
	{
	case FCC_VGM:
		FileMode = 0xFF;	// should already be opened
		break;
	case FCC_CMF:
		FileMode = 0x01;
		break;
	case FCC_DRO1:
		gzgetLE32(hFile, &fccHeader);
		if (fccHeader == FCC_DRO2)
			FileMode = 0x02;
		else
			FileMode = 0xFF;
		break;
	default:
		FileMode = 0xFF;
		break;
	}
	if (FileMode == 0xFF)
		goto OpenErr;
	
	VGMTag.strTrackNameE = L"";
	VGMTag.strTrackNameJ = L"";
	VGMTag.strGameNameE = L"";
	VGMTag.strGameNameJ = L"";
	VGMTag.strSystemNameE = L"";
	VGMTag.strSystemNameJ = L"";
	VGMTag.strAuthorNameE = L"";
	VGMTag.strAuthorNameJ = L"";
	VGMTag.strReleaseDate = L"";
	VGMTag.strCreator = L"";
	VGMTag.strNotes = L"";
	
	switch(FileMode)
	{
	case 0x00:	// VGM File
		break;
	case 0x01:	// CMF File
	case 0x02:	// DosBox RAW OPL
		VGMTag.strGameNameE = (wchar_t*)malloc(0x10 * sizeof(wchar_t*));
		wcscpy(VGMTag.strGameNameE, L"    Player");
		VGMTag.strSystemNameE = L"PC / MS-DOS";
		break;
	}
	
	VGMDataLen = FileSize;
	
	switch(FileMode)
	{
	case 0x00:	// VGM File
		// already done by OpenVGMFile
		break;
	case 0x01:	// CMF File
		// Read Data
		VGMData = (UINT8*)malloc(VGMDataLen);
		if (VGMData == NULL)
			goto OpenErr;
		gzseek(hFile, 0x00, SEEK_SET);
		gzread(hFile, VGMData, VGMDataLen);
		
#ifndef VGM_BIG_ENDIAN
		memcpy(&CMFHead, &VGMData[0x00], sizeof(CMF_HEADER));
#else
		CMFHead.fccCMF = ReadLE32(&VGMData[0x00]);
		CMFHead.shtVersion = ReadLE16(&VGMData[0x04]);
		CMFHead.shtOffsetInsData = ReadLE16(&VGMData[0x06]);
		CMFHead.shtOffsetMusData = ReadLE16(&VGMData[0x08]);
		CMFHead.shtTickspQuarter = ReadLE16(&VGMData[0x0A]);
		CMFHead.shtTickspSecond = ReadLE16(&VGMData[0x0C]);
		CMFHead.shtOffsetTitle = ReadLE16(&VGMData[0x0E]);
		CMFHead.shtOffsetAuthor = ReadLE16(&VGMData[0x10]);
		CMFHead.shtOffsetComments = ReadLE16(&VGMData[0x12]);
		memcpy(CMFHead.bytChnUsed, &VGMData[0x14], 0x10);
		CMFHead.shtInstrumentCount = ReadLE16(&VGMData[0x24]);
		CMFHead.shtTempo = ReadLE16(&VGMData[0x26]);
#endif
		
		if (CMFHead.shtVersion == 0x0100)
		{
			CMFHead.shtInstrumentCount &= 0x00FF;
			CMFHead.shtTempo = (UINT16)(60.0 *
								CMFHead.shtTickspQuarter / CMFHead.shtTickspSecond + 0.5);
		}
		
		if (CMFHead.shtOffsetTitle)
		{
			TempStr = (char*)&VGMData[CMFHead.shtOffsetTitle];
			TempLng = strlen(TempStr) + 0x01;
			VGMTag.strTrackNameE = (wchar_t*)malloc(TempLng * sizeof(wchar_t));
			mbstowcs(VGMTag.strTrackNameE, TempStr, TempLng);
		}
		VGMTag.strGameNameE[0x00] = 'C';
		VGMTag.strGameNameE[0x01] = 'M';
		VGMTag.strGameNameE[0x02] = 'F';
		if (CMFHead.shtOffsetAuthor)
		{
			TempStr = (char*)&VGMData[CMFHead.shtOffsetAuthor];
			TempLng = strlen(TempStr) + 0x01;
			VGMTag.strAuthorNameE = (wchar_t*)malloc(TempLng * sizeof(wchar_t));
			mbstowcs(VGMTag.strAuthorNameE, TempStr, TempLng);
		}
		if (CMFHead.shtOffsetComments)
		{
			TempStr = (char*)&VGMData[CMFHead.shtOffsetComments];
			TempLng = strlen(TempStr) + 0x01;
			VGMTag.strNotes = (wchar_t*)malloc(TempLng * sizeof(wchar_t));
			mbstowcs(VGMTag.strNotes, TempStr, TempLng);
		}
		
		CMFInsCount = CMFHead.shtInstrumentCount;
		TempLng = CMFInsCount * sizeof(CMF_INSTRUMENT);
		CMFIns = (CMF_INSTRUMENT*)malloc(TempLng);
		memcpy(CMFIns, &VGMData[CMFHead.shtOffsetInsData], TempLng);
		
		memset(&VGMHead, 0x00, sizeof(VGM_HEADER));
		VGMHead.lngEOFOffset = VGMDataLen;
		VGMHead.lngVersion = CMFHead.shtVersion;
		VGMHead.lngDataOffset = CMFHead.shtOffsetMusData;
		VGMSampleRate = CMFHead.shtTickspSecond;
		VGMHead.lngTotalSamples = 0;
		VGMHead.lngHzYM3812 = 3579545 | 0x40000000;
		
		break;
	case 0x02:	// DosBox RAW OPL
		// Read Data
		VGMData = (UINT8*)malloc(VGMDataLen);
		if (VGMData == NULL)
			goto OpenErr;
		gzseek(hFile, 0x00, SEEK_SET);
		VGMDataLen = gzread(hFile, VGMData, VGMDataLen);
		
		VGMTag.strGameNameE[0x00] = 'D';
		VGMTag.strGameNameE[0x01] = 'R';
		VGMTag.strGameNameE[0x02] = 'O';
		
		memset(&VGMHead, 0x00, sizeof(VGM_HEADER));
		CurPos = 0x00;
#ifndef VGM_BIG_ENDIAN
		memcpy(&DROHead, &VGMData[CurPos], sizeof(DRO_HEADER));
#else
		memcpy(DROHead.cSignature,			&VGMData[CurPos + 0x00], 0x08);
		DROHead.iVersionMajor = ReadLE16(	&VGMData[CurPos + 0x08]);
		DROHead.iVersionMinor = ReadLE16(	&VGMData[CurPos + 0x0A]);
#endif
		CurPos += sizeof(DRO_HEADER);
		
		memcpy(&TempLng, &VGMData[0x08], sizeof(UINT32));
		if (TempLng & 0xFF00FF00)
		{
			// DosBox Version 0.61
			// this version didn't contain Version Bytes
			CurPos = 0x08;
			DROHead.iVersionMajor = 0x00;
			DROHead.iVersionMinor = 0x00;
		}
		else if (! (TempLng & 0x0000FFFF))
		{
			// DosBox Version 0.63
			// the order of the Version Bytes is swapped in this version
			FileVer = DROHead.iVersionMinor;
			if (FileVer == 0x01)
			{
				DROHead.iVersionMinor = DROHead.iVersionMajor;
				DROHead.iVersionMajor = FileVer;
			}
		}
		VGMHead.lngEOFOffset = VGMDataLen;
		VGMHead.lngVersion = (DROHead.iVersionMajor << 8) |
							((DROHead.iVersionMinor & 0xFF) << 0);
		VGMSampleRate = 1000;
		
		if (DROHead.iVersionMajor > 2)
			DROHead.iVersionMajor = 2;
		switch(DROHead.iVersionMajor)
		{
		case 0:	// Version 0 (DosBox Version 0.61)
		case 1:	// Version 1 (DosBox Version 0.63)
			switch(DROHead.iVersionMajor)
			{
			case 0:	// Version 0
				DRO_V1.iLengthMS = ReadLE32(&VGMData[CurPos + 0x00]);
				DRO_V1.iLengthBytes = ReadLE32(&VGMData[CurPos + 0x04]);
				DRO_V1.iHardwareType = VGMData[CurPos + 0x08];
				CurPos += 0x09;
				break;
			case 1:	// Version 1
				DRO_V1.iLengthMS = ReadLE32(&VGMData[CurPos + 0x00]);
				DRO_V1.iLengthBytes = ReadLE32(&VGMData[CurPos + 0x04]);
				DRO_V1.iHardwareType = ReadLE32(&VGMData[CurPos + 0x08]);
				CurPos += 0x0C;
				break;
			}
			
			DROInf.iLengthPairs = DRO_V1.iLengthBytes >> 1;
			DROInf.iLengthMS = DRO_V1.iLengthMS;
			switch(DRO_V1.iHardwareType)
			{
			case 0x01:	// Single OPL3
				DROInf.iHardwareType = 0x02;
				break;
			case 0x02:	// Dual OPL2
				DROInf.iHardwareType = 0x01;
				break;
			default:
				DROInf.iHardwareType = (UINT8)DRO_V1.iHardwareType;
				break;
			}
			DROInf.iFormat = 0x00;
			DROInf.iCompression = 0x00;
			DROInf.iShortDelayCode = 0x00;
			DROInf.iLongDelayCode = 0x01;
			DROInf.iCodemapLength = 0x00;
			
			break;
		case 2:	// Version 2 (DosBox Version 0.73)
			// sizeof(DRO_VER_HEADER_2) returns 0x10, but the exact size is 0x0E
			//memcpy(&DROInf, &VGMData[CurPos], 0x0E);
			DROInf.iLengthPairs =	ReadLE32(	&VGMData[CurPos + 0x00]);
			DROInf.iLengthMS =		ReadLE32(	&VGMData[CurPos + 0x04]);
			DROInf.iHardwareType =				 VGMData[CurPos + 0x08];
			DROInf.iFormat =					 VGMData[CurPos + 0x09];
			DROInf.iCompression =				 VGMData[CurPos + 0x0A];
			DROInf.iShortDelayCode =			 VGMData[CurPos + 0x0B];
			DROInf.iLongDelayCode =				 VGMData[CurPos + 0x0C];
			DROInf.iCodemapLength =				 VGMData[CurPos + 0x0D];
			CurPos += 0x0E;
			
			break;
		}
		
		if (DROInf.iCodemapLength)
		{
			DROCodemap = (UINT8*)malloc(DROInf.iCodemapLength * sizeof(UINT8));
			memcpy(DROCodemap, &VGMData[CurPos], DROInf.iCodemapLength);
			CurPos += DROInf.iCodemapLength;
		}
		else
		{
			DROCodemap = NULL;
		}
		
		VGMHead.lngDataOffset = CurPos;
		VGMHead.lngTotalSamples = DROInf.iLengthMS;
		switch(DROInf.iHardwareType)
		{
		case 0x00:	// Single OPL2 Chip
			VGMHead.lngHzYM3812 = 3579545;
			break;
		case 0x01:	// Dual OPL2 Chip
			VGMHead.lngHzYM3812 = 3579545 | 0xC0000000;
			break;
		case 0x02:	// Single OPL3 Chip
			VGMHead.lngHzYMF262 = 14318180;
			break;
		default:
			VGMHead.lngHzYM3812 = 3579545 | 0x40000000;
			break;
		}
		
		break;
	}
	
	gzclose(hFile);
	return true;

OpenErr:

	gzclose(hFile);
	return false;
}

INLINE UINT16 ReadLE16(const UINT8* Data)
{
	// read 16-Bit Word (Little Endian/Intel Byte Order)
#ifndef VGM_BIG_ENDIAN
	return *(UINT16*)Data;
#else
	return (Data[0x01] << 8) | (Data[0x00] << 0);
#endif
}

INLINE UINT32 ReadLE32(const UINT8* Data)
{
	// read 32-Bit Word (Little Endian/Intel Byte Order)
#ifndef VGM_BIG_ENDIAN
	return	*(UINT32*)Data;
#else
	return	(Data[0x03] << 24) | (Data[0x02] << 16) |
			(Data[0x01] <<  8) | (Data[0x00] <<  0);
#endif
}

INLINE int gzgetLE32(gzFile hFile, UINT32* RetValue)
{
#ifndef VGM_BIG_ENDIAN
	return gzread(hFile, RetValue, 0x04);
#else
	int RetVal;
	UINT8 Data[0x04];
	
	RetVal = gzread(hFile, Data, 0x04);
	*RetValue =	(Data[0x03] << 24) | (Data[0x02] << 16) |
				(Data[0x01] <<  8) | (Data[0x00] <<  0);
	return RetVal;
#endif
}

static UINT32 GetMIDIDelay(UINT32* DelayLen)
{
	UINT32 CurPos;
	UINT32 DelayVal;
	
	CurPos = VGMPos;
	DelayVal = 0x00;
	do
	{
		DelayVal = (DelayVal << 7) | (VGMData[CurPos] & 0x7F);
	} while(VGMData[CurPos ++] & 0x80);
	
	if (DelayLen != NULL)
		*DelayLen = CurPos - VGMPos;
	return DelayVal;
}

static UINT16 MIDINote2FNum(UINT8 Note, INT8 Pitch)
{
	const double CHIP_RATE = 3579545.0 / 72.0;	// ~49716
	double FreqVal;
	INT8 BlockVal;
	UINT16 KeyVal;
	
	FreqVal = 440.0 * pow(2, (Note - 69 + Pitch / 256.0) / 12.0);
	BlockVal = (Note / 12) - 1;
	if (BlockVal < 0x00)
		BlockVal = 0x00;
	else if (BlockVal > 0x07)
		BlockVal = 0x07;
	KeyVal = (UINT16)(FreqVal * pow(2, 20 - BlockVal) / CHIP_RATE + 0.5);
	
	return (BlockVal << 10) | KeyVal;	// << (8+2)
}

static void SendMIDIVolume(UINT8 ChipID, UINT8 Channel, UINT8 Command,
						   UINT8 ChnIns, UINT8 Volume)
{
	bool RhythmOn;
	UINT8 TempByt;
	//UINT16 TempSht;
	UINT32 TempLng;
	UINT8 OpBase;	// Operator Base
	CMF_INSTRUMENT* TempIns;
	UINT8 OpMask;
	INT8 OpVol;
	INT8 NoteVol;
	
	RhythmOn = (Channel >> 7) & 0x01;
	Channel &= 0x7F;
	
	// Refresh Total Level (Volume)
	TempIns = CMFIns + ChnIns;
	OpBase = (Channel / 0x03) * 0x08 + (Channel % 0x03);
	
	if (! RhythmOn)
	{
		TempLng = 0x01;
		OpMask = 0x03;
	}
	else
	{
		//TempLng = 0x01;
		switch(Command & 0x0F)
		{
		case 0x0B:	// Bass Drum
			OpMask = 0x00;
			break;
		case 0x0F:	// Hi Hat
			OpMask = 0x00;
			break;
		case 0x0C:	// Snare Drum
			OpMask = 0x01;
			break;
		case 0x0D:	// Tom Tom
			OpMask = 0x00;
			break;
		case 0x0E:	// Cymbal
			OpMask = 0x01;
			break;
		default:
			OpMask = 0x00;
			break;
		}
		TempLng = OpMask;
		OpMask *= 0x03;
	}
	
	// Verified with PLAY.EXE
	OpVol = (Volume + 0x04) >> 3;
	OpVol = 0x10 - (OpVol << 1);
	if (OpVol < 0x00)
		OpVol >>= 1;
	NoteVol = (TempIns->ScaleLevel[TempLng] & 0x3F) + OpVol;
	if (NoteVol < 0x00)
		NoteVol = 0x00;
	
	TempByt = NoteVol | TempIns->ScaleLevel[TempLng] & 0xC0;
	chip_reg_write(0x09, ChipID, 0x00, 0x40 | (OpBase + OpMask), TempByt);
	
	return;
}

void InterpretOther(UINT32 SampleCount)
{
	static UINT8 LastCmd = 0x90;
	static UINT8 DrumReg[0x02] = {0x00, 0x00};
	static UINT8 ChnIns[0x10];
	static UINT8 ChnNote[0x20];
	static INT8 ChnPitch[0x10];
	INT32 SmplPlayed;
	UINT8 Command;
	UINT8 Channel;
	UINT8 TempByt;
	UINT16 TempSht;
	UINT32 TempLng;
	UINT32 DataLen;
	static UINT8 CurChip = 0x00;
	UINT8 OpBase;	// Operator Base
	CMF_INSTRUMENT* TempIns;
	bool RhythmOn;
	bool NoteOn;
	UINT8 OpMask;
	
	if (VGMEnd)
		return;
	if (PausePlay && ! ForceVGMExec)
		return;
	
	switch(FileMode)
	{
	case 0x01:	// CMF File Mode
		if (! SampleCount)
		{
			memset(ChnIns, 0xFF, 0x10);
			memset(ChnNote, 0xFF, 0x20);
			memset(ChnPitch, 0x00, 0x10);
			
			TempLng = VGMPos;
			SmplPlayed = VGMSmplPos;
			VGMPos = VGMHead.lngDataOffset;
			RhythmOn = false;
			while(! RhythmOn)
			{
				VGMSmplPos += GetMIDIDelay(&DataLen);
				VGMPos += DataLen;
				
				Command = VGMData[VGMPos];
				if (Command & 0x80)
					VGMPos ++;
				else
					Command = LastCmd;
				Channel = Command & 0x0F;
				
				switch(Command & 0xF0)
				{
				case 0xF0:	// End Of File
					switch(Command)
					{
					case 0xFF:
						switch(VGMData[VGMPos + 0x00])
						{
						case 0x2F:
							VGMHead.lngTotalSamples = VGMSmplPos;
							VGMHead.lngLoopSamples = VGMHead.lngTotalSamples;
							RhythmOn = true;
							break;
						}
						break;
					default:
						VGMPos += 0x01;
					}
					break;
				case 0x80:
				case 0x90:
				case 0xA0:
				case 0xB0:
				case 0xE0:
					VGMPos += 0x02;
					break;
				case 0xC0:
				case 0xD0:
					VGMPos += 0x01;
					break;
				}
				if (Command < 0xF0)
					LastCmd = Command;
			}
			
			VGMPos = TempLng;
			VGMSmplPos = SmplPlayed;
		}
		
		SmplPlayed = SamplePlayback2VGM(VGMSmplPlayed + SampleCount);
		while(true)
		{
			TempLng = GetMIDIDelay(&DataLen);
			if (VGMSmplPos + (INT32)TempLng > SmplPlayed)
				break;
			VGMSmplPos += TempLng;
			VGMPos += DataLen;
			
			Command = VGMData[VGMPos];
			if (Command & 0x80)
				VGMPos ++;
			else
				Command = LastCmd;
			Channel = Command & 0x0F;
			
			if (DrumReg[0x00] & 0x20)
			{
				if (Channel < 0x0B)
				{
					CurChip = Channel / 0x06;
					Channel = Channel % 0x06;
				}
				else
				{
					Channel -= 0x0B;
					CurChip = Channel / 0x03;
					Channel = Channel % 0x03;
					if (CurChip == 0x01 && Channel == 0x00)
						Channel = 0x02;
					Channel += 0x06;
					// Map all drums to one chip
					CurChip = 0x00;
				}
			}
			else
			{
				CurChip = Channel / 0x09;
				Channel = Channel % 0x09;
			}
			CurChip = 0x00;
			
			RhythmOn = (Channel >= 0x06) && (DrumReg[CurChip] & 0x20);
			switch(Command & 0xF0)
			{
			case 0xF0:	// End Of File
				switch(Command)
				{
				case 0xFF:
					switch(VGMData[VGMPos + 0x00])
					{
					case 0x2F:
						if (CMFMaxLoop != 0x01)
						{
							VGMPos = VGMHead.lngDataOffset;
							VGMSmplPos -= VGMHead.lngLoopSamples;
							VGMSmplPlayed -= SampleVGM2Playback(VGMHead.lngLoopSamples);
							SmplPlayed = SamplePlayback2VGM(VGMSmplPlayed + SampleCount);
							VGMCurLoop ++;
							
							if (CMFMaxLoop && VGMCurLoop >= CMFMaxLoop)
								FadePlay = true;
							if (FadePlay && ! FadeTime)
								VGMEnd = true;
						}
						else
						{
							VGMEnd = true;
							break;
						}
						break;
					}
					break;
				default:
					VGMPos += 0x01;
				}
				break;
			case 0x80:
			case 0x90:
				TempSht = MIDINote2FNum(VGMData[VGMPos + 0x00], ChnPitch[Channel]);
				if ((Command & 0xF0) == 0x80)
					NoteOn = false;
				else
					NoteOn = VGMData[VGMPos + 0x01] ? true : false;
				
				if (! RhythmOn)	// Set "Key On"
					TempSht |= (UINT8)NoteOn << 13;	// << (8+5)
				
				if (NoteOn)
				{
					for (CurChip = 0x00; CurChip < 0x02; CurChip ++)
					{
						if (ChnNote[(CurChip << 4) | Channel] == 0xFF)
						{
							ChnNote[(CurChip << 4) | Channel] = VGMData[VGMPos + 0x00];
							break;
						}
					}
					if (CurChip >= 0x02)
					{
						CurChip = 0x00;
						ChnNote[(CurChip << 4) | Channel] = VGMData[VGMPos + 0x00];
					}
				}
				else
				{
					for (CurChip = 0x00; CurChip < 0x02; CurChip ++)
					{
						if (ChnNote[(CurChip << 4) | Channel] != 0xFF)
						{
							ChnNote[(CurChip << 4) | Channel] = 0xFF;
							break;
						}
					}
				}
				if (CurChip >= 0x02)
					CurChip = 0xFF;
				
				if (CurChip != 0xFF)
				{
					if (NoteOn)
					{
						if (! RhythmOn)
							SendMIDIVolume(CurChip, Channel | (RhythmOn << 7), Command,
											ChnIns[Command & 0x0F], VGMData[VGMPos + 0x01]);
					}
					if (NoteOn || ! RhythmOn)
					{
						chip_reg_write(0x09, CurChip, 0x00, 0xA0 | Channel, TempSht & 0xFF);
						chip_reg_write(0x09, CurChip, 0x00, 0xB0 | Channel, TempSht >> 8);
					}
					
					if (RhythmOn)
					{
						TempByt = 0x0F - (Command & 0x0F);
						DrumReg[CurChip] &= ~(0x01 << TempByt);
						if (NoteOn)
							chip_reg_write(0x09, CurChip, 0x00, 0xBD, DrumReg[CurChip]);
						DrumReg[CurChip] |= (UINT8)NoteOn << TempByt;
						chip_reg_write(0x09, CurChip, 0x00, 0xBD, DrumReg[CurChip]);
					}
				}
				VGMPos += 0x02;
				break;
			case 0xB0:
				NoteOn = false;
				switch(VGMData[VGMPos + 0x00])
				{
				case 0x66:	// Marker
					break;
				case 0x67:	// Rhythm Mode
					if (! VGMData[VGMPos + 0x01])
					{	// Set Rhythm Mode off
						DrumReg[0x00] = 0xC0;
						DrumReg[0x01] = 0xC0;
					}
					else
					{	// Set Rhythm Mode on
						DrumReg[0x00] = 0xE0;
						DrumReg[0x01] = 0xE0;
					}
					chip_reg_write(0x09, CurChip, 0x00, 0xBD, DrumReg[0x00]);
					chip_reg_write(0x09, CurChip, 0x00, 0xBD, DrumReg[0x01]);
					break;
				case 0x68:	// Pitch Upward
					ChnPitch[Channel] = +VGMData[VGMPos + 0x01];
					NoteOn = true;
					break;
				case 0x69:	// Pitch Downward
					ChnPitch[Channel] = -VGMData[VGMPos + 0x01];
					NoteOn = true;
					break;
				}
				if (NoteOn)
				{
					for (CurChip = 0x00; CurChip < 0x02; CurChip ++)
					{
						TempByt = ChnNote[(CurChip << 4) | Channel];
						if (! RhythmOn && TempByt != 0xFF)
						{
							TempSht = MIDINote2FNum(TempByt, ChnPitch[Channel]);
							TempSht |= 0x01 << 13;	// << (8+5)
							
							chip_reg_write(0x09, CurChip, 0x00, 0xA0 | Channel, TempSht & 0xFF);
							chip_reg_write(0x09, CurChip, 0x00, 0xB0 | Channel, TempSht >> 8);
						}
					}
				}
				VGMPos += 0x02;
				break;
			case 0xC0:
				TempByt = VGMData[VGMPos + 0x00];
				if (TempByt >= CMFInsCount)
				{
					//VGMPos += 0x01;
					//break;
					TempByt %= CMFInsCount;
				}
				
				TempIns = CMFIns + TempByt;
				ChnIns[Command & 0x0F] = TempByt;
				
				OpBase = (Channel / 0x03) * 0x08 + (Channel % 0x03);
				if (! RhythmOn)
				{
					OpMask = 0x03;
				}
				else
				{
					switch(Command & 0x0F)
					{
					case 0x0B:	// Bass Drum
						OpMask = 0x03;
						break;
					case 0x0F:	// Hi Hat
						OpMask = 0x01;
						//Channel = 0x01;	// PLAY.EXE really does this sometimes
						//OpBase = 0x01;
						break;
					case 0x0C:	// Snare Drum
						OpMask = 0x02;
						break;
					case 0x0D:	// Tom Tom
						OpMask = 0x01;
						break;
					case 0x0E:	// Cymbal
						OpMask = 0x02;
						break;
					default:
						OpMask = 0x03;
						break;
					}
				}
				
				for (CurChip = 0x00; CurChip < 0x02; CurChip ++)
				{
					TempByt = 0x00;
					if (OpMask & 0x01)
					{
						// Write Operator 1
						chip_reg_write(0x09, CurChip, 0x00,
										0x20 | (OpBase + 0x00), TempIns->Character[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0x40 | (OpBase + 0x00), TempIns->ScaleLevel[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0x60 | (OpBase + 0x00), TempIns->AttackDelay[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0x80 | (OpBase + 0x00), TempIns->SustnRelease[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0xE0 | (OpBase + 0x00), TempIns->WaveSelect[TempByt]);
						TempByt ++;
					}
					if (OpMask & 0x02)
					{
						// Write Operator 2
						chip_reg_write(0x09, CurChip, 0x00,
										0x20 | (OpBase + 0x03), TempIns->Character[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0x40 | (OpBase + 0x03), TempIns->ScaleLevel[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0x60 | (OpBase + 0x03), TempIns->AttackDelay[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0x80 | (OpBase + 0x03), TempIns->SustnRelease[TempByt]);
						chip_reg_write(0x09, CurChip, 0x00,
										0xE0 | (OpBase + 0x03), TempIns->WaveSelect[TempByt]);
						TempByt ++;
					}
					
					chip_reg_write(0x09, CurChip, 0x00, 0xC0 | Channel, TempIns->FeedbConnect);
				}
				
				VGMPos += 0x01;
				break;
			case 0xA0:
			case 0xE0:
				VGMPos += 0x02;
				break;
			case 0xD0:
				VGMPos += 0x01;
				break;
			}
			if (Command < 0xF0)
				LastCmd = Command;
			
			if (VGMEnd)
				break;
		}
		break;
	case 0x02:	// DosBox RAW OPL File Mode
		NoteOn = false;
		if (! SampleCount)
		{
			// was done during Init (Emulation Core / Chip Mapper for real FM),
			// but Chip Mapper's init-routine now works different
			switch(DROInf.iHardwareType)
			{
			case 0x00:	// OPL 2
				for (TempByt = 0xFF; TempByt >= 0x20; TempByt --)
					chip_reg_write(0x09, 0x00, 0x00, TempByt, 0x00);
				chip_reg_write(0x09, 0x00, 0x00, 0x08, 0x00);
				chip_reg_write(0x09, 0x00, 0x00, 0x01, 0x00);
				break;
			case 0x01:	// Dual OPL 2
				for (TempByt = 0xFF; TempByt >= 0x20; TempByt --)
					chip_reg_write(0x09, 0x00, 0x00, TempByt, 0x00);
				chip_reg_write(0x09, 0x00, 0x00, 0x08, 0x00);
				chip_reg_write(0x09, 0x00, 0x00, 0x01, 0x00);
				//Sleep(1);
				for (TempByt = 0xFF; TempByt >= 0x20; TempByt --)
					chip_reg_write(0x09, 0x01, 0x00, TempByt, 0x00);
				chip_reg_write(0x09, 0x01, 0x00, 0x08, 0x00);
				chip_reg_write(0x09, 0x01, 0x00, 0x01, 0x00);
				break;
			case 0x02:	// OPL 3
				for (TempByt = 0xFF; TempByt >= 0x20; TempByt --)
					chip_reg_write(0x0C, 0x00, 0x00, TempByt, 0x00);
				chip_reg_write(0x0C, 0x00, 0x00, 0x08, 0x00);
				chip_reg_write(0x0C, 0x00, 0x00, 0x01, 0x00);
				//Sleep(1);
				for (TempByt = 0xFF; TempByt >= 0x20; TempByt --)
					chip_reg_write(0x0C, 0x00, 0x01, TempByt, 0x00);
				//chip_reg_write(0x0C, 0x00, 0x01, 0x05, 0x00);
				chip_reg_write(0x0C, 0x00, 0x01, 0x04, 0x00);
				break;
			default:
				for (TempByt = 0xFF; TempByt >= 0x20; TempByt --)
					chip_reg_write(0x09, 0x00, 0x00, TempByt, 0x00);
				break;
			}
			Sleep(1);
			NoteOn = true && (DROHead.iVersionMajor < 2);
			OpBase = 0x00;
		}
		
		SmplPlayed = SamplePlayback2VGM(VGMSmplPlayed + SampleCount);
		while(VGMSmplPos <= SmplPlayed)
		{
			Command = VGMData[VGMPos + 0x00];
			
			if (Command == DROInf.iShortDelayCode)
				Command = 0x00;
			else if (Command == DROInf.iLongDelayCode)
				Command = 0x01;
			else
			{
				switch(DROHead.iVersionMajor)
				{
				case 0:
				case 1:
					if (Command <= 0x01)
						Command = 0xFF;
					break;
				case 2:
					Command = 0xFF;
					break;
				}
			}
			
			// DRO v0/v1 only: "Delay-Command" fix
			if (NoteOn)	// "Delay"-Command during init-phase?
			{
				if (Command < OpBase)	// out of operator range?
				{
					NoteOn = false;	// it's a delay
				}
				else
				{
					OpBase = Command;	// it's a command
					Command = 0xFF;
				}
			}
DRO_CommandSwitch:
			switch(Command)
			{
			case 0x00:	// 1-Byte Delay
				VGMSmplPos += 0x01 + VGMData[VGMPos + 0x01];
				VGMPos += 0x02;
				break;
			case 0x01:	// 2-Byte Delay
				switch(DROHead.iVersionMajor)
				{
				case 0:
				case 1:
					memcpy(&TempSht, &VGMData[VGMPos + 0x01], 0x02);
					if ((TempSht & 0xFF00) == 0xBD00)
					{
						Command = 0xFF;
						goto DRO_CommandSwitch;
					}
					VGMSmplPos += 0x01 + TempSht;
					VGMPos += 0x03;
					break;
				case 2:
					VGMSmplPos += (0x01 + VGMData[VGMPos + 0x01]) << 8;
					VGMPos += 0x02;
					break;
				}
				break;
			case 0x02:	// Use 1st OPL Chip
			case 0x03:	// Use 2nd OPL Chip
				CurChip = Command & 0x01;
				if (CurChip > 0x00 && DROInf.iHardwareType == 0x00)
				{
					//CurChip = 0x00;
					if (! CmdList[0x02])
					{
						printf("More chips used than defined in header!\n");
						CmdList[0x02] = true;
					}
				}
				VGMPos += 0x01;
				break;
			case 0x04:	// Escape
				VGMPos += 0x01;
				// No break (execute following Register)
			default:
				Command = VGMData[VGMPos + 0x00];
				if (DROCodemap)
				{
					CurChip = (Command & 0x80) ? 0x01 : 0x00;
					Command &= 0x7F;
					if (Command < DROInf.iCodemapLength)
						Command = DROCodemap[Command];
					else
						Command = Command;
					switch(DROInf.iHardwareType)
					{
					case 0x00:
						if (CurChip)
						{
							if (! CmdList[0x02])
							{
								printf("More chips used than defined in header!\n");
								CmdList[0x02] = true;
							}
							//CurChip = 0x00;
							//Command = 0x00;
						}
						break;
					case 0x01:
					case 0x02:
						break;
					}
				}
				switch(DROInf.iHardwareType)
				{
				case 0x00:	// OPL 2
					if (CurChip > 0x00)
						break;
					chip_reg_write(0x09, 0x00, 0x00, Command, VGMData[VGMPos + 0x01]);
					break;
				case 0x01:
					chip_reg_write(0x09, CurChip, 0x00, Command, VGMData[VGMPos + 0x01]);
					break;
				case 0x02:	// OPL 3
					chip_reg_write(0x0C, 0x00, CurChip, Command, VGMData[VGMPos + 0x01]);
					break;
				default:
					chip_reg_write(0x09, CurChip, 0x00, Command, VGMData[VGMPos + 0x01]);
					break;
				}
				VGMPos += 0x02;
				break;
			}
			
			if (VGMPos >= VGMDataLen)
			{
				if (VGMHead.lngTotalSamples != (UINT32)VGMSmplPos)
				{
					printf("Warning! Header Samples: %u\t Counted Samples: %u\n",
							VGMHead.lngTotalSamples, VGMSmplPos);
					VGMHead.lngTotalSamples = VGMSmplPos;
					ErrorHappened = true;
				}
				VGMEnd = true;
			}
			if (VGMEnd)
				break;
		}
		break;
	}
	
	return;
}

INLINE INT32 SampleVGM2Playback(INT32 SampleVal)
{
	return (INT32)((INT64)SampleVal * SampleRate / VGMSampleRate);
}

INLINE INT32 SamplePlayback2VGM(INT32 SampleVal)
{
	return (INT32)((INT64)SampleVal * VGMSampleRate / SampleRate);
}

#endif
