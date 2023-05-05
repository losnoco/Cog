/*
 * InstrumentExtensions.cpp
 * ------------------------
 * Purpose: Instrument properties I/O
 * Notes  : Welcome to the absolutely horrible abominations that are the "extended instrument properties"
 *          which are some of the earliest additions OpenMPT did to the IT / XM format. They are ugly,
 *          and the way they work even differs between IT/XM and ITI/XI/ITP.
 *          Yes, the world would be a better place without this stuff.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#ifndef MODPLUG_NO_FILESAVE
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"
#endif

OPENMPT_NAMESPACE_BEGIN

/*---------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
MODULAR (in/out) ModInstrument :
-----------------------------------------------------------------------------------------------

* to update:
------------

- both following functions need to be updated when adding a new member in ModInstrument :

void WriteInstrumentHeaderStructOrField(ModInstrument * input, std::ostream &file, uint32 only_this_code, int16 fixedsize);
bool ReadInstrumentHeaderField(ModInstrument * input, uint32 fcode, int16 fsize, FileReader &file);

- see below for body declaration.


* members:
----------

- 32bit identification CODE tag (must be unique)
- 16bit content SIZE in byte(s)
- member field


* CODE tag naming convention:
-----------------------------

- have a look below in current tag dictionnary
- take the initial ones of the field name
- 4 caracters code (not more, not less)
- must be filled with '.' caracters if code has less than 4 caracters
- for arrays, must include a '[' caracter following significant caracters ('.' not significant!!!)
- use only caracters used in full member name, ordered as they appear in it
- match caracter attribute (small,capital)

Example with "PanEnv.nLoopEnd" , "PitchEnv.nLoopEnd" & "VolEnv.Values[MAX_ENVPOINTS]" members :
- use 'PLE.' for PanEnv.nLoopEnd
- use 'PiLE' for PitchEnv.nLoopEnd
- use 'VE[.' for VolEnv.Values[MAX_ENVPOINTS]


* In use CODE tag dictionary (alphabetical order):
--------------------------------------------------

						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						!!! SECTION TO BE UPDATED !!!
						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		[EXT]	means external (not related) to ModInstrument content

AUTH	[EXT]	Song artist
C...	[EXT]	nChannels
ChnS	[EXT]	IT/MPTM: Channel settings for channels 65-127 if needed (doesn't fit to IT header).
CS..			nCutSwing
CUES	[EXT]	Sample cue points
CWV.	[EXT]	dwCreatedWithVersion
DCT.			nDCT;
dF..			dwFlags;
DGV.	[EXT]	nDefaultGlobalVolume
DT..	[EXT]	nDefaultTempo;
DTFR	[EXT]	Fractional part of default tempo
DNA.			nDNA;
EBIH	[EXT]	embeded instrument header tag (ITP file format)
FM..			filterMode;
fn[.			filename[12];
FO..			nFadeOut;
GV..			nGlobalVol;
IFC.			nIFC;
IFR.			nIFR;
K[.				Keyboard[128];
LSWV	[EXT]	Last Saved With Version
MB..			wMidiBank;
MC..			nMidiChannel;
MDK.			nMidiDrumKey;
MIMA	[EXT]									MIdi MApping directives
MiP.			nMixPlug;
MP..			nMidiProgram;
MPTS	[EXT]									Extra song info tag
MPTX	[EXT]									EXTRA INFO tag
MSF.	[EXT]									Mod(Specific)Flags
n[..			name[32];
NNA.			nNNA;
NM[.			NoteMap[128];
P...			nPan;
PE..			PanEnv.nNodes;
PE[.			PanEnv.Values[MAX_ENVPOINTS];
PiE.			PitchEnv.nNodes;
PiE[			PitchEnv.Values[MAX_ENVPOINTS];
PiLE			PitchEnv.nLoopEnd;
PiLS			PitchEnv.nLoopStart;
PiP[			PitchEnv.Ticks[MAX_ENVPOINTS];
PiSB			PitchEnv.nSustainStart;
PiSE			PitchEnv.nSustainEnd;
PLE.			PanEnv.nLoopEnd;
PLS.			PanEnv.nLoopStart;
PMM.	[EXT]	nPlugMixMode;
PP[.			PanEnv.Ticks[MAX_ENVPOINTS];
PPC.			nPPC;
PPS.			nPPS;
PS..			nPanSwing;
PSB.			PanEnv.nSustainStart;
PSE.			PanEnv.nSustainEnd;
PTTL			pitchToTempoLock;
PTTF			pitchToTempoLock (fractional part);
PVEH			pluginVelocityHandling;
PVOH			pluginVolumeHandling;
R...			resampling;
RP..	[EXT]	nRestartPos;
RPB.	[EXT]	nRowsPerBeat;
RPM.	[EXT]	nRowsPerMeasure;
RS..			nResSwing;
RSMP	[EXT]	Global resampling
SEP@	[EXT]	chunk SEPARATOR tag
SPA.	[EXT]	m_nSamplePreAmp;
TM..	[EXT]	nTempoMode;
VE..			VolEnv.nNodes;
VE[.			VolEnv.Values[MAX_ENVPOINTS];
VLE.			VolEnv.nLoopEnd;
VLS.			VolEnv.nLoopStart;
VP[.			VolEnv.Ticks[MAX_ENVPOINTS];
VR..			nVolRampUp;
VS..			nVolSwing;
VSB.			VolEnv.nSustainStart;
VSE.			VolEnv.nSustainEnd;
VSTV	[EXT]	nVSTiVolume;
PERN			PitchEnv.nReleaseNode
AERN			PanEnv.nReleaseNode
VERN			VolEnv.nReleaseNode
PFLG			PitchEnv.dwFlag
AFLG			PanEnv.dwFlags
VFLG			VolEnv.dwFlags
MPWD			MIDI Pitch Wheel Depth
-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------*/

