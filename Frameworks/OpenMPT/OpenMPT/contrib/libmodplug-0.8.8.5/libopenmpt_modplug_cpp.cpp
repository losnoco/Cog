/*
 * libopenmpt_modplug_cpp.cpp
 * --------------------------
 * Purpose: libopenmpt emulation of the libmodplug c++ interface
 * Notes  : WARNING! THIS IS A HACK!
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef NO_LIBMODPLUG

/*

***********************************************************************
WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
***********************************************************************

This is a dirty hack to emulate just so much of the libmodplug c++
interface so that the current known users (mainly xmms-modplug itself,
gstreamer modplug, audacious, and stuff based on those) work. This is
neither a complete nor a correct implementation.
Metadata and other state is not provided or updated.

*/

#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif /* _MSC_VER */

#include <libopenmpt/libopenmpt.hpp>

#include <string>
#include <vector>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define MODPLUG_BUILD
#ifdef _MSC_VER
/* libmodplug C++ header is broken for MSVC DLL builds */
#define MODPLUG_STATIC
#endif /* _MSC_VER */
#ifdef _MSC_VER
#define LIBOPENMPT_MODPLUG_API
#else /* !_MSC_VER */
#define LIBOPENMPT_MODPLUG_API LIBOPENMPT_API_HELPER_EXPORT
#endif /* _MSC_VER */
class LIBOPENMPT_MODPLUG_API CSoundFile;
#include "libmodplug/stdafx.h"
#include "libmodplug/sndfile.h"

namespace {
template <class T>
void Clear( T & x )
{
	std::memset( &x, 0, sizeof(T) );
}
}

//#define mpcpplog() fprintf(stderr, "%s %i\n", __func__, __LINE__)
#define mpcpplog() do{}while(0)

#define UNUSED(x) (void)((x))

union self_t {
	CHAR CompressionTable[16];
	openmpt::module * self_;
};

static void set_self( CSoundFile * that, openmpt::module * self_ ) {
	self_t self_union;
	Clear(self_union);
	self_union.self_ = self_;
	std::memcpy( that->CompressionTable, self_union.CompressionTable, sizeof( self_union.CompressionTable ) );
}

static openmpt::module * get_self( const CSoundFile * that ) {
	self_t self_union;
	Clear(self_union);
	std::memcpy( self_union.CompressionTable, that->CompressionTable, sizeof( self_union.CompressionTable ) );
	return self_union.self_;
}

#define mod ( get_self( this ) )

#define update_state() \
	if ( mod ) m_nCurrentPattern = mod->get_current_order(); \
	if ( mod ) m_nPattern = mod->get_current_pattern(); \
	if ( mod ) m_nMusicSpeed = mod->get_current_speed(); \
	if ( mod ) m_nMusicTempo = mod->get_current_tempo(); \
/**/

UINT CSoundFile::m_nXBassDepth = 0;
UINT CSoundFile::m_nXBassRange = 0;
UINT CSoundFile::m_nReverbDepth = 0;
UINT CSoundFile::m_nReverbDelay = 0;
UINT CSoundFile::gnReverbType = 0;
UINT CSoundFile::m_nProLogicDepth = 0;
UINT CSoundFile::m_nProLogicDelay = 0;
UINT CSoundFile::m_nStereoSeparation = 128;
UINT CSoundFile::m_nMaxMixChannels = 256;
LONG CSoundFile::m_nStreamVolume = 0x8000;
DWORD CSoundFile::gdwSysInfo = 0;
DWORD CSoundFile::gdwSoundSetup = 0;
DWORD CSoundFile::gdwMixingFreq = 44100;
DWORD CSoundFile::gnBitsPerSample = 16;
DWORD CSoundFile::gnChannels = 2;
UINT CSoundFile::gnAGC = 0;
UINT CSoundFile::gnVolumeRampSamples = 0;
UINT CSoundFile::gnVUMeter = 0;
UINT CSoundFile::gnCPUUsage = 0;
LPSNDMIXHOOKPROC CSoundFile::gpSndMixHook = 0;
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = 0;

