/*
 * in_openmpt.cpp
 * --------------
 * Purpose: libopenmpt winamp input plugin implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef NO_WINAMP

#if defined(_MFC_VER) || 1
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // _WIN32_WINNT_WINXP
#endif
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS // Avoid binary bloat from linking unused MFC controls
#define NOMINMAX
#include <afxwin.h>
#include <afxcmn.h>
#include <windows.h>
#endif // _MFC_VER

#ifdef LIBOPENMPT_BUILD_DLL
#undef LIBOPENMPT_BUILD_DLL
#endif

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
#endif // _MSC_VER

#include "libopenmpt.hpp"

#include "libopenmpt_plugin_gui.hpp"

#include "svn_version.h"
#if defined(OPENMPT_VERSION_REVISION)
static const char * in_openmpt_string = "in_openmpt " OPENMPT_API_VERSION_STRING "." OPENMPT_API_VERSION_STRINGIZE(OPENMPT_VERSION_REVISION);
#else
static const char * in_openmpt_string = "in_openmpt " OPENMPT_API_VERSION_STRING;
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#define UNICODE_INPUT_PLUGIN
#include "winamp/Winamp/IN2.H"
#include "winamp/Winamp/wa_ipc.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#define BPS 16

#define WINAMP_DSP_HEADROOM_FACTOR 2
#define WINAMP_BUFFER_SIZE_FRAMES  576

#define WM_OPENMPT_SEEK (WM_USER+3)

#define SHORT_TITLE "in_openmpt"

static void apply_options();

static std::string StringEncode( const std::wstring &src, UINT codepage )
{
	int required_size = WideCharToMultiByte( codepage, 0, src.c_str(), -1, NULL, 0, NULL, NULL );
	if(required_size <= 0)
	{
		return std::string();
	}
	std::vector<CHAR> encoded_string( required_size );
	WideCharToMultiByte( codepage, 0, src.c_str(), -1, &encoded_string[0], encoded_string.size(), NULL, NULL );
	return &encoded_string[0];
}

static std::wstring StringDecode( const std::string & src, UINT codepage )
{
	int required_size = MultiByteToWideChar( codepage, 0, src.c_str(), -1, NULL, 0 );
	if(required_size <= 0)
	{
		return std::wstring();
	}
	std::vector<WCHAR> decoded_string( required_size );
	MultiByteToWideChar( codepage, 0, src.c_str(), -1, &decoded_string[0], decoded_string.size() );
	return &decoded_string[0];
}

template <typename Tstring, typename Tstring2, typename Tstring3>
static inline Tstring StringReplace( Tstring str, const Tstring2 & oldStr_, const Tstring3 & newStr_ ) {
	std::size_t pos = 0;
	const Tstring oldStr = oldStr_;
	const Tstring newStr = newStr_;
	while ( ( pos = str.find( oldStr, pos ) ) != Tstring::npos ) {
		str.replace( pos, oldStr.length(), newStr );
		pos += newStr.length();
	}
	return str;
}

struct self_winamp_t {
	std::vector<char> filetypes_string;
	libopenmpt::plugin::settings settings;
	int samplerate;
	int channels;
	std::wstring cached_filename;
	std::wstring cached_title;
	int cached_length;
	std::wstring cached_infotext;
	std::int64_t decode_position_frames;
	openmpt::module * mod;
	HANDLE PlayThread;
	DWORD PlayThreadID;
	bool paused;
	std::vector<std::int16_t> buffer;
	std::vector<std::int16_t> interleaved_buffer;
	self_winamp_t() : settings(TEXT(SHORT_TITLE), true) {
		filetypes_string.clear();
		settings.changed = apply_options;
		settings.load();
		std::vector<std::string> extensions = openmpt::get_supported_extensions();
		for ( std::vector<std::string>::iterator ext = extensions.begin(); ext != extensions.end(); ++ext ) {
			std::copy( (*ext).begin(), (*ext).end(), std::back_inserter( filetypes_string ) );
			filetypes_string.push_back('\0');
			std::copy( SHORT_TITLE, SHORT_TITLE + std::strlen(SHORT_TITLE), std::back_inserter( filetypes_string ) );
			filetypes_string.push_back('\0');
		}
		filetypes_string.push_back('\0');
		samplerate = settings.samplerate;
		channels = settings.channels;
		cached_filename = std::wstring();
		cached_title = std::wstring();
		cached_length = 0;
		cached_infotext = std::wstring();
		decode_position_frames = 0;
		mod = 0;
		PlayThread = 0;
		PlayThreadID = 0;
		paused = false;
		buffer.resize( WINAMP_BUFFER_SIZE_FRAMES * channels );
		interleaved_buffer.resize( WINAMP_BUFFER_SIZE_FRAMES * channels * WINAMP_DSP_HEADROOM_FACTOR );
	}
	~self_winamp_t() {
		return;
	}
};

static self_winamp_t * self = 0;

static void apply_options() {
	if ( self->mod ) {
		self->mod->set_repeat_count( self->settings.repeatcount );
		self->mod->set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, self->settings.mastergain_millibel );
		self->mod->set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, self->settings.stereoseparation );
		self->mod->set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, self->settings.interpolationfilterlength );
		self->mod->set_render_param( openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, self->settings.ramping );
		self->mod->ctl_set( "render.resampler.emulate_amiga", self->settings.use_amiga_resampler ? "1" : "0" );
	}
	self->settings.save();
}

extern In_Module inmod;

static DWORD WINAPI DecodeThread( LPVOID );

static std::wstring generate_infotext( const std::wstring & filename, const openmpt::module & mod ) {
	std::wostringstream str;
	str << L"filename: " << filename << std::endl;
	str << L"duration: " << mod.get_duration_seconds() << L"seconds" << std::endl;
	std::vector<std::string> metadatakeys = mod.get_metadata_keys();
	for ( std::vector<std::string>::iterator key = metadatakeys.begin(); key != metadatakeys.end(); ++key ) {
		if ( *key == "message_raw" ) {
			continue;
		}
		str << StringDecode( *key, CP_UTF8 ) << L": " << StringDecode( mod.get_metadata(*key), CP_UTF8 ) << std::endl;
	}
	return str.str();
}

static void config( HWND hwndParent ) {
	libopenmpt::plugin::gui_edit_settings( &self->settings, hwndParent, TEXT(SHORT_TITLE) );
	apply_options();
}

static void about( HWND hwndParent ) {
	std::ostringstream about;
	about << SHORT_TITLE << " version " << openmpt::string::get( "library_version" ) << " " << "(built " << openmpt::string::get( "build" ) << ")" << std::endl;
	about << " Copyright (c) 2013-2019 OpenMPT developers (https://lib.openmpt.org/)" << std::endl;
	about << " OpenMPT version " << openmpt::string::get( "core_version" ) << std::endl;
	about << std::endl;
	about << openmpt::string::get( "contact" ) << std::endl;
	about << std::endl;
	about << "Show full credits?" << std::endl;
	if ( MessageBox( hwndParent, StringDecode( about.str(), CP_UTF8 ).c_str(), TEXT(SHORT_TITLE), MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON1 ) != IDYES ) {
		return;
	}
	std::ostringstream credits;
	credits << openmpt::string::get( "credits" );
	libopenmpt::plugin::gui_show_file_info( hwndParent, TEXT(SHORT_TITLE), StringReplace( StringDecode( credits.str(), CP_UTF8 ), L"\n", L"\r\n" ) );
}

static void init() {
	if ( !self ) {
		self = new self_winamp_t();
		inmod.FileExtensions = &(self->filetypes_string[0]);
	}
}

static void quit() {
	if ( self ) {
		inmod.FileExtensions = NULL;
		delete self;
		self = 0;
	}
}

static int isourfile( const in_char * fn ) {
	return 0;
}

static int play( const in_char * fn ) {
	if ( !fn ) {
		return -1;
	}
	try {
		std::ifstream s( fn, std::ios::binary );
		std::map< std::string, std::string > ctls;
		ctls["seek.sync_samples"] = "1";
		self->mod = new openmpt::module( s, std::clog, ctls );
		self->cached_filename = fn;
		self->cached_title = StringDecode( self->mod->get_metadata( "title" ), CP_UTF8 );
		self->cached_length = static_cast<int>( self->mod->get_duration_seconds() * 1000.0 );
		self->cached_infotext = generate_infotext( self->cached_filename, *self->mod );
		apply_options();
		self->samplerate = self->settings.samplerate;
		self->channels = self->settings.channels;
		int maxlatency = inmod.outMod->Open( self->samplerate, self->channels, BPS, -1, -1 );
		std::ostringstream str;
		str << maxlatency;
		inmod.SetInfo( self->mod->get_num_channels(), self->samplerate/1000, self->channels, 1 );
		inmod.SAVSAInit( maxlatency, self->samplerate );
		inmod.VSASetInfo( self->channels, self->samplerate );
		inmod.outMod->SetVolume( -666 );
		inmod.outMod->SetPan( 0 );
		self->paused = false;
		self->decode_position_frames = 0;
		self->PlayThread = CreateThread( NULL, 0, DecodeThread, NULL, 0, &self->PlayThreadID );
		return 0;
	} catch ( ... ) {
		if ( self->mod ) {
			delete self->mod;
			self->mod = 0;
		}
		return -1;
	}
}

static void pause() {
	self->paused = true;
	inmod.outMod->Pause( 1 );
}

static void unpause() {
	self->paused = false;
	inmod.outMod->Pause( 0 );
}

static int ispaused() {
	return self->paused ? 1 : 0;
}

static void stop() {
	PostThreadMessage( self->PlayThreadID, WM_QUIT, 0, 0 );
	WaitForSingleObject( self->PlayThread, INFINITE );
	CloseHandle( self->PlayThread );
	self->PlayThread = 0;
	self->PlayThreadID = 0;
	delete self->mod;
	self->mod = 0;
	inmod.outMod->Close();
	inmod.SAVSADeInit();
}

static int getlength() {
	return self->cached_length;
}

static int getoutputtime() {
	//return (int)( self->decode_position_frames * 1000 / self->mod->get_render_param( openmpt::module::RENDER_SAMPLERATE_HZ ) /* + ( inmod.outMod->GetOutputTime() - inmod.outMod->GetWrittenTime() ) */ );
	return inmod.outMod->GetOutputTime();
}