#ifndef MODPLUG_NO_FILESAVE

template<typename T, bool is_signed> struct IsNegativeFunctor { bool operator()(T val) const { return val < 0; } };
template<typename T> struct IsNegativeFunctor<T, true> { bool operator()(T val) const { return val < 0; } };
template<typename T> struct IsNegativeFunctor<T, false> { bool operator()(T /*val*/) const { return false; } };

template<typename T>
bool IsNegative(const T &val)
{
	return IsNegativeFunctor<T, std::numeric_limits<T>::is_signed>()(val);
}

// ------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members ONLY (non-array)
// ------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_sized_member(name,type,code) \
	static_assert(sizeof(input->name) == sizeof(type), "Instrument property does match specified type!");\
	fcode = code;\
	fsize = sizeof( type );\
	if(writeAll) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<uint16>(file, fsize); \
	} else if(only_this_code == fcode)\
	{ \
		MPT_ASSERT(fixedsize == fsize); \
	} \
	if(only_this_code == fcode || only_this_code == Util::MaxValueOfType(only_this_code)) \
	{ \
		type tmp = (type)(input-> name ); \
		mpt::IO::WriteIntLE(file, tmp); \
	} \
/**/

// -----------------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members which are written truncated
// -----------------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_trunc_member(name,type,code) \
	static_assert(sizeof(input->name) > sizeof(type), "Instrument property would not be truncated, use WRITE_MPTHEADER_sized_member instead!");\
	fcode = code;\
	fsize = sizeof( type );\
	if(writeAll) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<uint16>(file, fsize); \
		type tmp = (type)(input-> name ); \
		mpt::IO::WriteIntLE(file, tmp); \
	} else if(only_this_code == fcode)\
	{ \
		/* hackish workaround to resolve mismatched size values: */ \
		/* nResampling was a long time declared as uint32 but these macro tables used uint16 and UINT. */ \
		/* This worked fine on little-endian, on big-endian not so much. Thus support writing size-mismatched fields. */ \
		MPT_ASSERT(fixedsize >= fsize); \
		type tmp = (type)(input-> name ); \
		mpt::IO::WriteIntLE(file, tmp); \
		if(fixedsize > fsize) \
		{ \
			for(int16 i = 0; i < fixedsize - fsize; ++i) \
			{ \
				uint8 fillbyte = !IsNegative(tmp) ? 0 : 0xff; /* sign extend */ \
				mpt::IO::WriteIntLE(file, fillbyte); \
			} \
		} \
	} \
/**/

// ------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for array members ONLY
// ------------------------------------------------------------------------
#define WRITE_MPTHEADER_array_member(name,type,code,arraysize) \
	static_assert(sizeof(type) == sizeof(input-> name [0])); \
	MPT_ASSERT(sizeof(input->name) >= sizeof(type) * arraysize);\
	fcode = code;\
	fsize = sizeof( type ) * arraysize;\
	if(writeAll) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<uint16>(file, fsize); \
	} else if(only_this_code == fcode)\
	{ \
		/* MPT_ASSERT(fixedsize <= fsize); */ \
		fsize = fixedsize; /* just trust the size we got passed */ \
	} \
	if(only_this_code == fcode || only_this_code == Util::MaxValueOfType(only_this_code)) \
	{ \
		for(std::size_t i = 0; i < fsize/sizeof(type); ++i) \
		{ \
			type tmp; \
			tmp = input-> name [i]; \
			mpt::IO::WriteIntLE(file, tmp); \
		} \
	} \
/**/