CSoundFile::CSoundFile() {
	mpcpplog();
	Clear(Chn);
	Clear(ChnMix);
	Clear(Ins);
	Clear(Headers);
	Clear(ChnSettings);
	Clear(Patterns);
	Clear(PatternSize);
	Clear(Order);
	Clear(m_MidiCfg);
	Clear(m_MixPlugins);
	Clear(m_nDefaultSpeed);
	Clear(m_nDefaultTempo);
	Clear(m_nDefaultGlobalVolume);
	Clear(m_dwSongFlags);
	Clear(m_nChannels);
	Clear(m_nMixChannels);
	Clear(m_nMixStat);
	Clear(m_nBufferCount);
	Clear(m_nType);
	Clear(m_nSamples);
	Clear(m_nInstruments);
	Clear(m_nTickCount);
	Clear(m_nTotalCount);
	Clear(m_nPatternDelay);
	Clear(m_nFrameDelay);
	Clear(m_nMusicSpeed);
	Clear(m_nMusicTempo);
	Clear(m_nNextRow);
	Clear(m_nRow);
	Clear(m_nPattern);
	Clear(m_nCurrentPattern);
	Clear(m_nNextPattern);
	Clear(m_nRestartPos);
	Clear(m_nMasterVolume);
	Clear(m_nGlobalVolume);
	Clear(m_nSongPreAmp);
	Clear(m_nFreqFactor);
	Clear(m_nTempoFactor);
	Clear(m_nOldGlbVolSlide);
	Clear(m_nMinPeriod);
	Clear(m_nMaxPeriod);
	Clear(m_nRepeatCount);
	Clear(m_nInitialRepeatCount);
	Clear(m_nGlobalFadeSamples);
	Clear(m_nGlobalFadeMaxSamples);
	Clear(m_nMaxOrderPosition);
	Clear(m_nPatternNames);
	Clear(m_lpszSongComments);
	Clear(m_lpszPatternNames);
	Clear(m_szNames);
	Clear(CompressionTable);
}

CSoundFile::~CSoundFile() {
	mpcpplog();
	Destroy();
}

BOOL CSoundFile::Create( LPCBYTE lpStream, DWORD dwMemLength ) {
	mpcpplog();
	try {
		openmpt::module * m = new openmpt::module( lpStream, dwMemLength );
		set_self( this, m );
		std::strncpy( m_szNames[0], mod->get_metadata("title").c_str(), sizeof( m_szNames[0] ) - 1 );
		m_szNames[0][ sizeof( m_szNames[0] ) - 1 ] = '\0';
		std::string type = mod->get_metadata("type");
		m_nType = MOD_TYPE_NONE;
		if ( type == "mod" ) {
			m_nType = MOD_TYPE_MOD;
		} else if ( type == "s3m" ) {
			m_nType = MOD_TYPE_S3M;
		} else if ( type == "xm" ) {
			m_nType = MOD_TYPE_XM;
		} else if ( type == "med" ) {
			m_nType = MOD_TYPE_MED;
		} else if ( type == "mtm" ) {
			m_nType = MOD_TYPE_MTM;
		} else if ( type == "it" ) {
			m_nType = MOD_TYPE_IT;
		} else if ( type == "669" ) {
			m_nType = MOD_TYPE_669;
		} else if ( type == "ult" ) {
			m_nType = MOD_TYPE_ULT;
		} else if ( type == "stm" ) {
			m_nType = MOD_TYPE_STM;
		} else if ( type == "far" ) {
			m_nType = MOD_TYPE_FAR;
		} else if ( type == "s3m" ) {
			m_nType = MOD_TYPE_WAV;
		} else if ( type == "amf" ) {
			m_nType = MOD_TYPE_AMF;
		} else if ( type == "ams" ) {
			m_nType = MOD_TYPE_AMS;
		} else if ( type == "dsm" ) {
			m_nType = MOD_TYPE_DSM;
		} else if ( type == "mdl" ) {
			m_nType = MOD_TYPE_MDL;
		} else if ( type == "okt" ) {
			m_nType = MOD_TYPE_OKT;
		} else if ( type == "mid" ) {
			m_nType = MOD_TYPE_MID;
		} else if ( type == "dmf" ) {
			m_nType = MOD_TYPE_DMF;
		} else if ( type == "ptm" ) {
			m_nType = MOD_TYPE_PTM;
		} else if ( type == "dbm" ) {
			m_nType = MOD_TYPE_DBM;
		} else if ( type == "mt2" ) {
			m_nType = MOD_TYPE_MT2;
		} else if ( type == "amf0" ) {
			m_nType = MOD_TYPE_AMF0;
		} else if ( type == "psm" ) {
			m_nType = MOD_TYPE_PSM;
		} else if ( type == "j2b" ) {
			m_nType = MOD_TYPE_J2B;
		} else if ( type == "abc" ) {
			m_nType = MOD_TYPE_ABC;
		} else if ( type == "pat" ) {
			m_nType = MOD_TYPE_PAT;
		} else if ( type == "umx" ) {
			m_nType = MOD_TYPE_UMX;
		} else {
			m_nType = MOD_TYPE_IT; // fallback, most complex type
		}
		m_nChannels = mod->get_num_channels();
		m_nMasterVolume = 128;
		m_nSamples = mod->get_num_samples();
		update_state();
		return TRUE;
	} catch ( ... ) {
		Destroy();
		return FALSE;
	}
}

