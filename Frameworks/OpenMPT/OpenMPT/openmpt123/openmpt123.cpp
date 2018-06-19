/*
 * openmpt123.cpp
 * --------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

static const char * const license =
"The OpenMPT code is licensed under the BSD license." "\n"
"" "\n"
"Copyright (c) 2004-2018, OpenMPT contributors" "\n"
"Copyright (c) 1997-2003, Olivier Lapicque" "\n"
"All rights reserved." "\n"
"" "\n"
"Redistribution and use in source and binary forms, with or without" "\n"
"modification, are permitted provided that the following conditions are met:" "\n"
"    * Redistributions of source code must retain the above copyright" "\n"
"      notice, this list of conditions and the following disclaimer." "\n"
"    * Redistributions in binary form must reproduce the above copyright" "\n"
"      notice, this list of conditions and the following disclaimer in the" "\n"
"      documentation and/or other materials provided with the distribution." "\n"
"    * Neither the name of the OpenMPT project nor the" "\n"
"      names of its contributors may be used to endorse or promote products" "\n"
"      derived from this software without specific prior written permission." "\n"
"" "\n"
"THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY" "\n"
"EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED" "\n"
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE" "\n"
"DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY" "\n"
"DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES" "\n"
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;" "\n"
"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND" "\n"
"ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT" "\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS" "\n"
"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE." "\n"
;

#include "openmpt123_config.hpp"

#include <algorithm>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if defined(WIN32)
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#else
#if defined(MPT_NEEDS_THREADS)
#include <pthread.h>
#endif
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <libopenmpt/libopenmpt.hpp>

#include "openmpt123.hpp"

#include "openmpt123_flac.hpp"
#include "openmpt123_mmio.hpp"
#include "openmpt123_sndfile.hpp"
#include "openmpt123_raw.hpp"
#include "openmpt123_stdout.hpp"
#include "openmpt123_portaudio.hpp"
#include "openmpt123_pulseaudio.hpp"
#include "openmpt123_sdl.hpp"
#include "openmpt123_sdl2.hpp"
#include "openmpt123_waveout.hpp"

namespace openmpt123 {

struct silent_exit_exception : public std::exception {
	silent_exit_exception() throw() { }
};

struct show_license_exception : public std::exception {
	show_license_exception() throw() { }
};

struct show_credits_exception : public std::exception {
	show_credits_exception() throw() { }
};

struct show_man_version_exception : public std::exception {
	show_man_version_exception() throw() { }
};

struct show_man_help_exception : public std::exception {
	show_man_help_exception() throw() { }
};

struct show_short_version_number_exception : public std::exception {
	show_short_version_number_exception() throw() { }
};

struct show_version_number_exception : public std::exception {
	show_version_number_exception() throw() { }
};

struct show_long_version_number_exception : public std::exception {
	show_long_version_number_exception() throw() { }
};

bool IsTerminal( int fd ) {
#if defined( WIN32 )
	return true
		&& ( _isatty( fd ) ? true : false )
		&& GetConsoleWindow() != NULL
		;
#else
	return isatty( fd ) ? true : false;
#endif
}

#if !defined( WIN32 )

static termios saved_attributes;

static void reset_input_mode() {
	tcsetattr( STDIN_FILENO, TCSANOW, &saved_attributes );
}

static void set_input_mode() {
	termios tattr;
	if ( !isatty( STDIN_FILENO ) ) {
		return;
	}
	tcgetattr( STDIN_FILENO, &saved_attributes );
	atexit( reset_input_mode );
	tcgetattr( STDIN_FILENO, &tattr );
	tattr.c_lflag &= ~( ICANON | ECHO );
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &tattr );
}

#endif

class file_audio_stream_raii : public file_audio_stream_base {
private:
	file_audio_stream_base * impl;
public:
	file_audio_stream_raii( const commandlineflags & flags, const std::string & filename, std::ostream & log )
		: impl(0)
	{
		if ( !flags.force_overwrite ) {
			std::ifstream testfile( filename, std::ios::binary );
			if ( testfile ) {
				throw exception( "file already exists" );
			}
		}
		if ( false ) {
			// nothing
		} else if ( flags.output_extension == "raw" ) {
			impl = new raw_stream_raii( filename, flags, log );
#ifdef MPT_WITH_MMIO
		} else if ( flags.output_extension == "wav" ) {
			impl = new mmio_stream_raii( filename, flags, log );
#endif				
#ifdef MPT_WITH_FLAC
		} else if ( flags.output_extension == "flac" ) {
			impl = new flac_stream_raii( filename, flags, log );
#endif				
#ifdef MPT_WITH_SNDFILE
		} else {
			impl = new sndfile_stream_raii( filename, flags, log );
#endif
		}
		if ( !impl ) {
			throw exception( "file format handler '" + flags.output_extension + "' not found" );
		}
	}
	virtual ~file_audio_stream_raii() {
		if ( impl ) {
			delete impl;
			impl = 0;
		}
	}
	virtual void write_metadata( std::map<std::string,std::string> metadata ) {
		impl->write_metadata( metadata );
	}
	virtual void write_updated_metadata( std::map<std::string,std::string> metadata ) {
		impl->write_updated_metadata( metadata );
	}
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) {
		impl->write( buffers, frames );
	}
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		impl->write( buffers, frames );
	}
};                                                                                                                

static std::string ctls_to_string( const std::map<std::string, std::string> & ctls ) {
	std::string result;
	for ( std::map<std::string, std::string>::const_iterator it = ctls.begin(); it != ctls.end(); ++it ) {
		if ( !result.empty() ) {
			result += "; ";
		}
		result += it->first + "=" + it->second;
	}
	return result;
}

static double tempo_flag_to_double( std::int32_t tempo ) {
	return std::pow( 2.0, tempo / 24.0 );
}

static double pitch_flag_to_double( std::int32_t pitch ) {
	return std::pow( 2.0, pitch / 24.0 );
}

static double my_round( double val ) {
	if ( val >= 0.0 ) {
		return std::floor( val + 0.5 );
	} else {
		return std::ceil( val - 0.5 );
	}
}

static std::int32_t double_to_tempo_flag( double factor ) {
	return static_cast<std::int32_t>( my_round( std::log( factor ) / std::log( 2.0 ) * 24.0 ) );
}

static std::int32_t double_to_pitch_flag( double factor ) {
	return static_cast<std::int32_t>( my_round( std::log( factor ) / std::log( 2.0 ) * 24.0 ) );
}

static std::ostream & operator << ( std::ostream & s, const commandlineflags & flags ) {
	s << "Quiet: " << flags.quiet << std::endl;
	s << "Verbose: " << flags.verbose << std::endl;
	s << "Mode : " << mode_to_string( flags.mode ) << std::endl;
	s << "Show progress: " << flags.show_progress << std::endl;
	s << "Show peak meters: " << flags.show_meters << std::endl;
	s << "Show channel peak meters: " << flags.show_channel_meters << std::endl;
	s << "Show details: " << flags.show_details << std::endl;
	s << "Show message: " << flags.show_message << std::endl;
	s << "Update: " << flags.ui_redraw_interval << "ms" << std::endl;
	s << "Device: " << flags.device << std::endl;
	s << "Buffer: " << flags.buffer << "ms" << std::endl;
	s << "Period: " << flags.period << "ms" << std::endl;
	s << "Samplerate: " << flags.samplerate << std::endl;
	s << "Channels: " << flags.channels << std::endl;
	s << "Float: " << flags.use_float << std::endl;
	s << "Gain: " << flags.gain / 100.0 << std::endl;
	s << "Stereo separation: " << flags.separation << std::endl;
	s << "Interpolation filter taps: " << flags.filtertaps << std::endl;
	s << "Volume ramping strength: " << flags.ramping << std::endl;
	s << "Tempo: " << tempo_flag_to_double( flags.tempo ) << std::endl;
	s << "Pitch: " << pitch_flag_to_double( flags.pitch ) << std::endl;
	s << "Output dithering: " << flags.dither << std::endl;
	s << "Repeat count: " << flags.repeatcount << std::endl;
	s << "Seek target: " << flags.seek_target << std::endl;
	s << "End time: " << flags.end_time << std::endl;
	s << "Standard output: " << flags.use_stdout << std::endl;
	s << "Output filename: " << flags.output_filename << std::endl;
	s << "Force overwrite output file: " << flags.force_overwrite << std::endl;
	s << "Ctls: " << ctls_to_string( flags.ctls ) << std::endl;
	s << std::endl;
	s << "Files: " << std::endl;
	for ( std::vector<std::string>::const_iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
		s << " " << *filename << std::endl;
	}
	s << std::endl;
	return s;
}

static std::string replace( std::string str, const std::string & oldstr, const std::string & newstr ) {
	std::size_t pos = 0;
	while ( ( pos = str.find( oldstr, pos ) ) != std::string::npos ) {
		str.replace( pos, oldstr.length(), newstr );
		pos += newstr.length();
	}
	return str;
}

static bool begins_with( const std::string & str, const std::string & match ) {
	return ( str.find( match ) == 0 );
}

static bool ends_with( const std::string & str, const std::string & match ) {
	return ( str.rfind( match ) == ( str.length() - match.length() ) );
}

static std::string trim_left(std::string str, const std::string &whitespace = std::string()) {
	std::string::size_type pos = str.find_first_not_of(whitespace);
	if(pos != std::string::npos) {
		str.erase(str.begin(), str.begin() + pos);
	} else if(pos == std::string::npos && str.length() > 0 && str.find_last_of(whitespace) == str.length() - 1) {
		return std::string();
	}
	return str;
}

static std::string trim_right(std::string str, const std::string &whitespace = std::string()) {
	std::string::size_type pos = str.find_last_not_of(whitespace);
	if(pos != std::string::npos) {
		str.erase(str.begin() + pos + 1, str.end());
	} else if(pos == std::string::npos && str.length() > 0 && str.find_first_of(whitespace) == 0) {
		return std::string();
	}
	return str;
}

static std::string trim(std::string str, const std::string &whitespace = std::string()) {
	return trim_right(trim_left(str, whitespace), whitespace);
}

static std::string trim_eol( const std::string & str ) {
	return trim( str, "\r\n" );
}

static std::string default_path_separator() {
#if defined(WIN32)
	return "\\";
#else
	return "/";
#endif
}

static std::string path_separators() {
#if defined(WIN32)
	return "\\/";
#else
	return "/";
#endif
}

static bool is_path_separator( char c ) {
#if defined(WIN32)
	return ( c == '\\' ) || ( c == '/' );
#else
	return c == '/';
#endif
}

static std::string get_basepath( std::string filename ) {
	std::string::size_type pos = filename.find_last_of( path_separators() );
	if ( pos == std::string::npos ) {
		return std::string();
	}
	return filename.substr( 0, pos ) + default_path_separator();
}

static bool is_absolute( std::string filename ) {
#if defined(WIN32)
	if ( begins_with( filename, "\\\\?\\UNC\\" ) ) {
		return true;
	}
	if ( begins_with( filename, "\\\\?\\" ) ) {
		return true;
	}
	if ( begins_with( filename, "\\\\" ) ) {
		return true; // UNC
	}
	if ( begins_with( filename, "//" ) ) {
		return true; // UNC
	}
	return ( filename.length() ) >= 3 && ( filename[1] == ':' ) && is_path_separator( filename[2] );
#else
	return ( filename.length() >= 1 ) && is_path_separator( filename[0] );
#endif
}

static std::string get_filename( const std::string & filepath ) {
	if ( filepath.find_last_of( path_separators() ) == std::string::npos ) {
		return filepath;
	}
	return filepath.substr( filepath.find_last_of( path_separators() ) + 1 );
}

static std::string prepend_lines( std::string str, const std::string & prefix ) {
	if ( str.empty() ) {
		return str;
	}
	if ( str.substr( str.length() - 1, 1 ) == std::string("\n") ) {
		str = str.substr( 0, str.length() - 1 );
	}
	return replace( str, std::string("\n"), std::string("\n") + prefix );
}

static std::string bytes_to_string( std::uint64_t bytes ) {
	static const char * const suffixes[] = { "B", "kB", "MB", "GB", "TB", "PB" };
	int offset = 0;
	while ( bytes > 9999 ) {
		bytes /= 1000;
		offset += 1;
		if ( offset == 5 ) {
			break;
		}
	}
	std::ostringstream result;
	result << bytes << suffixes[offset];
	return result.str();
}

static std::string seconds_to_string( double time ) {
	std::int64_t time_ms = static_cast<std::int64_t>( time * 1000 );
	std::int64_t milliseconds = time_ms % 1000;
	std::int64_t seconds = ( time_ms / 1000 ) % 60;
	std::int64_t minutes = ( time_ms / ( 1000 * 60 ) ) % 60;
	std::int64_t hours = ( time_ms / ( 1000 * 60 * 60 ) );
	std::ostringstream str;
	if ( hours > 0 ) {
		str << hours << ":";
	}
	str << std::setfill('0') << std::setw(2) << minutes;
	str << ":";
	str << std::setfill('0') << std::setw(2) << seconds;
	str << ".";
	str << std::setfill('0') << std::setw(3) << milliseconds;
	return str.str();
}

static void show_info( std::ostream & log, bool verbose ) {
	log << "openmpt123" << " v" << OPENMPT123_VERSION_STRING << ", libopenmpt " << openmpt::string::get( "library_version" ) << " (" << "OpenMPT " << openmpt::string::get( "core_version" ) << ")" << std::endl;
	log << "Copyright (c) 2013-2018 OpenMPT developers <https://lib.openmpt.org/>" << std::endl;
	if ( !verbose ) {
		log << std::endl;
		return;
	}
	log << "  libopenmpt source..: " << openmpt::string::get( "source_url" ) << std::endl;
	log << "  libopenmpt date....: " << openmpt::string::get( "source_date" ) << std::endl;
	log << "  libopenmpt srcinfo.: ";
	{
		std::vector<std::string> fields;
		if ( openmpt::string::get( "source_is_package" ) == "1" ) {
			fields.push_back( "package" );
		}
		if ( openmpt::string::get( "source_is_release" ) == "1" ) {
			fields.push_back( "release" );
		}
		if ( ( !openmpt::string::get( "source_revision" ).empty() ) && ( openmpt::string::get( "source_revision" ) != "0" ) ) {
			std::string field = "rev" + openmpt::string::get( "source_revision" );
			if ( openmpt::string::get( "source_has_mixed_revisions" ) == "1" ) {
				field += "+mixed";
			}
			if ( openmpt::string::get( "source_is_modified" ) == "1" ) {
				field += "+modified";
			}
			fields.push_back( field );
		}
		bool first = true;
		for ( std::vector<std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it ) {
			if ( first ) {
				first = false;
			} else {
				log << ", ";
			}
			log << (*it);
		}
	}
	log << std::endl;
	log << "  libopenmpt compiler: " << openmpt::string::get( "build_compiler" ) << std::endl;
	log << "  libopenmpt features: " << openmpt::string::get( "library_features" ) << std::endl;
#ifdef MPT_WITH_SDL2
	log << " libSDL2 ";
	SDL_version sdlver;
	std::memset( &sdlver, 0, sizeof( SDL_version ) );
	SDL_GetVersion( &sdlver );
	log << static_cast<int>( sdlver.major ) << "." << static_cast<int>( sdlver.minor ) << "." << static_cast<int>( sdlver.patch ) << "." << SDL_GetRevisionNumber();
	const char * revision = SDL_GetRevision();
	if ( revision ) {
		log << " (" << revision << ")";
	}
	log << ", ";
	std::memset( &sdlver, 0, sizeof( SDL_version ) );
	SDL_VERSION( &sdlver );
	log << "API: " << static_cast<int>( sdlver.major ) << "." << static_cast<int>( sdlver.minor ) << "." << static_cast<int>( sdlver.patch ) << "";
	log << " <https://libsdl.org/>" << std::endl;
#endif
#ifdef MPT_WITH_SDL
	const SDL_version * linked_sdlver = SDL_Linked_Version();
	log << " libSDL ";
	if ( linked_sdlver ) {
		log << static_cast<int>( linked_sdlver->major ) << "." << static_cast<int>( linked_sdlver->minor ) << "." << static_cast<int>( linked_sdlver->patch ) << " ";
	}
	SDL_version sdlver;
	std::memset( &sdlver, 0, sizeof( SDL_version ) );
	SDL_VERSION( &sdlver );
	log << "(API: " << static_cast<int>( sdlver.major ) << "." << static_cast<int>( sdlver.minor ) << "." << static_cast<int>( sdlver.patch ) << ")";
	log << " <https://libsdl.org/>" << std::endl;
#endif
#ifdef MPT_WITH_PULSEAUDIO
	log << " " << "libpulse, libpulse-simple" << " (headers " << pa_get_headers_version()  << ", API " << PA_API_VERSION << ", PROTOCOL " << PA_PROTOCOL_VERSION << ", library " << ( pa_get_library_version() ? pa_get_library_version() : "unknown" ) << ") <https://www.freedesktop.org/wiki/Software/PulseAudio/>" << std::endl;
#endif
#ifdef MPT_WITH_PORTAUDIO
	log << " " << Pa_GetVersionText() << " (" << Pa_GetVersion() << ") <http://portaudio.com/>" << std::endl;
#endif
#ifdef MPT_WITH_FLAC
	log << " FLAC " << FLAC__VERSION_STRING << ", " << FLAC__VENDOR_STRING << ", API " << FLAC_API_VERSION_CURRENT << "." << FLAC_API_VERSION_REVISION << "." << FLAC_API_VERSION_AGE << " <https://xiph.org/flac/>" << std::endl;
#endif
#ifdef MPT_WITH_SNDFILE
	char sndfile_info[128];
	std::memset( sndfile_info, 0, sizeof( sndfile_info ) );
	sf_command( 0, SFC_GET_LIB_VERSION, sndfile_info, sizeof( sndfile_info ) );
	sndfile_info[127] = '\0';
	log << " libsndfile " << sndfile_info << " <http://mega-nerd.com/libsndfile/>" << std::endl;
#endif
	log << std::endl;
}

static void show_man_version( textout & log ) {
	log << "openmpt123" << " v" << OPENMPT123_VERSION_STRING << std::endl;
	log << std::endl;
	log << "Copyright (c) 2013-2018 OpenMPT developers <https://lib.openmpt.org/>" << std::endl;
}

static void show_short_version( textout & log ) {
	log << OPENMPT123_VERSION_STRING << " / " << openmpt::string::get( "library_version" ) << " / " << openmpt::string::get( "core_version" ) << std::endl;
	log.writeout();
}

static void show_version( textout & log ) {
	show_info( log, false );
	log.writeout();
}

static void show_long_version( textout & log ) {
	show_info( log, true );
	log.writeout();
}

static void show_credits( textout & log ) {
	show_info( log, false );
	log << openmpt::string::get( "contact" ) << std::endl;
	log << std::endl;
	log << openmpt::string::get( "credits" ) << std::endl;
	log.writeout();
}

static void show_license( textout & log ) {
	show_info( log, false );
	log << license << std::endl;
	log.writeout();
}

static std::string get_driver_string( const std::string & driver ) {
	if ( driver.empty() ) {
		return "default";
	}
	return driver;
}

static std::string get_device_string( const std::string & device ) {
	if ( device.empty() ) {
		return "default";
	}
	return device;
}

static void show_help( textout & log, bool with_info = true, bool longhelp = false, bool man_version = false, const std::string & message = std::string() ) {
	if ( with_info ) {
		show_info( log, false );
	}
	{
		log << "Usage: openmpt123 [options] [--] file1 [file2] ..." << std::endl;
		log << std::endl;
		if ( man_version ) {
			log << "openmpt123 plays module music files." << std::endl;
			log << std::endl;
		}
		if ( man_version ) {
			log << "Options:" << std::endl;
		}
		log << " -h, --help                 Show help" << std::endl;
		log << "     --help-keyboard        Show keyboard hotkeys in ui mode" << std::endl;
		log << " -q, --quiet                Suppress non-error screen output" << std::endl;
		log << " -v, --verbose              Show more screen output" << std::endl;
		log << "     --version              Show version information and exit" << std::endl;
		log << "     --short-version        Show version number and nothing else" << std::endl;
		log << "     --long-version         Show long version information and exit" << std::endl;
		log << "     --credits              Show elaborate contributors list" << std::endl;
		log << "     --license              Show license" << std::endl;
		log << std::endl;
		log << "     --probe                Probe each file whether it is a supported file format" << std::endl;
		log << "     --info                 Display information about each file" << std::endl;
		log << "     --ui                   Interactively play each file" << std::endl;
		log << "     --batch                Play each file" << std::endl;
		log << "     --render               Render each file to individual PCM data files" << std::endl;
		if ( !longhelp ) {
			log << std::endl;
			log.writeout();
			return;
		}
		log << std::endl;
		log << "     --terminal-width n     Assume terminal is n characters wide [default: " << commandlineflags().terminal_width << "]" << std::endl;
		log << "     --terminal-height n    Assume terminal is n characters high [default: " << commandlineflags().terminal_height << "]" << std::endl;
		log << std::endl;
		log << "     --[no-]progress        Show playback progress [default: " << commandlineflags().show_progress << "]" << std::endl;
		log << "     --[no-]meters          Show peak meters [default: " << commandlineflags().show_meters << "]" << std::endl;
		log << "     --[no-]channel-meters  Show channel peak meters (EXPERIMENTAL) [default: " << commandlineflags().show_channel_meters << "]" << std::endl;
		log << "     --[no-]pattern         Show pattern (EXPERIMENTAL) [default: " << commandlineflags().show_pattern << "]" << std::endl;
		log << std::endl;
		log << "     --[no-]details         Show song details [default: " << commandlineflags().show_details << "]" << std::endl;
		log << "     --[no-]message         Show song message [default: " << commandlineflags().show_message << "]" << std::endl;
		log << std::endl;
		log << "     --update n             Set output update interval to n ms [default: " << commandlineflags().ui_redraw_interval << "]" << std::endl;
		log << std::endl;
		log << "     --samplerate n         Set samplerate to n Hz [default: " << commandlineflags().samplerate << "]" << std::endl;
		log << "     --channels n           use n [1,2,4] output channels [default: " << commandlineflags().channels << "]" << std::endl;
		log << "     --[no-]float           Output 32bit floating point instead of 16bit integer [default: " << commandlineflags().use_float << "]" << std::endl;
		log << std::endl;
		log << "     --gain n               Set output gain to n dB [default: " << commandlineflags().gain / 100.0 << "]" << std::endl;
		log << "     --stereo n             Set stereo separation to n % [default: " << commandlineflags().separation << "]" << std::endl;
		log << "     --filter n             Set interpolation filter taps to n [1,2,4,8] [default: " << commandlineflags().filtertaps << "]" << std::endl;
		log << "     --ramping n            Set volume ramping strength n [0..5] [default: " << commandlineflags().ramping << "]" << std::endl;
		log << "     --tempo f              Set tempo factor f [default: " << tempo_flag_to_double( commandlineflags().tempo ) << "]" << std::endl;
		log << "     --pitch f              Set pitch factor f [default: " << pitch_flag_to_double( commandlineflags().pitch ) << "]" << std::endl;
		log << "     --dither n             Dither type to use (if applicable for selected output format): [0=off,1=auto,2=0.5bit,3=1bit] [default: " << commandlineflags().dither << "]" << std::endl;
		log << std::endl;
		log << "     --playlist file        Load playlist from file" << std::endl;
		log << "     --[no-]randomize       Randomize playlist [default: " << commandlineflags().randomize << "]" << std::endl;
		log << "     --[no-]shuffle         Shuffle through playlist [default: " << commandlineflags().shuffle << "]" << std::endl;
		log << "     --[no-]restart         Restart playlist when finished [default: " << commandlineflags().restart << "]" << std::endl;
		log << std::endl;
		log << "     --subsong n            Select subsong n (-1 means play all subsongs consecutively) [default: " << commandlineflags().subsong << "]" << std::endl;
		log << "     --repeat n             Repeat song n times (-1 means forever) [default: " << commandlineflags().repeatcount << "]" << std::endl;
		log << "     --seek n               Seek to n seconds on start [default: " << commandlineflags().seek_target << "]" << std::endl;
		log << "     --end-time n           Play until position is n seconds (0 means until the end) [default: " << commandlineflags().end_time << "]" << std::endl;
		log << std::endl;
		log << "     --ctl c=v              Set libopenmpt ctl c to value v" << std::endl;
		log << std::endl;
		log << "     --driver n             Set output driver [default: " << get_driver_string( commandlineflags().driver ) << "]," << std::endl;
		log << "     --device n             Set output device [default: " << get_device_string( commandlineflags().device ) << "]," << std::endl;
		log << "                            use --device help to show available devices" << std::endl;
		log << "     --buffer n             Set output buffer size to n ms [default: " << commandlineflags().buffer << "]" << std::endl;
		log << "     --period n             Set output period size to n ms [default: " << commandlineflags().period  << "]" << std::endl;
		log << "     --stdout               Write raw audio data to stdout [default: " << commandlineflags().use_stdout << "]" << std::endl;
		log << "     --output-type t        Use output format t when writing to a individual PCM files (only applies to --render mode) [default: " << commandlineflags().output_extension << "]" << std::endl;
		log << " -o, --output f             Write PCM output to file f instead of streaming to audio device (only applies to --ui and --batch modes) [default: " << commandlineflags().output_filename << "]" << std::endl;
		log << "     --force                Force overwriting of output file [default: " << commandlineflags().force_overwrite << "]" << std::endl;
		log << std::endl;
		log << "     --                     Interpret further arguments as filenames" << std::endl;
		log << std::endl;
		if ( !man_version ) {
			log << " Supported file formats: " << std::endl;
			log << "    ";
			std::vector<std::string> extensions = openmpt::get_supported_extensions();
			bool first = true;
			for ( std::vector<std::string>::iterator i = extensions.begin(); i != extensions.end(); ++i ) {
				if ( first ) {
					first = false;
				} else {
					log << ", ";
				}
				log << *i;
			}
			log << std::endl;
		}
	}

	log << std::endl;

	if ( message.size() > 0 ) {
		log << message;
		log << std::endl;
	}
	log.writeout();
}

static void show_help_keyboard( textout & log ) {
	show_info( log, false );
	log << "Keyboard hotkeys (use 'openmpt123 --ui'):" << std::endl;
	log << std::endl;
	log << " [q]     quit" << std::endl;
	log << " [ ]     pause / unpause" << std::endl;
	log << " [N]     skip 10 files backward" << std::endl;
	log << " [n]     prev file" << std::endl;
	log << " [m]     next file" << std::endl;
	log << " [M]     skip 10 files forward" << std::endl;
	log << " [h]     seek 10 seconds backward" << std::endl;
	log << " [j]     seek 1 seconds backward" << std::endl;
	log << " [k]     seek 1 seconds forward" << std::endl;
	log << " [l]     seek 10 seconds forward" << std::endl;
	log << " [u]|[i] +/- tempo" << std::endl;
	log << " [o]|[p] +/- pitch" << std::endl;
	log << " [3]|[4] +/- gain" << std::endl;
	log << " [5]|[6] +/- stereo separation" << std::endl;
	log << " [7]|[8] +/- filter taps" << std::endl;
	log << " [9]|[0] +/- volume ramping" << std::endl;
	log << std::endl;
	log.writeout();
}


template < typename T, typename Tmod >
T ctl_get( Tmod & mod, const std::string & ctl ) {
	T result = T();
	try {
		std::istringstream str;
		str.imbue( std::locale::classic() );
		str.str( mod.ctl_get( ctl ) );
		str >> std::fixed >> std::setprecision(16) >> result;
	} catch ( const openmpt::exception & ) {
		// ignore
	}
	return result;
}

template < typename T, typename Tmod >
void ctl_set( Tmod & mod, const std::string & ctl, const T & val ) {
	try {
		std::ostringstream str;
		str.imbue( std::locale::classic() );
		str << std::fixed << std::setprecision(16) << val;
		mod.ctl_set( ctl, str.str() );
	} catch ( const openmpt::exception & ) {
		// ignore
	}
	return;
}

template < typename Tmod >
static void apply_mod_settings( commandlineflags & flags, Tmod & mod ) {
	flags.separation = std::max( flags.separation,  0 );
	flags.filtertaps = std::max( flags.filtertaps,  1 );
	flags.filtertaps = std::min( flags.filtertaps,  8 );
	flags.ramping    = std::max( flags.ramping,    -1 );
	flags.ramping    = std::min( flags.ramping,    10 );
	flags.tempo      = std::max( flags.tempo,     -48 );
	flags.tempo      = std::min( flags.tempo,      48 );
	flags.pitch      = std::max( flags.pitch,     -48 );
	flags.pitch      = std::min( flags.pitch,      48 );
	mod.set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, flags.gain );
	mod.set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, flags.separation );
	mod.set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, flags.filtertaps );
	mod.set_render_param( openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, flags.ramping );
	ctl_set( mod, "play.tempo_factor", tempo_flag_to_double( flags.tempo ) );
	ctl_set( mod, "play.pitch_factor", pitch_flag_to_double( flags.pitch ) );
	std::ostringstream dither_str;
	dither_str.imbue( std::locale::classic() );
	dither_str << flags.dither;
	mod.ctl_set( "dither", dither_str.str() );
}

struct prev_file { int count; prev_file( int c ) : count(c) { } };
struct next_file { int count; next_file( int c ) : count(c) { } };

template < typename Tmod >
static bool handle_keypress( int c, commandlineflags & flags, Tmod & mod, write_buffers_interface & audio_stream ) {
	switch ( c ) {
		case 'q': throw silent_exit_exception(); break;
		case 'N': throw prev_file(10); break;
		case 'n': throw prev_file(1); break;
		case ' ': if ( !flags.paused ) { flags.paused = audio_stream.pause(); } else { flags.paused = false; audio_stream.unpause(); } break;
		case 'h': mod.set_position_seconds( mod.get_position_seconds() - 10.0 ); break;
		case 'j': mod.set_position_seconds( mod.get_position_seconds() - 1.0 ); break;
		case 'k': mod.set_position_seconds( mod.get_position_seconds() + 1.0 ); break;
		case 'l': mod.set_position_seconds( mod.get_position_seconds() + 10.0 ); break;
		case 'H': mod.set_position_order_row( mod.get_current_order() - 1, 0 ); break;
		case 'J': mod.set_position_order_row( mod.get_current_order(), mod.get_current_row() - 1 ); break;
		case 'K': mod.set_position_order_row( mod.get_current_order(), mod.get_current_row() + 1 ); break;
		case 'L': mod.set_position_order_row( mod.get_current_order() + 1, 0 ); break;
		case 'm': throw next_file(1); break;
		case 'M': throw next_file(10); break;
		case 'u': flags.tempo -= 1; apply_mod_settings( flags, mod ); break;
		case 'i': flags.tempo += 1; apply_mod_settings( flags, mod ); break;
		case 'o': flags.pitch -= 1; apply_mod_settings( flags, mod ); break;
		case 'p': flags.pitch += 1; apply_mod_settings( flags, mod ); break;
		case '3': flags.gain       -=100; apply_mod_settings( flags, mod ); break;
		case '4': flags.gain       +=100; apply_mod_settings( flags, mod ); break;
		case '5': flags.separation -=  5; apply_mod_settings( flags, mod ); break;
		case '6': flags.separation +=  5; apply_mod_settings( flags, mod ); break;
		case '7': flags.filtertaps /=  2; apply_mod_settings( flags, mod ); break;
		case '8': flags.filtertaps *=  2; apply_mod_settings( flags, mod ); break;
		case '9': flags.ramping    -=  1; apply_mod_settings( flags, mod ); break;
		case '0': flags.ramping    +=  1; apply_mod_settings( flags, mod ); break;
	}
	return true;
}

struct meter_channel {
	float peak;
	float clip;
	float hold;
	float hold_age;
	meter_channel()
		: peak(0.0f)
		, clip(0.0f)
		, hold(0.0f)
		, hold_age(0.0f)
	{
		return;
	}
};

struct meter_type {
	meter_channel channels[4];
};

static const float falloff_rate = 20.0f / 1.7f;

static void update_meter( meter_type & meter, const commandlineflags & flags, std::size_t count, const std::int16_t * const * buffers ) {
	float falloff_factor = std::pow( 10.0f, -falloff_rate / flags.samplerate / 20.0f );
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		meter.channels[channel].peak = 0.0f;
		for ( std::size_t frame = 0; frame < count; ++frame ) {
			if ( meter.channels[channel].clip != 0.0f ) {
				meter.channels[channel].clip -= ( 1.0f / 2.0f ) * 1.0f / static_cast<float>( flags.samplerate );
				if ( meter.channels[channel].clip <= 0.0f ) {
					meter.channels[channel].clip = 0.0f;
				}
			}
			float val = std::fabs( buffers[channel][frame] / 32768.0f );
			if ( val >= 1.0f ) {
				meter.channels[channel].clip = 1.0f;
			}
			if ( val > meter.channels[channel].peak ) {
				meter.channels[channel].peak = val;
			}
			meter.channels[channel].hold *= falloff_factor;
			if ( val > meter.channels[channel].hold ) {
				meter.channels[channel].hold = val;
				meter.channels[channel].hold_age = 0.0f;
			} else {
				meter.channels[channel].hold_age += 1.0f / static_cast<float>( flags.samplerate );
			}
		}
	}
}

static void update_meter( meter_type & meter, const commandlineflags & flags, std::size_t count, const float * const * buffers ) {
	float falloff_factor = std::pow( 10.0f, -falloff_rate / flags.samplerate / 20.0f );
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		if ( !count ) {
			meter = meter_type();
		}
		meter.channels[channel].peak = 0.0f;
		for ( std::size_t frame = 0; frame < count; ++frame ) {
			if ( meter.channels[channel].clip != 0.0f ) {
				meter.channels[channel].clip -= ( 1.0f / 2.0f ) * 1.0f / static_cast<float>( flags.samplerate );
				if ( meter.channels[channel].clip <= 0.0f ) {
					meter.channels[channel].clip = 0.0f;
				}
			}
			float val = std::fabs( buffers[channel][frame] );
			if ( val >= 1.0f ) {
				meter.channels[channel].clip = 1.0f;
			}
			if ( val > meter.channels[channel].peak ) {
				meter.channels[channel].peak = val;
			}
			meter.channels[channel].hold *= falloff_factor;
			if ( val > meter.channels[channel].hold ) {
				meter.channels[channel].hold = val;
				meter.channels[channel].hold_age = 0.0f;
			} else {
				meter.channels[channel].hold_age += 1.0f / static_cast<float>( flags.samplerate );
			}
		}
	}
}

static const char * const channel_tags[4][4] = {
	{ " C", "  ", "  ", "  " },
	{ " L", " R", "  ", "  " },
	{ "FL", "FR", "RC", "  " },
	{ "FL", "FR", "RL", "RR" },
};

static std::string channel_to_string( int channels, int channel, const meter_channel & meter, bool tiny = false ) {
	int val = std::numeric_limits<int>::min();
	int hold_pos = std::numeric_limits<int>::min();
	if ( meter.peak > 0.0f ) {
		float db = 20.0f * std::log10( meter.peak );
		val = static_cast<int>( db + 48.0f );
	}
	if ( meter.hold > 0.0f ) {
		float db_hold = 20.0f * std::log10( meter.hold );
		hold_pos = static_cast<int>( db_hold + 48.0f );
	}
	if ( val < 0 ) {
		val = 0;
	}
	int headroom = val;
	if ( val > 48 ) {
		val = 48;
	}
	headroom -= val;
	if ( headroom < 0 ) {
		headroom = 0;
	}
	if ( headroom > 12 ) {
		headroom = 12;
	}
	headroom -= 1; // clip indicator
	if ( headroom < 0 ) {
		headroom = 0;
	}
	if ( tiny ) {
		if ( meter.clip != 0.0f || meter.peak >= 1.0f ) {
			return "#";
		} else if ( meter.peak > std::pow( 10.0f, -6.0f / 20.0f ) ) {
			return "O";
		} else if ( meter.peak > std::pow( 10.0f, -12.0f / 20.0f ) ) {
			return "o";
		} else if ( meter.peak > std::pow( 10.0f, -18.0f / 20.0f ) ) {
			return ".";
		} else {
			return " ";
		}
	} else {
		std::ostringstream res1;
		std::ostringstream res2;
		res1
			<< "        "
			<< channel_tags[channels-1][channel]
			<< " : "
			;
		res2 
			<< std::string(val,'>') << std::string(48-val,' ')
			<< ( ( meter.clip != 0.0f ) ? "#" : ":" )
			<< std::string(headroom,'>') << std::string(12-headroom,' ')
			;
		std::string tmp = res2.str();
		if ( 0 <= hold_pos && hold_pos <= 60 ) {
			if ( hold_pos == 48 ) {
				tmp[hold_pos] = '#';
			} else {
				tmp[hold_pos] = ':';
			}
		}
		return res1.str() + tmp;
	}
}

static char peak_to_char( float peak ) {
	if ( peak >= 1.0f ) {
		return '#';
	} else if ( peak >= 0.5f ) {
		return 'O';
	} else if ( peak >= 0.25f ) {
		return 'o';
	} else if ( peak >= 0.125f ) {
		return '.';
	} else {
		return ' ';
	}
}

static std::string peak_to_string_left( float peak, int width ) {
	std::string result;
	float thresh = 1.0f;
	while ( width-- ) {
		if ( peak >= thresh ) {
			if ( thresh == 1.0f ) {
				result.push_back( '#' );
			} else {
				result.push_back( '<' );
			}
		} else {
			result.push_back( ' ' );
		}
		thresh *= 0.5f;
	}
	return result;
}

static std::string peak_to_string_right( float peak, int width ) {
	std::string result;
	float thresh = 1.0f;
	while ( width-- ) {
		if ( peak >= thresh ) {
			if ( thresh == 1.0f ) {
				result.push_back( '#' );
			} else {
				result.push_back( '>' );
			}
		} else {
			result.push_back( ' ' );
		}
		thresh *= 0.5f;
	}
	std::reverse( result.begin(), result.end() );
	return result;
}

static void draw_meters( std::ostream & log, const meter_type & meter, const commandlineflags & flags ) {
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		log << channel_to_string( flags.channels, channel, meter.channels[channel] ) << std::endl;
	}
}

static void draw_meters_tiny( std::ostream & log, const meter_type & meter, const commandlineflags & flags ) {
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		log << channel_to_string( flags.channels, channel, meter.channels[channel], true );
	}
}

static void draw_channel_meters_tiny( std::ostream & log, float peak ) {
	log << peak_to_char( peak );
}

static void draw_channel_meters_tiny( std::ostream & log, float peak_left, float peak_right ) {
	log << peak_to_char( peak_left ) << peak_to_char( peak_right );
}

static void draw_channel_meters( std::ostream & log, float peak_left, float peak_right, int width ) {
	if ( width >= 8 + 1 + 8 ) {
		width = 8 + 1 + 8;
	}
	log << peak_to_string_left( peak_left, width / 2 ) << ( width % 2 == 1 ? ":" : "" ) << peak_to_string_right( peak_right, width / 2 );
}

template < typename Tsample, typename Tmod >
void render_loop( commandlineflags & flags, Tmod & mod, double & duration, textout & log, write_buffers_interface & audio_stream ) {

	log.writeout();

	std::size_t bufsize;
	if ( flags.mode == ModeUI ) {
		bufsize = std::min( flags.ui_redraw_interval, flags.period ) * flags.samplerate / 1000;
	} else if ( flags.mode == ModeBatch ) {
		bufsize = flags.period * flags.samplerate / 1000;
	} else {
		bufsize = 1024;
	}

	std::int64_t last_redraw_frame = 0 - flags.ui_redraw_interval;
	std::int64_t rendered_frames = 0;

	std::vector<Tsample> left( bufsize );
	std::vector<Tsample> right( bufsize );
	std::vector<Tsample> rear_left( bufsize );
	std::vector<Tsample> rear_right( bufsize );
	std::vector<Tsample*> buffers( 4 ) ;
	buffers[0] = left.data();
	buffers[1] = right.data();
	buffers[2] = rear_left.data();
	buffers[3] = rear_right.data();
	buffers.resize( flags.channels );
	
	meter_type meter;
	
	const bool multiline = flags.show_ui;
	
	int lines = 0;

	int pattern_lines = 0;
	
	if ( multiline ) {
		lines += 1;
		if ( flags.show_ui ) {
			lines += 1;
		}
		if ( flags.show_meters ) {
			for ( int channel = 0; channel < flags.channels; ++channel ) {
				lines += 1;
			}
		}
		if ( flags.show_channel_meters ) {
			lines += 1;
		}
		if ( flags.show_details ) {
			lines += 1;
			if ( flags.show_progress ) {
				lines += 1;
			}
		}
		if ( flags.show_progress ) {
			lines += 1;
		}
		if ( flags.show_pattern ) {
			pattern_lines = flags.terminal_height - lines - 1;
			lines = flags.terminal_height - 1;
		}
	} else if ( flags.show_ui || flags.show_details || flags.show_progress ) {
		log << std::endl;
	}
	for ( int line = 0; line < lines; ++line ) {
		log << std::endl;
	}

	log.writeout();

#if defined( WIN32 )
	HANDLE hStdErr = NULL;
	COORD coord_cursor = COORD();
	if ( multiline ) {
		log.flush();
		hStdErr = GetStdHandle( STD_ERROR_HANDLE );
		if ( hStdErr ) {
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			ZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
			GetConsoleScreenBufferInfo( hStdErr, &csbi );
			coord_cursor = csbi.dwCursorPosition;
			coord_cursor.X = 1;
			coord_cursor.Y -= lines;
		}
	}
#endif

	double cpu_smooth = 0.0;

	while ( true ) {

		if ( flags.mode == ModeUI ) {

#if defined( WIN32 )

			while ( _kbhit() ) {
				int c = _getch();
				if ( !handle_keypress( c, flags, mod, audio_stream ) ) {
					return;
				}
			}

#else

			while ( true ) {
				pollfd pollfds;
				pollfds.fd = STDIN_FILENO;
				pollfds.events = POLLIN;
				poll(&pollfds, 1, 0);
				if ( !( pollfds.revents & POLLIN ) ) {
					break;
				}
				char c = 0;
				if ( read( STDIN_FILENO, &c, 1 ) != 1 ) {
					break;
				}
				if ( !handle_keypress( c, flags, mod, audio_stream ) ) {
					return;
				}
			}

#endif

			if ( flags.paused ) {
				audio_stream.sleep( flags.ui_redraw_interval );
				continue;
			}

		}
		
		std::clock_t cpu_beg = 0;
		std::clock_t cpu_end = 0;
		if ( flags.show_details ) {
			cpu_beg = std::clock();
		}

		std::size_t count = 0;

		switch ( flags.channels ) {
			case 1: count = mod.read( flags.samplerate, bufsize, left.data() ); break;
			case 2: count = mod.read( flags.samplerate, bufsize, left.data(), right.data() ); break;
			case 4: count = mod.read( flags.samplerate, bufsize, left.data(), right.data(), rear_left.data(), rear_right.data() ); break;
		}
		
		char cpu_str[64] = "";
		if ( flags.show_details ) {
			cpu_end = std::clock();
			if ( count > 0 ) {
				double cpu = 1.0;
				cpu *= ( static_cast<double>( cpu_end ) - static_cast<double>( cpu_beg ) ) / static_cast<double>( CLOCKS_PER_SEC );
				cpu /= ( static_cast<double>( count ) ) / static_cast<double>( flags.samplerate );
				double mix = ( static_cast<double>( count ) ) / static_cast<double>( flags.samplerate );
				cpu_smooth = ( 1.0 - mix ) * cpu_smooth + mix * cpu;
				sprintf( cpu_str, "%.2f%%", cpu_smooth * 100.0 );
			}
		}

		if ( flags.show_meters ) {
			update_meter( meter, flags, count, buffers.data() );
		}

		if ( count > 0 ) {
			audio_stream.write( buffers, count );
		}

		if ( count > 0 ) {
			rendered_frames += count;
			if ( rendered_frames >= last_redraw_frame + ( flags.ui_redraw_interval * flags.samplerate / 1000 ) ) {
				last_redraw_frame = rendered_frames;
			} else {
				continue;
			}
		}

		if ( multiline ) {
#if defined( WIN32 )
			log.flush();
			if ( hStdErr ) {
				SetConsoleCursorPosition( hStdErr, coord_cursor );
			}
#else
			for ( int line = 0; line < lines; ++line ) {
				log << "\x1b[1A";
			}
#endif
			log << std::endl;
			if ( flags.show_meters ) {
				draw_meters( log, meter, flags );
			}
			if ( flags.show_channel_meters ) {
				int width = ( flags.terminal_width - 3 ) / mod.get_num_channels();
				if ( width > 11 ) {
					width = 11;
				}
				log << " ";
				for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
					if ( width >= 3 ) {
						log << ":";
					}
					if ( width == 1 ) {
						draw_channel_meters_tiny( log, ( mod.get_current_channel_vu_left( channel ) + mod.get_current_channel_vu_right( channel ) ) * (1.0f/std::sqrt(2.0f)) );
					} else if ( width <= 4 ) {
						draw_channel_meters_tiny( log, mod.get_current_channel_vu_left( channel ), mod.get_current_channel_vu_right( channel ) );
					} else {
						draw_channel_meters( log, mod.get_current_channel_vu_left( channel ), mod.get_current_channel_vu_right( channel ), width - 1 );
					}
				}
				if ( width >= 3 ) {
					log << ":";
				}
				log << std::endl;
			}
			if ( flags.show_pattern ) {
				int width = ( flags.terminal_width - 3 ) / mod.get_num_channels();
				if ( width > 13 + 1 ) {
					width = 13 + 1;
				}
				for ( std::int32_t line = 0; line < pattern_lines; ++line ) {
					std::int32_t row = mod.get_current_row() - ( pattern_lines / 2 ) + line;
					if ( row == mod.get_current_row() ) {
						log << ">";
					} else {
						log << " ";
					}
					if ( row < 0 || row >= mod.get_pattern_num_rows( mod.get_current_pattern() ) ) {
						for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
							if ( width >= 3 ) {
								log << ":";
							}
							log << std::string( width >= 3 ? width - 1 : width, ' ' );
						}
					} else {
						for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
							if ( width >= 3 ) {
								if ( row == mod.get_current_row() ) {
									log << "+";
								} else {
									log << ":";
								}
							}
							log << mod.format_pattern_row_channel( mod.get_current_pattern(), row, channel, width >= 3 ? width - 1 : width );
						}
					}
					if ( width >= 3 ) {
						log << ":";
					}
					log << std::endl;
				}
			}
			if ( flags.show_ui ) {
				log << "Settings...: ";
				log << "Gain: " << flags.gain * 0.01f << " dB" << "   ";
				log << "Stereo: " << flags.separation << " %" << "   ";
				log << "Filter: " << flags.filtertaps << " taps" << "   ";
				log << "Ramping: " << flags.ramping << "   ";
				log  << std::endl;
			}
			if ( flags.show_details ) {
				log << "Mixer......: ";
				log << "CPU:" << std::setw(6) << std::setfill(':') << cpu_str;
				log << "   ";
				log << "Chn:" << std::setw(3) << std::setfill(':') << mod.get_current_playing_channels();
				log << "   ";
				log << std::endl;
				if ( flags.show_progress ) {
					log << "Player.....: ";
					log << "Ord:" << std::setw(3) << std::setfill(':') << mod.get_current_order() << "/" << std::setw(3) << std::setfill(':') << mod.get_num_orders();
					log << " ";
					log << "Pat:" << std::setw(3) << std::setfill(':') << mod.get_current_pattern();
					log << " ";
					log << "Row:" << std::setw(3) << std::setfill(':') << mod.get_current_row();
					log << "   ";
					log << "Spd:" << std::setw(2) << std::setfill(':') << mod.get_current_speed();
					log << " ";
					log << "Tmp:" << std::setw(3) << std::setfill(':') << mod.get_current_tempo();
					log << "   ";
					log << std::endl;
				}
			}
			if ( flags.show_progress ) {
				log << "Position...: " << seconds_to_string( mod.get_position_seconds() ) << " / " << seconds_to_string( duration ) << "   " << std::endl;
			}
		} else if ( flags.show_channel_meters ) {
			if ( flags.show_ui || flags.show_details || flags.show_progress ) {
				int width = ( flags.terminal_width - 3 ) / mod.get_num_channels();
				log << " ";
				for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
					if ( width >= 3 ) {
						log << ":";
					}
					if ( width == 1 ) {
						draw_channel_meters_tiny( log, ( mod.get_current_channel_vu_left( channel ) + mod.get_current_channel_vu_right( channel ) ) * (1.0f/std::sqrt(2.0f)) );
					} else if ( width <= 4 ) {
						draw_channel_meters_tiny( log, mod.get_current_channel_vu_left( channel ), mod.get_current_channel_vu_right( channel ) );
					} else {
						draw_channel_meters( log, mod.get_current_channel_vu_left( channel ), mod.get_current_channel_vu_right( channel ), width - 1 );
					}
				}
				if ( width >= 3 ) {
					log << ":";
				}
			}
			log << "   " << "\r";
		} else {
			if ( flags.show_ui ) {
				log << " ";
				log << std::setw(3) << std::setfill(':') << flags.gain * 0.01f << "dB";
				log << "|";
				log << std::setw(3) << std::setfill(':') << flags.separation << "%";
				log << "|";
				log << std::setw(2) << std::setfill(':') << flags.filtertaps << "taps";
				log << "|";
				log << std::setw(3) << std::setfill(':') << flags.ramping;
			}
			if ( flags.show_meters ) {
				log << " ";
				draw_meters_tiny( log, meter, flags );
			}
			if ( flags.show_details && flags.show_ui ) {
				log << " ";
				log << "CPU:" << std::setw(6) << std::setfill(':') << cpu_str;
				log << "|";
				log << "Chn:" << std::setw(3) << std::setfill(':') << mod.get_current_playing_channels();
			}
			if ( flags.show_details && !flags.show_ui ) {
				if ( flags.show_progress ) {
					log << " ";
					log << "Ord:" << std::setw(3) << std::setfill(':') << mod.get_current_order() << "/" << std::setw(3) << std::setfill(':') << mod.get_num_orders();
					log << "|";
					log << "Pat:" << std::setw(3) << std::setfill(':') << mod.get_current_pattern();
					log << "|";
					log << "Row:" << std::setw(3) << std::setfill(':') << mod.get_current_row();
					log << " ";
					log << "Spd:" << std::setw(2) << std::setfill(':') << mod.get_current_speed();
					log << "|";
					log << "Tmp:" << std::setw(3) << std::setfill(':') << mod.get_current_tempo();
				}
			}
			if ( flags.show_progress ) {
				log << " ";
				log << seconds_to_string( mod.get_position_seconds() );
				log << "/";
				log << seconds_to_string( duration );
			}
			if ( flags.show_ui || flags.show_details || flags.show_progress ) {
				log << "   " << "\r";
			}
		}

		log.writeout();

		if ( count == 0 ) {
			break;
		}
		
		if ( flags.end_time > 0 && mod.get_position_seconds() >= flags.end_time ) {
			break;
		}

	}

	log.writeout();

}

template < typename Tmod >
std::map<std::string,std::string> get_metadata( const Tmod & mod ) {
	std::map<std::string,std::string> result;
	const std::vector<std::string> metadata_keys = mod.get_metadata_keys();
	for ( std::vector<std::string>::const_iterator key = metadata_keys.begin(); key != metadata_keys.end(); ++key ) {
		result[ *key ] = mod.get_metadata( *key );
	}
	return result;
}

class set_field : private std::ostringstream {
private:
	std::vector<openmpt123::field> & fields;
public:
	set_field( std::vector<openmpt123::field> & fields, const std::string & name )
		: fields(fields)
	{
		fields.push_back( name );
	}
	std::ostream & ostream() {
		return *this;
	}
	~set_field() {
		fields.back().val = str();
	}
};

static void show_fields( textout & log, const std::vector<field> & fields ) {
	const std::size_t fw = 11;
	for ( std::vector<field>::const_iterator it = fields.begin(); it != fields.end(); ++it ) {
		std::string key = it->key;
		std::string val = it->val;
		if ( key.length() < fw ) {
			key += std::string( fw - key.length(), '.' );
		}
		if ( key.length() > fw ) {
			key = key.substr( 0, fw );
		}
		key += ": ";
		val = prepend_lines( val, std::string( fw, ' ' ) + ": " );
		log << key << val << std::endl;
	}
}

static void probe_mod_file( commandlineflags & flags, const std::string & filename, std::uint64_t filesize, std::istream & data_stream, textout & log ) {

	log.writeout();

	std::vector<field> fields;

	if ( flags.filenames.size() > 1 ) {
		set_field( fields, "Playlist" ).ostream() << flags.playlist_index + 1 << "/" << flags.filenames.size();
		set_field( fields, "Prev/Next" ).ostream()
		    << "'"
		    << ( flags.playlist_index > 0 ? get_filename( flags.filenames[ flags.playlist_index - 1 ] ) : std::string() )
		    << "'"
		    << " / "
		    << "['" << get_filename( filename ) << "']"
		    << " / "
		    << "'"
		    << ( flags.playlist_index + 1 < flags.filenames.size() ? get_filename( flags.filenames[ flags.playlist_index + 1 ] ) : std::string() )
		    << "'"
		   ;
	}
	if ( flags.verbose ) {
		set_field( fields, "Path" ).ostream() << filename;
	}
	if ( flags.show_details ) {
		set_field( fields, "Filename" ).ostream() << get_filename( filename );
		set_field( fields, "Size" ).ostream() << bytes_to_string( filesize );
	}
	
	int probe_result = openmpt::probe_file_header( openmpt::probe_file_header_flags_default, data_stream );
	std::string probe_result_string;
	switch ( probe_result ) {
		case openmpt::probe_file_header_result_success:
			probe_result_string = "Success";
			break;
		case openmpt::probe_file_header_result_failure:
			probe_result_string = "Failure";
			break;
		case openmpt::probe_file_header_result_wantmoredata:
			probe_result_string = "Insufficient Data";
			break;
		default:
			probe_result_string = "Internal Error";
			break;
	}
	set_field( fields, "Probe" ).ostream() << probe_result_string;

	show_fields( log, fields );

	log.writeout();

}

template < typename Tmod >
void render_mod_file( commandlineflags & flags, const std::string & filename, std::uint64_t filesize, Tmod & mod, textout & log, write_buffers_interface & audio_stream ) {

	log.writeout();

	if ( flags.mode != ModeProbe && flags.mode != ModeInfo ) {
		mod.set_repeat_count( flags.repeatcount );
		apply_mod_settings( flags, mod );
	}
	
	double duration = mod.get_duration_seconds();

	std::vector<field> fields;

	if ( flags.filenames.size() > 1 ) {
		set_field( fields, "Playlist" ).ostream() << flags.playlist_index + 1 << "/" << flags.filenames.size();
		set_field( fields, "Prev/Next" ).ostream()
		    << "'"
		    << ( flags.playlist_index > 0 ? get_filename( flags.filenames[ flags.playlist_index - 1 ] ) : std::string() )
		    << "'"
		    << " / "
		    << "['" << get_filename( filename ) << "']"
		    << " / "
		    << "'"
		    << ( flags.playlist_index + 1 < flags.filenames.size() ? get_filename( flags.filenames[ flags.playlist_index + 1 ] ) : std::string() )
		    << "'"
		   ;
	}
	if ( flags.verbose ) {
		set_field( fields, "Path" ).ostream() << filename;
	}
	if ( flags.show_details ) {
		set_field( fields, "Filename" ).ostream() << get_filename( filename );
		set_field( fields, "Size" ).ostream() << bytes_to_string( filesize );
		if ( !mod.get_metadata( "warnings" ).empty() ) {
			set_field( fields, "Warnings" ).ostream() << mod.get_metadata( "warnings" );
		}
		if ( !mod.get_metadata( "container" ).empty() ) {
			set_field( fields, "Container" ).ostream() << mod.get_metadata( "container" ) << " (" << mod.get_metadata( "container_long" ) << ")";
		}
		set_field( fields, "Type" ).ostream() << mod.get_metadata( "type" ) << " (" << mod.get_metadata( "type_long" ) << ")";
		if ( ( mod.get_num_subsongs() > 1 ) && ( flags.subsong != -1 ) ) {
			set_field( fields, "Subsong" ).ostream() << flags.subsong;
		}
		set_field( fields, "Tracker" ).ostream() << mod.get_metadata( "tracker" );
		if ( !mod.get_metadata( "date" ).empty() ) {
			set_field( fields, "Date" ).ostream() << mod.get_metadata( "date" );
		}
		if ( !mod.get_metadata( "artist" ).empty() ) {
			set_field( fields, "Artist" ).ostream() << mod.get_metadata( "artist" );
		}
	}
	if ( true ) {
		set_field( fields, "Title" ).ostream() << mod.get_metadata( "title" );
		set_field( fields, "Duration" ).ostream() << seconds_to_string( duration );
	}
	if ( flags.show_details ) {
		set_field( fields, "Subsongs" ).ostream() << mod.get_num_subsongs();
		set_field( fields, "Channels" ).ostream() << mod.get_num_channels();
		set_field( fields, "Orders" ).ostream() << mod.get_num_orders();
		set_field( fields, "Patterns" ).ostream() << mod.get_num_patterns();
		set_field( fields, "Instruments" ).ostream() << mod.get_num_instruments();
		set_field( fields, "Samples" ).ostream() << mod.get_num_samples();
	}
	if ( flags.show_message ) {
		set_field( fields, "Message" ).ostream() << mod.get_metadata( "message" );
	}

	show_fields( log, fields );

	log.writeout();

	if ( flags.filenames.size() == 1 || flags.mode == ModeRender ) {
		audio_stream.write_metadata( get_metadata( mod ) );
	} else {
		audio_stream.write_updated_metadata( get_metadata( mod ) );
	}

	if ( flags.mode == ModeProbe || flags.mode == ModeInfo ) {
		return;
	}

	if ( flags.seek_target > 0.0 ) {
		mod.set_position_seconds( flags.seek_target );
	}

	try {
		if ( flags.use_float ) {
			render_loop<float>( flags, mod, duration, log, audio_stream );
		} else {
			render_loop<std::int16_t>( flags, mod, duration, log, audio_stream );
		}
		if ( flags.show_progress ) {
			log << std::endl;
		}
	} catch ( ... ) {
		if ( flags.show_progress ) {
			log << std::endl;
		}
		throw;
	}

	log.writeout();

}

static void probe_file( commandlineflags & flags, const std::string & filename, textout & log ) {

	log.writeout();

	std::ostringstream silentlog;

	try {

#if defined(WIN32) && defined(UNICODE) && !defined(_MSC_VER)
		std::istringstream file_stream;
#else
		std::ifstream file_stream;
#endif
		std::uint64_t filesize = 0;
		bool use_stdin = ( filename == "-" );
		if ( !use_stdin ) {
			#if defined(WIN32) && defined(UNICODE) && !defined(_MSC_VER)
				// Only MSVC has std::ifstream::ifstream(std::wstring).
				// Fake it for other compilers using _wfopen().
				std::string data;
				FILE * f = _wfopen( utf8_to_wstring( filename ).c_str(), L"rb" );
				if ( f ) {
					while ( !feof( f ) ) {
						static const std::size_t BUFFER_SIZE = 4096;
						char buffer[BUFFER_SIZE];
						size_t data_read = fread( buffer, 1, BUFFER_SIZE, f );
						std::copy( buffer, buffer + data_read, std::back_inserter( data ) );
					}
					fclose( f );
					f = NULL;
				}
				file_stream.str( data );
				filesize = data.length();
			#elif defined(_MSC_VER) && defined(UNICODE)
				file_stream.open( utf8_to_wstring( filename ), std::ios::binary );
				file_stream.seekg( 0, std::ios::end );
				filesize = file_stream.tellg();
				file_stream.seekg( 0, std::ios::beg );
			#else
				file_stream.open( filename, std::ios::binary );
				file_stream.seekg( 0, std::ios::end );
				filesize = file_stream.tellg();
				file_stream.seekg( 0, std::ios::beg );
			#endif
		}
		std::istream & data_stream = use_stdin ? std::cin : file_stream;
		if ( data_stream.fail() ) {
			throw exception( "file open error" );
		}
		
		probe_mod_file( flags, filename, filesize, data_stream, log );

	} catch ( silent_exit_exception & ) {
		throw;
	} catch ( std::exception & e ) {
		if ( !silentlog.str().empty() ) {
			log << "errors probing '" << filename << "': " << silentlog.str() << std::endl;
		} else {
			log << "errors probing '" << filename << "'" << std::endl;
		}
		log << "error probing '" << filename << "': " << e.what() << std::endl;
	} catch ( ... ) {
		if ( !silentlog.str().empty() ) {
			log << "errors probing '" << filename << "': " << silentlog.str() << std::endl;
		} else {
			log << "errors probing '" << filename << "'" << std::endl;
		}
		log << "unknown error probing '" << filename << "'" << std::endl;
	}

	log << std::endl;

	log.writeout();

}

static void render_file( commandlineflags & flags, const std::string & filename, textout & log, write_buffers_interface & audio_stream ) {

	log.writeout();

	std::ostringstream silentlog;

	try {

#if defined(WIN32) && defined(UNICODE) && !defined(_MSC_VER)
		std::istringstream file_stream;
#else
		std::ifstream file_stream;
#endif
		std::uint64_t filesize = 0;
		bool use_stdin = ( filename == "-" );
		if ( !use_stdin ) {
			#if defined(WIN32) && defined(UNICODE) && !defined(_MSC_VER)
				// Only MSVC has std::ifstream::ifstream(std::wstring).
				// Fake it for other compilers using _wfopen().
				std::string data;
				FILE * f = _wfopen( utf8_to_wstring( filename ).c_str(), L"rb" );
				if ( f ) {
					while ( !feof( f ) ) {
						static const std::size_t BUFFER_SIZE = 4096;
						char buffer[BUFFER_SIZE];
						size_t data_read = fread( buffer, 1, BUFFER_SIZE, f );
						std::copy( buffer, buffer + data_read, std::back_inserter( data ) );
					}
					fclose( f );
					f = NULL;
				}
				file_stream.str( data );
				filesize = data.length();
			#elif defined(_MSC_VER) && defined(UNICODE)
				file_stream.open( utf8_to_wstring( filename ), std::ios::binary );
				file_stream.seekg( 0, std::ios::end );
				filesize = file_stream.tellg();
				file_stream.seekg( 0, std::ios::beg );
			#else
				file_stream.open( filename, std::ios::binary );
				file_stream.seekg( 0, std::ios::end );
				filesize = file_stream.tellg();
				file_stream.seekg( 0, std::ios::beg );
			#endif
		}
		std::istream & data_stream = use_stdin ? std::cin : file_stream;
		if ( data_stream.fail() ) {
			throw exception( "file open error" );
		}

		{
			openmpt::module mod( data_stream, silentlog, flags.ctls );
			mod.select_subsong( flags.subsong );
			silentlog.str( std::string() ); // clear, loader messages get stored to get_metadata( "warnings" ) by libopenmpt internally
			render_mod_file( flags, filename, filesize, mod, log, audio_stream );
		}

	} catch ( prev_file & ) {
		throw;
	} catch ( next_file & ) {
		throw;
	} catch ( silent_exit_exception & ) {
		throw;
	} catch ( std::exception & e ) {
		if ( !silentlog.str().empty() ) {
			log << "errors loading '" << filename << "': " << silentlog.str() << std::endl;
		} else {
			log << "errors loading '" << filename << "'" << std::endl;
		}
		log << "error playing '" << filename << "': " << e.what() << std::endl;
	} catch ( ... ) {
		if ( !silentlog.str().empty() ) {
			log << "errors loading '" << filename << "': " << silentlog.str() << std::endl;
		} else {
			log << "errors loading '" << filename << "'" << std::endl;
		}
		log << "unknown error playing '" << filename << "'" << std::endl;
	}

	log << std::endl;

	log.writeout();

}


static std::string get_random_filename( std::set<std::string> & filenames, std::default_random_engine & prng ) {
	std::size_t index = std::uniform_int_distribution<std::size_t>( 0, filenames.size() - 1 )( prng );
	std::set<std::string>::iterator it = filenames.begin();
	std::advance( it, index );
	return *it;
}


static void render_files( commandlineflags & flags, textout & log, write_buffers_interface & audio_stream, std::default_random_engine & prng ) {
	if ( flags.randomize ) {
		std::shuffle( flags.filenames.begin(), flags.filenames.end(), prng );
	}
	try {
		while ( true ) {
			if ( flags.shuffle ) {
				// TODO: improve prev/next logic
				std::set<std::string> shuffle_set;
				shuffle_set.insert( flags.filenames.begin(), flags.filenames.end() );
				while ( true ) {
					if ( shuffle_set.empty() ) {
						break;
					}
					std::string filename = get_random_filename( shuffle_set, prng );
					try {
						flags.playlist_index = std::find( flags.filenames.begin(), flags.filenames.end(), filename ) - flags.filenames.begin();
						render_file( flags, filename, log, audio_stream );
						shuffle_set.erase( filename );
						continue;
					} catch ( prev_file & ) {
						shuffle_set.erase( filename );
						continue;
					} catch ( next_file & ) {
						shuffle_set.erase( filename );
						continue;
					} catch ( ... ) {
						throw;
					}
				}
			} else {
				std::vector<std::string>::iterator filename = flags.filenames.begin();
				while ( true ) {
					if ( filename == flags.filenames.end() ) {
						break;
					}
					try {
						flags.playlist_index = filename - flags.filenames.begin();
						render_file( flags, *filename, log, audio_stream );
						filename++;
						continue;
					} catch ( prev_file & e ) {
						while ( filename != flags.filenames.begin() && e.count ) {
							e.count--;
							--filename;
						}
						continue;
					} catch ( next_file & e ) {
						while ( filename != flags.filenames.end() && e.count ) {
							e.count--;
							++filename;
						}
						continue;
					} catch ( ... ) {
						throw;
					}
				}
			}
			if ( !flags.restart ) {
				break;
			}
		}
	} catch ( ... ) {
		throw;
	}
}


static bool parse_playlist( commandlineflags & flags, std::string filename, std::ostream & log ) {
	log.flush();
	bool is_playlist = false;
	bool m3u8 = false;
	if ( ends_with( filename, ".m3u") || ends_with( filename, ".m3U") || ends_with( filename, ".M3u") || ends_with( filename, ".M3U") ) {
		is_playlist = true;
	}
	if ( ends_with( filename, ".m3u8") || ends_with( filename, ".m3U8") || ends_with( filename, ".M3u8") || ends_with( filename, ".M3U8") ) {
		is_playlist = true;
		m3u8 = true;
	}
	if ( ends_with( filename, ".pls") || ends_with( filename, ".plS") || ends_with( filename, ".pLs") || ends_with( filename, ".pLS") || ends_with( filename, ".Pls")  || ends_with( filename, ".PlS")  || ends_with( filename, ".PLs")  || ends_with( filename, ".PLS") ) {
		is_playlist = true;
	}
	std::string basepath = get_basepath( filename );
	try {
#if defined(WIN32) && defined(UNICODE) && !defined(_MSC_VER)
		std::istringstream file_stream;
#else
		std::ifstream file_stream;
#endif
		#if defined(WIN32) && defined(UNICODE) && !defined(_MSC_VER)
			// Only MSVC has std::ifstream::ifstream(std::wstring).
			// Fake it for other compilers using _wfopen().
			std::string data;
			FILE * f = _wfopen( utf8_to_wstring( filename ).c_str(), L"rb" );
			if ( f ) {
				while ( !feof( f ) ) {
					static const std::size_t BUFFER_SIZE = 4096;
					char buffer[BUFFER_SIZE];
					size_t data_read = fread( buffer, 1, BUFFER_SIZE, f );
					std::copy( buffer, buffer + data_read, std::back_inserter( data ) );
				}
				fclose( f );
				f = NULL;
			}
			file_stream.str( data );
		#elif defined(_MSC_VER) && defined(UNICODE)
			file_stream.open( utf8_to_wstring( filename ), std::ios::binary );
		#else
			file_stream.open( filename, std::ios::binary );
		#endif
		std::string line;
		bool first = true;
		bool extm3u = false;
		bool pls = false;
		while ( std::getline( file_stream, line ) ) {
			std::string newfile;
			line = trim_eol( line );
			if ( first ) {
				first = false;
				if ( line == "#EXTM3U" ) {
					extm3u = true;
					continue;
				} else if ( line == "[playlist]" ) {
					pls = true;
				}
			}
			if ( line.empty() ) {
				continue;
			}
			if ( pls ) {
				if ( begins_with( line, "File" ) ) {
					if ( line.find( "=" ) != std::string::npos ) {
						flags.filenames.push_back( line.substr( line.find( "=" ) + 1 ) );
					}
				} else if ( begins_with( line, "Title" ) ) {
					continue;
				} else if ( begins_with( line, "Length" ) ) {
					continue;
				} else if ( begins_with( line, "NumberOfEntries" ) ) {
					continue;
				} else if ( begins_with( line, "Version" ) ) {
					continue;
				} else {
					continue;
				}
			} else if ( extm3u ) {
				if ( begins_with( line, "#EXTINF" ) ) {
					continue;
				} else if ( begins_with( line, "#" ) ) {
					continue;
				}
				if ( m3u8 ) {
					newfile = line;
				} else {
#if defined(WIN32)
					newfile = wstring_to_utf8( locale_to_wstring( line ) );
#else
					newfile = line;
#endif
				}
			} else {
				if ( m3u8 ) {
					newfile = line;
				} else {
#if defined(WIN32)
					newfile = wstring_to_utf8( locale_to_wstring( line ) );
#else
					newfile = line;
#endif
				}
			}
			if ( !newfile.empty() ) {
				if ( !is_absolute( newfile ) ) {
					newfile = basepath + newfile;
				}
				flags.filenames.push_back( newfile );
			}
		}
	} catch ( std::exception & e ) {
		log << "error loading '" << filename << "': " << e.what() << std::endl;
	} catch ( ... ) {
		log << "unknown error loading '" << filename << "'" << std::endl;
	}
	log.flush();
	return is_playlist;
}


static commandlineflags parse_openmpt123( const std::vector<std::string> & args, std::ostream & log ) {

	log.flush();

	if ( args.size() <= 1 ) {
		throw args_error_exception();
	}

	commandlineflags flags;

	bool files_only = false;
	for ( std::vector<std::string>::const_iterator i = args.begin(); i != args.end(); ++i ) {
		if ( i == args.begin() ) {
			// skip program name
			continue;
		}
		std::string arg = *i;
		std::string nextarg = ( i+1 != args.end() ) ? *(i+1) : "";
		if ( files_only ) {
			flags.filenames.push_back( arg );
		} else if ( arg.substr( 0, 1 ) != "-" ) {
			flags.filenames.push_back( arg );
		} else {
			if ( arg == "--" ) {
				files_only = true;
			} else if ( arg == "-h" || arg == "--help" ) {
				throw show_help_exception();
			} else if ( arg == "--help-keyboard" ) {
				throw show_help_keyboard_exception();
			} else if ( arg == "-q" || arg == "--quiet" ) {
				flags.quiet = true;
			} else if ( arg == "-v" || arg == "--verbose" ) {
				flags.verbose = true;
			} else if ( arg == "--man-version" ) {
				throw show_man_version_exception();
			} else if ( arg == "--man-help" ) {
				throw show_man_help_exception();
			} else if ( arg == "--version" ) {
				throw show_version_number_exception();
			} else if ( arg == "--short-version" ) {
				throw show_short_version_number_exception();
			} else if ( arg == "--long-version" ) {
				throw show_long_version_number_exception();
			} else if ( arg == "--credits" ) {
				throw show_credits_exception();
			} else if ( arg == "--license" ) {
				throw show_license_exception();
			} else if ( arg == "--probe" ) {
				flags.mode = ModeProbe;
			} else if ( arg == "--info" ) {
				flags.mode = ModeInfo;
			} else if ( arg == "--ui" ) {
				flags.mode = ModeUI;
			} else if ( arg == "--batch" ) {
				flags.mode = ModeBatch;
			} else if ( arg == "--render" ) {
				flags.mode = ModeRender;
			} else if ( arg == "--terminal-width" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.terminal_width;
				++i;
			} else if ( arg == "--terminal-height" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.terminal_height;
				++i;
			} else if ( arg == "--progress" ) {
				flags.show_progress = true;
			} else if ( arg == "--no-progress" ) {
				flags.show_progress = false;
			} else if ( arg == "--meters" ) {
				flags.show_meters = true;
			} else if ( arg == "--no-meters" ) {
				flags.show_meters = false;
			} else if ( arg == "--channel-meters" ) {
				flags.show_channel_meters = true;
			} else if ( arg == "--no-channel-meters" ) {
				flags.show_channel_meters = false;
			} else if ( arg == "--pattern" ) {
				flags.show_pattern = true;
			} else if ( arg == "--no-pattern" ) {
				flags.show_pattern = false;
			} else if ( arg == "--details" ) {
				flags.show_details = true;
			} else if ( arg == "--no-details" ) {
				flags.show_details = false;
			} else if ( arg == "--message" ) {
				flags.show_message = true;
			} else if ( arg == "--no-message" ) {
				flags.show_message = false;
			} else if ( arg == "--driver" && nextarg != "" ) {
				if ( false ) {
					// nothing
				} else if ( nextarg == "help" ) {
					std::ostringstream drivers;
					drivers << " Available drivers:" << std::endl;
					drivers << "    " << "default" << std::endl;
#if defined( MPT_WITH_PULSEAUDIO )
					drivers << "    " << "pulseaudio" << std::endl;
#endif
#if defined( MPT_WITH_SDL2 )
					drivers << "    " << "sdl2" << std::endl;
#endif
#if defined( MPT_WITH_SDL )
					drivers << "    " << "sdl" << std::endl;
#endif
#if defined( MPT_WITH_PORTAUDIO )
					drivers << "    " << "portaudio" << std::endl;
#endif
#if defined( WIN32 )
					drivers << "    " << "waveout" << std::endl;
#endif
					throw show_help_exception( drivers.str() );
				} else if ( nextarg == "default" ) {
					flags.driver = "";
				} else {
					flags.driver = nextarg;
				}
				++i;
			} else if ( arg == "--device" && nextarg != "" ) {
				if ( false ) {
					// nothing
				} else if ( nextarg == "help" ) {
					std::ostringstream devices;
					devices << " Available devices:" << std::endl;
					devices << "    " << "default" << ": " << "default" << std::endl;
#if defined( MPT_WITH_PULSEAUDIO )
					devices << show_pulseaudio_devices( log );
#endif
#if defined( MPT_WITH_SDL2 )
					devices << show_sdl2_devices( log );
#endif
#if defined( MPT_WITH_PORTAUDIO )
					devices << show_portaudio_devices( log );
#endif
#if defined( WIN32 )
					devices << show_waveout_devices( log );
#endif
					throw show_help_exception( devices.str() );
				} else if ( nextarg == "default" ) {
					flags.device = "";
				} else {
					flags.device = nextarg;
				}
				++i;
			} else if ( arg == "--buffer" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.buffer;
				++i;
			} else if ( arg == "--period" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.period;
				++i;
			} else if ( arg == "--update" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.ui_redraw_interval;
				++i;
			} else if ( arg == "--stdout" ) {
				flags.use_stdout = true;
			} else if ( ( arg == "-o" || arg == "--output" ) && nextarg != "" ) {
				flags.output_filename = nextarg;
				++i;
			} else if ( arg == "--force" ) {
				flags.force_overwrite = true;
			} else if ( arg == "--output-type" && nextarg != "" ) {
				flags.output_extension = nextarg;
				++i;
			} else if ( arg == "--samplerate" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.samplerate;
				++i;
			} else if ( arg == "--channels" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.channels;
				++i;
			} else if ( arg == "--float" ) {
				flags.use_float = true;
			} else if ( arg == "--no-float" ) {
				flags.use_float = false;
			} else if ( arg == "--gain" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				double gain = 0.0;
				istr >> gain;
				flags.gain = static_cast<std::int32_t>( gain * 100.0 );
				++i;
			} else if ( arg == "--stereo" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.separation;
				++i;
			} else if ( arg == "--filter" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.filtertaps;
				++i;
			} else if ( arg == "--ramping" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.ramping;
				++i;
			} else if ( arg == "--tempo" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				double tmp = 1.0;
				istr >> tmp;
				flags.tempo = double_to_tempo_flag( tmp );
				++i;
			} else if ( arg == "--pitch" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				double tmp = 1.0;
				istr >> tmp;
				flags.pitch = double_to_pitch_flag( tmp );
				++i;
			} else if ( arg == "--dither" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.dither;
				++i;
			} else if ( arg == "--playlist" && nextarg != "" ) {
				parse_playlist( flags, nextarg, log );
				++i;
			} else if ( arg == "--randomize" ) {
				flags.randomize = true;
			} else if ( arg == "--no-randomize" ) {
				flags.randomize = false;
			} else if ( arg == "--shuffle" ) {
				flags.shuffle = true;
			} else if ( arg == "--no-shuffle" ) {
				flags.shuffle = false;
			} else if ( arg == "--restart" ) {
				flags.restart = true;
			} else if ( arg == "--no-restart" ) {
				flags.restart = false;
			} else if ( arg == "--subsong" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.subsong;
				++i;
			} else if ( arg == "--repeat" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.repeatcount;
				++i;
			} else if ( arg == "--ctl" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				std::string ctl_c_v;
				istr >> ctl_c_v;
				if ( ctl_c_v.find( "=" ) == std::string::npos ) {
					throw args_error_exception();
				}
				std::string ctl = ctl_c_v.substr( 0, ctl_c_v.find( "=" ) );
				std::string val = ctl_c_v.substr( ctl_c_v.find( "=" ) + std::string("=").length(), std::string::npos );
				if ( ctl.empty() ) {
					throw args_error_exception();
				}
				flags.ctls[ ctl ] = val;
				++i;
			} else if ( arg == "--seek" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.seek_target;
				++i;
			} else if ( arg == "--end-time" && nextarg != "" ) {
				std::istringstream istr( nextarg );
				istr >> flags.end_time;
				++i;
			} else if ( arg.size() > 0 && arg.substr( 0, 1 ) == "-" ) {
				throw args_error_exception();
			}
		}
	}

	return flags;

}

#if defined(WIN32)

class ConsoleCP_utf8_raii {
private:
	const UINT oldCP;
	const UINT oldOutputCP;
public:
	ConsoleCP_utf8_raii()
		: oldCP(GetConsoleCP())
		, oldOutputCP(GetConsoleOutputCP())
	{
		SetConsoleCP( 65001 ); // UTF-8
		SetConsoleOutputCP( 65001 ); // UTF-8
	}
	~ConsoleCP_utf8_raii() {
		SetConsoleCP( oldCP );
		SetConsoleOutputCP( oldOutputCP );
	}
};

class FD_binary_raii {
private:
	FILE * file;
	int old_mode;
public:
	FD_binary_raii(FILE * file, bool set_binary)
		: file(file)
		, old_mode(-1)
	{
		if ( set_binary ) {
			fflush( file );
			old_mode = _setmode( _fileno( file ), _O_BINARY );
			if ( old_mode == -1 ) {
				throw exception( "failed to set binary mode on file descriptor" );
			}
		}
	}
	~FD_binary_raii()
	{
		if ( old_mode != -1 ) {
			fflush( file );
			old_mode = _setmode( _fileno( file ), old_mode );
		}
	}
};

#endif

#if defined(WIN32) && defined(UNICODE)
static int wmain( int wargc, wchar_t * wargv [] ) {
#else
static int main( int argc, char * argv [] ) {
#endif
	std::vector<std::string> args;
	#if defined(WIN32) && defined(UNICODE)
		for ( int arg = 0; arg < wargc; ++arg ) {
			args.push_back( wstring_to_utf8( wargv[arg] ) );
		}
	#else
		args = std::vector<std::string>( argv, argv + argc );
	#endif

#if defined(WIN32)
	ConsoleCP_utf8_raii console_cp;
#endif
	textout_dummy dummy_log;
#if defined(WIN32)
	textout_console std_out( GetStdHandle( STD_OUTPUT_HANDLE ) );
	textout_console std_err( GetStdHandle( STD_ERROR_HANDLE ) );
#else
	textout_ostream std_out( std::cout );
	textout_ostream std_err( std::clog );
#endif

	commandlineflags flags;

	try {

		flags = parse_openmpt123( args, std::cerr );

		flags.check_and_sanitize();

	} catch ( args_error_exception & ) {
		show_help( std_out );
		return 1;
	} catch ( show_man_help_exception & ) {
		show_help( std_out, false, true, true );
		return 0;
	} catch ( show_man_version_exception & ) {
		show_man_version( std_out );
		return 0;
	} catch ( show_help_exception & e ) {
		show_help( std_out, true, e.longhelp, false, e.message );
		if ( flags.verbose ) {
			show_credits( std_out );
		}
		return 0;
	} catch ( show_help_keyboard_exception & ) {
		show_help_keyboard( std_out );
		return 0;
	} catch ( show_long_version_number_exception & ) {
		show_long_version( std_out );
		return 0;
	} catch ( show_version_number_exception & ) {
		show_version( std_out );
		return 0;
	} catch ( show_short_version_number_exception & ) {
		show_short_version( std_out );
		return 0;
	} catch ( show_credits_exception & ) {
		show_credits( std_out );
		return 0;
	} catch ( show_license_exception & ) {
		show_license( std_out );
		return 0;
	} catch ( silent_exit_exception & ) {
		return 0;
	} catch ( std::exception & e ) {
		std_err << "error: " << e.what() << std::endl;
		std_err.writeout();
		return 1;
	} catch ( ... ) {
		std_err << "unknown error" << std::endl;
		std_err.writeout();
		return 1;
	}

	try {

		bool stdin_can_ui = true;
		for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
			if ( *filename == "-" ) {
				stdin_can_ui = false;
				break;
			}
		}

		bool stdout_can_ui = true;
		if ( flags.use_stdout ) {
			stdout_can_ui = false;
		}

		// set stdin binary
#if defined(WIN32)
		FD_binary_raii stdin_guard( stdin, !stdin_can_ui );
#endif

		// set stdout binary
#if defined(WIN32)
		FD_binary_raii stdout_guard( stdout, !stdout_can_ui );
#endif

		// setup terminal
		#if !defined(WIN32)
			if ( stdin_can_ui ) {
				if ( flags.mode == ModeUI ) {
					set_input_mode();
				}
			}
		#endif
		
		textout & log = flags.quiet ? *static_cast<textout*>( &dummy_log ) : *static_cast<textout*>( stdout_can_ui ? &std_out : &std_err );

		show_info( log, flags.verbose );
		
		if ( !flags.warnings.empty() ) {
			log << flags.warnings << std::endl;
		}

		if ( flags.verbose ) {
			log << flags;
		}

		log.writeout();

		std::random_device rd;
		std::seed_seq seq{ rd(), static_cast<unsigned int>( std::time( NULL ) ) };
		std::default_random_engine prng( seq );
		std::srand( std::uniform_int_distribution<unsigned int>()( prng ) );

		switch ( flags.mode ) {
			case ModeProbe: {
				for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
					probe_file( flags, *filename, log );
					flags.playlist_index++;
				}
			} break;
			case ModeInfo: {
				void_audio_stream dummy;
				render_files( flags, log, dummy, prng );
			} break;
			case ModeUI:
			case ModeBatch: {
				if ( flags.use_stdout ) {
					flags.apply_default_buffer_sizes();
					stdout_stream_raii stdout_audio_stream;
					render_files( flags, log, stdout_audio_stream, prng );
				} else if ( !flags.output_filename.empty() ) {
					flags.apply_default_buffer_sizes();
					file_audio_stream_raii file_audio_stream( flags, flags.output_filename, log );
					render_files( flags, log, file_audio_stream, prng );
#if defined( MPT_WITH_PULSEAUDIO )
				} else if ( flags.driver == "pulseaudio" || flags.driver.empty() ) {
					pulseaudio_stream_raii pulseaudio_stream( flags, log );
					render_files( flags, log, pulseaudio_stream, prng );
#endif
#if defined( MPT_WITH_SDL2 )
				} else if ( flags.driver == "sdl2" || flags.driver.empty() ) {
					sdl2_stream_raii sdl2_stream( flags, log );
					render_files( flags, log, sdl2_stream, prng );
#endif
#if defined( MPT_WITH_SDL )
				} else if ( flags.driver == "sdl" || flags.driver.empty() ) {
					sdl_stream_raii sdl_stream( flags, log );
					render_files( flags, log, sdl_stream, prng );
#endif
#if defined( MPT_WITH_PORTAUDIO )
				} else if ( flags.driver == "portaudio" || flags.driver.empty() ) {
					portaudio_stream_raii portaudio_stream( flags, log );
					render_files( flags, log, portaudio_stream, prng );
#endif
#if defined( WIN32 )
				} else if ( flags.driver == "waveout" || flags.driver.empty() ) {
					waveout_stream_raii waveout_stream( flags );
					render_files( flags, log, waveout_stream, prng );
#endif
				} else {
					if ( flags.driver.empty() ) {
						throw exception( "openmpt123 is compiled without any audio driver" );
					} else {
						throw exception( "audio driver '" + flags.driver + "' not found" );
					}
				}
			} break;
			case ModeRender: {
				for ( std::vector<std::string>::iterator filename = flags.filenames.begin(); filename != flags.filenames.end(); ++filename ) {
					flags.apply_default_buffer_sizes();
					file_audio_stream_raii file_audio_stream( flags, *filename + std::string(".") + flags.output_extension, log );
					render_file( flags, *filename, log, file_audio_stream );
					flags.playlist_index++;
				}
			} break;
			case ModeNone:
			break;
		}

	} catch ( args_error_exception & ) {
		show_help( std_out );
		return 1;
#ifdef MPT_WITH_PULSEAUDIO
	} catch ( pulseaudio_exception & e ) {
		std_err << "PulseAudio error: " << e.what() << std::endl;
		std_err.writeout();
		return 1;
#endif
#ifdef MPT_WITH_PORTAUDIO
	} catch ( portaudio_exception & e ) {
		std_err << "PortAudio error: " << e.what() << std::endl;
		std_err.writeout();
		return 1;
#endif
#ifdef MPT_WITH_SDL
	} catch ( sdl_exception & e ) {
		std_err << "SDL error: " << e.what() << std::endl;
		std_err.writeout();
		return 1;
#endif
#ifdef MPT_WITH_SDL2
	} catch ( sdl2_exception & e ) {
		std_err << "SDL2 error: " << e.what() << std::endl;
		std_err.writeout();
		return 1;
#endif
	} catch ( silent_exit_exception & ) {
		return 0;
	} catch ( std::exception & e ) {
		std_err << "error: " << e.what() << std::endl;
		std_err.writeout();
		return 1;
	} catch ( ... ) {
		std_err << "unknown error" << std::endl;
		std_err.writeout();
		return 1;
	}

	return 0;
}

} // namespace openmpt123

#if defined(WIN32) && defined(UNICODE)
#if defined(__GNUC__)
// mingw64 does only default to special C linkage for "main", but not for "wmain".
extern "C"
#endif
int wmain( int wargc, wchar_t * wargv [] ) {
	return openmpt123::wmain( wargc, wargv );
}
#else
int main( int argc, char * argv [] ) {
	return openmpt123::main( argc, argv );
}
#endif
