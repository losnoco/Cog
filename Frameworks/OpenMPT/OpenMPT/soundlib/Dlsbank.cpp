/*
 * DLSBank.cpp
 * -----------
 * Purpose: Sound bank loading.
 * Notes  : Supported sound bank types: DLS (including embedded DLS in MSS & RMI), SF2
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Mptrack.h"
#include "../common/mptFileIO.h"
#endif
#include "Dlsbank.h"
#include "../common/mptStringBuffer.h"
#include "../common/FileReader.h"
#include "../common/Endianness.h"
#include "SampleIO.h"
#include "modsmp_ctrl.h"

#include <math.h>

OPENMPT_NAMESPACE_BEGIN

#ifdef MODPLUG_TRACKER

#ifdef MPT_ALL_LOGGING
#define DLSBANK_LOG
#define DLSINSTR_LOG
#endif

#define F_RGN_OPTION_SELFNONEXCLUSIVE	0x0001

// Region Flags
enum RegionFlags
{
	DLSREGION_KEYGROUPMASK     = 0x0F,
	DLSREGION_OVERRIDEWSMP     = 0x10,
	DLSREGION_PINGPONGLOOP     = 0x20,
	DLSREGION_SAMPLELOOP       = 0x40,
	DLSREGION_SELFNONEXCLUSIVE = 0x80,
	DLSREGION_SUSTAINLOOP      = 0x100,
	DLSREGION_ISGLOBAL         = 0x200,
};

///////////////////////////////////////////////////////////////////////////
// Articulation connection graph definitions

enum ConnectionSource : uint16
{
	// Generic Sources
	CONN_SRC_NONE              = 0x0000,
	CONN_SRC_LFO               = 0x0001,
	CONN_SRC_KEYONVELOCITY     = 0x0002,
	CONN_SRC_KEYNUMBER         = 0x0003,
	CONN_SRC_EG1               = 0x0004,
	CONN_SRC_EG2               = 0x0005,
	CONN_SRC_PITCHWHEEL        = 0x0006,

	CONN_SRC_POLYPRESSURE      = 0x0007,
	CONN_SRC_CHANNELPRESSURE   = 0x0008,
	CONN_SRC_VIBRATO           = 0x0009,

	// Midi Controllers 0-127
	CONN_SRC_CC1               = 0x0081,
	CONN_SRC_CC7               = 0x0087,
	CONN_SRC_CC10              = 0x008a,
	CONN_SRC_CC11              = 0x008b,

	CONN_SRC_CC91              = 0x00db,
	CONN_SRC_CC93              = 0x00dd,

	CONN_SRC_RPN0              = 0x0100,
	CONN_SRC_RPN1              = 0x0101,
	CONN_SRC_RPN2              = 0x0102,
};

enum ConnectionDestination : uint16
{
	// Generic Destinations
	CONN_DST_NONE              = 0x0000,
	CONN_DST_ATTENUATION       = 0x0001,
	CONN_DST_RESERVED          = 0x0002,
	CONN_DST_PITCH             = 0x0003,
	CONN_DST_PAN               = 0x0004,

	// LFO Destinations
	CONN_DST_LFO_FREQUENCY     = 0x0104,
	CONN_DST_LFO_STARTDELAY    = 0x0105,

	CONN_DST_KEYNUMBER         = 0x0005,

	// EG1 Destinations
	CONN_DST_EG1_ATTACKTIME    = 0x0206,
	CONN_DST_EG1_DECAYTIME     = 0x0207,
	CONN_DST_EG1_RESERVED      = 0x0208,
	CONN_DST_EG1_RELEASETIME   = 0x0209,
	CONN_DST_EG1_SUSTAINLEVEL  = 0x020a,

	CONN_DST_EG1_DELAYTIME     = 0x020b,
	CONN_DST_EG1_HOLDTIME      = 0x020c,
	CONN_DST_EG1_SHUTDOWNTIME  = 0x020d,

	// EG2 Destinations
	CONN_DST_EG2_ATTACKTIME    = 0x030a,
	CONN_DST_EG2_DECAYTIME     = 0x030b,
	CONN_DST_EG2_RESERVED      = 0x030c,
	CONN_DST_EG2_RELEASETIME   = 0x030d,
	CONN_DST_EG2_SUSTAINLEVEL  = 0x030e,

	CONN_DST_EG2_DELAYTIME     = 0x030f,
	CONN_DST_EG2_HOLDTIME      = 0x0310,

	CONN_TRN_NONE              = 0x0000,
	CONN_TRN_CONCAVE           = 0x0001,
};


//////////////////////////////////////////////////////////
// Supported DLS1 Articulations

// [4-bit transform][12-bit dest][8-bit control][8-bit source] = 32-bit ID
constexpr uint32 DLSArt(uint8 src, uint8 ctl, uint16 dst)
{
	return (dst << 16u) | (ctl << 8u) | src;
}

enum DLSArt : uint32
{
	// Vibrato / Tremolo
	ART_LFO_FREQUENCY   = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_LFO_FREQUENCY),
	ART_LFO_STARTDELAY  = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_LFO_STARTDELAY),
	ART_LFO_ATTENUATION = DLSArt(CONN_SRC_LFO,  CONN_SRC_NONE, CONN_DST_ATTENUATION),
	ART_LFO_PITCH       = DLSArt(CONN_SRC_LFO,  CONN_SRC_NONE, CONN_DST_PITCH),
	ART_LFO_MODWTOATTN  = DLSArt(CONN_SRC_LFO,  CONN_SRC_CC1,  CONN_DST_ATTENUATION),
	ART_LFO_MODWTOPITCH = DLSArt(CONN_SRC_LFO,  CONN_SRC_CC1,  CONN_DST_PITCH),

	// Volume Envelope
	ART_VOL_EG_ATTACKTIME   = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG1_ATTACKTIME),
	ART_VOL_EG_DECAYTIME    = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG1_DECAYTIME),
	ART_VOL_EG_SUSTAINLEVEL = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG1_SUSTAINLEVEL),
	ART_VOL_EG_RELEASETIME  = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG1_RELEASETIME),
	ART_VOL_EG_DELAYTIME    = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG1_DELAYTIME),
	ART_VOL_EG_HOLDTIME     = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG1_HOLDTIME),
	ART_VOL_EG_SHUTDOWNTIME = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG1_SHUTDOWNTIME),
	ART_VOL_EG_VELTOATTACK  = DLSArt(CONN_SRC_KEYONVELOCITY, CONN_SRC_NONE, CONN_DST_EG1_ATTACKTIME),
	ART_VOL_EG_KEYTODECAY   = DLSArt(CONN_SRC_KEYNUMBER,     CONN_SRC_NONE, CONN_DST_EG1_DECAYTIME),

	// Pitch Envelope
	ART_PITCH_EG_ATTACKTIME   = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG2_ATTACKTIME),
	ART_PITCH_EG_DECAYTIME    = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG2_DECAYTIME),
	ART_PITCH_EG_SUSTAINLEVEL = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG2_SUSTAINLEVEL),
	ART_PITCH_EG_RELEASETIME  = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_EG2_RELEASETIME),
	ART_PITCH_EG_VELTOATTACK  = DLSArt(CONN_SRC_KEYONVELOCITY, CONN_SRC_NONE, CONN_DST_EG2_ATTACKTIME),
	ART_PITCH_EG_KEYTODECAY   = DLSArt(CONN_SRC_KEYNUMBER,     CONN_SRC_NONE, CONN_DST_EG2_DECAYTIME),

	// Default Pan
	ART_DEFAULTPAN = DLSArt(CONN_SRC_NONE, CONN_SRC_NONE, CONN_DST_PAN),
};

//////////////////////////////////////////////////////////
// DLS IFF Chunk IDs

enum IFFChunkID : uint32
{
	// Standard IFF chunks IDs
	IFFID_FORM = 0x4d524f46,
	IFFID_RIFF = 0x46464952,
	IFFID_LIST = 0x5453494C,
	IFFID_INFO = 0x4F464E49,

	// IFF Info fields
	IFFID_ICOP = 0x504F4349,
	IFFID_INAM = 0x4D414E49,
	IFFID_ICMT = 0x544D4349,
	IFFID_IENG = 0x474E4549,
	IFFID_ISFT = 0x54465349,
	IFFID_ISBJ = 0x4A425349,

	// Wave IFF chunks IDs
	IFFID_wave = 0x65766177,
	IFFID_wsmp = 0x706D7377,

	IFFID_XDLS = 0x534c4458,
	IFFID_DLS  = 0x20534C44,
	IFFID_MLS  = 0x20534C4D,
	IFFID_RMID = 0x44494D52,
	IFFID_colh = 0x686C6F63,
	IFFID_ins  = 0x20736E69,
	IFFID_insh = 0x68736E69,
	IFFID_ptbl = 0x6C627470,
	IFFID_wvpl = 0x6C707677,
	IFFID_rgn  = 0x206E6772,
	IFFID_rgn2 = 0x326E6772,
	IFFID_rgnh = 0x686E6772,
	IFFID_wlnk = 0x6B6E6C77,
	IFFID_art1 = 0x31747261,
	IFFID_art2 = 0x32747261,
};

//////////////////////////////////////////////////////////
// DLS Structures definitions

struct IFFCHUNK
{
	uint32le id;
	uint32le len;
};

MPT_BINARY_STRUCT(IFFCHUNK, 8)

struct RIFFCHUNKID
{
	uint32le id_RIFF;
	uint32le riff_len;
	uint32le id_DLS;
};

MPT_BINARY_STRUCT(RIFFCHUNKID, 12)

struct LISTChunk
{
	uint32le id;
	uint32le len;
	uint32le listid;
};

MPT_BINARY_STRUCT(LISTChunk, 12)

struct DLSRGNRANGE
{
	uint16le usLow;
	uint16le usHigh;
};

MPT_BINARY_STRUCT(DLSRGNRANGE, 4)

struct VERSCHUNK
{
	uint32le id;
	uint32le len;
	uint16le version[4];
};

MPT_BINARY_STRUCT(VERSCHUNK, 16)

struct PTBLCHUNK
{
	uint32le cbSize;
	uint32le cCues;
};

MPT_BINARY_STRUCT(PTBLCHUNK, 8)

struct INSHChunk
{
	uint32le cRegions;
	uint32le ulBank;
	uint32le ulInstrument;
};

MPT_BINARY_STRUCT(INSHChunk, 12)

struct RGNHChunk
{
	DLSRGNRANGE RangeKey;
	DLSRGNRANGE RangeVelocity;
	uint16le fusOptions;
	uint16le usKeyGroup;
};

MPT_BINARY_STRUCT(RGNHChunk, 12)

struct WLNKChunk
{
	uint16le fusOptions;
	uint16le usPhaseGroup;
	uint32le ulChannel;
	uint32le ulTableIndex;
};

MPT_BINARY_STRUCT(WLNKChunk, 12)

struct ART1Chunk
{
	uint32le cbSize;
	uint32le cConnectionBlocks;
};

MPT_BINARY_STRUCT(ART1Chunk, 8)

struct CONNECTIONBLOCK
{
	uint16le usSource;
	uint16le usControl;
	uint16le usDestination;
	uint16le usTransform;
	int32le  lScale;
};

MPT_BINARY_STRUCT(CONNECTIONBLOCK, 12)

struct WSMPCHUNK
{
	uint32le cbSize;
	uint16le usUnityNote;
	int16le  sFineTune;
	int32le  lAttenuation;
	uint32le fulOptions;
	uint32le cSampleLoops;
};

MPT_BINARY_STRUCT(WSMPCHUNK, 20)

struct WSMPSAMPLELOOP
{
	uint32le cbSize;
	uint32le ulLoopType;
	uint32le ulLoopStart;
	uint32le ulLoopLength;

};

MPT_BINARY_STRUCT(WSMPSAMPLELOOP, 16)


/////////////////////////////////////////////////////////////////////
// SF2 IFF Chunk IDs

enum SF2ChunkID : uint32
{
	IFFID_sfbk = 0x6b626673,
	IFFID_sfpk = 0x6b706673,
	IFFID_sdta = 0x61746473,
	IFFID_pdta = 0x61746470,
	IFFID_phdr = 0x72646870,
	IFFID_pbag = 0x67616270,
	IFFID_pgen = 0x6E656770,
	IFFID_inst = 0x74736E69,
	IFFID_ibag = 0x67616269,
	IFFID_igen = 0x6E656769,
	IFFID_shdr = 0x72646873,
};

///////////////////////////////////////////
// SF2 Generators IDs

enum SF2Generators : uint16
{
	SF2_GEN_START_LOOP_FINE		= 2,
	SF2_GEN_END_LOOP_FINE		= 3,
	SF2_GEN_MODENVTOFILTERFC	= 11,
	SF2_GEN_PAN					= 17,
	SF2_GEN_DECAYMODENV			= 28,
	SF2_GEN_ATTACKVOLENV		= 34,
	SF2_GEN_HOLDVOLENV			= 34,
	SF2_GEN_DECAYVOLENV			= 36,
	SF2_GEN_SUSTAINVOLENV		= 37,
	SF2_GEN_RELEASEVOLENV		= 38,
	SF2_GEN_INSTRUMENT			= 41,
	SF2_GEN_KEYRANGE			= 43,
	SF2_GEN_START_LOOP_COARSE	= 45,
	SF2_GEN_ATTENUATION			= 48,
	SF2_GEN_END_LOOP_COARSE		= 50,
	SF2_GEN_COARSETUNE			= 51,
	SF2_GEN_FINETUNE			= 52,
	SF2_GEN_SAMPLEID			= 53,
	SF2_GEN_SAMPLEMODES			= 54,
	SF2_GEN_SCALE_TUNING		= 56,
	SF2_GEN_KEYGROUP			= 57,
	SF2_GEN_UNITYNOTE			= 58,
};

/////////////////////////////////////////////////////////////////////
// SF2 Structures Definitions

struct SFPRESETHEADER
{
	char     achPresetName[20];
	uint16le wPreset;
	uint16le wBank;
	uint16le wPresetBagNdx;
	uint32le dwLibrary;
	uint32le dwGenre;
	uint32le dwMorphology;
};

MPT_BINARY_STRUCT(SFPRESETHEADER, 38)

struct SFPRESETBAG
{
	uint16le wGenNdx;
	uint16le wModNdx;
};

MPT_BINARY_STRUCT(SFPRESETBAG, 4)

struct SFGENLIST
{
	uint16le sfGenOper;
	uint16le genAmount;
};

MPT_BINARY_STRUCT(SFGENLIST, 4)

struct SFINST
{
	char     achInstName[20];
	uint16le wInstBagNdx;
};

MPT_BINARY_STRUCT(SFINST, 22)

struct SFINSTBAG
{
	uint16le wGenNdx;
	uint16le wModNdx;
};

MPT_BINARY_STRUCT(SFINSTBAG, 4)

struct SFINSTGENLIST
{
	uint16le sfGenOper;
	uint16le genAmount;
};

MPT_BINARY_STRUCT(SFINSTGENLIST, 4)

struct SFSAMPLE
{
	char     achSampleName[20];
	uint32le dwStart;
	uint32le dwEnd;
	uint32le dwStartloop;
	uint32le dwEndloop;
	uint32le dwSampleRate;
	uint8le  byOriginalPitch;
	int8le   chPitchCorrection;
	uint16le wSampleLink;
	uint16le sfSampleType;
};

MPT_BINARY_STRUCT(SFSAMPLE, 46)

// End of structures definitions
/////////////////////////////////////////////////////////////////////


struct SF2LoaderInfo
{
	FileReader presetBags;
	FileReader presetGens;
	FileReader insts;
	FileReader instBags;
	FileReader instGens;
};


/////////////////////////////////////////////////////////////////////
// Unit conversion

static uint8 DLSSustainLevelToLinear(int32 sustain)
{
	// 0.1% units
	if(sustain >= 0)
	{
		int32 l = sustain / (1000 * 512);
		if(l >= 0 && l <= 128)
			return static_cast<uint8>(l);
	}
	return 128;
}


static uint8 SF2SustainLevelToLinear(int32 sustain)
{
	// 0.1% units
	int32 l = 128 * (1000 - Clamp(sustain, 0, 1000)) / 1000;
	return static_cast<uint8>(l);
}


int32 CDLSBank::DLS32BitTimeCentsToMilliseconds(int32 lTimeCents)
{
	// tc = log2(time[secs]) * 1200*65536
	// time[secs] = 2^(tc/(1200*65536))
	if ((uint32)lTimeCents == 0x80000000) return 0;
	double fmsecs = 1000.0 * std::pow(2.0, ((double)lTimeCents)/(1200.0*65536.0));
	if (fmsecs < -32767) return -32767;
	if (fmsecs > 32767) return 32767;
	return (int32)fmsecs;
}


// 0dB = 0x10000
int32 CDLSBank::DLS32BitRelativeGainToLinear(int32 lCentibels)
{
	// v = 10^(cb/(200*65536)) * V
	return (int32)(65536.0 * std::pow(10.0, ((double)lCentibels)/(200*65536.0)) );
}


int32 CDLSBank::DLS32BitRelativeLinearToGain(int32 lGain)
{
	// cb = log10(v/V) * 200 * 65536
	if (lGain <= 0) return -960 * 65536;
	return (int32)(200 * 65536.0 * std::log10(((double)lGain) / 65536.0));
}


int32 CDLSBank::DLSMidiVolumeToLinear(uint32 nMidiVolume)
{
	return (nMidiVolume * nMidiVolume << 16) / (127*127);
}


/////////////////////////////////////////////////////////////////////
// Implementation

CDLSBank::CDLSBank()
{
	m_nMaxWaveLink = 0;
	m_nType = SOUNDBANK_TYPE_INVALID;
}


bool CDLSBank::IsDLSBank(const mpt::PathString &filename)
{
	RIFFCHUNKID riff;
	if(filename.empty()) return false;
	mpt::ifstream f(filename, std::ios::binary);
	if(!f)
	{
		return false;
	}
	MemsetZero(riff);
	mpt::IO::Read(f, riff);
	// Check for embedded DLS sections
	if (riff.id_RIFF == IFFID_FORM)
	{
		// Miles Sound System
		do
		{
			uint32 len = mpt::bit_cast<uint32be>(riff.riff_len);
			if (len <= 4) break;
			if (riff.id_DLS == IFFID_XDLS)
			{
				mpt::IO::Read(f, riff);
				break;
			}
			if((len % 2u) != 0)
				len++;
			if (!mpt::IO::SeekRelative(f, len-4)) break;
		} while (mpt::IO::Read(f, riff));
	} else
	if ((riff.id_RIFF == IFFID_RIFF) && (riff.id_DLS == IFFID_RMID))
	{
		for (;;)
		{
			if(!mpt::IO::Read(f, riff))
				break;
			if (riff.id_DLS == IFFID_DLS)
				break; // found it
			int len = riff.riff_len;
			if((len % 2u) != 0)
				len++;
			if ((len <= 4) || !mpt::IO::SeekRelative(f, len-4)) break;
		}
	}
	return ((riff.id_RIFF == IFFID_RIFF)
		&& ((riff.id_DLS == IFFID_DLS) || (riff.id_DLS == IFFID_MLS) || (riff.id_DLS == IFFID_sfbk))
		&& (riff.riff_len >= 256));
}


///////////////////////////////////////////////////////////////
// Find an instrument based on the given parameters

const DLSINSTRUMENT *CDLSBank::FindInstrument(bool isDrum, uint32 bank, uint32 program, uint32 key, uint32 *pInsNo) const
{
	if (m_Instruments.empty()) return nullptr;
	for (uint32 iIns=0; iIns<m_Instruments.size(); iIns++)
	{
		const DLSINSTRUMENT &dlsIns = m_Instruments[iIns];
		uint32 insbank = ((dlsIns.ulBank & 0x7F00) >> 1) | (dlsIns.ulBank & 0x7F);
		if ((bank >= 0x4000) || (insbank == bank))
		{
			if (isDrum)
			{
				if (dlsIns.ulBank & F_INSTRUMENT_DRUMS)
				{
					if ((program >= 0x80) || (program == (dlsIns.ulInstrument & 0x7F)))
					{
						for (uint32 iRgn=0; iRgn<dlsIns.nRegions; iRgn++)
						{
							if ((!key) || (key >= 0x80)
							 || ((key >= dlsIns.Regions[iRgn].uKeyMin)
							  && (key <= dlsIns.Regions[iRgn].uKeyMax)))
							{
								if (pInsNo) *pInsNo = iIns;
								return &dlsIns;
							}
						}
					}
				}
			} else
			{
				if (!(dlsIns.ulBank & F_INSTRUMENT_DRUMS))
				{
					if ((program >= 0x80) || (program == (dlsIns.ulInstrument & 0x7F)))
					{
						if (pInsNo) *pInsNo = iIns;
						return &dlsIns;
					}
				}
			}
		}
	}
	return nullptr;
}


bool CDLSBank::FindAndExtract(CSoundFile &sndFile, const INSTRUMENTINDEX ins, const bool isDrum) const
{
	ModInstrument *pIns = sndFile.Instruments[ins];
	if(pIns == nullptr)
		return false;

	uint32 dlsIns = 0, drumRgn = 0;
	const uint32 program = (pIns->nMidiProgram != 0) ? pIns->nMidiProgram - 1 : 0;
	const uint32 key = isDrum ? (pIns->nMidiDrumKey & 0x7F) : 0xFF;
	if(FindInstrument(isDrum, (pIns->wMidiBank - 1) & 0x3FFF, program, key, &dlsIns)
		|| FindInstrument(isDrum, 0xFFFF, isDrum ? 0xFF : program, key, &dlsIns))
	{
		if(key < 0x80) drumRgn = GetRegionFromKey(dlsIns, key);
		if(ExtractInstrument(sndFile, ins, dlsIns, drumRgn))
		{
			pIns = sndFile.Instruments[ins]; // Reset pointer because ExtractInstrument may delete the previous value.
			if((key >= 24) && (key < 24 + std::size(szMidiPercussionNames)))
			{
				pIns->name = szMidiPercussionNames[key - 24];
			}
			return true;
		}
	}
	return false;
}


///////////////////////////////////////////////////////////////
// Update DLS instrument definition from an IFF chunk

bool CDLSBank::UpdateInstrumentDefinition(DLSINSTRUMENT *pDlsIns, FileReader chunk)
{
	IFFCHUNK header;
	chunk.ReadStruct(header);
	if(!header.len || !chunk.CanRead(header.len))
		return false;
	if(header.id == IFFID_LIST)
	{
		uint32 listid = chunk.ReadUint32LE();
		while(chunk.CanRead(sizeof(IFFCHUNK)))
		{
			IFFCHUNK subHeader;
			chunk.ReadStruct(subHeader);
			chunk.SkipBack(sizeof(IFFCHUNK));
			FileReader subData = chunk.ReadChunk(subHeader.len + sizeof(IFFCHUNK));
			if(subHeader.len & 1)
			{
				chunk.Skip(1);
			}
			UpdateInstrumentDefinition(pDlsIns, subData);
		}
		switch(listid)
		{
		case IFFID_rgn:		// Level 1 region
		case IFFID_rgn2:	// Level 2 region
			if (pDlsIns->nRegions < DLSMAXREGIONS) pDlsIns->nRegions++;
			break;
		}
	} else
	{
		switch(header.id)
		{
		case IFFID_insh:
		{
			INSHChunk insh;
			chunk.ReadStruct(insh);
			pDlsIns->ulBank = insh.ulBank;
			pDlsIns->ulInstrument = insh.ulInstrument;
			//Log("%3d regions, bank 0x%04X instrument %3d\n", insh.cRegions, pDlsIns->ulBank, pDlsIns->ulInstrument);
			break;
		}

		case IFFID_rgnh:
			if (pDlsIns->nRegions < DLSMAXREGIONS)
			{
				RGNHChunk rgnh;
				chunk.ReadStruct(rgnh);
				DLSREGION &region = pDlsIns->Regions[pDlsIns->nRegions];
				region.uKeyMin = (uint8)rgnh.RangeKey.usLow;
				region.uKeyMax = (uint8)rgnh.RangeKey.usHigh;
				region.fuOptions = (uint8)(rgnh.usKeyGroup & DLSREGION_KEYGROUPMASK);
				if(rgnh.fusOptions & F_RGN_OPTION_SELFNONEXCLUSIVE)
					region.fuOptions |= DLSREGION_SELFNONEXCLUSIVE;
				//Log("  Region %d: fusOptions=0x%02X usKeyGroup=0x%04X ", pDlsIns->nRegions, rgnh.fusOptions, rgnh.usKeyGroup);
				//Log("KeyRange[%3d,%3d] ", rgnh.RangeKey.usLow, rgnh.RangeKey.usHigh);
			}
			break;

		case IFFID_wlnk:
			if (pDlsIns->nRegions < DLSMAXREGIONS)
			{
				WLNKChunk wlnk;
				chunk.ReadStruct(wlnk);
				DLSREGION &region = pDlsIns->Regions[pDlsIns->nRegions];
				region.nWaveLink = (uint16)wlnk.ulTableIndex;
				if((region.nWaveLink < Util::MaxValueOfType(region.nWaveLink)) && (region.nWaveLink >= m_nMaxWaveLink))
					m_nMaxWaveLink = region.nWaveLink + 1;
				//Log("  WaveLink %d: fusOptions=0x%02X usPhaseGroup=0x%04X ", pDlsIns->nRegions, wlnk.fusOptions, wlnk.usPhaseGroup);
				//Log("ulChannel=%d ulTableIndex=%4d\n", wlnk.ulChannel, wlnk.ulTableIndex);
			}
			break;

		case IFFID_wsmp:
			if (pDlsIns->nRegions < DLSMAXREGIONS)
			{
				DLSREGION &region = pDlsIns->Regions[pDlsIns->nRegions];
				WSMPCHUNK wsmp;
				chunk.ReadStruct(wsmp);
				region.fuOptions |= DLSREGION_OVERRIDEWSMP;
				region.uUnityNote = (uint8)wsmp.usUnityNote;
				region.sFineTune = wsmp.sFineTune;
				int32 lVolume = DLS32BitRelativeGainToLinear(wsmp.lAttenuation) / 256;
				if (lVolume > 256) lVolume = 256;
				if (lVolume < 4) lVolume = 4;
				region.usVolume = (uint16)lVolume;
				//Log("  WaveSample %d: usUnityNote=%2d sFineTune=%3d ", pDlsEnv->nRegions, p->usUnityNote, p->sFineTune);
				//Log("fulOptions=0x%04X loops=%d\n", p->fulOptions, p->cSampleLoops);
				if((wsmp.cSampleLoops) && (wsmp.cbSize + sizeof(WSMPSAMPLELOOP) <= header.len))
				{
					WSMPSAMPLELOOP loop;
					chunk.Seek(sizeof(IFFCHUNK) + wsmp.cbSize);
					chunk.ReadStruct(loop);
					//Log("looptype=%2d loopstart=%5d loopend=%5d\n", ploop->ulLoopType, ploop->ulLoopStart, ploop->ulLoopLength);
					if(loop.ulLoopLength > 3)
					{
						region.fuOptions |= DLSREGION_SAMPLELOOP;
						//if(loop.ulLoopType) region.fuOptions |= DLSREGION_PINGPONGLOOP;
						region.ulLoopStart = loop.ulLoopStart;
						region.ulLoopEnd = loop.ulLoopStart + loop.ulLoopLength;
					}
				}
			}
			break;

		case IFFID_art1:
		case IFFID_art2:
			{
				ART1Chunk art1;
				chunk.ReadStruct(art1);
				if (pDlsIns->ulBank & F_INSTRUMENT_DRUMS)
				{
					if (pDlsIns->nRegions >= DLSMAXREGIONS) break;
				} else
				{
					pDlsIns->nMelodicEnv = static_cast<uint32>(m_Envelopes.size() + 1);
				}
				if(art1.cbSize + art1.cConnectionBlocks * sizeof(CONNECTIONBLOCK) > header.len)
					break;
				DLSENVELOPE dlsEnv;
				MemsetZero(dlsEnv);
				dlsEnv.nDefPan = 128;
				dlsEnv.nVolSustainLevel = 128;
				//Log("  art1 (%3d bytes): cbSize=%d cConnectionBlocks=%d\n", p->len, p->cbSize, p->cConnectionBlocks);
				chunk.Seek(sizeof(IFFCHUNK) + art1.cbSize);
				for (uint32 iblk = 0; iblk < art1.cConnectionBlocks; iblk++)
				{
					CONNECTIONBLOCK blk;
					chunk.ReadStruct(blk);
					// [4-bit transform][12-bit dest][8-bit control][8-bit source] = 32-bit ID
					uint32 dwArticulation = blk.usTransform;
					dwArticulation = (dwArticulation << 12) | (blk.usDestination & 0x0FFF);
					dwArticulation = (dwArticulation << 8) | (blk.usControl & 0x00FF);
					dwArticulation = (dwArticulation << 8) | (blk.usSource & 0x00FF);
					switch(dwArticulation)
					{
					case ART_DEFAULTPAN:
						{
							int32 pan = 128 + blk.lScale / (65536000/128);
							if (pan < 0) pan = 0;
							if (pan > 255) pan = 255;
							dlsEnv.nDefPan = (uint8)pan;
						}
						break;

					case ART_VOL_EG_ATTACKTIME:
						// 32-bit time cents units. range = [0s, 20s]
						dlsEnv.wVolAttack = 0;
						if(blk.lScale > -0x40000000)
						{
							int32 l = blk.lScale - 78743200; // maximum velocity
							if (l > 0) l = 0;
							int32 attacktime = DLS32BitTimeCentsToMilliseconds(l);
							if (attacktime < 0) attacktime = 0;
							if (attacktime > 20000) attacktime = 20000;
							if (attacktime >= 20) dlsEnv.wVolAttack = (uint16)(attacktime / 20);
							//Log("%3d: Envelope Attack Time set to %d (%d time cents)\n", (uint32)(dlsEnv.ulInstrument & 0x7F)|((dlsEnv.ulBank >> 16) & 0x8000), attacktime, pblk->lScale);
						}
						break;

					case ART_VOL_EG_DECAYTIME:
						// 32-bit time cents units. range = [0s, 20s]
						dlsEnv.wVolDecay = 0;
						if(blk.lScale > -0x40000000)
						{
							int32 decaytime = DLS32BitTimeCentsToMilliseconds(blk.lScale);
							if (decaytime > 20000) decaytime = 20000;
							if (decaytime >= 20) dlsEnv.wVolDecay = (uint16)(decaytime / 20);
							//Log("%3d: Envelope Decay Time set to %d (%d time cents)\n", (uint32)(dlsEnv.ulInstrument & 0x7F)|((dlsEnv.ulBank >> 16) & 0x8000), decaytime, pblk->lScale);
						}
						break;

					case ART_VOL_EG_RELEASETIME:
						// 32-bit time cents units. range = [0s, 20s]
						dlsEnv.wVolRelease = 0;
						if(blk.lScale > -0x40000000)
						{
							int32 releasetime = DLS32BitTimeCentsToMilliseconds(blk.lScale);
							if (releasetime > 20000) releasetime = 20000;
							if (releasetime >= 20) dlsEnv.wVolRelease = (uint16)(releasetime / 20);
							//Log("%3d: Envelope Release Time set to %d (%d time cents)\n", (uint32)(dlsEnv.ulInstrument & 0x7F)|((dlsEnv.ulBank >> 16) & 0x8000), dlsEnv.wVolRelease, pblk->lScale);
						}
						break;

					case ART_VOL_EG_SUSTAINLEVEL:
						// 0.1% units
						if(blk.lScale >= 0)
						{
							dlsEnv.nVolSustainLevel = DLSSustainLevelToLinear(blk.lScale);
						}
						break;

					//default:
					//	Log("    Articulation = 0x%08X value=%d\n", dwArticulation, blk.lScale);
					}
				}
				m_Envelopes.push_back(dlsEnv);
			}
			break;

		case IFFID_INAM:
			chunk.ReadString<mpt::String::spacePadded>(pDlsIns->szName, header.len);
			break;
	#if 0
		default:
			{
				char sid[5];
				memcpy(sid, &header.id, 4);
				sid[4] = 0;
				Log("    \"%s\": %d bytes\n", (uint32)sid, header.len.get());
			}
	#endif
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////
// Converts SF2 chunks to DLS

bool CDLSBank::UpdateSF2PresetData(SF2LoaderInfo &sf2info, const IFFCHUNK &header, FileReader &chunk)
{
	if (!chunk.IsValid()) return false;
	switch(header.id)
	{
	case IFFID_phdr:
		if(m_Instruments.empty())
		{
			uint32 numIns = static_cast<uint32>(chunk.GetLength() / sizeof(SFPRESETHEADER));
			if(numIns <= 1)
				break;
			// The terminal sfPresetHeader record should never be accessed, and exists only to provide a terminal wPresetBagNdx with which to determine the number of zones in the last preset.
			numIns--;
			m_Instruments.resize(numIns);

		#ifdef DLSBANK_LOG
			MPT_LOG(LogDebug, "DLSBank", mpt::format(U_("phdr: %1 instruments"))(m_Instruments.size()));
		#endif
			SFPRESETHEADER psfh;
			chunk.ReadStruct(psfh);
			for (auto &dlsIns : m_Instruments)
			{
				mpt::String::WriteAutoBuf(dlsIns.szName) = mpt::String::ReadAutoBuf(psfh.achPresetName);
				dlsIns.ulInstrument = psfh.wPreset & 0x7F;
				dlsIns.ulBank = (psfh.wBank >= 128) ? F_INSTRUMENT_DRUMS : (psfh.wBank << 8);
				dlsIns.wPresetBagNdx = psfh.wPresetBagNdx;
				dlsIns.wPresetBagNum = 1;
				chunk.ReadStruct(psfh);
				if (psfh.wPresetBagNdx > dlsIns.wPresetBagNdx) dlsIns.wPresetBagNum = static_cast<uint16>(psfh.wPresetBagNdx - dlsIns.wPresetBagNdx);
			}
		}
		break;

	case IFFID_pbag:
		if(!m_Instruments.empty() && chunk.CanRead(sizeof(SFPRESETBAG)))
		{
			sf2info.presetBags = chunk.GetChunk(chunk.BytesLeft());
		}
	#ifdef DLSINSTR_LOG
		else MPT_LOG(LogDebug, "DLSINSTR", U_("pbag: no instruments!"));
	#endif
		break;

	case IFFID_pgen:
		if(!m_Instruments.empty() && chunk.CanRead(sizeof(SFGENLIST)))
		{
			sf2info.presetGens = chunk.GetChunk(chunk.BytesLeft());
		}
	#ifdef DLSINSTR_LOG
		else MPT_LOG(LogDebug, "DLSINSTR", U_("pgen: no instruments!"));
	#endif
		break;

	case IFFID_inst:
		if(!m_Instruments.empty() && chunk.CanRead(sizeof(SFINST)))
		{
			sf2info.insts = chunk.GetChunk(chunk.BytesLeft());
		}
		break;

	case IFFID_ibag:
		if(!m_Instruments.empty() && chunk.CanRead(sizeof(SFINSTBAG)))
		{
			sf2info.instBags = chunk.GetChunk(chunk.BytesLeft());
		}
		break;

	case IFFID_igen:
		if(!m_Instruments.empty() && chunk.CanRead(sizeof(SFINSTGENLIST)))
		{
			sf2info.instGens = chunk.GetChunk(chunk.BytesLeft());
		}
		break;

	case IFFID_shdr:
		if (m_SamplesEx.empty())
		{
			uint32 numSmp = static_cast<uint32>(chunk.GetLength() / sizeof(SFSAMPLE));
			if (numSmp < 1) break;
			m_SamplesEx.resize(numSmp);
			m_WaveForms.resize(numSmp);
			#ifdef DLSINSTR_LOG
				MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("shdr: %1 samples"))(m_SamplesEx.size()));
			#endif

			for (uint32 i = 0; i < numSmp; i++)
			{
				SFSAMPLE p;
				chunk.ReadStruct(p);
				DLSSAMPLEEX &dlsSmp = m_SamplesEx[i];
				mpt::String::WriteAutoBuf(dlsSmp.szName) = mpt::String::ReadAutoBuf(p.achSampleName);
				dlsSmp.dwLen = 0;
				dlsSmp.dwSampleRate = p.dwSampleRate;
				dlsSmp.byOriginalPitch = p.byOriginalPitch;
				dlsSmp.chPitchCorrection = static_cast<int8>(Util::muldivr(p.chPitchCorrection, 128, 100));
				if (((p.sfSampleType & 0x7FFF) <= 4) && (p.dwEnd >= p.dwStart + 4))
				{
					dlsSmp.dwLen = (p.dwEnd - p.dwStart) * 2;
					if ((p.dwEndloop > p.dwStartloop + 7) && (p.dwStartloop >= p.dwStart))
					{
						dlsSmp.dwStartloop = p.dwStartloop - p.dwStart;
						dlsSmp.dwEndloop = p.dwEndloop - p.dwStart;
					}
					m_WaveForms[i] = p.dwStart * 2;
					//Log("  offset[%d]=%d len=%d\n", i, p.dwStart*2, psmp->dwLen);
				}
			}
		}
		break;

	#ifdef DLSINSTR_LOG
	default:
		{
			char sdbg[5];
			memcpy(sdbg, &header.id, 4);
			sdbg[4] = 0;
			MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("Unsupported SF2 chunk: %1 (%2 bytes)"))(mpt::ToUnicode(mpt::Charset::ASCII, mpt::String::ReadAutoBuf(sdbg)), header.len.get()));
		}
	#endif
	}
	return true;
}


static int16 SF2TimeToDLS(int16 amount)
{
	int32 time = CDLSBank::DLS32BitTimeCentsToMilliseconds(static_cast<int32>(amount) << 16);
	return static_cast<int16>(Clamp(time, 20, 20000) / 20);
}


// Convert all instruments to the DLS format
bool CDLSBank::ConvertSF2ToDLS(SF2LoaderInfo &sf2info)
{
	if (m_Instruments.empty() || m_SamplesEx.empty())
		return false;

	const uint32 numPresetBags = static_cast<uint32>(sf2info.presetBags.GetLength() / sizeof(SFPRESETBAG));
	const uint32 numPresetGens = static_cast<uint32>(sf2info.presetGens.GetLength() / sizeof(SFGENLIST));
	const uint32 numInsts = static_cast<uint32>(sf2info.insts.GetLength() / sizeof(SFINST));
	const uint32 numInstBags = static_cast<uint32>(sf2info.instBags.GetLength() / sizeof(SFINSTBAG));
	const uint32 numInstGens = static_cast<uint32>(sf2info.instGens.GetLength() / sizeof(SFINSTGENLIST));

	for (auto &dlsIns : m_Instruments)
	{
		DLSENVELOPE dlsEnv;
		std::vector<uint32> instruments;
		int32 instrAttenuation = 0;
		// Default Envelope Values
		dlsEnv.wVolAttack = 0;
		dlsEnv.wVolDecay = 0;
		dlsEnv.wVolRelease = 0;
		dlsEnv.nVolSustainLevel = 128;
		dlsEnv.nDefPan = 128;
		// Load Preset Bags
		sf2info.presetBags.Seek(dlsIns.wPresetBagNdx * sizeof(SFPRESETBAG));
		for (uint32 ipbagcnt=0; ipbagcnt<(uint32)dlsIns.wPresetBagNum; ipbagcnt++)
		{
			// Load generators for each preset bag
			SFPRESETBAG bag[2];
			if(!sf2info.presetBags.ReadArray(bag))
				break;
			sf2info.presetBags.SkipBack(sizeof(SFPRESETBAG));

			sf2info.presetGens.Seek(bag[0].wGenNdx * sizeof(SFGENLIST));
			for (uint32 ipgenndx = bag[0].wGenNdx; ipgenndx < bag[1].wGenNdx; ipgenndx++)
			{
				SFGENLIST gen;
				if(!sf2info.presetGens.ReadStruct(gen))
					break;
				switch(gen.sfGenOper)
				{
				case SF2_GEN_ATTACKVOLENV:
					dlsEnv.wVolAttack = SF2TimeToDLS(gen.genAmount);
					break;
				case SF2_GEN_DECAYVOLENV:
					dlsEnv.wVolDecay = SF2TimeToDLS(gen.genAmount);
					break;
				case SF2_GEN_SUSTAINVOLENV:
					// 0.1% units
					if(gen.genAmount >= 0)
					{
						dlsEnv.nVolSustainLevel = SF2SustainLevelToLinear(gen.genAmount);
					}
					break;
				case SF2_GEN_RELEASEVOLENV:
					dlsEnv.wVolRelease = SF2TimeToDLS(gen.genAmount);
					break;
				case SF2_GEN_INSTRUMENT:
					if(std::find(instruments.begin(), instruments.end(), gen.genAmount) == instruments.end())
						instruments.push_back(gen.genAmount);
					break;
				case SF2_GEN_ATTENUATION:
					instrAttenuation = -static_cast<int16>(gen.genAmount);
					break;
#if 0
				default:
					Log("Ins %3d: bag %3d gen %3d: ", nIns, ipbagndx, ipgenndx);
					Log("genoper=%d amount=0x%04X ", gen.sfGenOper, gen.genAmount);
					Log((pSmp->ulBank & F_INSTRUMENT_DRUMS) ? "(drum)\n" : "\n");
#endif
				}
			}
		}
		// Envelope
		if (!(dlsIns.ulBank & F_INSTRUMENT_DRUMS))
		{
			m_Envelopes.push_back(dlsEnv);
			dlsIns.nMelodicEnv = static_cast<uint32>(m_Envelopes.size());
		}
		// Load Instrument Bags
		dlsIns.nRegions = 0;
		for(const auto nInstrNdx : instruments)
		{
			if(nInstrNdx >= numInsts)
				continue;
			sf2info.insts.Seek(nInstrNdx * sizeof(SFINST));
			SFINST insts[2];
			sf2info.insts.ReadArray(insts);
			const auto startRegion = dlsIns.nRegions;
			dlsIns.nRegions += insts[1].wInstBagNdx - insts[0].wInstBagNdx;
			//Log("\nIns %3d, %2d regions:\n", nIns, pSmp->nRegions);
			if (dlsIns.nRegions > DLSMAXREGIONS) dlsIns.nRegions = DLSMAXREGIONS;
			DLSREGION *pRgn = &dlsIns.Regions[startRegion];
			bool hasGlobalZone = false;
			for(uint32 nRgn = startRegion; nRgn < dlsIns.nRegions; nRgn++, pRgn++)
			{
				uint32 ibagcnt = insts[0].wInstBagNdx + nRgn - startRegion;
				if(ibagcnt >= numInstBags)
					break;
				// Create a new envelope for drums
				DLSENVELOPE *pDlsEnv = &dlsEnv;
				if(!(dlsIns.ulBank & F_INSTRUMENT_DRUMS) && dlsIns.nMelodicEnv > 0 && dlsIns.nMelodicEnv <= m_Envelopes.size())
				{
					pDlsEnv = &m_Envelopes[dlsIns.nMelodicEnv - 1];
				}
				// Region Default Values
				int32 regionAttn = 0;
				pRgn->uKeyMin = 0;
				pRgn->uKeyMax = 127;
				pRgn->uUnityNote = 0xFF;  // 0xFF means undefined -> use sample root note
				pRgn->tuning = 100;
				pRgn->sFineTune = 0;
				pRgn->nWaveLink = Util::MaxValueOfType(pRgn->nWaveLink);
				if(hasGlobalZone)
					*pRgn = dlsIns.Regions[startRegion];
				// Load Generators
				sf2info.instBags.Seek(ibagcnt * sizeof(SFINSTBAG));
				SFINSTBAG bags[2];
				sf2info.instBags.ReadArray(bags);
				sf2info.instGens.Seek(bags[0].wGenNdx * sizeof(SFINSTGENLIST));
				uint16 lastOp = SF2_GEN_SAMPLEID;
				int32 loopStart = 0, loopEnd = 0;
				for(uint32 igenndx = bags[0].wGenNdx; igenndx < bags[1].wGenNdx; igenndx++)
				{
					if(igenndx >= numInstGens)
						break;
					SFINSTGENLIST gen;
					sf2info.instGens.ReadStruct(gen);
					uint16 value = gen.genAmount;
					lastOp = gen.sfGenOper;

					switch(gen.sfGenOper)
					{
					case SF2_GEN_KEYRANGE:
						pRgn->uKeyMin = (uint8)(value & 0xFF);
						pRgn->uKeyMax = (uint8)(value >> 8);
						if(pRgn->uKeyMin > pRgn->uKeyMax)
							std::swap(pRgn->uKeyMin, pRgn->uKeyMax);
						break;
					case SF2_GEN_UNITYNOTE:
						if (value < 128) pRgn->uUnityNote = (uint8)value;
						break;
					case SF2_GEN_ATTACKVOLENV:
						pDlsEnv->wVolAttack = SF2TimeToDLS(gen.genAmount);
						break;
					case SF2_GEN_DECAYVOLENV:
						pDlsEnv->wVolDecay = SF2TimeToDLS(gen.genAmount);
						break;
					case SF2_GEN_SUSTAINVOLENV:
						// 0.1% units
						if(gen.genAmount >= 0)
						{
							pDlsEnv->nVolSustainLevel = SF2SustainLevelToLinear(gen.genAmount);
						}
						break;
					case SF2_GEN_RELEASEVOLENV:
						pDlsEnv->wVolRelease = SF2TimeToDLS(gen.genAmount);
						break;
					case SF2_GEN_PAN:
						{
							int32 pan = static_cast<int16>(value);
							pan = (((pan + 500) * 127) / 500) + 128;
							pDlsEnv->nDefPan = mpt::saturate_cast<uint8>(pan);
						}
						break;
					case SF2_GEN_ATTENUATION:
						regionAttn = -static_cast<int16>(value);
						break;
					case SF2_GEN_SAMPLEID:
						if (value < m_SamplesEx.size())
						{
							pRgn->nWaveLink = value;
							pRgn->ulLoopStart = mpt::saturate_cast<uint32>(m_SamplesEx[value].dwStartloop + loopStart);
							pRgn->ulLoopEnd = mpt::saturate_cast<uint32>(m_SamplesEx[value].dwEndloop + loopEnd);
						}
						break;
					case SF2_GEN_SAMPLEMODES:
						value &= 3;
						pRgn->fuOptions &= uint16(~(DLSREGION_SAMPLELOOP|DLSREGION_PINGPONGLOOP|DLSREGION_SUSTAINLOOP));
						if(value == 1)
							pRgn->fuOptions |= DLSREGION_SAMPLELOOP;
						else if(value == 2)
							pRgn->fuOptions |= DLSREGION_SAMPLELOOP | DLSREGION_PINGPONGLOOP;
						else if(value == 3)
							pRgn->fuOptions |= DLSREGION_SAMPLELOOP | DLSREGION_SUSTAINLOOP;
						pRgn->fuOptions |= DLSREGION_OVERRIDEWSMP;
						break;
					case SF2_GEN_KEYGROUP:
						pRgn->fuOptions |= (value & DLSREGION_KEYGROUPMASK);
						break;
					case SF2_GEN_COARSETUNE:
						pRgn->sFineTune += static_cast<int16>(value) * 128;
						break;
					case SF2_GEN_FINETUNE:
						pRgn->sFineTune += static_cast<int16>(Util::muldiv(static_cast<int8>(value), 128, 100));
						break;
					case SF2_GEN_SCALE_TUNING:
						pRgn->tuning = mpt::saturate_cast<uint8>(value);
						break;
					case SF2_GEN_START_LOOP_FINE:
						loopStart += static_cast<int16>(value);
						break;
					case SF2_GEN_END_LOOP_FINE:
						loopEnd += static_cast<int16>(value);
						break;
					case SF2_GEN_START_LOOP_COARSE:
						loopStart += static_cast<int16>(value) * 32768;
						break;
					case SF2_GEN_END_LOOP_COARSE:
						loopEnd += static_cast<int16>(value) * 32768;
						break;
					//default:
					//	Log("    gen=%d value=%04X\n", pgen->sfGenOper, pgen->genAmount);
					}
				}
				if(lastOp != SF2_GEN_SAMPLEID && nRgn == startRegion)
				{
					hasGlobalZone = true;
					pRgn->fuOptions |= DLSREGION_ISGLOBAL;
				}
				int32 linearVol = DLS32BitRelativeGainToLinear(((instrAttenuation + regionAttn) * 65536) / 10) / 256;
				Limit(linearVol, 16, 256);
				pRgn->usVolume = static_cast<uint16>(linearVol);
				//Log("\n");
			}
		}
	}
	return true;
}


///////////////////////////////////////////////////////////////
// Open: opens a DLS bank

bool CDLSBank::Open(const mpt::PathString &filename)
{
	if(filename.empty()) return false;
	m_szFileName = filename;
	InputFile f(filename, SettingCacheCompleteFileBeforeLoading());
	if(!f.IsValid()) return false;
	return Open(GetFileReader(f));
}


bool CDLSBank::Open(FileReader file)
{
	uint32 nInsDef;

	if(!file.GetFileName().empty())
		m_szFileName = file.GetFileName();

	file.Rewind();
	size_t dwMemLength = file.GetLength();
	size_t dwMemPos = 0;
	if(!file.CanRead(256))
	{
		return false;
	}

	RIFFCHUNKID riff;
	file.ReadStruct(riff);
	// Check DLS sections embedded in RMI midi files
	if(riff.id_RIFF == IFFID_RIFF && riff.id_DLS == IFFID_RMID)
	{
		while(file.ReadStruct(riff))
		{
			if(riff.id_RIFF == IFFID_RIFF && riff.id_DLS == IFFID_DLS)
			{
				file.SkipBack(sizeof(riff));
				break;
			}
			uint32 len = riff.riff_len;
			if((len % 2u) != 0)
				len++;
			file.SkipBack(4);
			file.Skip(len);
		}
	}

	// Check XDLS sections embedded in big endian IFF files (Miles Sound System)
	if (riff.id_RIFF == IFFID_FORM)
	{
		do
		{
			if(riff.id_DLS == IFFID_XDLS)
			{
				file.ReadStruct(riff);
				break;
			}
			uint32 len = mpt::bit_cast<uint32be>(riff.riff_len);
			if((len % 2u) != 0)
				len++;
			file.SkipBack(4);
			file.Skip(len);
		} while(file.ReadStruct(riff));
	}
	if (riff.id_RIFF != IFFID_RIFF
		|| (riff.id_DLS != IFFID_DLS && riff.id_DLS != IFFID_MLS && riff.id_DLS != IFFID_sfbk)
		|| !file.CanRead(riff.riff_len - 4))
	{
	#ifdef DLSBANK_LOG
		MPT_LOG(LogDebug, "DLSBANK", U_("Invalid DLS bank!"));
	#endif
		return false;
	}
	SF2LoaderInfo sf2info;
	m_nType = (riff.id_DLS == IFFID_sfbk) ? SOUNDBANK_TYPE_SF2 : SOUNDBANK_TYPE_DLS;
	m_dwWavePoolOffset = 0;
	m_Instruments.clear();
	m_WaveForms.clear();
	m_Envelopes.clear();
	nInsDef = 0;
	if (dwMemLength > 8 + riff.riff_len + dwMemPos) dwMemLength = 8 + riff.riff_len + dwMemPos;
	while(file.CanRead(sizeof(IFFCHUNK)))
	{
		IFFCHUNK chunkHeader;
		file.ReadStruct(chunkHeader);
		dwMemPos = file.GetPosition();
		FileReader chunk = file.ReadChunk(chunkHeader.len);
		if(chunkHeader.len % 2u)
			file.Skip(1);

		if(!chunk.LengthIsAtLeast(chunkHeader.len))
			break;

		switch(chunkHeader.id)
		{
		// DLS 1.0: Instruments Collection Header
		case IFFID_colh:
		#ifdef DLSBANK_LOG
			MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("colh (%1 bytes)"))(chunkHeader.len.get()));
		#endif
			if (m_Instruments.empty())
			{
				m_Instruments.resize(chunk.ReadUint32LE());
			#ifdef DLSBANK_LOG
				MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("  %1 instruments"))(m_Instruments.size()));
			#endif
			}
			break;

		// DLS 1.0: Instruments Pointers Table
		case IFFID_ptbl:
		#ifdef DLSBANK_LOG
			MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("ptbl (%1 bytes)"))(chunkHeader.len.get()));
		#endif
			if (m_WaveForms.empty())
			{
				PTBLCHUNK ptbl;
				chunk.ReadStruct(ptbl);
				chunk.Skip(ptbl.cbSize - 8);
				uint32 cues = std::min(ptbl.cCues.get(), mpt::saturate_cast<uint32>(chunk.BytesLeft() / sizeof(uint32)));
				m_WaveForms.reserve(cues);
				for(uint32 i = 0; i < cues; i++)
				{
					m_WaveForms.push_back(chunk.ReadUint32LE());
				}
			#ifdef DLSBANK_LOG
				MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("  %1 waveforms"))(m_WaveForms.size()));
			#endif
			}
			break;

		// DLS 1.0: LIST section
		case IFFID_LIST:
		#ifdef DLSBANK_LOG
			MPT_LOG(LogDebug, "DLSBANK", U_("LIST"));
		#endif
			{
				uint32 listid = chunk.ReadUint32LE();
				if (((listid == IFFID_wvpl) && (m_nType & SOUNDBANK_TYPE_DLS))
				 || ((listid == IFFID_sdta) && (m_nType & SOUNDBANK_TYPE_SF2)))
				{
					m_dwWavePoolOffset = dwMemPos + 4;
				#ifdef DLSBANK_LOG
					MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("Wave Pool offset: %1"))(m_dwWavePoolOffset));
				#endif
					break;
				}

				while (chunk.CanRead(12))
				{
					IFFCHUNK listHeader;
					chunk.ReadStruct(listHeader);

					if(!chunk.CanRead(listHeader.len))
						break;

					FileReader subData = chunk.GetChunkAt(chunk.GetPosition() - sizeof(IFFCHUNK), listHeader.len + 8);
					FileReader listChunk = chunk.ReadChunk(listHeader.len);
					if(listHeader.len % 2u)
						chunk.Skip(1);
					// DLS Instrument Headers
					if (listHeader.id == IFFID_LIST && (m_nType & SOUNDBANK_TYPE_DLS))
					{
						uint32 subID = listChunk.ReadUint32LE();
						if ((subID == IFFID_ins) && (nInsDef < m_Instruments.size()))
						{
							DLSINSTRUMENT *pDlsIns = &m_Instruments[nInsDef];
							//Log("Instrument %d:\n", nInsDef);
							UpdateInstrumentDefinition(pDlsIns, subData);
							nInsDef++;
						}
					} else
					// DLS/SF2 Bank Information
					if (listid == IFFID_INFO && listHeader.len)
					{
						switch(listHeader.id)
						{
						case IFFID_INAM:
							listChunk.ReadString<mpt::String::maybeNullTerminated>(m_BankInfo.szBankName, listChunk.BytesLeft());
							break;
						case IFFID_IENG:
							listChunk.ReadString<mpt::String::maybeNullTerminated>(m_BankInfo.szEngineer, listChunk.BytesLeft());
							break;
						case IFFID_ICOP:
							listChunk.ReadString<mpt::String::maybeNullTerminated>(m_BankInfo.szCopyRight, listChunk.BytesLeft());
							break;
						case IFFID_ICMT:
							listChunk.ReadString<mpt::String::maybeNullTerminated>(m_BankInfo.szComments, listChunk.BytesLeft());
							break;
						case IFFID_ISFT:
							listChunk.ReadString<mpt::String::maybeNullTerminated>(m_BankInfo.szSoftware, listChunk.BytesLeft());
							break;
						case IFFID_ISBJ:
							listChunk.ReadString<mpt::String::maybeNullTerminated>(m_BankInfo.szDescription, listChunk.BytesLeft());
							break;
						}
					} else
					if ((listid == IFFID_pdta) && (m_nType & SOUNDBANK_TYPE_SF2))
					{
						UpdateSF2PresetData(sf2info, listHeader, listChunk);
					}
				}
			}
			break;

		#ifdef DLSBANK_LOG
		default:
			{
				char sdbg[5];
				memcpy(sdbg, &chunkHeader.id, 4);
				sdbg[4] = 0;
				MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("Unsupported chunk: %1 (%2 bytes)"))(mpt::ToUnicode(mpt::Charset::ASCII, mpt::String::ReadAutoBuf(sdbg)), chunkHeader.len.get()));
			}
			break;
		#endif
		}
	}
	// Build the ptbl is not present in file
	if ((m_WaveForms.empty()) && (m_dwWavePoolOffset) && (m_nType & SOUNDBANK_TYPE_DLS) && (m_nMaxWaveLink > 0))
	{
	#ifdef DLSBANK_LOG
		MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("ptbl not present: building table (%1 wavelinks)..."))(m_nMaxWaveLink));
	#endif
		m_WaveForms.reserve(m_nMaxWaveLink);
		file.Seek(m_dwWavePoolOffset);
		while(m_WaveForms.size() < m_nMaxWaveLink && file.CanRead(sizeof(IFFCHUNK)))
		{
			IFFCHUNK chunk;
			file.ReadStruct(chunk);
			if (chunk.id == IFFID_LIST)
				m_WaveForms.push_back(file.GetPosition() - m_dwWavePoolOffset - sizeof(IFFCHUNK));
			file.Skip(chunk.len);
		}
#ifdef DLSBANK_LOG
		MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("Found %1 waveforms"))(m_WaveForms.size()));
#endif
	}
	// Convert the SF2 data to DLS
	if ((m_nType & SOUNDBANK_TYPE_SF2) && !m_SamplesEx.empty() && !m_Instruments.empty())
	{
		ConvertSF2ToDLS(sf2info);
	}
#ifdef DLSBANK_LOG
	MPT_LOG(LogDebug, "DLSBANK", U_("DLS bank closed"));
#endif
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
// Extracts the WaveForms from a DLS bank

uint32 CDLSBank::GetRegionFromKey(uint32 nIns, uint32 nKey) const
{
	if (nIns >= m_Instruments.size()) return 0;
	const DLSINSTRUMENT &dlsIns = m_Instruments[nIns];
	for (uint32 rgn = 0; rgn < dlsIns.nRegions; rgn++)
	{
		if ((nKey >= dlsIns.Regions[rgn].uKeyMin) && (nKey <= dlsIns.Regions[rgn].uKeyMax))
		{
			return rgn;
		}
	}
	return 0;
}


bool CDLSBank::ExtractWaveForm(uint32 nIns, uint32 nRgn, std::vector<uint8> &waveData, uint32 &length) const
{
	waveData.clear();
	length = 0;

	if (nIns >= m_Instruments.size() || !m_dwWavePoolOffset)
	{
	#ifdef DLSBANK_LOG
		MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("ExtractWaveForm(%1) failed: m_Instruments.size()=%2 m_dwWavePoolOffset=%3 m_WaveForms.size()=%4"))(nIns, m_Instruments.size(), m_dwWavePoolOffset, m_WaveForms.size()));
	#endif
		return false;
	}
	const DLSINSTRUMENT &dlsIns = m_Instruments[nIns];
	if (nRgn >= dlsIns.nRegions)
	{
	#ifdef DLSBANK_LOG
		MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("invalid waveform region: nIns=%1 nRgn=%2 pSmp->nRegions=%3"))(nIns, nRgn, dlsIns.nRegions));
	#endif
		return false;
	}
	uint32 nWaveLink = dlsIns.Regions[nRgn].nWaveLink;
	if(nWaveLink >= m_WaveForms.size())
	{
	#ifdef DLSBANK_LOG
		MPT_LOG(LogDebug, "DLSBANK", mpt::format(U_("Invalid wavelink id: nWaveLink=%1 nWaveForms=%2"))(nWaveLink, m_WaveForms.size()));
	#endif
		return false;
	}

	mpt::ifstream f(m_szFileName, std::ios::binary);
	if(!f)
	{
		return false;
	}
	mpt::IO::Offset sampleOffset = mpt::saturate_cast<mpt::IO::Offset>(m_WaveForms[nWaveLink] + m_dwWavePoolOffset);
	if(mpt::IO::SeekAbsolute(f, sampleOffset))
	{
		if (m_nType & SOUNDBANK_TYPE_SF2)
		{
			if (m_SamplesEx[nWaveLink].dwLen)
			{
				if (mpt::IO::SeekRelative(f, 8))
				{
					length = m_SamplesEx[nWaveLink].dwLen;
					try
					{
						waveData.assign(length + 8, 0);
						mpt::IO::ReadRaw(f, waveData.data(), length);
					} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
					{
						MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
					}
				}
			}
		} else
		{
			LISTChunk chunk;
			if(mpt::IO::Read(f, chunk))
			{
				if((chunk.id == IFFID_LIST) && (chunk.listid == IFFID_wave) && (chunk.len > 4))
				{
					length = chunk.len + 8;
					try
					{
						waveData.assign(chunk.len + sizeof(IFFCHUNK), 0);
						memcpy(waveData.data(), &chunk, sizeof(chunk));
						mpt::IO::ReadRaw(f, &waveData[sizeof(chunk)], length - sizeof(chunk));
					} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
					{
						MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
					}
				}
			}
		}
	}
	return !waveData.empty();
}


bool CDLSBank::ExtractSample(CSoundFile &sndFile, SAMPLEINDEX nSample, uint32 nIns, uint32 nRgn, int transpose) const
{
	std::vector<uint8> pWaveForm;
	uint32 dwLen = 0;
	bool ok, hasWaveform;

	if (nIns >= m_Instruments.size()) return false;
	const DLSINSTRUMENT *pDlsIns = &m_Instruments[nIns];
	if (nRgn >= pDlsIns->nRegions) return false;
	if (!ExtractWaveForm(nIns, nRgn, pWaveForm, dwLen)) return false;
	if (dwLen < 16) return false;
	ok = false;

	FileReader wsmpChunk;
	if (m_nType & SOUNDBANK_TYPE_SF2)
	{
		sndFile.DestroySample(nSample);
		uint32 nWaveLink = pDlsIns->Regions[nRgn].nWaveLink;
		ModSample &sample = sndFile.GetSample(nSample);
		if (sndFile.m_nSamples < nSample) sndFile.m_nSamples = nSample;
		if (nWaveLink < m_SamplesEx.size())
		{
			const DLSSAMPLEEX &p = m_SamplesEx[nWaveLink];
		#ifdef DLSINSTR_LOG
			MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("  SF2 WaveLink #%1: %2Hz"))(nWaveLink, p.dwSampleRate));
		#endif
			sample.Initialize();
			sample.nLength = dwLen / 2;
			sample.nLoopStart = pDlsIns->Regions[nRgn].ulLoopStart;
			sample.nLoopEnd = pDlsIns->Regions[nRgn].ulLoopEnd;
			sample.nC5Speed = p.dwSampleRate;
			sample.RelativeTone = p.byOriginalPitch;
			sample.nFineTune = p.chPitchCorrection;
			if (p.szName[0])
				sndFile.m_szNames[nSample] = mpt::String::ReadAutoBuf(p.szName);
			else if(pDlsIns->szName[0])
				sndFile.m_szNames[nSample] = mpt::String::ReadAutoBuf(pDlsIns->szName);

			FileReader chunk(mpt::as_span(pWaveForm.data(), dwLen));
			SampleIO(
				SampleIO::_16bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::signedPCM)
				.ReadSample(sample, chunk);
		}
		hasWaveform = sample.HasSampleData();
	} else
	{
		FileReader file(mpt::as_span(pWaveForm.data(), dwLen));
		hasWaveform = sndFile.ReadWAVSample(nSample, file, false, &wsmpChunk);
		if(pDlsIns->szName[0])
			sndFile.m_szNames[nSample] = mpt::String::ReadAutoBuf(pDlsIns->szName);
	}
	if (hasWaveform)
	{
		ModSample &sample = sndFile.GetSample(nSample);
		const DLSREGION &rgn = pDlsIns->Regions[nRgn];
		sample.uFlags.reset(CHN_LOOP | CHN_PINGPONGLOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);
		if (rgn.fuOptions & DLSREGION_SAMPLELOOP) sample.uFlags.set(CHN_LOOP);
		if (rgn.fuOptions & DLSREGION_SUSTAINLOOP) sample.uFlags.set(CHN_SUSTAINLOOP);
		if (rgn.fuOptions & DLSREGION_PINGPONGLOOP) sample.uFlags.set(CHN_PINGPONGLOOP);
		if (sample.uFlags[CHN_LOOP | CHN_SUSTAINLOOP])
		{
			if (rgn.ulLoopEnd > rgn.ulLoopStart)
			{
				if (sample.uFlags[CHN_SUSTAINLOOP])
				{
					sample.nSustainStart = rgn.ulLoopStart;
					sample.nSustainEnd = rgn.ulLoopEnd;
				} else
				{
					sample.nLoopStart = rgn.ulLoopStart;
					sample.nLoopEnd = rgn.ulLoopEnd;
				}
			} else
			{
				sample.uFlags.reset(CHN_LOOP|CHN_SUSTAINLOOP);
			}
		}
		// WSMP chunk
		{
			uint32 usUnityNote = rgn.uUnityNote;
			int sFineTune = rgn.sFineTune;
			int lVolume = rgn.usVolume;

			WSMPCHUNK wsmp;
			if(!(rgn.fuOptions & DLSREGION_OVERRIDEWSMP) && wsmpChunk.IsValid() && wsmpChunk.Skip(sizeof(IFFCHUNK)) && wsmpChunk.ReadStructPartial(wsmp))
			{
				usUnityNote = wsmp.usUnityNote;
				sFineTune = wsmp.sFineTune;
				lVolume = DLS32BitRelativeGainToLinear(wsmp.lAttenuation) / 256;
				if(wsmp.cSampleLoops)
				{
					WSMPSAMPLELOOP loop;
					wsmpChunk.Seek(sizeof(IFFCHUNK) + wsmp.cbSize);
					wsmpChunk.ReadStruct(loop);
					if(loop.ulLoopLength > 3)
					{
						sample.uFlags.set(CHN_LOOP);
						//if (loop.ulLoopType) sample.uFlags |= CHN_PINGPONGLOOP;
						sample.nLoopStart = loop.ulLoopStart;
						sample.nLoopEnd = loop.ulLoopStart + loop.ulLoopLength;
					}
				}
			} else if (m_nType & SOUNDBANK_TYPE_SF2)
			{
				usUnityNote = (usUnityNote < 0x80) ? usUnityNote : sample.RelativeTone;
				sFineTune += sample.nFineTune;
			}
		#ifdef DLSINSTR_LOG
			MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("WSMP: usUnityNote=%1.%2, %3Hz (transp=%4)"))(usUnityNote, sFineTune, sample.nC5Speed, transpose));
		#endif
			if (usUnityNote > 0x7F) usUnityNote = 60;
			int steps = (60 + transpose - usUnityNote) * 128 + sFineTune;
			sample.Transpose(steps * (1.0 / (12.0 * 128.0)));

			Limit(lVolume, 16, 256);
			sample.nGlobalVol = (uint8)(lVolume / 4);	// 0-64
		}
		sample.nPan = GetPanning(nIns, nRgn);

		sample.Convert(MOD_TYPE_IT, sndFile.GetType());
		sample.PrecomputeLoops(sndFile, false);
		ok = true;
	}
	return ok;
}


static uint16 ScaleEnvelope(uint32 time, float tempoScale)
{
	return std::max(mpt::saturate_round<uint16>(time * tempoScale), uint16(1));
}


bool CDLSBank::ExtractInstrument(CSoundFile &sndFile, INSTRUMENTINDEX nInstr, uint32 nIns, uint32 nDrumRgn) const
{
	SAMPLEINDEX RgnToSmp[DLSMAXREGIONS];
	uint32 nRgnMin, nRgnMax, nEnv;

	if (nIns >= m_Instruments.size()) return false;
	const DLSINSTRUMENT *pDlsIns = &m_Instruments[nIns];
	if (pDlsIns->ulBank & F_INSTRUMENT_DRUMS)
	{
		if (nDrumRgn >= pDlsIns->nRegions) return false;
		nRgnMin = nDrumRgn;
		nRgnMax = nDrumRgn+1;
		nEnv = pDlsIns->Regions[nDrumRgn].uPercEnv;
	} else
	{
		if (!pDlsIns->nRegions) return false;
		nRgnMin = 0;
		nRgnMax = pDlsIns->nRegions;
		nEnv = pDlsIns->nMelodicEnv;
	}
	if(nRgnMin == 0 && (pDlsIns->Regions[0].fuOptions & DLSREGION_ISGLOBAL))
		nRgnMin++;
#ifdef DLSINSTR_LOG
	MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("DLS Instrument #%1: %2"))(nIns, mpt::ToUnicode(mpt::Charset::ASCII, mpt::String::ReadAutoBuf(pDlsIns->szName))));
	MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("  Bank=0x%1 Instrument=0x%2"))(mpt::ufmt::HEX0<4>(pDlsIns->ulBank), mpt::ufmt::HEX0<4>(pDlsIns->ulInstrument)));
	MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("  %1 regions, nMelodicEnv=%2"))(pDlsIns->nRegions, pDlsIns->nMelodicEnv));
	for (uint32 iDbg=0; iDbg<pDlsIns->nRegions; iDbg++)
	{
		const DLSREGION *prgn = &pDlsIns->Regions[iDbg];
		MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_(" Region %1:"))(iDbg));
		MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("  WaveLink = %1 (loop [%2, %3])"))(prgn->nWaveLink, prgn->ulLoopStart, prgn->ulLoopEnd));
		MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("  Key Range: [%1, %2]"))(prgn->uKeyMin, prgn->uKeyMax));
		MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("  fuOptions = 0x%1"))(mpt::ufmt::HEX0<4>(prgn->fuOptions)));
		MPT_LOG(LogDebug, "DLSINSTR", mpt::format(U_("  usVolume = %1, Unity Note = %2"))(prgn->usVolume, prgn->uUnityNote));
	}
#endif

	ModInstrument *pIns = new (std::nothrow) ModInstrument();
	if(pIns == nullptr)
	{
		return false;
	}

	if (sndFile.Instruments[nInstr])
	{
		sndFile.DestroyInstrument(nInstr, deleteAssociatedSamples);
	}
	// Initializes Instrument
	if (pDlsIns->ulBank & F_INSTRUMENT_DRUMS)
	{
		uint32 key = pDlsIns->Regions[nDrumRgn].uKeyMin;
		if((key >= 24) && (key <= 84))
		{
			std::string s = szMidiPercussionNames[key-24];
			if(!mpt::String::ReadAutoBuf(pDlsIns->szName).empty())
			{
				s += mpt::format(" (%1)")(mpt::String::RTrim<std::string>(mpt::String::ReadAutoBuf(pDlsIns->szName)));
			}
			pIns->name = s;
		} else
		{
			pIns->name = mpt::String::ReadAutoBuf(pDlsIns->szName);
		}
	} else
	{
		pIns->name = mpt::String::ReadAutoBuf(pDlsIns->szName);
	}
	int nTranspose = 0;
	if(pDlsIns->ulBank & F_INSTRUMENT_DRUMS)
	{
		for(uint32 iNoteMap = 0; iNoteMap < NOTE_MAX; iNoteMap++)
		{
			if(sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MID | MOD_TYPE_MPT))
			{
				// Format has instrument note mapping
				if(pDlsIns->Regions[nDrumRgn].tuning == 0)
					pIns->NoteMap[iNoteMap] = NOTE_MIDDLEC;
				else if(iNoteMap < pDlsIns->Regions[nDrumRgn].uKeyMin)
					pIns->NoteMap[iNoteMap] = (uint8)(pDlsIns->Regions[nDrumRgn].uKeyMin + NOTE_MIN);
				else if(iNoteMap > pDlsIns->Regions[nDrumRgn].uKeyMax)
					pIns->NoteMap[iNoteMap] = (uint8)(pDlsIns->Regions[nDrumRgn].uKeyMax + NOTE_MIN);
			} else
			{
				if(iNoteMap == pDlsIns->Regions[nDrumRgn].uKeyMin)
				{
					nTranspose = (pDlsIns->Regions[nDrumRgn].uKeyMin + (pDlsIns->Regions[nDrumRgn].uKeyMax - pDlsIns->Regions[nDrumRgn].uKeyMin) / 2) - 60;
				}
			}
		}
	}
	pIns->nFadeOut = 1024;
	pIns->nMidiProgram = (uint8)(pDlsIns->ulInstrument & 0x7F) + 1;
	pIns->nMidiChannel = (uint8)((pDlsIns->ulBank & F_INSTRUMENT_DRUMS) ? 10 : 0);
	pIns->wMidiBank = (uint16)(((pDlsIns->ulBank & 0x7F00) >> 1) | (pDlsIns->ulBank & 0x7F));
	pIns->nNNA = NNA_NOTEOFF;
	pIns->nDCT = DCT_NOTE;
	pIns->nDNA = DNA_NOTEFADE;
	sndFile.Instruments[nInstr] = pIns;
	uint32 nLoadedSmp = 0;
	SAMPLEINDEX nextSample = 0;
	// Extract Samples
	for (uint32 nRgn=nRgnMin; nRgn<nRgnMax; nRgn++)
	{
		bool duplicateRegion = false;
		SAMPLEINDEX nSmp = 0;
		const DLSREGION *pRgn = &pDlsIns->Regions[nRgn];
		// Elimitate Duplicate Regions
		uint32 iDup;
		for (iDup=nRgnMin; iDup<nRgn; iDup++)
		{
			const DLSREGION *pRgn2 = &pDlsIns->Regions[iDup];
			if (((pRgn2->nWaveLink == pRgn->nWaveLink)
			  && (pRgn2->ulLoopEnd == pRgn->ulLoopEnd)
			  && (pRgn2->ulLoopStart == pRgn->ulLoopStart))
			 || ((pRgn2->uKeyMin == pRgn->uKeyMin)
			  && (pRgn2->uKeyMax == pRgn->uKeyMax)))
			{
				duplicateRegion = true;
				nSmp = RgnToSmp[iDup];
				break;
			}
		}
		// Create a new sample
		if (!duplicateRegion)
		{
			uint32 nmaxsmp = (m_nType & MOD_TYPE_XM) ? 16 : 32;
			if (nLoadedSmp >= nmaxsmp)
			{
				nSmp = RgnToSmp[nRgn-1];
			} else
			{
				nextSample = sndFile.GetNextFreeSample(nInstr, nextSample + 1);
				if (nextSample == SAMPLEINDEX_INVALID) break;
				if (nextSample > sndFile.GetNumSamples()) sndFile.m_nSamples = nextSample;
				nSmp = nextSample;
				nLoadedSmp++;
			}
		}

		RgnToSmp[nRgn] = nSmp;
		// Map all notes to the right sample
		if (nSmp)
		{
			for (uint32 iKey=0; iKey<NOTE_MAX; iKey++)
			{
				if ((nRgn == nRgnMin) || ((iKey >= pRgn->uKeyMin) && (iKey <= pRgn->uKeyMax)))
				{
					pIns->Keyboard[iKey] = nSmp;
				}
			}
			// Load the sample
			if(!duplicateRegion || !sndFile.GetSample(nSmp).HasSampleData())
			{
				ExtractSample(sndFile, nSmp, nIns, nRgn, nTranspose);
			} else if(sndFile.GetSample(nSmp).GetNumChannels() == 1)
			{
				// Try to combine stereo samples
				uint8 pan1 = GetPanning(nIns, nRgn), pan2 = GetPanning(nIns, iDup);
				if((pan1 == 0 || pan1 == 255) && (pan2 == 0 || pan2 == 255))
				{
					ModSample &sample = sndFile.GetSample(nSmp);
					ctrlSmp::ConvertToStereo(sample, sndFile);
					std::vector<uint8> pWaveForm;
					uint32 dwLen = 0;
					if(ExtractWaveForm(nIns, nRgn, pWaveForm, dwLen) && dwLen >= sample.GetSampleSizeInBytes() / 2)
					{
						SmpLength len = sample.nLength;
						const int16 *src = reinterpret_cast<int16 *>(pWaveForm.data());
						int16 *dst = sample.sample16() + ((pan1 == 0) ? 0 : 1);
						while(len--)
						{
							*dst = *src;
							src++;
							dst += 2;
						}
					}
				}
			}
		}
	}

	float tempoScale = 1.0f;
	if(sndFile.m_nTempoMode == tempoModeModern)
	{
		uint32 ticksPerBeat = sndFile.m_nDefaultRowsPerBeat * sndFile.m_nDefaultSpeed;
		if(ticksPerBeat == 0)
			ticksPerBeat = 24;
		tempoScale = ticksPerBeat / 24.0f;
	}

	// Initializes Envelope
	if ((nEnv) && (nEnv <= m_Envelopes.size()))
	{
		const DLSENVELOPE *part = &m_Envelopes[nEnv-1];
		// Volume Envelope
		if ((part->wVolAttack) || (part->wVolDecay < 20*50) || (part->nVolSustainLevel) || (part->wVolRelease < 20*50))
		{
			pIns->VolEnv.dwFlags.set(ENV_ENABLED);
			// Delay section
			// -> DLS level 2
			// Attack section
			pIns->VolEnv.clear();
			if (part->wVolAttack)
			{
				pIns->VolEnv.push_back(0, (uint8)(ENVELOPE_MAX / (part->wVolAttack / 2 + 2) + 8)); // /-----
				pIns->VolEnv.push_back(ScaleEnvelope(part->wVolAttack, tempoScale), ENVELOPE_MAX); // |
			} else
			{
				pIns->VolEnv.push_back(0, ENVELOPE_MAX);
			}
			// Hold section
			// -> DLS Level 2
			// Sustain Level
			if (part->nVolSustainLevel > 0)
			{
				if (part->nVolSustainLevel < 128)
				{
					uint16 lStartTime = pIns->VolEnv.back().tick;
					int32 lSusLevel = - DLS32BitRelativeLinearToGain(part->nVolSustainLevel << 9) / 65536;
					int32 lDecayTime = 1;
					if (lSusLevel > 0)
					{
						lDecayTime = (lSusLevel * (int32)part->wVolDecay) / 960;
						for (uint32 i=0; i<7; i++)
						{
							int32 lFactor = 128 - (1 << i);
							if (lFactor <= part->nVolSustainLevel) break;
							int32 lev = - DLS32BitRelativeLinearToGain(lFactor << 9) / 65536;
							if (lev > 0)
							{
								int32 ltime = (lev * (int32)part->wVolDecay) / 960;
								if ((ltime > 1) && (ltime < lDecayTime))
								{
									uint16 tick = lStartTime + ScaleEnvelope(ltime, tempoScale);
									if(tick > pIns->VolEnv.back().tick)
									{
										pIns->VolEnv.push_back(tick, (uint8)(lFactor / 2));
									}
								}
							}
						}
					}

					uint16 decayEnd = lStartTime + ScaleEnvelope(lDecayTime, tempoScale);
					if (decayEnd > pIns->VolEnv.back().tick)
					{
						pIns->VolEnv.push_back(decayEnd, (uint8)((part->nVolSustainLevel+1) / 2));
					}
				}
				pIns->VolEnv.dwFlags.set(ENV_SUSTAIN);
			} else
			{
				pIns->VolEnv.dwFlags.set(ENV_SUSTAIN);
				pIns->VolEnv.push_back(pIns->VolEnv.back().tick + 1u, pIns->VolEnv.back().value);
			}
			pIns->VolEnv.nSustainStart = pIns->VolEnv.nSustainEnd = (uint8)(pIns->VolEnv.size() - 1);
			// Release section
			if ((part->wVolRelease) && (pIns->VolEnv.back().value > 1))
			{
				int32 lReleaseTime = part->wVolRelease;
				uint16 lStartTime = pIns->VolEnv.back().tick;
				int32 lStartFactor = pIns->VolEnv.back().value;
				int32 lSusLevel = - DLS32BitRelativeLinearToGain(lStartFactor << 10) / 65536;
				int32 lDecayEndTime = (lReleaseTime * lSusLevel) / 960;
				lReleaseTime -= lDecayEndTime;
				if(pIns->VolEnv.nSustainEnd > 0)
					pIns->VolEnv.nReleaseNode = pIns->VolEnv.nSustainEnd;
				for (uint32 i=0; i<5; i++)
				{
					int32 lFactor = 1 + ((lStartFactor * 3) >> (i+2));
					if ((lFactor <= 1) || (lFactor >= lStartFactor)) continue;
					int32 lev = - DLS32BitRelativeLinearToGain(lFactor << 10) / 65536;
					if (lev > 0)
					{
						int32 ltime = (((int32)part->wVolRelease * lev) / 960) - lDecayEndTime;
						if ((ltime > 1) && (ltime < lReleaseTime))
						{
							uint16 tick = lStartTime + ScaleEnvelope(ltime, tempoScale);
							if(tick > pIns->VolEnv.back().tick)
							{
								pIns->VolEnv.push_back(tick, (uint8)lFactor);
							}
						}
					}
				}
				if (lReleaseTime < 1) lReleaseTime = 1;
				auto releaseTicks = ScaleEnvelope(lReleaseTime, tempoScale);
				pIns->VolEnv.push_back(lStartTime + releaseTicks, ENVELOPE_MIN);
				if(releaseTicks > 0)
				{
					pIns->nFadeOut = 32768 / releaseTicks;
				}
			} else
			{
				pIns->VolEnv.push_back(pIns->VolEnv.back().tick + 1u, ENVELOPE_MIN);
			}
		}
	}
	if (pDlsIns->ulBank & F_INSTRUMENT_DRUMS)
	{
		// Create a default envelope for drums
		pIns->VolEnv.dwFlags.reset(ENV_SUSTAIN);
		if(!pIns->VolEnv.dwFlags[ENV_ENABLED])
		{
			pIns->VolEnv.dwFlags.set(ENV_ENABLED);
			pIns->VolEnv.resize(4);
			pIns->VolEnv[0] = EnvelopeNode(0, ENVELOPE_MAX);
			pIns->VolEnv[1] = EnvelopeNode(ScaleEnvelope(5, tempoScale), ENVELOPE_MAX);
			pIns->VolEnv[2] = EnvelopeNode(pIns->VolEnv[1].tick * 2u, ENVELOPE_MID);
			pIns->VolEnv[3] = EnvelopeNode(pIns->VolEnv[2].tick * 2u, ENVELOPE_MIN);	// 1 second max. for drums
		}
	}
	pIns->Convert(MOD_TYPE_MPT, sndFile.GetType());
	return true;
}


const char *CDLSBank::GetRegionName(uint32 nIns, uint32 nRgn) const
{
	if (nIns >= m_Instruments.size()) return nullptr;
	const DLSINSTRUMENT &dlsIns = m_Instruments[nIns];
	if (nRgn >= dlsIns.nRegions) return nullptr;

	if (m_nType & SOUNDBANK_TYPE_SF2)
	{
		uint32 nWaveLink = dlsIns.Regions[nRgn].nWaveLink;
		if (nWaveLink < m_SamplesEx.size())
		{
			return m_SamplesEx[nWaveLink].szName;
		}
	}
	return nullptr;
}


uint8 CDLSBank::GetPanning(uint32 ins, uint32 region) const
{
	const DLSINSTRUMENT &dlsIns = m_Instruments[ins];
	if(region >= CountOf(dlsIns.Regions))
		return 128;
	const DLSREGION &rgn = dlsIns.Regions[region];
	if(dlsIns.ulBank & F_INSTRUMENT_DRUMS)
	{
		if(rgn.uPercEnv > 0 && rgn.uPercEnv <= m_Envelopes.size())
		{
			return m_Envelopes[rgn.uPercEnv - 1].nDefPan;
		}
	} else
	{
		if(dlsIns.nMelodicEnv > 0 && dlsIns.nMelodicEnv <= m_Envelopes.size())
		{
			return m_Envelopes[dlsIns.nMelodicEnv - 1].nDefPan;
		}
	}
	return 128;
}


#else // !MODPLUG_TRACKER

MPT_MSVC_WORKAROUND_LNK4221(Dlsbank)

#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