// ------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for envelope members ONLY
// ------------------------------------------------------------------------
#define WRITE_MPTHEADER_envelope_member(envType,envField,type,code) \
	{\
		const InstrumentEnvelope &env = input->GetEnvelope(envType); \
		static_assert(sizeof(type) == sizeof(env[0]. envField)); \
		fcode = code;\
		fsize = mpt::saturate_cast<int16>(sizeof( type ) * env.size());\
		MPT_ASSERT(size_t(fsize) == sizeof( type ) * env.size()); \
		\
		if(writeAll) \
		{ \
			mpt::IO::WriteIntLE<uint32>(file, fcode); \
			mpt::IO::WriteIntLE<uint16>(file, fsize); \
		} else if(only_this_code == fcode)\
		{ \
			fsize = fixedsize; /* just trust the size we got passed */ \
		} \
		if(only_this_code == fcode || only_this_code == Util::MaxValueOfType(only_this_code)) \
		{ \
			uint32 maxNodes = std::min(static_cast<uint32>(fsize/sizeof(type)), static_cast<uint32>(env.size())); \
			for(uint32 i = 0; i < maxNodes; ++i) \
			{ \
				type tmp; \
				tmp = env[i]. envField ; \
				mpt::IO::WriteIntLE(file, tmp); \
			} \
			/* Not every instrument's envelope will be the same length. fill up with zeros. */ \
			for(uint32 i = maxNodes; i < fsize/sizeof(type); ++i) \
			{ \
				type tmp = 0; \
				mpt::IO::WriteIntLE(file, tmp); \
			} \
		} \
	}\
/**/