BOOL CSoundFile::Destroy() {
	mpcpplog();
	if ( mod ) {
		delete mod;
		set_self( this, 0 );
	}
	return TRUE;
}

UINT CSoundFile::GetNumChannels() const {
	mpcpplog();
	return mod->get_num_channels();
}

static std::int32_t vol128_To_millibel( unsigned int vol ) {
	return static_cast<std::int32_t>( 2000.0 * std::log10( static_cast<int>( vol ) / 128.0 ) );
}

BOOL CSoundFile::SetMasterVolume( UINT vol, BOOL bAdjustAGC ) {
	UNUSED(bAdjustAGC);
	mpcpplog();
	m_nMasterVolume = vol;
	mod->set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, vol128_To_millibel( m_nMasterVolume ) );
	return TRUE;
}

UINT CSoundFile::GetNumPatterns() const {
	mpcpplog();
	return mod->get_num_patterns();
}

UINT CSoundFile::GetNumInstruments() const {
	mpcpplog();
	return mod->get_num_instruments();
}

void CSoundFile::SetCurrentOrder( UINT nOrder ) {
	mpcpplog();
	mod->set_position_order_row( nOrder, 0 );
	update_state();
}

UINT CSoundFile::GetSampleName( UINT nSample, LPSTR s ) const {
	mpcpplog();
	char buf[32];
	std::memset( buf, 0, 32 );
	if ( mod ) {
		std::vector<std::string> names = mod->get_sample_names();
		if ( 1 <= nSample && nSample <= names.size() ) {
			std::strncpy( buf, names[ nSample - 1 ].c_str(), 31 );
		}
	}
	if ( s ) {
		std::strncpy( s, buf, 32 );
	}
	return static_cast<UINT>( std::strlen( buf ) );
}

UINT CSoundFile::GetInstrumentName( UINT nInstr, LPSTR s ) const {
	mpcpplog();
	char buf[32];
	std::memset( buf, 0, 32 );
	if ( mod ) {
		std::vector<std::string> names = mod->get_instrument_names();
		if ( 1 <= nInstr && nInstr <= names.size() ) {
			std::strncpy( buf, names[ nInstr - 1 ].c_str(), 31 );
		}
	}
	if ( s ) {
		std::strncpy( s, buf, 32 );
	}
	return static_cast<UINT>( std::strlen( buf ) );
}

void CSoundFile::LoopPattern( int nPat, int nRow ) {
	UNUSED(nPat);
	UNUSED(nRow);
	mpcpplog();
	// todo
}

void CSoundFile::CheckCPUUsage( UINT nCPU ) {
	UNUSED(nCPU);
	mpcpplog();
}