static void setoutputtime( int time_in_ms ) {
	PostThreadMessage( self->PlayThreadID, WM_OPENMPT_SEEK, 0, time_in_ms );
}

static void setvolume( int volume ) {
	inmod.outMod->SetVolume( volume );
}

static void setpan( int pan ) { 
	inmod.outMod->SetPan( pan );
}

static int infobox( const in_char * fn, HWND hWndParent ) {
	if ( fn && fn[0] != '\0' && self->cached_filename != std::wstring(fn) ) {
		try {
			std::ifstream s( fn, std::ios::binary );
			openmpt::module mod( s );
			libopenmpt::plugin::gui_show_file_info( hWndParent, TEXT(SHORT_TITLE), StringReplace( generate_infotext( fn, mod ), L"\n", L"\r\n" ) );
		} catch ( ... ) {
		}
	} else {
		libopenmpt::plugin::gui_show_file_info( hWndParent, TEXT(SHORT_TITLE), StringReplace( self->cached_infotext, L"\n", L"\r\n" ) );
	}
	return 0;
}

static void getfileinfo( const in_char * filename, in_char * title, int * length_in_ms ) {
	if ( !filename || *filename == '\0' ) {
		if ( length_in_ms ) {
			*length_in_ms = self->cached_length;
		}
		if ( title ) {
			std::wstring truncated_title = self->cached_title;
			if ( truncated_title.length() >= GETFILEINFO_TITLE_LENGTH ) {
				truncated_title.resize( GETFILEINFO_TITLE_LENGTH - 1 );
			}
			wcscpy( title, truncated_title.c_str() );
		}
	} else {
		try {
			std::ifstream s( filename, std::ios::binary );
			openmpt::module mod( s );
			if ( length_in_ms ) {
				*length_in_ms = static_cast<int>( mod.get_duration_seconds() * 1000.0 );
			}
			if ( title ) {
				std::wstring truncated_title = StringDecode( mod.get_metadata("title"), CP_UTF8 );
				if ( truncated_title.length() >= GETFILEINFO_TITLE_LENGTH ) {
					truncated_title.resize( GETFILEINFO_TITLE_LENGTH - 1 );
				}
				wcscpy( title, truncated_title.c_str() );
			}
		} catch ( ... ) {
		}
	}
}