// Write (in 'file') 'input' ModInstrument with 'code' & 'size' extra field infos for each member
void WriteInstrumentHeaderStructOrField(ModInstrument * input, std::ostream &file, uint32 only_this_code, uint16 fixedsize)
{
	uint32 fcode;
	uint16 fsize;
	// If true, all extension are written to the file; otherwise only the specified extension is written.
	// writeAll is true iff we are saving an instrument (or, hypothetically, the legacy ITP format)
	const bool writeAll = only_this_code == Util::MaxValueOfType(only_this_code);

	if(!writeAll)
	{
		MPT_ASSERT(fixedsize > 0);
	}

	// clang-format off
	WRITE_MPTHEADER_sized_member(	nFadeOut					, uint32	, MagicBE("FO..")	)
	WRITE_MPTHEADER_sized_member(	nPan						, uint32	, MagicBE("P...")	)
	WRITE_MPTHEADER_sized_member(	VolEnv.size()				, uint32	, MagicBE("VE..")	)
	WRITE_MPTHEADER_sized_member(	PanEnv.size()				, uint32	, MagicBE("PE..")	)
	WRITE_MPTHEADER_sized_member(	PitchEnv.size()				, uint32	, MagicBE("PiE.")	)
	WRITE_MPTHEADER_sized_member(	wMidiBank					, uint16	, MagicBE("MB..")	)
	WRITE_MPTHEADER_sized_member(	nMidiProgram				, uint8		, MagicBE("MP..")	)
	WRITE_MPTHEADER_sized_member(	nMidiChannel				, uint8		, MagicBE("MC..")	)
	WRITE_MPTHEADER_envelope_member(	ENV_VOLUME	, tick		, uint16	, MagicBE("VP[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PANNING	, tick		, uint16	, MagicBE("PP[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PITCH	, tick		, uint16	, MagicBE("PiP[")	)
	WRITE_MPTHEADER_envelope_member(	ENV_VOLUME	, value		, uint8		, MagicBE("VE[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PANNING	, value		, uint8		, MagicBE("PE[.")	)
	WRITE_MPTHEADER_envelope_member(	ENV_PITCH	, value		, uint8		, MagicBE("PiE[")	)
	WRITE_MPTHEADER_sized_member(	nMixPlug					, uint8		, MagicBE("MiP.")	)
	WRITE_MPTHEADER_sized_member(	nVolRampUp					, uint16	, MagicBE("VR..")	)
	WRITE_MPTHEADER_sized_member(	resampling					, uint8		, MagicBE("R...")	)
	WRITE_MPTHEADER_sized_member(	nCutSwing					, uint8		, MagicBE("CS..")	)
	WRITE_MPTHEADER_sized_member(	nResSwing					, uint8		, MagicBE("RS..")	)
	WRITE_MPTHEADER_sized_member(	filterMode					, uint8		, MagicBE("FM..")	)
	WRITE_MPTHEADER_sized_member(	pluginVelocityHandling		, uint8		, MagicBE("PVEH")	)
	WRITE_MPTHEADER_sized_member(	pluginVolumeHandling		, uint8		, MagicBE("PVOH")	)
	WRITE_MPTHEADER_trunc_member(	pitchToTempoLock.GetInt()	, uint16	, MagicBE("PTTL")	)
	WRITE_MPTHEADER_trunc_member(	pitchToTempoLock.GetFract() , uint16	, MagicLE("PTTF")	)
	WRITE_MPTHEADER_sized_member(	PitchEnv.nReleaseNode		, uint8		, MagicBE("PERN")	)
	WRITE_MPTHEADER_sized_member(	PanEnv.nReleaseNode			, uint8		, MagicBE("AERN")	)
	WRITE_MPTHEADER_sized_member(	VolEnv.nReleaseNode			, uint8		, MagicBE("VERN")	)
	WRITE_MPTHEADER_sized_member(	PitchEnv.dwFlags			, uint8		, MagicBE("PFLG")	)
	WRITE_MPTHEADER_sized_member(	PanEnv.dwFlags				, uint8		, MagicBE("AFLG")	)
	WRITE_MPTHEADER_sized_member(	VolEnv.dwFlags				, uint8		, MagicBE("VFLG")	)
	WRITE_MPTHEADER_sized_member(	midiPWD						, int8		, MagicBE("MPWD")	)
	// clang-format on

}


template<typename TIns, typename PropType>
static bool IsPropertyNeeded(const TIns &Instruments, PropType ModInstrument::*Prop)
{
	const ModInstrument defaultIns;
	for(const auto ins : Instruments)
	{
		if(ins != nullptr && defaultIns.*Prop != ins->*Prop)
			return true;
	}
	return false;
}


template<typename PropType>
static void WritePropertyIfNeeded(const CSoundFile &sndFile, PropType ModInstrument::*Prop, uint32 code, uint16 size, std::ostream &f, INSTRUMENTINDEX numInstruments)
{
	if(IsPropertyNeeded(sndFile.Instruments, Prop))
	{
		sndFile.WriteInstrumentPropertyForAllInstruments(code, size, f, numInstruments);
	}
}


// Used only when saving IT, XM and MPTM.
// ITI, ITP saves using Ericus' macros etc...
// The reason is that ITs and XMs save [code][size][ins1.Value][ins2.Value]...
// whereas ITP saves [code][size][ins1.Value][code][size][ins2.Value]...
// too late to turn back....
void CSoundFile::SaveExtendedInstrumentProperties(INSTRUMENTINDEX numInstruments, std::ostream &f) const
{
	uint32 code = MagicBE("MPTX");	// write extension header code
	mpt::IO::WriteIntLE<uint32>(f, code);

	if (numInstruments == 0)
		return;

	WritePropertyIfNeeded(*this, &ModInstrument::nVolRampUp,   MagicBE("VR.."), sizeof(ModInstrument::nVolRampUp),   f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::nMixPlug,     MagicBE("MiP."), sizeof(ModInstrument::nMixPlug),     f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::nMidiChannel, MagicBE("MC.."), sizeof(ModInstrument::nMidiChannel), f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::nMidiProgram, MagicBE("MP.."), sizeof(ModInstrument::nMidiProgram), f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::wMidiBank,    MagicBE("MB.."), sizeof(ModInstrument::wMidiBank),    f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::resampling,  MagicBE("R..."), sizeof(ModInstrument::resampling),  f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::pluginVelocityHandling, MagicBE("PVEH"), sizeof(ModInstrument::pluginVelocityHandling), f, numInstruments);
	WritePropertyIfNeeded(*this, &ModInstrument::pluginVolumeHandling, MagicBE("PVOH"), sizeof(ModInstrument::pluginVolumeHandling), f, numInstruments);

	if(!(GetType() & MOD_TYPE_XM))
	{
		// XM instrument headers already stores full-precision fade-out
		bool writeFadeOut = false, writePan = false, writePWD = false;
		int32 prevPWD = int32_min;
		for(const auto ins : Instruments)
		{
			if(ins == nullptr)
				continue;
			if((ins->nFadeOut % 32u) || ins->nFadeOut > 8192)
				writeFadeOut = true;
			if(ins->dwFlags[INS_SETPANNING] && (ins->nPan % 4u))
				writePan = true;
			if((prevPWD != int32_min && ins->midiPWD != prevPWD) || (ins->midiPWD < 0))
				writePWD = true;
			prevPWD = ins->midiPWD;
		}
		if(writeFadeOut)
			WriteInstrumentPropertyForAllInstruments(MagicBE("FO.."), sizeof(ModInstrument::nFadeOut), f, numInstruments);
		// XM instrument headers already have support for this
		if(writePWD)
			WriteInstrumentPropertyForAllInstruments(MagicBE("MPWD"), sizeof(ModInstrument::midiPWD), f, numInstruments);
		// We never supported these as hacks in XM (luckily!)
		if(writePan)
			WriteInstrumentPropertyForAllInstruments(MagicBE("P..."), sizeof(ModInstrument::nPan), f, numInstruments);
		WritePropertyIfNeeded(*this, &ModInstrument::nCutSwing, MagicBE("CS.."), sizeof(ModInstrument::nCutSwing), f, numInstruments);
		WritePropertyIfNeeded(*this, &ModInstrument::nResSwing, MagicBE("RS.."), sizeof(ModInstrument::nResSwing), f, numInstruments);
		WritePropertyIfNeeded(*this, &ModInstrument::filterMode, MagicBE("FM.."), sizeof(ModInstrument::filterMode), f, numInstruments);
		if(IsPropertyNeeded(Instruments, &ModInstrument::pitchToTempoLock))
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("PTTL"), sizeof(uint16), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicLE("PTTF"), sizeof(uint16), f, numInstruments);
		}
	}

	if(GetType() & MOD_TYPE_MPT)
	{
		uint32 maxNodes[3] = { 0, 0, 0 };
		bool hasReleaseNode[3] = { false, false, false };
		for(INSTRUMENTINDEX i = 1; i <= numInstruments; i++) if(Instruments[i] != nullptr)
		{
			maxNodes[0] = std::max(maxNodes[0], Instruments[i]->VolEnv.size());
			maxNodes[1] = std::max(maxNodes[1], Instruments[i]->PanEnv.size());
			maxNodes[2] = std::max(maxNodes[2], Instruments[i]->PitchEnv.size());
			hasReleaseNode[0] |= (Instruments[i]->VolEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET);
			hasReleaseNode[1] |= (Instruments[i]->PanEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET);
			hasReleaseNode[2] |= (Instruments[i]->PitchEnv.nReleaseNode != ENV_RELEASE_NODE_UNSET);
		}
		// write full envelope information for MPTM files (more env points)
		if(maxNodes[0] > 25)
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("VE.."), sizeof(ModInstrument::VolEnv.size()), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("VP[."), static_cast<uint16>(maxNodes[0] * sizeof(EnvelopeNode::tick)),  f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("VE[."), static_cast<uint16>(maxNodes[0] * sizeof(EnvelopeNode::value)), f, numInstruments);
		}
		if(maxNodes[1] > 25)
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("PE.."), sizeof(ModInstrument::PanEnv.size()), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PP[."), static_cast<uint16>(maxNodes[1] * sizeof(EnvelopeNode::tick)),  f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PE[."), static_cast<uint16>(maxNodes[1] * sizeof(EnvelopeNode::value)), f, numInstruments);
		}
		if(maxNodes[2] > 25)
		{
			WriteInstrumentPropertyForAllInstruments(MagicBE("PiE."), sizeof(ModInstrument::PitchEnv.size()), f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PiP["), static_cast<uint16>(maxNodes[2] * sizeof(EnvelopeNode::tick)),  f, numInstruments);
			WriteInstrumentPropertyForAllInstruments(MagicBE("PiE["), static_cast<uint16>(maxNodes[2] * sizeof(EnvelopeNode::value)), f, numInstruments);
		}
		if(hasReleaseNode[0])
			WriteInstrumentPropertyForAllInstruments(MagicBE("VERN"), sizeof(ModInstrument::VolEnv.nReleaseNode), f, numInstruments);
		if(hasReleaseNode[1])
			WriteInstrumentPropertyForAllInstruments(MagicBE("AERN"), sizeof(ModInstrument::PanEnv.nReleaseNode), f, numInstruments);
		if(hasReleaseNode[2])
			WriteInstrumentPropertyForAllInstruments(MagicBE("PERN"), sizeof(ModInstrument::PitchEnv.nReleaseNode), f, numInstruments);
	}
}

void CSoundFile::WriteInstrumentPropertyForAllInstruments(uint32 code, uint16 size, std::ostream &f, INSTRUMENTINDEX nInstruments) const
{
	mpt::IO::WriteIntLE<uint32>(f, code);		//write code
	mpt::IO::WriteIntLE<uint16>(f, size);		//write size
	for(INSTRUMENTINDEX i = 1; i <= nInstruments; i++)	//for all instruments...
	{
		if (Instruments[i])
		{
			WriteInstrumentHeaderStructOrField(Instruments[i], f, code, size);
		} else
		{
			ModInstrument emptyInstrument;
			WriteInstrumentHeaderStructOrField(&emptyInstrument, f, code, size);
		}
	}
}


#endif // !MODPLUG_NO_FILESAVE


// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for single type members ONLY (non-array)
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_sized_member(name,type,code) \
	case code: \
	{\
		if( fsize <= sizeof( type ) ) \
		{ \
			/* hackish workaround to resolve mismatched size values: */ \
			/* nResampling was a long time declared as uint32 but these macro tables used uint16 and UINT. */ \
			/* This worked fine on little-endian, on big-endian not so much. Thus support reading size-mismatched fields. */ \
			if(file.CanRead(fsize)) \
			{ \
				type tmp; \
				tmp = file.ReadTruncatedIntLE<type>(fsize); \
				static_assert(sizeof(tmp) == sizeof(input-> name )); \
				input-> name = decltype(input-> name )(tmp); \
				result = true; \
			} \
		} \
	} break;

// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for array members ONLY
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_array_member(name,type,code) \
	case code: \
	{\
		if( fsize <= sizeof( type ) * std::size(input-> name) ) \
		{ \
			FileReader arrayChunk = file.ReadChunk(fsize); \
			for(std::size_t i = 0; i < std::size(input-> name); ++i) \
			{ \
				input-> name [i] = arrayChunk.ReadIntLE<type>(); \
			} \
			result = true; \
		} \
	} break;

// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for character buffer members ONLY
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_charbuf_member(name,type,code) \
	case code: \
	{\
		if( fsize <= sizeof( type ) * input-> name .static_length() ) \
		{ \
			FileReader arrayChunk = file.ReadChunk(fsize); \
			std::string tmp; \
			for(std::size_t i = 0; i < fsize; ++i) \
			{ \
				tmp += arrayChunk.ReadChar(); \
			} \
			input-> name = tmp; \
			result = true; \
		} \
	} break;

// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for envelope tick/value members
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_envelope_member(envType,envField,type,code) \
	case code: \
	{\
		FileReader arrayChunk = file.ReadChunk(fsize); \
		InstrumentEnvelope &env = input->GetEnvelope(envType); \
		for(uint32 i = 0; i < env.size(); i++) \
		{ \
			env[i]. envField = arrayChunk.ReadIntLE<type>(); \
		} \
		result = true; \
	} break;


// Return a pointer on the wanted field in 'input' ModInstrument given field code & size
bool ReadInstrumentHeaderField(ModInstrument *input, uint32 fcode, uint16 fsize, FileReader &file)
{
	if(input == nullptr) return false;

	bool result = false;

	// Members which can be found in this table but not in the write table are only required in the legacy ITP format.
	switch(fcode)
	{
	// clang-format off
	GET_MPTHEADER_sized_member(	nFadeOut				, uint32		, MagicBE("FO..")	)
	GET_MPTHEADER_sized_member(	dwFlags					, uint8			, MagicBE("dF..")	)
	GET_MPTHEADER_sized_member(	nGlobalVol				, uint32		, MagicBE("GV..")	)
	GET_MPTHEADER_sized_member(	nPan					, uint32		, MagicBE("P...")	)
	GET_MPTHEADER_sized_member(	VolEnv.nLoopStart		, uint8			, MagicBE("VLS.")	)
	GET_MPTHEADER_sized_member(	VolEnv.nLoopEnd			, uint8			, MagicBE("VLE.")	)
	GET_MPTHEADER_sized_member(	VolEnv.nSustainStart	, uint8			, MagicBE("VSB.")	)
	GET_MPTHEADER_sized_member(	VolEnv.nSustainEnd		, uint8			, MagicBE("VSE.")	)
	GET_MPTHEADER_sized_member(	PanEnv.nLoopStart		, uint8			, MagicBE("PLS.")	)
	GET_MPTHEADER_sized_member(	PanEnv.nLoopEnd			, uint8			, MagicBE("PLE.")	)
	GET_MPTHEADER_sized_member(	PanEnv.nSustainStart	, uint8			, MagicBE("PSB.")	)
	GET_MPTHEADER_sized_member(	PanEnv.nSustainEnd		, uint8			, MagicBE("PSE.")	)
	GET_MPTHEADER_sized_member(	PitchEnv.nLoopStart		, uint8			, MagicBE("PiLS")	)
	GET_MPTHEADER_sized_member(	PitchEnv.nLoopEnd		, uint8			, MagicBE("PiLE")	)
	GET_MPTHEADER_sized_member(	PitchEnv.nSustainStart	, uint8			, MagicBE("PiSB")	)
	GET_MPTHEADER_sized_member(	PitchEnv.nSustainEnd	, uint8			, MagicBE("PiSE")	)
	GET_MPTHEADER_sized_member(	nNNA					, uint8			, MagicBE("NNA.")	)
	GET_MPTHEADER_sized_member(	nDCT					, uint8			, MagicBE("DCT.")	)
	GET_MPTHEADER_sized_member(	nDNA					, uint8			, MagicBE("DNA.")	)
	GET_MPTHEADER_sized_member(	nPanSwing				, uint8			, MagicBE("PS..")	)
	GET_MPTHEADER_sized_member(	nVolSwing				, uint8			, MagicBE("VS..")	)
	GET_MPTHEADER_sized_member(	nIFC					, uint8			, MagicBE("IFC.")	)
	GET_MPTHEADER_sized_member(	nIFR					, uint8			, MagicBE("IFR.")	)
	GET_MPTHEADER_sized_member(	wMidiBank				, uint16		, MagicBE("MB..")	)
	GET_MPTHEADER_sized_member(	nMidiProgram			, uint8			, MagicBE("MP..")	)
	GET_MPTHEADER_sized_member(	nMidiChannel			, uint8			, MagicBE("MC..")	)
	GET_MPTHEADER_sized_member(	nPPS					, int8			, MagicBE("PPS.")	)
	GET_MPTHEADER_sized_member(	nPPC					, uint8			, MagicBE("PPC.")	)
	GET_MPTHEADER_envelope_member(ENV_VOLUME	, tick	, uint16		, MagicBE("VP[.")	)
	GET_MPTHEADER_envelope_member(ENV_PANNING	, tick	, uint16		, MagicBE("PP[.")	)
	GET_MPTHEADER_envelope_member(ENV_PITCH		, tick	, uint16		, MagicBE("PiP[")	)
	GET_MPTHEADER_envelope_member(ENV_VOLUME	, value	, uint8			, MagicBE("VE[.")	)
	GET_MPTHEADER_envelope_member(ENV_PANNING	, value	, uint8			, MagicBE("PE[.")	)
	GET_MPTHEADER_envelope_member(ENV_PITCH		, value	, uint8			, MagicBE("PiE[")	)
	GET_MPTHEADER_array_member(	NoteMap					, uint8			, MagicBE("NM[.")	)
	GET_MPTHEADER_array_member(	Keyboard				, uint16		, MagicBE("K[..")	)
	GET_MPTHEADER_charbuf_member(	name				, char			, MagicBE("n[..")	)
	GET_MPTHEADER_charbuf_member(	filename			, char			, MagicBE("fn[.")	)
	GET_MPTHEADER_sized_member(	nMixPlug				, uint8			, MagicBE("MiP.")	)
	GET_MPTHEADER_sized_member(	nVolRampUp				, uint16		, MagicBE("VR..")	)
	GET_MPTHEADER_sized_member(	nCutSwing				, uint8			, MagicBE("CS..")	)
	GET_MPTHEADER_sized_member(	nResSwing				, uint8			, MagicBE("RS..")	)
	GET_MPTHEADER_sized_member(	filterMode				, uint8			, MagicBE("FM..")	)
	GET_MPTHEADER_sized_member(	pluginVelocityHandling	, uint8			, MagicBE("PVEH")	)
	GET_MPTHEADER_sized_member(	pluginVolumeHandling	, uint8			, MagicBE("PVOH")	)
	GET_MPTHEADER_sized_member(	PitchEnv.nReleaseNode	, uint8			, MagicBE("PERN")	)
	GET_MPTHEADER_sized_member(	PanEnv.nReleaseNode		, uint8			, MagicBE("AERN")	)
	GET_MPTHEADER_sized_member(	VolEnv.nReleaseNode		, uint8			, MagicBE("VERN")	)
	GET_MPTHEADER_sized_member(	PitchEnv.dwFlags		, uint8			, MagicBE("PFLG")	)
	GET_MPTHEADER_sized_member(	PanEnv.dwFlags			, uint8			, MagicBE("AFLG")	)
	GET_MPTHEADER_sized_member(	VolEnv.dwFlags			, uint8			, MagicBE("VFLG")	)
	GET_MPTHEADER_sized_member(	midiPWD					, int8			, MagicBE("MPWD")	)
	// clang-format on
	case MagicBE("R..."):
	{
		// Resampling has been written as various sizes including uint16 and uint32 in the past
		uint32 tmp = file.ReadSizedIntLE<uint32>(fsize);
		if(Resampling::IsKnownMode(tmp))
			input->resampling = static_cast<ResamplingMode>(tmp);
		result = true;
	} break;
	case MagicBE("PTTL"):
	{
		// Integer part of pitch/tempo lock
		uint16 tmp = file.ReadSizedIntLE<uint16>(fsize);
		input->pitchToTempoLock.Set(tmp, input->pitchToTempoLock.GetFract());
		result = true;
	} break;
	case MagicLE("PTTF"):
	{
		// Fractional part of pitch/tempo lock
		uint16 tmp = file.ReadSizedIntLE<uint16>(fsize);
		input->pitchToTempoLock.Set(input->pitchToTempoLock.GetInt(), tmp);
		result = true;
	} break;
	case MagicBE("VE.."):
		input->VolEnv.resize(std::min(uint32(MAX_ENVPOINTS), file.ReadSizedIntLE<uint32>(fsize)));
		result = true;
		break;
	case MagicBE("PE.."):
		input->PanEnv.resize(std::min(uint32(MAX_ENVPOINTS), file.ReadSizedIntLE<uint32>(fsize)));
		result = true;
		break;
	case MagicBE("PiE."):
		input->PitchEnv.resize(std::min(uint32(MAX_ENVPOINTS), file.ReadSizedIntLE<uint32>(fsize)));
		result = true;
		break;
	}

	return result;
}


// Convert instrument flags which were read from 'dF..' extension to proper internal representation.
static void ConvertReadExtendedFlags(ModInstrument *pIns)
{
	// Flags of 'dF..' datafield in extended instrument properties.
	enum
	{
		dFdd_VOLUME 		= 0x0001,
		dFdd_VOLSUSTAIN 	= 0x0002,
		dFdd_VOLLOOP 		= 0x0004,
		dFdd_PANNING 		= 0x0008,
		dFdd_PANSUSTAIN 	= 0x0010,
		dFdd_PANLOOP 		= 0x0020,
		dFdd_PITCH 			= 0x0040,
		dFdd_PITCHSUSTAIN 	= 0x0080,
		dFdd_PITCHLOOP 		= 0x0100,
		dFdd_SETPANNING 	= 0x0200,
		dFdd_FILTER 		= 0x0400,
		dFdd_VOLCARRY 		= 0x0800,
		dFdd_PANCARRY 		= 0x1000,
		dFdd_PITCHCARRY 	= 0x2000,
		dFdd_MUTE 			= 0x4000,
	};

	const uint32 dwOldFlags = pIns->dwFlags.GetRaw();

	pIns->VolEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_VOLUME) != 0);
	pIns->VolEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_VOLSUSTAIN) != 0);
	pIns->VolEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_VOLLOOP) != 0);
	pIns->VolEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_VOLCARRY) != 0);

	pIns->PanEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_PANNING) != 0);
	pIns->PanEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_PANSUSTAIN) != 0);
	pIns->PanEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_PANLOOP) != 0);
	pIns->PanEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_PANCARRY) != 0);

	pIns->PitchEnv.dwFlags.set(ENV_ENABLED, (dwOldFlags & dFdd_PITCH) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_SUSTAIN, (dwOldFlags & dFdd_PITCHSUSTAIN) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_LOOP, (dwOldFlags & dFdd_PITCHLOOP) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_CARRY, (dwOldFlags & dFdd_PITCHCARRY) != 0);
	pIns->PitchEnv.dwFlags.set(ENV_FILTER, (dwOldFlags & dFdd_FILTER) != 0);

	pIns->dwFlags.reset();
	pIns->dwFlags.set(INS_SETPANNING, (dwOldFlags & dFdd_SETPANNING) != 0);
	pIns->dwFlags.set(INS_MUTE, (dwOldFlags & dFdd_MUTE) != 0);
}