BOOL CSoundFile::SetPatternName( UINT nPat, LPCSTR lpszName ) {
	UNUSED(nPat);
	mpcpplog();
	if ( !lpszName ) {
		return FALSE;
	}
	// todo
	return TRUE;
}

BOOL CSoundFile::GetPatternName( UINT nPat, LPSTR lpszName, UINT cbSize ) const {
	UNUSED(nPat);
	mpcpplog();
	if ( !lpszName || cbSize <= 0 ) {
		return FALSE;
	}
	std::memset( lpszName, 0, cbSize );
	// todo
	return TRUE;
}

BOOL CSoundFile::ReadXM(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadS3M(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadMod(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadMed(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadMTM(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadSTM(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadIT(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::Read669(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadUlt(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadWav(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadDSM(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadFAR(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadAMS(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadAMS2(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadMDL(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadOKT(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadDMF(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadPTM(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadDBM(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadAMF(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadMT2(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadPSM(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadJ2B(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadUMX(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadABC(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::TestABC(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadMID(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::TestMID(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::ReadPAT(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }
BOOL CSoundFile::TestPAT(LPCBYTE lpStream, DWORD dwMemLength) { UNUSED(lpStream); UNUSED(dwMemLength); mpcpplog(); return FALSE; }

#ifndef MODPLUG_NO_FILESAVE

UINT CSoundFile::WriteSample( FILE * f, MODINSTRUMENT * pins, UINT nFlags, UINT nMaxLen ) {
	UNUSED(f);
	UNUSED(pins);
	UNUSED(nFlags);
	UNUSED(nMaxLen);
	mpcpplog();
	return 0;
}

BOOL CSoundFile::SaveXM( LPCSTR lpszFileName, UINT nPacking ) {
	UNUSED(lpszFileName);
	UNUSED(nPacking);
	mpcpplog();
	return FALSE;
}

BOOL CSoundFile::SaveS3M( LPCSTR lpszFileName, UINT nPacking ) {
	UNUSED(lpszFileName);
	UNUSED(nPacking);
	mpcpplog();
	return FALSE;
}

BOOL CSoundFile::SaveMod( LPCSTR lpszFileName, UINT nPacking ) {
	UNUSED(lpszFileName);
	UNUSED(nPacking);
	mpcpplog();
	return FALSE;
}
	
BOOL CSoundFile::SaveIT( LPCSTR lpszFileName, UINT nPacking ) {
	UNUSED(lpszFileName);
	UNUSED(nPacking);
	mpcpplog();
	return FALSE;
}

#endif

UINT CSoundFile::GetBestSaveFormat() const {
	mpcpplog();
	return MOD_TYPE_IT;
}

UINT CSoundFile::GetSaveFormats() const {
	mpcpplog();
	return MOD_TYPE_IT;
}

void CSoundFile::ConvertModCommand( MODCOMMAND * ) const {
	mpcpplog();
}

void CSoundFile::S3MConvert( MODCOMMAND * m, BOOL bIT ) const {
	UNUSED(m);
	UNUSED(bIT);
	mpcpplog();
}

void CSoundFile::S3MSaveConvert( UINT * pcmd, UINT * pprm, BOOL bIT ) const {
	UNUSED(pcmd);
	UNUSED(pprm);
	UNUSED(bIT);
	mpcpplog();
}

WORD CSoundFile::ModSaveCommand( const MODCOMMAND * m, BOOL bXM ) const {
	UNUSED(m);
	UNUSED(bXM);
	mpcpplog();
	return 0;
}

VOID CSoundFile::ResetChannels() {
	mpcpplog();
}

UINT CSoundFile::CreateStereoMix( int count ) {
	UNUSED(count);
	mpcpplog();
	return 0;
}

BOOL CSoundFile::FadeSong( UINT msec ) {
	UNUSED(msec);
	mpcpplog();
	return TRUE;
}

BOOL CSoundFile::GlobalFadeSong( UINT msec ) {
	UNUSED(msec);
	mpcpplog();
	return TRUE;
}

BOOL CSoundFile::InitPlayer( BOOL bReset ) {
	UNUSED(bReset);
	mpcpplog();
	return TRUE;
}

BOOL CSoundFile::SetMixConfig( UINT nStereoSeparation, UINT nMaxMixChannels ) {
	UNUSED(nMaxMixChannels);
	mpcpplog();
	m_nStereoSeparation = nStereoSeparation;
 	return TRUE;
}

DWORD CSoundFile::InitSysInfo() {
	mpcpplog();
	return 0;
}

void CSoundFile::SetAGC( BOOL b ) {
	UNUSED(b);
	mpcpplog();
}

void CSoundFile::ResetAGC() {
	mpcpplog();
}

void CSoundFile::ProcessAGC( int count ) {
	UNUSED(count);
	mpcpplog();
}

BOOL CSoundFile::SetWaveConfig( UINT nRate, UINT nBits, UINT nChannels, BOOL bMMX ) {
	UNUSED(bMMX);
	mpcpplog();
	gdwMixingFreq = nRate;
	gnBitsPerSample = nBits;
	gnChannels = nChannels;
	return TRUE;
}

BOOL CSoundFile::SetWaveConfigEx( BOOL bSurround, BOOL bNoOverSampling, BOOL bReverb, BOOL hqido, BOOL bMegaBass, BOOL bNR, BOOL bEQ ) {
	UNUSED(bSurround);
	UNUSED(bReverb);
	UNUSED(hqido);
	UNUSED(bMegaBass);
	UNUSED(bEQ);
	mpcpplog();
	DWORD d = gdwSoundSetup & ~(SNDMIX_NORESAMPLING|SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
	if ( bNoOverSampling ) {
		d |= SNDMIX_NORESAMPLING;
	} else if ( !hqido ) {
		d |= 0;
	} else if ( !bNR ) {
		d |= SNDMIX_HQRESAMPLER;
	} else {
			d |= (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
	}
	gdwSoundSetup = d;
	return TRUE;
}

BOOL CSoundFile::SetResamplingMode( UINT nMode ) {
	mpcpplog();
	DWORD d = gdwSoundSetup & ~(SNDMIX_NORESAMPLING|SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
	switch ( nMode ) {
		case SRCMODE_NEAREST:
			d |= SNDMIX_NORESAMPLING;
			break;
		case SRCMODE_LINEAR:
			break;
		case SRCMODE_SPLINE:
			d |= SNDMIX_HQRESAMPLER;
			break;
		case SRCMODE_POLYPHASE:
			d |= (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
			break;
		default:
			return FALSE;
			break;
	}
	gdwSoundSetup = d;
	return TRUE;
}

BOOL CSoundFile::SetReverbParameters( UINT nDepth, UINT nDelay ) {
	UNUSED(nDepth);
	UNUSED(nDelay);
	mpcpplog();
	return TRUE;
}

BOOL CSoundFile::SetXBassParameters( UINT nDepth, UINT nRange ) {
	UNUSED(nDepth);
	UNUSED(nRange);
	mpcpplog();
	return TRUE;
}

BOOL CSoundFile::SetSurroundParameters( UINT nDepth, UINT nDelay ) {
	UNUSED(nDepth);
	UNUSED(nDelay);
	mpcpplog();
	return TRUE;
}

UINT CSoundFile::GetMaxPosition() const {
	mpcpplog();
	// rows in original, just use seconds here
	if ( mod ) return static_cast<UINT>( mod->get_duration_seconds() + 0.5 );
	return 0;
}

DWORD CSoundFile::GetLength( BOOL bAdjust, BOOL bTotal ) {
	UNUSED(bAdjust);
	UNUSED(bTotal);
	mpcpplog();
	if ( mod ) return static_cast<DWORD>( mod->get_duration_seconds() + 0.5 );
	return 0;
}

UINT CSoundFile::GetSongComments( LPSTR s, UINT cbsize, UINT linesize ) {
	UNUSED(linesize);
	mpcpplog();
	if ( !s ) {
		return 0;
	}
	if ( cbsize <= 0 ) {
		return 0;
	}
	if ( !mod ) {
		s[0] = '\0';
		return 1;
	}
	std::strncpy( s, mod->get_metadata("message").c_str(), cbsize );
	s[ cbsize - 1 ] = '\0';
	return static_cast<UINT>( std::strlen( s ) + 1 );
}

UINT CSoundFile::GetRawSongComments( LPSTR s, UINT cbsize, UINT linesize ) {
	UNUSED(linesize);
	mpcpplog();
	if ( !s ) {
		return 0;
	}
	if ( cbsize <= 0 ) {
		return 0;
	}
	if ( !mod ) {
		s[0] = '\0';
		return 1;
	}
	std::strncpy( s, mod->get_metadata("message_raw").c_str(), cbsize );
	s[ cbsize - 1 ] = '\0';
	return static_cast<UINT>( std::strlen( s ) + 1 );
}

void CSoundFile::SetCurrentPos( UINT nPos ) {
	mpcpplog();
	if ( mod ) mod->set_position_seconds( nPos );
	update_state();
}

UINT CSoundFile::GetCurrentPos() const {
	mpcpplog();
	if ( mod ) return static_cast<UINT>( mod->get_position_seconds() + 0.5 );
	return 0;
}

static int get_stereo_separation() {
	mpcpplog();
	return CSoundFile::m_nStereoSeparation * 100 / 128;
}

static int get_filter_length() {
	mpcpplog();
	if ( ( CSoundFile::gdwSoundSetup & (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE) ) == (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE) ) {
		return 8;
	} else if ( ( CSoundFile::gdwSoundSetup & SNDMIX_HQRESAMPLER ) == SNDMIX_HQRESAMPLER ) {
		return 4;
	} else if ( ( CSoundFile::gdwSoundSetup & SNDMIX_NORESAMPLING ) == SNDMIX_NORESAMPLING ) {
		return 1;
	} else {
		return 2;
	}
}

static std::size_t get_sample_size() {
	return (CSoundFile::gnBitsPerSample/8);
}

static std::size_t get_num_channels() {
	return CSoundFile::gnChannels;
}

static std::size_t get_frame_size() {
	return get_sample_size() * get_num_channels();
}

static int get_samplerate() {
	return CSoundFile::gdwMixingFreq;
}

UINT CSoundFile::Read( LPVOID lpBuffer, UINT cbBuffer ) {
	mpcpplog();
	if ( !mod ) {
		return 0;
	}
	mpcpplog();
	if ( !lpBuffer ) {
		return 0;
	}
	mpcpplog();
	if ( cbBuffer <= 0 ) {
		return 0;
	}
	mpcpplog();
	if ( get_samplerate() <= 0 ) {
		return 0;
	}
	mpcpplog();
	if ( get_sample_size() != 1 && get_sample_size() != 2 && get_sample_size() != 4 ) {
		return 0;
	}
	mpcpplog();
	if ( get_num_channels() != 1 && get_num_channels() != 2 && get_num_channels() != 4 ) {
		return 0;
	}
	mpcpplog();
	std::memset( lpBuffer, 0, cbBuffer );
	const std::size_t frames_torender = cbBuffer / get_frame_size();
	short * out = reinterpret_cast<short*>( lpBuffer );
	std::vector<short> tmpbuf;
	if ( get_sample_size() == 1 || get_sample_size() == 4 ) {
		tmpbuf.resize( frames_torender * get_num_channels() );
		out = &tmpbuf[0];
	}

	mod->set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, get_stereo_separation() );
	mod->set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, get_filter_length() );
	std::size_t frames_rendered = 0;
	if ( get_num_channels() == 1 ) {
		frames_rendered = mod->read( get_samplerate(), frames_torender, out );
	} else if ( get_num_channels() == 4 ) {
		frames_rendered = mod->read_interleaved_quad( get_samplerate(), frames_torender, out );
	} else {
		frames_rendered = mod->read_interleaved_stereo( get_samplerate(), frames_torender, out );
	}

	if ( get_sample_size() == 1 ) {
		unsigned char * dst = reinterpret_cast<unsigned char*>( lpBuffer );
		for ( std::size_t sample = 0; sample < frames_rendered * get_num_channels(); ++sample ) {
			dst[sample] = ( tmpbuf[sample] / 0x100 ) + 0x80;
		}
	} else if ( get_sample_size() == 4 ) {
		int * dst = reinterpret_cast<int*>( lpBuffer );
		for ( std::size_t sample = 0; sample < frames_rendered * get_num_channels(); ++sample ) {
			dst[sample] = tmpbuf[sample] << (32-16-1-MIXING_ATTENUATION);
		}
	}
	update_state();
	return static_cast<UINT>( frames_rendered );
}


/*

gstreamer modplug calls:

mSoundFile->Create
mSoundFile->Destroy

mSoundFile->SetWaveConfig
mSoundFile->SetWaveConfigEx
mSoundFile->SetResamplingMode
mSoundFile->SetSurroundParameters
mSoundFile->SetXBassParameters
mSoundFile->SetReverbParameters

mSoundFile->GetMaxPosition (inline, -> GetLength)
mSoundFile->GetSongTime

mSoundFile->GetTitle (inline)
mSoundFile->GetSongComments

mSoundFile->SetCurrentPos
mSoundFile->Read

mSoundFile->GetCurrentPos
mSoundFile->GetMusicTempo (inline)

*/


// really very internal symbols, probably nothing calls these directly

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4100)
#endif

BOOL CSoundFile::ReadNote() { mpcpplog(); return 0; }
BOOL CSoundFile::ProcessRow() { mpcpplog(); return 0; }
BOOL CSoundFile::ProcessEffects() { mpcpplog(); return 0; }
UINT CSoundFile::GetNNAChannel(UINT nChn) const { mpcpplog(); return 0; }
void CSoundFile::CheckNNA(UINT nChn, UINT instr, int note, BOOL bForceCut) { mpcpplog(); }
void CSoundFile::NoteChange(UINT nChn, int note, BOOL bPorta, BOOL bResetEnv) { mpcpplog(); }
void CSoundFile::InstrumentChange(MODCHANNEL *pChn, UINT instr, BOOL bPorta,BOOL bUpdVol,BOOL bResetEnv) { mpcpplog(); }
void CSoundFile::PortamentoUp(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::PortamentoDown(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::FinePortamentoUp(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::FinePortamentoDown(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::ExtraFinePortamentoUp(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::ExtraFinePortamentoDown(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::TonePortamento(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::Vibrato(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::FineVibrato(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::VolumeSlide(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::PanningSlide(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::ChannelVolSlide(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::FineVolumeUp(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::FineVolumeDown(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::Tremolo(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::Panbrello(MODCHANNEL *pChn, UINT param) { mpcpplog(); }
void CSoundFile::RetrigNote(UINT nChn, UINT param) { mpcpplog(); }
void CSoundFile::NoteCut(UINT nChn, UINT nTick) { mpcpplog(); }
void CSoundFile::KeyOff(UINT nChn) { mpcpplog(); }
int CSoundFile::PatternLoop(MODCHANNEL *, UINT param) { mpcpplog(); return 0; }
void CSoundFile::ExtendedMODCommands(UINT nChn, UINT param) { mpcpplog(); }
void CSoundFile::ExtendedS3MCommands(UINT nChn, UINT param) { mpcpplog(); }
void CSoundFile::ExtendedChannelEffect(MODCHANNEL *, UINT param) { mpcpplog(); }
void CSoundFile::ProcessMidiMacro(UINT nChn, LPCSTR pszMidiMacro, UINT param) { mpcpplog(); }
void CSoundFile::SetupChannelFilter(MODCHANNEL *pChn, BOOL bReset, int flt_modifier) const { mpcpplog(); }
void CSoundFile::DoFreqSlide(MODCHANNEL *pChn, LONG nFreqSlide) { mpcpplog(); }
void CSoundFile::SetTempo(UINT param) { mpcpplog(); }
void CSoundFile::SetSpeed(UINT param) { mpcpplog(); }
void CSoundFile::GlobalVolSlide(UINT param) { mpcpplog(); }
DWORD CSoundFile::IsSongFinished(UINT nOrder, UINT nRow) const { mpcpplog(); return 0; }
BOOL CSoundFile::IsValidBackwardJump(UINT nStartOrder, UINT nStartRow, UINT nJumpOrder, UINT nJumpRow) const { mpcpplog(); return 0; }
UINT CSoundFile::PackSample(int &sample, int next) { mpcpplog(); return 0; }
BOOL CSoundFile::CanPackSample(LPSTR pSample, UINT nLen, UINT nPacking, BYTE *result) { mpcpplog(); return 0; }
UINT CSoundFile::ReadSample(MODINSTRUMENT *pIns, UINT nFlags, LPCSTR pMemFile, DWORD dwMemLength) { mpcpplog(); return 0; }
BOOL CSoundFile::DestroySample(UINT nSample) { mpcpplog(); return 0; }
BOOL CSoundFile::DestroyInstrument(UINT nInstr) { mpcpplog(); return 0; }
BOOL CSoundFile::IsSampleUsed(UINT nSample) { mpcpplog(); return 0; }
BOOL CSoundFile::IsInstrumentUsed(UINT nInstr) { mpcpplog(); return 0; }
BOOL CSoundFile::RemoveInstrumentSamples(UINT nInstr) { mpcpplog(); return 0; }
UINT CSoundFile::DetectUnusedSamples(BOOL *) { mpcpplog(); return 0; }
BOOL CSoundFile::RemoveSelectedSamples(BOOL *) { mpcpplog(); return 0; }
void CSoundFile::AdjustSampleLoop(MODINSTRUMENT *pIns) { mpcpplog(); }
BOOL CSoundFile::ReadInstrumentFromSong(UINT nInstr, CSoundFile *, UINT nSrcInstrument) { mpcpplog(); return 0; }
BOOL CSoundFile::ReadSampleFromSong(UINT nSample, CSoundFile *, UINT nSrcSample) { mpcpplog(); return 0; }
UINT CSoundFile::GetNoteFromPeriod(UINT period) const { mpcpplog(); return 0; }
UINT CSoundFile::GetPeriodFromNote(UINT note, int nFineTune, UINT nC4Speed) const { mpcpplog(); return 0; }
UINT CSoundFile::GetFreqFromPeriod(UINT period, UINT nC4Speed, int nPeriodFrac) const { mpcpplog(); return 0; }
void CSoundFile::ResetMidiCfg() { mpcpplog(); }
UINT CSoundFile::MapMidiInstrument(DWORD dwProgram, UINT nChannel, UINT nNote) { mpcpplog(); return 0; }
BOOL CSoundFile::ITInstrToMPT(const void *p, INSTRUMENTHEADER *penv, UINT trkvers) { mpcpplog(); return 0; }
UINT CSoundFile::SaveMixPlugins(FILE *f, BOOL bUpdate) { mpcpplog(); return 0; }
UINT CSoundFile::LoadMixPlugins(const void *pData, UINT nLen) { mpcpplog(); return 0; }
#ifndef NO_FILTER
DWORD CSoundFile::CutOffToFrequency(UINT nCutOff, int flt_modifier) const { mpcpplog(); return 0; }
#endif
DWORD CSoundFile::TransposeToFrequency(int transp, int ftune) { mpcpplog(); return 0; }
int CSoundFile::FrequencyToTranspose(DWORD freq) { mpcpplog(); return 0; }
void CSoundFile::FrequencyToTranspose(MODINSTRUMENT *psmp) { mpcpplog(); }
MODCOMMAND *CSoundFile::AllocatePattern(UINT rows, UINT nchns) { mpcpplog(); return 0; }
signed char* CSoundFile::AllocateSample(UINT nbytes) { mpcpplog(); return 0; }
void CSoundFile::FreePattern(LPVOID pat) { mpcpplog(); }
void CSoundFile::FreeSample(LPVOID p) { mpcpplog(); }
UINT CSoundFile::Normalize24BitBuffer(LPBYTE pbuffer, UINT cbsizebytes, DWORD lmax24, DWORD dwByteInc) { mpcpplog(); return 0; }

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif


#endif // NO_LIBMODPLUG