static void eq_set( int on, char data[10], int preamp ) {
	return;
}

static DWORD WINAPI DecodeThread( LPVOID ) {
	MSG msg;
	PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	bool eof = false;
	while ( true ) {
		bool quit = false;
		while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
			if ( msg.message == WM_QUIT ) {
				quit = true;
			} else if ( msg.message == WM_OPENMPT_SEEK ) {
				double pos_seconds = self->mod->set_position_seconds( msg.lParam * 0.001 );
				self->decode_position_frames = (std::int64_t)( pos_seconds * (double)self->samplerate);
				eof = false;
				inmod.outMod->Flush( (int)( pos_seconds * 1000.0 ) );
			}
		}
		if ( quit ) {
			break;
		}
		if ( eof ) {
			inmod.outMod->CanWrite(); // update output plugin state
			if ( !inmod.outMod->IsPlaying() ) {
				PostMessage( inmod.hMainWindow, WM_WA_MPEG_EOF, 0, 0 );
				return 0;
			}
			Sleep( 10 );
		} else {
			bool dsp_active = inmod.dsp_isactive() ? true : false;
			if ( inmod.outMod->CanWrite() >= (int)( WINAMP_BUFFER_SIZE_FRAMES * self->channels * sizeof( signed short ) ) * ( dsp_active ? WINAMP_DSP_HEADROOM_FACTOR : 1 ) ) {
				int frames = 0;
				switch ( self->channels ) {
				case 1:
					frames = self->mod->read( self->samplerate, WINAMP_BUFFER_SIZE_FRAMES, (&(self->buffer[0]))+0*WINAMP_BUFFER_SIZE_FRAMES );
					for ( int frame = 0; frame < frames; frame++ ) {
						self->interleaved_buffer[frame*1+0] = self->buffer[0*WINAMP_BUFFER_SIZE_FRAMES+frame];
					}
					break;
				case 2:
					frames = self->mod->read( self->samplerate, WINAMP_BUFFER_SIZE_FRAMES, (&(self->buffer[0]))+0*WINAMP_BUFFER_SIZE_FRAMES, (&(self->buffer[0]))+1*WINAMP_BUFFER_SIZE_FRAMES );
					for ( int frame = 0; frame < frames; frame++ ) {
						self->interleaved_buffer[frame*2+0] = self->buffer[0*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*2+1] = self->buffer[1*WINAMP_BUFFER_SIZE_FRAMES+frame];
					}
					break;
				case 4:
					frames = self->mod->read( self->samplerate, WINAMP_BUFFER_SIZE_FRAMES, (&(self->buffer[0]))+0*WINAMP_BUFFER_SIZE_FRAMES, (&(self->buffer[0]))+1*WINAMP_BUFFER_SIZE_FRAMES, (&(self->buffer[0]))+2*WINAMP_BUFFER_SIZE_FRAMES, (&(self->buffer[0]))+3*WINAMP_BUFFER_SIZE_FRAMES );
					for ( int frame = 0; frame < frames; frame++ ) {
						self->interleaved_buffer[frame*4+0] = self->buffer[0*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*4+1] = self->buffer[1*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*4+2] = self->buffer[2*WINAMP_BUFFER_SIZE_FRAMES+frame];
						self->interleaved_buffer[frame*4+3] = self->buffer[3*WINAMP_BUFFER_SIZE_FRAMES+frame];
					}
					break;
				}
				if ( frames == 0 ) {
					eof = true;
				} else {
					self->decode_position_frames += frames;
					std::int64_t decode_pos_ms = (self->decode_position_frames * 1000 / self->samplerate );
					inmod.SAAddPCMData( &( self->interleaved_buffer[0] ), self->channels, BPS, (int)decode_pos_ms );
					inmod.VSAAddPCMData( &( self->interleaved_buffer[0] ), self->channels, BPS, (int)decode_pos_ms );
					if ( dsp_active ) {
						frames = inmod.dsp_dosamples( &( self->interleaved_buffer[0] ), frames, BPS, self->channels, self->samplerate );
					}
					int bytes = frames * self->channels * sizeof( signed short );
					inmod.outMod->Write( (char*)&( self->interleaved_buffer[0] ), bytes );
				}
			} else {
				Sleep( 10 );
			}
		}
	}
	return 0;
}

In_Module inmod = {
	IN_VER,
	const_cast< char * >( in_openmpt_string ), // SHORT_TITLE,
	0, // hMainWindow
	0, // hDllInstance
	NULL, // filled later in Init() "mptm\0ModPlug Tracker Module (*.mptm)\0",
	1, // is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infobox,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	getlength,
	getoutputtime,
	setoutputtime,
	setvolume,
	setpan,
	0,0,0,0,0,0,0,0,0, // vis
	0,0, // dsp
	eq_set,
	NULL, // setinfo
	0 // out_mod
};

extern "C" __declspec(dllexport) In_Module * winampGetInModule2() {
	return &inmod;
}


#ifdef _MFC_VER

namespace libopenmpt {
namespace plugin {

void DllMainAttach() {
	// nothing
}

void DllMainDetach() {
	// nothing
}

} // namespace plugin
} // namespace libopenmpt

#else

// nothing

#endif


#endif // NO_WINAMP