void ReadInstrumentExtensionField(ModInstrument* pIns, const uint32 code, const uint16 size, FileReader &file)
{
	if(code == MagicBE("K[.."))
	{
		// skip keyboard mapping
		file.Skip(size);
		return;
	}

	bool success = ReadInstrumentHeaderField(pIns, code, size, file);

	if(!success)
	{
		file.Skip(size);
		return;
	}

	if(code == MagicBE("dF..")) // 'dF..' field requires additional processing.
		ConvertReadExtendedFlags(pIns);
}


void ReadExtendedInstrumentProperty(ModInstrument* pIns, const uint32 code, FileReader &file)
{
	uint16 size = file.ReadUint16LE();
	if(!file.CanRead(size))
	{
		return;
	}
	ReadInstrumentExtensionField(pIns, code, size, file);
}


void ReadExtendedInstrumentProperties(ModInstrument* pIns, FileReader &file)
{
	if(!file.ReadMagic("XTPM"))	// 'MPTX'
	{
		return;
	}

	while(file.CanRead(7))
	{
		ReadExtendedInstrumentProperty(pIns, file.ReadUint32LE(), file);
	}
}


bool CSoundFile::LoadExtendedInstrumentProperties(FileReader &file)
{
	if(!file.ReadMagic("XTPM"))	// 'MPTX'
	{
		return false;
	}

	while(file.CanRead(6))
	{
		uint32 code = file.ReadUint32LE();

		if(code == MagicBE("MPTS")	// Reached song extensions, break out of this loop
			|| code == MagicLE("228\x04")	// Reached MPTM extensions (in case there are no song extensions)
			|| (code & 0x80808080) || !(code & 0x60606060))	// Non-ASCII chunk ID
		{
			file.SkipBack(4);
			break;
		}

		// Read size of this property for *one* instrument
		const uint16 size = file.ReadUint16LE();

		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			if(Instruments[i])
			{
				ReadInstrumentExtensionField(Instruments[i], code, size, file);
			}
		}
	}
	return true;
}


OPENMPT_NAMESPACE_END
