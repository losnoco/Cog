/*
 * openmpt123.cpp
 * --------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

static const char * const license =
"Copyright (c) 2004-2025, OpenMPT Project Developers and Contributors" "\n"
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
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"" "\n"
"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE" "\n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE" "\n"
"DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE" "\n"
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL" "\n"
"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR" "\n"
"SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER" "\n"
"CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY," "\n"
"OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE" "\n"
"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE." "\n"
;

#include "openmpt123_config.hpp"

#include "mpt/base/detect_compiler.hpp"

#if MPT_COMPILER_GCC

#ifdef MPT_COMPILER_SETTING_QUIRK_GCC_BROKEN_IPA
// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115049>.
#if MPT_GCC_BEFORE(9, 0, 0)
// Earlier GCC get confused about setting a global function attribute.
// We need to check for 9.0 instead of 9.1 because of
// <https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1028580>.
// It also gets confused when setting global optimization -O1,
// so we have no way of fixing GCC 8 or earlier.
//#pragma GCC optimize("O1")
#else
#pragma GCC optimize("no-ipa-ra")
#endif
#endif // MPT_COMPILER_SETTING_QUIRK_GCC_BROKEN_IPA

#endif // MPT_COMPILER_GCC

#include "mpt/base/detect_os.hpp"
#include "mpt/base/detect_quirks.hpp"

#if defined(MPT_LIBC_QUIRK_REQUIRES_SYS_TYPES_H)
#include <sys/types.h>
#endif

#include "mpt/base/algorithm.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/main/main.hpp"

#include "mpt/random/crand.hpp"
#include "mpt/random/default_engines.hpp"
#include "mpt/random/device.hpp"
#include "mpt/random/engine.hpp"
#include "mpt/random/seed.hpp"

#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if MPT_OS_WINDOWS
#include <mmsystem.h>
#include <mmreg.h>
#endif

#include <libopenmpt/libopenmpt.hpp>

#include "openmpt123.hpp"
#include "openmpt123_exception.hpp"
#include "openmpt123_stdio.hpp"
#include "openmpt123_terminal.hpp"

#include "openmpt123_flac.hpp"
#include "openmpt123_mmio.hpp"
#include "openmpt123_sndfile.hpp"
#include "openmpt123_raw.hpp"
#include "openmpt123_stdout.hpp"
#include "openmpt123_allegro42.hpp"
#include "openmpt123_portaudio.hpp"
#include "openmpt123_pulseaudio.hpp"
#include "openmpt123_sdl2.hpp"
#include "openmpt123_waveout.hpp"

namespace openmpt123 {

struct silent_exit_exception : public std::exception {
};

struct show_license_exception : public std::exception {
};

struct show_credits_exception : public std::exception {
};

struct show_man_version_exception : public std::exception {
};

struct show_man_help_exception : public std::exception {
};

struct show_short_version_number_exception : public std::exception {
};

struct show_version_number_exception : public std::exception {
};

struct show_long_version_number_exception : public std::exception {
};

constexpr auto libopenmpt_encoding = mpt::common_encoding::utf8;


class file_audio_stream : public file_audio_stream_base {
private:
	std::unique_ptr<file_audio_stream_base> impl;
public:
	static void show_versions([[maybe_unused]] concat_stream<mpt::ustring> & log ) {
#ifdef MPT_WITH_FLAC
		log << MPT_USTRING(" FLAC ") << mpt::transcode<mpt::ustring>( mpt::source_encoding, FLAC__VERSION_STRING ) << MPT_USTRING(", ") << mpt::transcode<mpt::ustring>( mpt::source_encoding, FLAC__VENDOR_STRING ) << MPT_USTRING(", API ") << FLAC_API_VERSION_CURRENT << MPT_USTRING(".") << FLAC_API_VERSION_REVISION << MPT_USTRING(".") << FLAC_API_VERSION_AGE << MPT_USTRING(" <https://xiph.org/flac/>") << lf;
#endif
#ifdef MPT_WITH_SNDFILE
		char sndfile_info[128];
		std::memset( sndfile_info, 0, sizeof( sndfile_info ) );
		sf_command( 0, SFC_GET_LIB_VERSION, sndfile_info, sizeof( sndfile_info ) );
		sndfile_info[127] = '\0';
		log << MPT_USTRING(" libsndfile ") << mpt::transcode<mpt::ustring>( sndfile_encoding, sndfile_info ) << MPT_USTRING(" <http://mega-nerd.com/libsndfile/>") << lf;
#endif
	}
public:
	file_audio_stream( const commandlineflags & flags, const mpt::native_path & filename, [[maybe_unused]] concat_stream<mpt::ustring> & log )
		: impl(nullptr)
	{
		if ( !flags.force_overwrite ) {
			mpt::IO::ifstream testfile( filename, std::ios::binary );
			if ( testfile ) {
				throw exception( MPT_USTRING("file already exists") );
			}
		}
		if ( false ) {
			// nothing
		} else if ( flags.output_extension == MPT_NATIVE_PATH("raw") ) {
			impl = std::make_unique<raw_stream_raii>( filename, flags, log );
#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT
		} else if ( flags.output_extension == MPT_NATIVE_PATH("wav") ) {
			impl = std::make_unique<mmio_stream_raii>( filename, flags, log );
#endif
#ifdef MPT_WITH_FLAC
		} else if ( flags.output_extension == MPT_NATIVE_PATH("flac") ) {
			impl = std::make_unique<flac_stream_raii>( filename, flags, log );
#endif
#ifdef MPT_WITH_SNDFILE
		} else {
			impl = std::make_unique<sndfile_stream_raii>( filename, flags, log );
#endif
		}
		if ( !impl ) {
			throw exception( MPT_USTRING("file format handler '") + mpt::transcode<mpt::ustring>( flags.output_extension ) + MPT_USTRING("' not found") );
		}
	}
	virtual ~file_audio_stream() {
		return;
	}
	void write_metadata( std::map<mpt::ustring, mpt::ustring> metadata ) override {
		impl->write_metadata( metadata );
	}
	void write_updated_metadata( std::map<mpt::ustring, mpt::ustring> metadata ) override {
		impl->write_updated_metadata( metadata );
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		impl->write( buffers, frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		impl->write( buffers, frames );
	}
};

class realtime_audio_stream : public write_buffers_interface {
private:
	std::unique_ptr<write_buffers_interface> impl;
public:
	static void show_versions( [[maybe_unused]] concat_stream<mpt::ustring> & log ) {
#ifdef MPT_WITH_SDL2
		log << MPT_USTRING(" ") << show_sdl2_version() << lf;
#endif
#ifdef MPT_WITH_PULSEAUDIO
		log << MPT_USTRING(" ") << show_pulseaudio_version() << lf;
#endif
#ifdef MPT_WITH_PORTAUDIO
		log << MPT_USTRING(" ") << show_portaudio_version() << lf;
#endif
	}
	static void show_drivers( concat_stream<mpt::ustring> & drivers ) {
		drivers << MPT_USTRING(" Available drivers:") << lf;
		drivers << MPT_USTRING("    default") << lf;
#if defined( MPT_WITH_PULSEAUDIO )
		drivers << MPT_USTRING("    pulseaudio") << lf;
#endif
#if defined( MPT_WITH_SDL2 )
		drivers << MPT_USTRING("    sdl2") << lf;
#endif
#if defined( MPT_WITH_PORTAUDIO )
		drivers << MPT_USTRING("    portaudio") << lf;
#endif
#if MPT_OS_WINDOWS
		drivers << MPT_USTRING("    waveout") << lf;
#endif
#if defined( MPT_WITH_ALLEGRO42 )
		drivers << MPT_USTRING("    allegro42") << lf;
#endif
	}
	static void show_devices( concat_stream<mpt::ustring> & devices, [[maybe_unused]] concat_stream<mpt::ustring> & log ) {
		devices << MPT_USTRING(" Available devices:") << lf;
		devices << MPT_USTRING("    default: default") << lf;
#if defined( MPT_WITH_PULSEAUDIO )
		devices << MPT_USTRING(" pulseaudio:") << lf;
		{
			auto devs = show_pulseaudio_devices( log );
			for ( const auto & dev : devs ) {
				devices << MPT_USTRING("    ") << dev << lf;
			}
		}
#endif
#if defined( MPT_WITH_SDL2 )
		devices << MPT_USTRING(" SDL2:") << lf;
		{
			auto devs = show_sdl2_devices( log );
			for ( const auto & dev : devs ) {
				devices << MPT_USTRING("    ") << dev << lf;
			}
		}
#endif
#if defined( MPT_WITH_PORTAUDIO )
		devices << MPT_USTRING(" portaudio:") << lf;
		{
			auto devs = show_portaudio_devices( log );
			for ( const auto & dev : devs ) {
				devices << MPT_USTRING("    ") << dev << lf;
			}
		}
#endif
#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT
		devices << MPT_USTRING(" waveout:") << lf;
		{
			auto devs = show_waveout_devices( log );
			for ( const auto & dev : devs ) {
				devices << MPT_USTRING("    ") << dev << lf;
			}
		}
#endif
#if defined( MPT_WITH_ALLEGRO42 )
		devices << MPT_USTRING(" allegro42:") << lf;
		{
			auto devs = show_allegro42_devices( log );
			for ( const auto & dev : devs ) {
				devices << MPT_USTRING("    ") << dev << lf;
			}
		}
#endif
	}
public:
	realtime_audio_stream( commandlineflags & flags, [[maybe_unused]] concat_stream<mpt::ustring> & log )
		: impl(nullptr)
	{
		if constexpr ( false ) {
			// nothing
#if defined( MPT_WITH_PULSEAUDIO )
		} else if ( flags.driver == MPT_USTRING("pulseaudio") || flags.driver.empty() ) {
			impl = std::make_unique<pulseaudio_stream_raii>( flags, log );
#endif
#if defined( MPT_WITH_SDL2 )
		} else if ( flags.driver == MPT_USTRING("sdl2") || flags.driver.empty() ) {
			impl = std::make_unique<sdl2_stream_raii>( flags, log );
#endif
#if defined( MPT_WITH_PORTAUDIO )
		} else if ( flags.driver == MPT_USTRING("portaudio") || flags.driver.empty() ) {
			impl = std::make_unique<portaudio_stream_raii>( flags, log );
#endif
#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT
		} else if ( flags.driver == MPT_USTRING("waveout") || flags.driver.empty() ) {
			impl = std::make_unique<waveout_stream_raii>( flags, log );
#endif
#if defined( MPT_WITH_ALLEGRO42 )
		} else if ( flags.driver == MPT_USTRING("allegro42") || flags.driver.empty() ) {
			impl = std::make_unique<allegro42_stream_raii>( flags, log );
#endif
		} else if ( flags.driver.empty() ) {
			throw exception(MPT_USTRING("openmpt123 is compiled without any audio driver"));
		} else {
			throw exception( MPT_USTRING("audio driver '") + flags.driver + MPT_USTRING("' not found") );
		}
	}
	virtual ~realtime_audio_stream() {
		return;
	}
	void write_metadata( std::map<mpt::ustring, mpt::ustring> metadata ) override {
		impl->write_metadata( metadata );
	}
	void write_updated_metadata( std::map<mpt::ustring, mpt::ustring> metadata ) override {
		impl->write_updated_metadata( metadata );
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		impl->write( buffers, frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		impl->write( buffers, frames );
	}
	bool unpause() override {
		return impl->unpause();
	}
	bool sleep( int ms ) override {
		return impl->sleep( ms );
	}
	bool is_dummy() const override {
		return impl->is_dummy();
	}
};

static mpt::ustring ctls_to_string( const std::map<std::string, std::string> & ctls ) {
	mpt::ustring result;
	for ( const auto & ctl : ctls ) {
		if ( !result.empty() ) {
			result += MPT_USTRING("; ");
		}
		result += mpt::transcode<mpt::ustring>( libopenmpt_encoding, ctl.first ) + MPT_USTRING("=") + mpt::transcode<mpt::ustring>( libopenmpt_encoding, ctl.second );
	}
	return result;
}

static double tempo_flag_to_double( std::int32_t tempo ) {
	return std::pow( 2.0, tempo / 24.0 );
}

static double pitch_flag_to_double( std::int32_t pitch ) {
	return std::pow( 2.0, pitch / 24.0 );
}

static std::int32_t double_to_tempo_flag( double factor ) {
	return static_cast<std::int32_t>( mpt::round( std::log( factor ) / std::log( 2.0 ) * 24.0 ) );
}

static std::int32_t double_to_pitch_flag( double factor ) {
	return static_cast<std::int32_t>( mpt::round( std::log( factor ) / std::log( 2.0 ) * 24.0 ) );
}

static concat_stream<mpt::ustring> & operator << ( concat_stream<mpt::ustring> & s, const commandlineflags & flags ) {
	s << MPT_USTRING("Quiet: ") << flags.quiet << lf;
	s << MPT_USTRING("Banner: ") << flags.banner << lf;
	s << MPT_USTRING("Verbose: ") << flags.verbose << lf;
	s << MPT_USTRING("Mode : ") << mode_to_string( flags.mode ) << lf;
	s << MPT_USTRING("Terminal size : ") << flags.terminal_width << MPT_USTRING("*") << flags.terminal_height << lf;
	s << MPT_USTRING("Show progress: ") << flags.show_progress << lf;
	s << MPT_USTRING("Show peak meters: ") << flags.show_meters << lf;
	s << MPT_USTRING("Show channel peak meters: ") << flags.show_channel_meters << lf;
	s << MPT_USTRING("Show details: ") << flags.show_details << lf;
	s << MPT_USTRING("Show message: ") << flags.show_message << lf;
	s << MPT_USTRING("Update: ") << flags.ui_redraw_interval << MPT_USTRING("ms") << lf;
	s << MPT_USTRING("Device: ") << flags.device << lf;
	s << MPT_USTRING("Buffer: ") << flags.buffer << MPT_USTRING("ms") << lf;
	s << MPT_USTRING("Period: ") << flags.period << MPT_USTRING("ms") << lf;
	s << MPT_USTRING("Samplerate: ") << flags.samplerate << lf;
	s << MPT_USTRING("Channels: ") << flags.channels << lf;
	s << MPT_USTRING("Float: ") << flags.use_float << lf;
	s << MPT_USTRING("Gain: ") << flags.gain / 100.0 << lf;
	s << MPT_USTRING("Stereo separation: ") << flags.separation << lf;
	s << MPT_USTRING("Interpolation filter taps: ") << flags.filtertaps << lf;
	s << MPT_USTRING("Volume ramping strength: ") << flags.ramping << lf;
	s << MPT_USTRING("Tempo: ") << tempo_flag_to_double( flags.tempo ) << lf;
	s << MPT_USTRING("Pitch: ") << pitch_flag_to_double( flags.pitch ) << lf;
	s << MPT_USTRING("Output dithering: ") << flags.dither << lf;
	s << MPT_USTRING("Repeat count: ") << flags.repeatcount << lf;
	s << MPT_USTRING("Seek target: ") << flags.seek_target << lf;
	s << MPT_USTRING("End time: ") << flags.end_time << lf;
	s << MPT_USTRING("Standard output: ") << flags.use_stdout << lf;
	s << MPT_USTRING("Output filename: ") << mpt::transcode<mpt::ustring>( flags.output_filename ) << lf;
	s << MPT_USTRING("Force overwrite output file: ") << flags.force_overwrite << lf;
	s << MPT_USTRING("Ctls: ") << ctls_to_string( flags.ctls ) << lf;
	s << lf;
	s << MPT_USTRING("Files: ") << lf;
	for ( const auto & filename : flags.filenames ) {
		s << MPT_USTRING(" ") << mpt::transcode<mpt::ustring>( filename ) << lf;
	}
	s << lf;
	return s;
}

static std::string trim_eol( const std::string & str ) {
	return mpt::trim( str, std::string("\r\n") );
}

static mpt::native_path get_basepath( mpt::native_path filename ) {
	return (filename.GetPrefix() + filename.GetDirectoryWithDrive()).WithTrailingSlash();
}

static bool is_absolute( mpt::native_path filename ) {
	return filename.IsAbsolute();
}

static mpt::native_path get_filename( const mpt::native_path & filepath ) {
	return filepath.GetFilename();
}

static mpt::ustring prepend_lines( mpt::ustring str, const mpt::ustring & prefix ) {
	if ( str.empty() ) {
		return str;
	}
	if ( str.substr( str.length() - 1, 1 ) == MPT_USTRING("\n") ) {
		str = str.substr( 0, str.length() - 1 );
	}
	return mpt::replace( str, MPT_USTRING("\n"), MPT_USTRING("\n") + prefix );
}

static mpt::ustring bytes_to_string( std::uint64_t bytes ) {
	static const mpt::uchar * const suffixes[] = { MPT_ULITERAL("B"), MPT_ULITERAL("kB"), MPT_ULITERAL("MB"), MPT_ULITERAL("GB"), MPT_ULITERAL("TB"), MPT_ULITERAL("PB") };
	int offset = 0;
	while ( bytes > 9999 ) {
		bytes /= 1000;
		offset += 1;
		if ( offset == 5 ) {
			break;
		}
	}
	return mpt::format<mpt::ustring>::val( bytes ) + suffixes[offset];
}

static mpt::ustring seconds_to_string( double time ) {
	std::int64_t time_ms = static_cast<std::int64_t>( time * 1000 );
	std::int64_t milliseconds = time_ms % 1000;
	std::int64_t seconds = ( time_ms / 1000 ) % 60;
	std::int64_t minutes = ( time_ms / ( 1000 * 60 ) ) % 60;
	std::int64_t hours = ( time_ms / ( 1000 * 60 * 60 ) );
	mpt::ustring str;
	if ( hours > 0 ) {
		str += mpt::format<mpt::ustring>::val( hours ) + MPT_USTRING(":");
	}
	str += mpt::format<mpt::ustring>::dec0<2>( minutes );
	str += MPT_USTRING(":");
	str += mpt::format<mpt::ustring>::dec0<2>( seconds );
	str += MPT_USTRING(".");
	str += mpt::format<mpt::ustring>::dec0<3>( milliseconds );
	return str;
}

static void show_banner( concat_stream<mpt::ustring> & log, verbosity banner ) {
	if ( banner == verbosity_hidden ) {
		return;
	}
	if ( banner == verbosity_shortversion ) {
		log << mpt::transcode<mpt::ustring>( mpt::source_encoding, OPENMPT123_VERSION_STRING ) << MPT_USTRING(" / ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "library_version" ) ) << MPT_USTRING(" / ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "core_version" ) ) << lf;
		return;
	}
	log << MPT_USTRING("openmpt123") << MPT_USTRING(" v") << mpt::transcode<mpt::ustring>( mpt::source_encoding, OPENMPT123_VERSION_STRING ) << MPT_USTRING(", libopenmpt ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "library_version" ) ) << MPT_USTRING(" (") << MPT_USTRING("OpenMPT ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "core_version" ) ) << MPT_USTRING(")") << lf;
	log << MPT_USTRING("Copyright (c) 2013-2025 OpenMPT Project Developers and Contributors <https://lib.openmpt.org/>") << lf;
	if ( banner == verbosity_normal ) {
		log << lf;
		return;
	}
	log << MPT_USTRING("  libopenmpt source..: ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "source_url" ) ) << lf;
	log << MPT_USTRING("  libopenmpt date....: ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "source_date" ) ) << lf;
	log << MPT_USTRING("  libopenmpt srcinfo.: ");
	{
		std::vector<mpt::ustring> fields;
		if ( openmpt::string::get( "source_is_package" ) == "1" ) {
			fields.push_back( MPT_USTRING("package") );
		}
		if ( openmpt::string::get( "source_is_release" ) == "1" ) {
			fields.push_back( MPT_USTRING("release") );
		}
		if ( ( !openmpt::string::get( "source_revision" ).empty() ) && ( openmpt::string::get( "source_revision" ) != "0" ) ) {
			mpt::ustring field = MPT_USTRING("rev") + mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "source_revision" ) );
			if ( openmpt::string::get( "source_has_mixed_revisions" ) == "1" ) {
				field += MPT_USTRING("+mixed");
			}
			if ( openmpt::string::get( "source_is_modified" ) == "1" ) {
				field += MPT_USTRING("+modified");
			}
			fields.push_back( field );
		}
		bool first = true;
		for ( const auto & field : fields ) {
			if ( first ) {
				first = false;
			} else {
				log << MPT_USTRING(", ");
			}
			log << field;
		}
	}
	log << lf;
	log << MPT_USTRING("  libopenmpt compiler: ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "build_compiler" ) ) << lf;
	log << MPT_USTRING("  libopenmpt features: ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "library_features" ) ) << lf;
	realtime_audio_stream::show_versions( log );
	file_audio_stream::show_versions( log );
	log << lf;
}

static void show_man_version( textout & log ) {
	log << MPT_USTRING("openmpt123") << MPT_USTRING(" v") << mpt::transcode<mpt::ustring>( mpt::source_encoding, OPENMPT123_VERSION_STRING ) << lf;
	log << lf;
	log << MPT_USTRING("Copyright (c) 2013-2025 OpenMPT Project Developers and Contributors <https://lib.openmpt.org/>") << lf;
}

static void show_short_version( textout & log ) {
	show_banner( log, verbosity_shortversion );
	log.writeout();
}

static void show_version( textout & log ) {
	show_banner( log, verbosity_normal );
	log.writeout();
}

static void show_long_version( textout & log ) {
	show_banner( log, verbosity_verbose );
	log.writeout();
}

static void show_credits( textout & log, verbosity banner ) {
	show_banner( log, banner );
	log << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "contact" ) ) << lf;
	log << lf;
	log << mpt::transcode<mpt::ustring>( libopenmpt_encoding, openmpt::string::get( "credits" ) ) << lf;
	log.writeout();
}

static void show_license( textout & log, verbosity banner ) {
	show_banner( log, banner );
	log << mpt::transcode<mpt::ustring>( mpt::source_encoding, license ) << lf;
	log.writeout();
}

static mpt::ustring get_driver_string( const mpt::ustring & driver ) {
	if ( driver.empty() ) {
		return MPT_USTRING("default");
	}
	return driver;
}

static mpt::ustring get_device_string( const mpt::ustring & device ) {
	if ( device.empty() ) {
		return MPT_USTRING("default");
	}
	return device;
}

static void show_help_keyboard( textout & log, bool man_version = false ) {
	log << MPT_USTRING("Keyboard hotkeys (use 'openmpt123 --ui'):") << lf;
	log << lf;
	log << MPT_USTRING(" [q]      quit") << lf;
	log << MPT_USTRING(" [ ]      pause / unpause") << lf;
	log << MPT_USTRING(" [N]      skip 10 files backward") << lf;
	log << MPT_USTRING(" [n]      prev file") << lf;
	log << MPT_USTRING(" [m]      next file") << lf;
	log << MPT_USTRING(" [M]      skip 10 files forward") << lf;
	log << MPT_USTRING(" [h]      seek 10 seconds backward") << lf;
	log << MPT_USTRING(" [j]      seek 1 seconds backward") << lf;
	log << MPT_USTRING(" [k]      seek 1 seconds forward") << lf;
	log << MPT_USTRING(" [l]      seek 10 seconds forward") << lf;
	log << MPT_USTRING(" [u]|[i]  +/- tempo") << lf;
	log << MPT_USTRING(" [o]|[p]  +/- pitch") << lf;
	log << MPT_USTRING(" [3]|[4]  +/- gain") << lf;
	log << MPT_USTRING(" [5]|[6]  +/- stereo separation") << lf;
	log << MPT_USTRING(" [7]|[8]  +/- filter taps") << lf;
	log << MPT_USTRING(" [9]|[0]  +/- volume ramping") << lf;
	log << lf;
	if ( !man_version ) {
		log.writeout();
	}
}

static void show_help( textout & log, bool longhelp = false, bool man_version = false, const mpt::ustring & message = mpt::ustring() ) {
	{
		log << MPT_USTRING("Usage: openmpt123 [options] [--] file1 [file2] ...") << lf;
		log << lf;
		if ( man_version ) {
			log << MPT_USTRING("openmpt123 plays module music files.") << lf;
			log << lf;
		}
		if ( man_version ) {
			log << MPT_USTRING("Options:") << lf;
			log << lf;
		}
		log << MPT_USTRING(" -h, --help                 Show help") << lf;
		log << MPT_USTRING("     --help-keyboard        Show keyboard hotkeys in ui mode") << lf;
		log << MPT_USTRING(" -q, --quiet                Suppress non-error screen output") << lf;
		log << MPT_USTRING(" -v, --verbose              Show more screen output") << lf;
		log << MPT_USTRING("     --version              Show version information and exit") << lf;
		log << MPT_USTRING("     --short-version        Show version number and nothing else") << lf;
		log << MPT_USTRING("     --long-version         Show long version information and exit") << lf;
		log << MPT_USTRING("     --credits              Show elaborate contributors list") << lf;
		log << MPT_USTRING("     --license              Show license") << lf;
		log << lf;
		log << MPT_USTRING("     --probe                Probe each file whether it is a supported file format") << lf;
		log << MPT_USTRING("     --info                 Display information about each file") << lf;
		log << MPT_USTRING("     --ui                   Interactively play each file") << lf;
		log << MPT_USTRING("     --batch                Play each file") << lf;
		log << MPT_USTRING("     --render               Render each file to individual PCM data files") << lf;
		if ( !longhelp ) {
			log << lf;
			log.writeout();
			return;
		}
		log << lf;
		log << MPT_USTRING("     --banner n             openmpt123 banner style [0=hide,1=show,2=verbose] [default: ") << commandlineflags().banner << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --assume-terminal      Skip checking whether stdin/stderr are a terminal, and always allow UI [default: ") << commandlineflags().assume_terminal << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --terminal-width n     Assume terminal is n characters wide [default: ") << commandlineflags().terminal_width << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --terminal-height n    Assume terminal is n characters high [default: ") << commandlineflags().terminal_height << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --[no-]progress        Show playback progress [default: ") << commandlineflags().show_progress << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --[no-]meters          Show peak meters [default: ") << commandlineflags().show_meters << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --[no-]channel-meters  Show channel peak meters (EXPERIMENTAL) [default: ") << commandlineflags().show_channel_meters << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --[no-]pattern         Show pattern (EXPERIMENTAL) [default: ") << commandlineflags().show_pattern << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --[no-]details         Show song details [default: ") << commandlineflags().show_details << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --[no-]message         Show song message [default: ") << commandlineflags().show_message << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --update n             Set output update interval to n ms [default: ") << commandlineflags().ui_redraw_interval << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --samplerate n         Set samplerate to n Hz [default: ") << commandlineflags().samplerate << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --channels n           use n [1,2,4] output channels [default: ") << commandlineflags().channels << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --[no-]float           Output 32bit floating point instead of 16bit integer [default: ") << commandlineflags().use_float << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --gain n               Set output gain to n dB [default: ") << commandlineflags().gain / 100.0 << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --stereo n             Set stereo separation to n % [default: ") << commandlineflags().separation << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --filter n             Set interpolation filter taps to n [1,2,4,8] [default: ") << commandlineflags().filtertaps << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --ramping n            Set volume ramping strength n [0..5] [default: ") << commandlineflags().ramping << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --tempo f              Set tempo factor f [default: ") << tempo_flag_to_double( commandlineflags().tempo ) << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --pitch f              Set pitch factor f [default: ") << pitch_flag_to_double( commandlineflags().pitch ) << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --dither n             Dither type to use (if applicable for selected output format): [0=off,1=auto,2=0.5bit,3=1bit] [default: ") << commandlineflags().dither << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --playlist file        Load playlist from file") << lf;
		log << MPT_USTRING("     --[no-]randomize       Randomize playlist [default: ") << commandlineflags().randomize << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --[no-]shuffle         Shuffle through playlist [default: ") << commandlineflags().shuffle << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --[no-]restart         Restart playlist when finished [default: ") << commandlineflags().restart << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --subsong n            Select subsong n (-1 means play all subsongs consecutively) [default: ") << commandlineflags().subsong << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --repeat n             Repeat song n times (-1 means forever) [default: ") << commandlineflags().repeatcount << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --seek n               Seek to n seconds on start [default: ") << commandlineflags().seek_target << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --end-time n           Play until position is n seconds (0 means until the end) [default: ") << commandlineflags().end_time << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --ctl c=v              Set libopenmpt ctl c to value v") << lf;
		log << lf;
		log << MPT_USTRING("     --driver n             Set output driver [default: ") << get_driver_string( commandlineflags().driver ) << MPT_USTRING("],") << lf;
		log << MPT_USTRING("     --device n             Set output device [default: ") << get_device_string( commandlineflags().device ) << MPT_USTRING("],") << lf;
		log << MPT_USTRING("                            use --device help to show available devices") << lf;
		log << MPT_USTRING("     --buffer n             Set output buffer size to n ms [default: ") << commandlineflags().buffer << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --period n             Set output period size to n ms [default: ") << commandlineflags().period  << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --stdout               Write raw audio data to stdout [default: ") << commandlineflags().use_stdout << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --output-type t        Use output format t when writing to a individual PCM files (only applies to --render mode) [default: ") << mpt::transcode<mpt::ustring>( commandlineflags().output_extension ) << MPT_USTRING("]") << lf;
		log << MPT_USTRING(" -o, --output f             Write PCM output to file f instead of streaming to audio device (only applies to --ui and --batch modes) [default: ") << mpt::transcode<mpt::ustring>( commandlineflags().output_filename ) << MPT_USTRING("]") << lf;
		log << MPT_USTRING("     --force                Force overwriting of output file [default: ") << commandlineflags().force_overwrite << MPT_USTRING("]") << lf;
		log << lf;
		log << MPT_USTRING("     --                     Interpret further arguments as filenames") << lf;
		log << lf;
		if ( !man_version ) {
			log << MPT_USTRING(" Supported file formats: ") << lf;
			log << MPT_USTRING("    ");
			std::vector<std::string> extensions = openmpt::get_supported_extensions();
			bool first = true;
			for ( const auto & extension : extensions ) {
				if ( first ) {
					first = false;
				} else {
					log << MPT_USTRING(", ");
				}
				log << mpt::transcode<mpt::ustring>( libopenmpt_encoding, extension );
			}
			log << lf;
		} else {
			show_help_keyboard( log, true );
		}
	}

	log << lf;

	if ( message.size() > 0 ) {
		log << message;
		log << lf;
	}
	log.writeout();
}


template < typename Tmod >
static void apply_mod_settings( commandlineflags & flags, Tmod & mod ) {
	flags.separation = std::max( flags.separation, std::int32_t(   0 ) );
	flags.filtertaps = std::max( flags.filtertaps, std::int32_t(   1 ) );
	flags.filtertaps = std::min( flags.filtertaps, std::int32_t(   8 ) );
	flags.ramping    = std::max( flags.ramping,    std::int32_t(  -1 ) );
	flags.ramping    = std::min( flags.ramping,    std::int32_t(  10 ) );
	flags.tempo      = std::max( flags.tempo,      std::int32_t( -48 ) );
	flags.tempo      = std::min( flags.tempo,      std::int32_t(  48 ) );
	flags.pitch      = std::max( flags.pitch,      std::int32_t( -48 ) );
	flags.pitch      = std::min( flags.pitch,      std::int32_t(  48 ) );
	mod.set_render_param( openmpt::module::RENDER_MASTERGAIN_MILLIBEL, flags.gain );
	mod.set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, flags.separation );
	mod.set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, flags.filtertaps );
	mod.set_render_param( openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, flags.ramping );
	try {
		mod.ctl_set_floatingpoint( "play.tempo_factor", tempo_flag_to_double( flags.tempo ) );
	} catch ( const openmpt::exception & ) {
		// ignore
	}
	try {
		mod.ctl_set_floatingpoint( "play.pitch_factor", pitch_flag_to_double( flags.pitch ) );
	} catch ( const openmpt::exception & ) {
		// ignore
	}
	mod.ctl_set_integer( "dither", flags.dither );
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
	float falloff_factor = std::pow( 10.0f, -falloff_rate / static_cast<float>( flags.samplerate ) / 20.0f );
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
	float falloff_factor = std::pow( 10.0f, -falloff_rate / static_cast<float>( flags.samplerate ) / 20.0f );
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

static const mpt::uchar * const channel_tags[4][4] = {
	{ MPT_ULITERAL(" C"), MPT_ULITERAL("  "), MPT_ULITERAL("  "), MPT_ULITERAL("  ") },
	{ MPT_ULITERAL(" L"), MPT_ULITERAL(" R"), MPT_ULITERAL("  "), MPT_ULITERAL("  ") },
	{ MPT_ULITERAL("FL"), MPT_ULITERAL("FR"), MPT_ULITERAL("RC"), MPT_ULITERAL("  ") },
	{ MPT_ULITERAL("FL"), MPT_ULITERAL("FR"), MPT_ULITERAL("RL"), MPT_ULITERAL("RR") },
};

static mpt::ustring channel_to_string( int channels, int channel, const meter_channel & meter, bool tiny = false ) {
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
			return MPT_USTRING("#");
		} else if ( meter.peak > std::pow( 10.0f, -6.0f / 20.0f ) ) {
			return MPT_USTRING("O");
		} else if ( meter.peak > std::pow( 10.0f, -12.0f / 20.0f ) ) {
			return MPT_USTRING("o");
		} else if ( meter.peak > std::pow( 10.0f, -18.0f / 20.0f ) ) {
			return MPT_USTRING(".");
		} else {
			return MPT_USTRING(" ");
		}
	} else {
		mpt::ustring res1;
		mpt::ustring res2;
		res1 += MPT_USTRING("        ");
		res1 += channel_tags[channels-1][channel];
		res1 += MPT_USTRING(" : ");
		res2 += mpt::ustring( val, MPT_UCHAR('>') ) + mpt::ustring( std::size_t{48} - val, MPT_UCHAR(' ') );
		res2 += ( ( meter.clip != 0.0f ) ? MPT_USTRING("#") : MPT_USTRING(":") );
		res2 += mpt::ustring( headroom, MPT_UCHAR('>') ) + mpt::ustring( std::size_t{12} - headroom, MPT_UCHAR(' ') );
		mpt::ustring tmp = res2;
		if ( 0 <= hold_pos && hold_pos <= 60 ) {
			if ( hold_pos == 48 ) {
				tmp[hold_pos] = MPT_UCHAR('#');
			} else {
				tmp[hold_pos] = MPT_UCHAR(':');
			}
		}
		return res1 + tmp;
	}
}

static mpt::ustring peak_to_string( float peak ) {
	if ( peak >= 1.0f ) {
		return MPT_USTRING("#");
	} else if ( peak >= 0.5f ) {
		return MPT_USTRING("O");
	} else if ( peak >= 0.25f ) {
		return MPT_USTRING("o");
	} else if ( peak >= 0.125f ) {
		return MPT_USTRING(".");
	} else {
		return MPT_USTRING(" ");
	}
}

static mpt::ustring peak_to_string_left( float peak, int width ) {
	mpt::ustring result;
	float thresh = 1.0f;
	while ( width-- ) {
		if ( peak >= thresh ) {
			if ( thresh == 1.0f ) {
				result.push_back( MPT_UCHAR('#') );
			} else {
				result.push_back( MPT_UCHAR('<') );
			}
		} else {
			result.push_back( MPT_UCHAR(' ') );
		}
		thresh *= 0.5f;
	}
	return result;
}

static mpt::ustring peak_to_string_right( float peak, int width ) {
	mpt::ustring result;
	float thresh = 1.0f;
	while ( width-- ) {
		if ( peak >= thresh ) {
			if ( thresh == 1.0f ) {
				result.push_back( MPT_UCHAR('#') );
			} else {
				result.push_back( MPT_UCHAR('>') );
			}
		} else {
			result.push_back( MPT_UCHAR(' ') );
		}
		thresh *= 0.5f;
	}
	std::reverse( result.begin(), result.end() );
	return result;
}

static void draw_meters( concat_stream<mpt::ustring> & log, const meter_type & meter, const commandlineflags & flags ) {
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		log << channel_to_string( flags.channels, channel, meter.channels[channel] ) << lf;
	}
}

static void draw_meters_tiny( concat_stream<mpt::ustring> & log, const meter_type & meter, const commandlineflags & flags ) {
	for ( int channel = 0; channel < flags.channels; ++channel ) {
		log << channel_to_string( flags.channels, channel, meter.channels[channel], true );
	}
}

static void draw_channel_meters_tiny( concat_stream<mpt::ustring> & log, float peak ) {
	log << peak_to_string( peak );
}

static void draw_channel_meters_tiny( concat_stream<mpt::ustring> & log, float peak_left, float peak_right ) {
	log << peak_to_string( peak_left ) << peak_to_string( peak_right );
}

static void draw_channel_meters( concat_stream<mpt::ustring> & log, float peak_left, float peak_right, int width ) {
	if ( width >= 8 + 1 + 8 ) {
		width = 8 + 1 + 8;
	}
	log << peak_to_string_left( peak_left, width / 2 ) << ( width % 2 == 1 ? MPT_USTRING(":") : MPT_USTRING("") ) << peak_to_string_right( peak_right, width / 2 );
}

template < typename Tsample, typename Tmod >
void render_loop( commandlineflags & flags, Tmod & mod, double & duration, textout & log, write_buffers_interface & audio_stream ) {

	log.writeout();

	std::size_t bufsize;
	if ( flags.mode == Mode::UI ) {
		bufsize = std::min( flags.ui_redraw_interval, flags.period ) * flags.samplerate / 1000;
	} else if ( flags.mode == Mode::Batch ) {
		bufsize = flags.period * flags.samplerate / 1000;
	} else {
		bufsize = 1024;
	}

	std::int64_t last_redraw_frame = std::int64_t{0} - flags.ui_redraw_interval;
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

	const bool narrow = (flags.terminal_width < 72) && (flags.terminal_height > 25);

	int lines = 0;

	int pattern_lines = 0;
	
	if ( multiline ) {
		lines += 1;
		// cppcheck-suppress identicalInnerCondition
		if ( flags.show_ui ) {
			lines += 1;
			if ( narrow ) {
				lines += 1;
			}
		}
		if ( flags.show_meters ) {
			if ( narrow ) {
				lines += 1;
			} else {
				for ( int channel = 0; channel < flags.channels; ++channel ) {
					lines += 1;
				}
			}
		}
		if ( flags.show_channel_meters ) {
			lines += 1;
		}
		if ( flags.show_details ) {
			lines += 1;
			if ( flags.show_progress ) {
				lines += 1;
				if ( narrow ) {
					lines += 2;
				}
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
		log << lf;
	}
	for ( int line = 0; line < lines; ++line ) {
		log << lf;
	}

	log.writeout();

	double cpu_smooth = 0.0;

	while ( true ) {

		if ( flags.mode == Mode::UI ) {

			while ( terminal_input::is_input_available() ) {
				auto c = terminal_input::read_input_char();
				if ( !c ) {
					break;
				}
				if ( !handle_keypress( *c, flags, mod, audio_stream ) ) {
					return;
				}
			}

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
		
		mpt::ustring cpu_str;
		if ( flags.show_details ) {
			cpu_end = std::clock();
			if ( count > 0 ) {
				double cpu = 1.0;
				cpu *= ( static_cast<double>( cpu_end ) - static_cast<double>( cpu_beg ) ) / static_cast<double>( CLOCKS_PER_SEC );
				cpu /= ( static_cast<double>( count ) ) / static_cast<double>( flags.samplerate );
				double mix = ( static_cast<double>( count ) ) / static_cast<double>( flags.samplerate );
				cpu_smooth = ( 1.0 - mix ) * cpu_smooth + mix * cpu;
				cpu_str = mpt::format<mpt::ustring>::fix( cpu_smooth * 100.0, 2 ) + MPT_USTRING("%");
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
			log.cursor_up( lines );
			log << lf;
			if ( flags.show_meters ) {
				if ( narrow ) {
					log << MPT_USTRING("Level......: ");
					draw_meters_tiny( log, meter, flags );
					log << lf;
				} else {
					draw_meters( log, meter, flags );
				}
			}
			if ( flags.show_channel_meters ) {
				int width = ( flags.terminal_width - 3 ) / mod.get_num_channels();
				if ( width > 11 ) {
					width = 11;
				}
				log << MPT_USTRING(" ");
				for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
					if ( width >= 3 ) {
						log << MPT_USTRING(":");
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
					log << MPT_USTRING(":");
				}
				log << lf;
			}
			if ( flags.show_pattern ) {
				int width = ( flags.terminal_width - 3 ) / mod.get_num_channels();
				if ( width > 13 + 1 ) {
					width = 13 + 1;
				}
				for ( std::int32_t line = 0; line < pattern_lines; ++line ) {
					std::int32_t row = mod.get_current_row() - ( pattern_lines / 2 ) + line;
					if ( row == mod.get_current_row() ) {
						log << MPT_USTRING(">");
					} else {
						log << MPT_USTRING(" ");
					}
					if ( row < 0 || row >= mod.get_pattern_num_rows( mod.get_current_pattern() ) ) {
						for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
							if ( width >= 3 ) {
								log << MPT_USTRING(":");
							}
							log << mpt::ustring( width >= 3 ? width - 1 : width, MPT_UCHAR(' ') );
						}
					} else {
						for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
							if ( width >= 3 ) {
								if ( row == mod.get_current_row() ) {
									log << MPT_USTRING("+");
								} else {
									log << MPT_USTRING(":");
								}
							}
							log << mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.format_pattern_row_channel( mod.get_current_pattern(), row, channel, width >= 3 ? width - 1 : width ) );
						}
					}
					if ( width >= 3 ) {
						log << MPT_USTRING(":");
					}
					log << lf;
				}
			}
			if ( flags.show_ui ) {
				if ( narrow ) {
					log << MPT_USTRING("Settings...: ");
					log << MPT_USTRING("Gain: ") << static_cast<float>( flags.gain ) * 0.01f << MPT_USTRING(" dB") << MPT_USTRING("   ");
					log << MPT_USTRING("Stereo: ") << flags.separation << MPT_USTRING(" %") << MPT_USTRING("   ");
					log << lf;
					log << MPT_USTRING("Filter.....: ");
					log << MPT_USTRING("Length: ") << flags.filtertaps << MPT_USTRING(" taps") << MPT_USTRING("   ");
					log << MPT_USTRING("Ramping: ") << flags.ramping << MPT_USTRING("   ");
					log << lf;
				} else {
					log << MPT_USTRING("Settings...: ");
					log << MPT_USTRING("Gain: ") << static_cast<float>( flags.gain ) * 0.01f << MPT_USTRING(" dB") << MPT_USTRING("   ");
					log << MPT_USTRING("Stereo: ") << flags.separation << MPT_USTRING(" %") << MPT_USTRING("   ");
					log << MPT_USTRING("Filter: ") << flags.filtertaps << MPT_USTRING(" taps") << MPT_USTRING("   ");
					log << MPT_USTRING("Ramping: ") << flags.ramping << MPT_USTRING("   ");
					log << lf;
				}
			}
			if ( flags.show_details ) {
				log << MPT_USTRING("Mixer......: ");
				log << MPT_USTRING("CPU:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 6, cpu_str );
				log << MPT_USTRING("   ");
				log << MPT_USTRING("Chn:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_playing_channels() );
				log << MPT_USTRING("   ");
				log << lf;
				if ( flags.show_progress ) {
					if ( narrow ) {
						log << MPT_USTRING("Player.....: ");
						log << MPT_USTRING("Ord:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_order() ) << MPT_USTRING("/") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_num_orders() );
						log << MPT_USTRING("   ");
						log << lf;
						log << MPT_USTRING("Pattern....: ");
						log << MPT_USTRING("Pat:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_pattern() );
						log << MPT_USTRING(" ");
						log << MPT_USTRING("Row:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_row() );
						log << MPT_USTRING("   ");
						log << lf;
						log << MPT_USTRING("Tempo......: ");
						log << MPT_USTRING("Spd:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 2, mod.get_current_speed() );
						log << MPT_USTRING(" ");
						log << MPT_USTRING("Tmp:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 6, mpt::format<mpt::ustring>::fix( mod.get_current_tempo2(), 2 ) );
						log << MPT_USTRING("   ");
						log << lf;
					} else {
						log << MPT_USTRING("Player.....: ");
						log << MPT_USTRING("Ord:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_order() ) << MPT_USTRING("/") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_num_orders() );
						log << MPT_USTRING(" ");
						log << MPT_USTRING("Pat:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_pattern() );
						log << MPT_USTRING(" ");
						log << MPT_USTRING("Row:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_row() );
						log << MPT_USTRING("   ");
						log << MPT_USTRING("Spd:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 2, mod.get_current_speed() );
						log << MPT_USTRING(" ");
						log << MPT_USTRING("Tmp:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 6, mpt::format<mpt::ustring>::fix( mod.get_current_tempo2(), 2 ) );
						log << MPT_USTRING("   ");
						log << lf;
					}
				}
			}
			if ( flags.show_progress ) {
				log << MPT_USTRING("Position...: ") << seconds_to_string( mod.get_position_seconds() ) << MPT_USTRING(" / ") << seconds_to_string( duration ) << MPT_USTRING("   ") << lf;
			}
		} else if ( flags.show_channel_meters ) {
			if ( flags.show_ui || flags.show_details || flags.show_progress ) {
				int width = ( flags.terminal_width - 3 ) / mod.get_num_channels();
				log << MPT_USTRING(" ");
				for ( std::int32_t channel = 0; channel < mod.get_num_channels(); ++channel ) {
					if ( width >= 3 ) {
						log << MPT_USTRING(":");
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
					log << MPT_USTRING(":");
				}
			}
			log << MPT_USTRING("   ") << MPT_USTRING("\r");
		} else {
			if ( flags.show_ui ) {
				log << MPT_USTRING(" ");
				log << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, static_cast<float>( flags.gain ) * 0.01f ) << MPT_USTRING("dB");
				log << MPT_USTRING("|");
				log << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, flags.separation ) << MPT_USTRING("%");
				log << MPT_USTRING("|");
				log << align_right<mpt::ustring>( MPT_UCHAR(':'), 2, flags.filtertaps ) << MPT_USTRING("taps");
				log << MPT_USTRING("|");
				log << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, flags.ramping );
			}
			if ( flags.show_meters ) {
				log << MPT_USTRING(" ");
				draw_meters_tiny( log, meter, flags );
			}
			if ( flags.show_details && flags.show_ui ) {
				log << MPT_USTRING(" ");
				log << MPT_USTRING("CPU:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 6, cpu_str );
				log << MPT_USTRING("|");
				log << MPT_USTRING("Chn:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_playing_channels() );
			}
			if ( flags.show_details && !flags.show_ui ) {
				if ( flags.show_progress ) {
					log << MPT_USTRING(" ");
					log << MPT_USTRING("Ord:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_order() ) << MPT_USTRING("/") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_num_orders() );
					log << MPT_USTRING("|");
					log << MPT_USTRING("Pat:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_pattern() );
					log << MPT_USTRING("|");
					log << MPT_USTRING("Row:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mod.get_current_row() );
					log << MPT_USTRING(" ");
					log << MPT_USTRING("Spd:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 2, mod.get_current_speed() );
					log << MPT_USTRING("|");
					log << MPT_USTRING("Tmp:") << align_right<mpt::ustring>( MPT_UCHAR(':'), 3, mpt::format<mpt::ustring>::fix( mod.get_current_tempo2(), 2 ) );
				}
			}
			if ( flags.show_progress ) {
				log << MPT_USTRING(" ");
				log << seconds_to_string( mod.get_position_seconds() );
				log << MPT_USTRING("/");
				log << seconds_to_string( duration );
			}
			if ( flags.show_ui || flags.show_details || flags.show_progress ) {
				log << MPT_USTRING("   ") << MPT_USTRING("\r");
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
std::map<mpt::ustring, mpt::ustring> get_metadata( const Tmod & mod ) {
	std::map<mpt::ustring, mpt::ustring> result;
	const std::vector<std::string> metadata_keys = mod.get_metadata_keys();
	for ( const auto & key : metadata_keys ) {
		result[ mpt::transcode<mpt::ustring>( libopenmpt_encoding, key ) ] = mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( key ) );
	}
	return result;
}

static void set_field( std::vector<field> & fields, const mpt::ustring & name, const mpt::ustring & value ) {
	fields.push_back( field{ name, value } );
}

static void show_fields( textout & log, const std::vector<field> & fields ) {
	const std::size_t fw = 11;
	for ( const auto & field : fields ) {
		mpt::ustring key = field.key;
		mpt::ustring val = field.val;
		if ( key.length() < fw ) {
			key += mpt::ustring( fw - key.length(), MPT_UCHAR('.') );
		}
		if ( key.length() > fw ) {
			key = key.substr( 0, fw );
		}
		key += MPT_USTRING(": ");
		val = prepend_lines( val, mpt::ustring( fw, MPT_UCHAR(' ') ) + MPT_USTRING(": ") );
		log << key << val << lf;
	}
}

static void probe_mod_file( commandlineflags & flags, const mpt::native_path & filename, std::uint64_t filesize, std::istream & data_stream, textout & log ) {

	log.writeout();

	std::vector<field> fields;

	if ( flags.filenames.size() > 1 ) {
		set_field( fields, MPT_USTRING("Playlist"), MPT_UFORMAT_MESSAGE( "{}/{}" )( flags.playlist_index + 1, flags.filenames.size() ) );
		set_field( fields, MPT_USTRING("Prev/Next"), MPT_UFORMAT_MESSAGE( "'{}' / ['{}'] / '{}'" )(
		    ( flags.playlist_index > 0 ? mpt::transcode<mpt::ustring>( get_filename( flags.filenames[ flags.playlist_index - 1 ] ) ) : mpt::ustring() ),
		    mpt::transcode<mpt::ustring>( get_filename( filename ) ),
		    ( flags.playlist_index + 1 < flags.filenames.size() ? mpt::transcode<mpt::ustring>( get_filename( flags.filenames[ flags.playlist_index + 1 ] ) ) : mpt::ustring() )
		    ) );
	}
	if ( flags.verbose ) {
		set_field( fields, MPT_USTRING("Path"), mpt::transcode<mpt::ustring>( filename ) );
	}
	if ( flags.show_details ) {
		set_field( fields, MPT_USTRING("Filename"), mpt::transcode<mpt::ustring>(  get_filename( filename ) ) );
		set_field( fields, MPT_USTRING("Size"), bytes_to_string( filesize ) );
	}
	
	int probe_result = openmpt::probe_file_header( openmpt::probe_file_header_flags_default2, data_stream );
	mpt::ustring probe_result_string;
	switch ( probe_result ) {
		case openmpt::probe_file_header_result_success:
			probe_result_string = MPT_USTRING("Success");
			break;
		case openmpt::probe_file_header_result_failure:
			probe_result_string = MPT_USTRING("Failure");
			break;
		case openmpt::probe_file_header_result_wantmoredata:
			probe_result_string = MPT_USTRING("Insufficient Data");
			break;
		default:
			probe_result_string = MPT_USTRING("Internal Error");
			break;
	}
	set_field( fields, MPT_USTRING("Probe"), probe_result_string );

	show_fields( log, fields );

	log.writeout();

}

template < typename Tmod >
void render_mod_file( commandlineflags & flags, const mpt::native_path & filename, std::uint64_t filesize, Tmod & mod, textout & log, write_buffers_interface & audio_stream ) {

	log.writeout();

	if ( flags.mode != Mode::Probe && flags.mode != Mode::Info ) {
		mod.set_repeat_count( flags.repeatcount );
		apply_mod_settings( flags, mod );
	}
	
	double duration = mod.get_duration_seconds();

	std::vector<field> fields;

	if ( flags.filenames.size() > 1 ) {
		set_field( fields, MPT_USTRING("Playlist"), MPT_UFORMAT_MESSAGE("{}/{}")( flags.playlist_index + 1, flags.filenames.size() ) );
		set_field( fields, MPT_USTRING("Prev/Next"), MPT_UFORMAT_MESSAGE("'{}' / ['{}'] / '{}'")(
		    ( flags.playlist_index > 0 ? mpt::transcode<mpt::ustring>( get_filename( flags.filenames[ flags.playlist_index - 1 ] ) ) : mpt::ustring() ),
		    mpt::transcode<mpt::ustring>( get_filename( filename ) ),
		    ( flags.playlist_index + 1 < flags.filenames.size() ? mpt::transcode<mpt::ustring>( get_filename( flags.filenames[ flags.playlist_index + 1 ] ) ) : mpt::ustring() )
		   ) );
	}
	if ( flags.verbose ) {
		set_field( fields, MPT_USTRING("Path"), mpt::transcode<mpt::ustring>( filename ) );
	}
	if ( flags.show_details ) {
		set_field( fields, MPT_USTRING("Filename"), mpt::transcode<mpt::ustring>( get_filename( filename ) ) );
		set_field( fields, MPT_USTRING("Size"), bytes_to_string( filesize ) );
		if ( !mod.get_metadata( "warnings" ).empty() ) {
			set_field( fields, MPT_USTRING("Warnings"), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "warnings" ) ) );
		}
		if ( !mod.get_metadata( "container" ).empty() ) {
			set_field( fields, MPT_USTRING("Container"), MPT_UFORMAT_MESSAGE("{} ({})")( mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "container" ) ), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "container_long" ) ) ) );
		}
		set_field( fields, MPT_USTRING("Type"), MPT_UFORMAT_MESSAGE("{} ({})")( mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "type" ) ), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "type_long" ) ) ) );
		if ( !mod.get_metadata( "originaltype" ).empty() ) {
			set_field( fields, MPT_USTRING("Orig. Type"), MPT_UFORMAT_MESSAGE("{} ({})")( mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "originaltype" ) ), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "originaltype_long" ) ) ) );
		}
		if ( ( mod.get_num_subsongs() > 1 ) && ( flags.subsong != -1 ) ) {
			set_field( fields, MPT_USTRING("Subsong"), mpt::format<mpt::ustring>::val( flags.subsong ) );
		}
		set_field( fields, MPT_USTRING("Tracker"), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "tracker" ) ) );
		if ( !mod.get_metadata( "date" ).empty() ) {
			set_field( fields, MPT_USTRING("Date"), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "date" ) ) );
		}
		if ( !mod.get_metadata( "artist" ).empty() ) {
			set_field( fields, MPT_USTRING("Artist"), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "artist" ) ) );
		}
	}
	if ( true ) {
		set_field( fields, MPT_USTRING("Title"), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "title" ) ) );
		set_field( fields, MPT_USTRING("Duration"), seconds_to_string( duration ) );
	}
	if ( flags.show_details ) {
		set_field( fields, MPT_USTRING("Subsongs"), mpt::format<mpt::ustring>::val( mod.get_num_subsongs() ) );
		set_field( fields, MPT_USTRING("Channels"), mpt::format<mpt::ustring>::val( mod.get_num_channels() ) );
		set_field( fields, MPT_USTRING("Orders"), mpt::format<mpt::ustring>::val( mod.get_num_orders() ) );
		set_field( fields, MPT_USTRING("Patterns"), mpt::format<mpt::ustring>::val( mod.get_num_patterns() ) );
		set_field( fields, MPT_USTRING("Instruments"), mpt::format<mpt::ustring>::val( mod.get_num_instruments() ) );
		set_field( fields, MPT_USTRING("Samples"), mpt::format<mpt::ustring>::val( mod.get_num_samples() ) );
	}
	if ( flags.show_message ) {
		set_field( fields, MPT_USTRING("Message"), mpt::transcode<mpt::ustring>( libopenmpt_encoding, mod.get_metadata( "message" ) ) );
	}

	show_fields( log, fields );

	log.writeout();

	if ( flags.filenames.size() == 1 || flags.mode == Mode::Render ) {
		audio_stream.write_metadata( get_metadata( mod ) );
	} else {
		audio_stream.write_updated_metadata( get_metadata( mod ) );
	}

	if ( flags.mode == Mode::Probe || flags.mode == Mode::Info ) {
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
			log << lf;
		}
	} catch ( ... ) {
		if ( flags.show_progress ) {
			log << lf;
		}
		throw;
	}

	log.writeout();

}

static void probe_file( commandlineflags & flags, const mpt::native_path & filename, textout & log ) {

	log.writeout();

	std::ostringstream silentlog;

	try {

		std::optional<mpt::IO::ifstream> optional_file_stream;
		std::uint64_t filesize = 0;
		bool use_stdin = ( filename == MPT_NATIVE_PATH("-") );
		if ( !use_stdin ) {
			optional_file_stream.emplace( filename, std::ios::binary );
			std::istream & file_stream = *optional_file_stream;
			file_stream.seekg( 0, std::ios::end );
			filesize = file_stream.tellg();
			file_stream.seekg( 0, std::ios::beg );
		}
		std::istream & data_stream = use_stdin ? std::cin : *optional_file_stream;
		if ( data_stream.fail() ) {
			throw exception( MPT_USTRING("file open error") );
		}
		
		probe_mod_file( flags, filename, filesize, data_stream, log );

	} catch ( silent_exit_exception & ) {
		throw;
	} catch ( std::exception & e ) {
		if ( !silentlog.str().empty() ) {
			log << MPT_USTRING("errors probing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("': ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, silentlog.str() ) << lf;
		} else {
			log << MPT_USTRING("errors probing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("'") << lf;
		}
		log << MPT_USTRING("error probing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("': ") << mpt::get_exception_text<mpt::ustring>( e ) << lf;
	} catch ( ... ) {
		if ( !silentlog.str().empty() ) {
			log << MPT_USTRING("errors probing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("': ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, silentlog.str() ) << lf;
		} else {
			log << MPT_USTRING("errors probing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("'") << lf;
		}
		log << MPT_USTRING("unknown error probing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("'") << lf;
	}

	log << lf;

	log.writeout();

}

static void render_file( commandlineflags & flags, const mpt::native_path & filename, textout & log, write_buffers_interface & audio_stream ) {

	log.writeout();

	std::ostringstream silentlog;

	try {

		std::optional<mpt::IO::ifstream> optional_file_stream;
		std::uint64_t filesize = 0;
		bool use_stdin = ( filename == MPT_NATIVE_PATH("-") );
		if ( !use_stdin ) {
			optional_file_stream.emplace( filename, std::ios::binary );
			std::istream & file_stream = *optional_file_stream;
			file_stream.seekg( 0, std::ios::end );
			filesize = file_stream.tellg();
			file_stream.seekg( 0, std::ios::beg );
		}
		std::istream & data_stream = use_stdin ? std::cin : *optional_file_stream;
		if ( data_stream.fail() ) {
			throw exception( MPT_USTRING("file open error") );
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
			log << MPT_USTRING("errors loading '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("': ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding, silentlog.str() ) << lf;
		} else {
			log << MPT_USTRING("errors loading '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("'") << lf;
		}
		log << MPT_USTRING("error playing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("': ") << mpt::get_exception_text<mpt::ustring>( e ) << lf;
	} catch ( ... ) {
		if ( !silentlog.str().empty() ) {
			log << MPT_USTRING("errors loading '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("': ") << mpt::transcode<mpt::ustring>( libopenmpt_encoding,silentlog.str() ) << lf;
		} else {
			log << MPT_USTRING("errors loading '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("'") << lf;
		}
		log << MPT_USTRING("unknown error playing '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("'") << lf;
	}

	log << lf;

	log.writeout();

}


static mpt::native_path get_random_filename( std::set<mpt::native_path> & filenames, mpt::good_engine & prng ) {
	std::size_t index = mpt::random<std::size_t>( prng, 0, filenames.size() - 1 );
	std::set<mpt::native_path>::iterator it = filenames.begin();
	std::advance( it, index );
	return *it;
}


static void render_files( commandlineflags & flags, textout & log, write_buffers_interface & audio_stream, mpt::good_engine & prng ) {
	if ( flags.randomize ) {
		std::shuffle( flags.filenames.begin(), flags.filenames.end(), prng );
	}
	try {
		while ( true ) {
			if ( flags.shuffle ) {
				// TODO: improve prev/next logic
				std::set<mpt::native_path> shuffle_set;
				shuffle_set.insert( flags.filenames.begin(), flags.filenames.end() );
				while ( true ) {
					if ( shuffle_set.empty() ) {
						break;
					}
					mpt::native_path filename = get_random_filename( shuffle_set, prng );
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
				std::vector<mpt::native_path>::iterator filename = flags.filenames.begin();
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


static bool parse_playlist( commandlineflags & flags, mpt::native_path filename, concat_stream<mpt::ustring> & log ) {
	bool is_playlist = false;
	bool m3u8 = false;
	if ( get_extension( filename ) == MPT_NATIVE_PATH("m3u") || get_extension( filename ) == MPT_NATIVE_PATH("m3U") || get_extension( filename ) == MPT_NATIVE_PATH("M3u") || get_extension( filename ) == MPT_NATIVE_PATH("M3U") ) {
		is_playlist = true;
	}
	if ( get_extension( filename ) == MPT_NATIVE_PATH("m3u8") || get_extension( filename ) == MPT_NATIVE_PATH("m3U8") || get_extension( filename ) == MPT_NATIVE_PATH("M3u8") || get_extension( filename ) == MPT_NATIVE_PATH("M3U8") ) {
		is_playlist = true;
		m3u8 = true;
	}
	if ( get_extension( filename ) == MPT_NATIVE_PATH("pls") || get_extension( filename ) == MPT_NATIVE_PATH("plS") || get_extension( filename ) == MPT_NATIVE_PATH("pLs") || get_extension( filename ) == MPT_NATIVE_PATH("pLS") || get_extension( filename ) == MPT_NATIVE_PATH("Pls") || get_extension( filename ) == MPT_NATIVE_PATH("PlS")  || get_extension( filename ) == MPT_NATIVE_PATH("PLs") || get_extension( filename ) == MPT_NATIVE_PATH("PLS") ) {
		is_playlist = true;
	}
	mpt::native_path basepath = get_basepath( filename );
	try {
		mpt::IO::ifstream file_stream( filename, std::ios::binary );
		std::string line;
		bool first = true;
		bool extm3u = false;
		bool pls = false;
		while ( std::getline( file_stream, line ) ) {
			mpt::native_path newfile;
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
			constexpr auto pls_encoding = mpt::common_encoding::utf8;
			constexpr auto m3u8_encoding = mpt::common_encoding::utf8;
#if MPT_OS_WINDOWS
			constexpr auto m3u_encoding = mpt::logical_encoding::locale;
#else
			constexpr auto m3u_encoding = mpt::common_encoding::utf8;
#endif
			if ( pls ) {
				if ( mpt::starts_with( line, "File" ) ) {
					if ( line.find( "=" ) != std::string::npos ) {
						flags.filenames.push_back( mpt::transcode<mpt::native_path>( pls_encoding, line.substr( line.find( "=" ) + 1 ) ) );
					}
				} else if ( mpt::starts_with( line, "Title" ) ) {
					continue;
				} else if ( mpt::starts_with( line, "Length" ) ) {
					continue;
				} else if ( mpt::starts_with( line, "NumberOfEntries" ) ) {
					continue;
				} else if ( mpt::starts_with( line, "Version" ) ) {
					continue;
				} else {
					continue;
				}
			} else if ( extm3u ) {
				if ( mpt::starts_with( line, "#EXTINF" ) ) {
					continue;
				} else if ( mpt::starts_with( line, "#" ) ) {
					continue;
				}
				if ( m3u8 ) {
					newfile = mpt::transcode<mpt::native_path>( m3u8_encoding, line );
				} else {
					newfile = mpt::transcode<mpt::native_path>( m3u_encoding, line );
				}
			} else {
				if ( m3u8 ) {
					newfile = mpt::transcode<mpt::native_path>( m3u8_encoding, line );
				} else {
					newfile = mpt::transcode<mpt::native_path>( m3u_encoding, line );
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
		log << MPT_USTRING("error loading '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("': ") << mpt::get_exception_text<mpt::ustring>( e ) << lf;
	} catch ( ... ) {
		log << MPT_USTRING("unknown error loading '") << mpt::transcode<mpt::ustring>( filename ) << MPT_USTRING("'") << lf;
	}
	return is_playlist;
}


static void parse_openmpt123( commandlineflags & flags, const std::vector<mpt::ustring> & args, concat_stream<mpt::ustring> & log ) {

	enum class action {
		help,
		help_keyboard,
		man_version,
		man_help,
		version,
		short_version,
		long_version,
		credits,
		license,
	};

	std::optional<action> return_action;

	if ( args.size() <= 1 ) {
		throw args_error_exception();
	}

	bool files_only = false;
	// cppcheck false-positive
	// cppcheck-suppress StlMissingComparison
	for ( auto i = args.begin(); i != args.end(); ++i ) {
		if ( i == args.begin() ) {
			// skip program name
			continue;
		}
		mpt::ustring arg = *i;
		mpt::ustring nextarg = ( i+1 != args.end() ) ? *(i+1) : MPT_USTRING("");
		if ( files_only ) {
			flags.filenames.push_back( mpt::transcode<mpt::native_path>( arg ) );
		} else if ( arg.substr( 0, 1 ) != MPT_USTRING("-") ) {
			flags.filenames.push_back( mpt::transcode<mpt::native_path>( arg ) );
		} else {
			if ( arg == MPT_USTRING("--") ) {
				files_only = true;
			} else if ( arg == MPT_USTRING("-h") || arg == MPT_USTRING("--help") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::help;
			} else if ( arg == MPT_USTRING("--help-keyboard") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::help_keyboard;
			} else if ( arg == MPT_USTRING("--man-version") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::man_version;
			} else if ( arg == MPT_USTRING("--man-help") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::man_help;
			} else if ( arg == MPT_USTRING("--version") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::version;
			} else if ( arg == MPT_USTRING("--short-version") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::short_version;
			} else if ( arg == MPT_USTRING("--long-version") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::long_version;
			} else if ( arg == MPT_USTRING("--credits") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::credits;
			} else if ( arg == MPT_USTRING("--license") ) {
				if ( return_action ) {
					throw args_error_exception();
				}
				return_action = action::license;
			} else if ( arg == MPT_USTRING("-q") || arg == MPT_USTRING("--quiet") ) {
				flags.quiet = true;
			} else if ( arg == MPT_USTRING("-v") || arg == MPT_USTRING("--verbose") ) {
				flags.verbose = true;
			} else if ( arg == MPT_USTRING("--probe") ) {
				flags.mode = Mode::Probe;
			} else if ( arg == MPT_USTRING("--info") ) {
				flags.mode = Mode::Info;
			} else if ( arg == MPT_USTRING("--ui") ) {
				flags.mode = Mode::UI;
			} else if ( arg == MPT_USTRING("--batch") ) {
				flags.mode = Mode::Batch;
			} else if ( arg == MPT_USTRING("--render") ) {
				flags.mode = Mode::Render;
			} else if ( arg == MPT_USTRING("--assume-terminal") ) {
				flags.assume_terminal = true;
			} else if ( arg == MPT_USTRING("--banner") && nextarg != MPT_USTRING("") ) {
				std::int8_t value = static_cast<std::int8_t>( flags.banner );
				mpt::parse_into( value, nextarg );
				flags.banner = static_cast<verbosity>( value );
				++i;
			} else if ( arg == MPT_USTRING("--terminal-width") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.terminal_width, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--terminal-height") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.terminal_height, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--progress") ) {
				flags.show_progress = true;
			} else if ( arg == MPT_USTRING("--no-progress") ) {
				flags.show_progress = false;
			} else if ( arg == MPT_USTRING("--meters") ) {
				flags.show_meters = true;
			} else if ( arg == MPT_USTRING("--no-meters") ) {
				flags.show_meters = false;
			} else if ( arg == MPT_USTRING("--channel-meters") ) {
				flags.show_channel_meters = true;
			} else if ( arg == MPT_USTRING("--no-channel-meters") ) {
				flags.show_channel_meters = false;
			} else if ( arg == MPT_USTRING("--pattern") ) {
				flags.show_pattern = true;
			} else if ( arg == MPT_USTRING("--no-pattern") ) {
				flags.show_pattern = false;
			} else if ( arg == MPT_USTRING("--details") ) {
				flags.show_details = true;
			} else if ( arg == MPT_USTRING("--no-details") ) {
				flags.show_details = false;
			} else if ( arg == MPT_USTRING("--message") ) {
				flags.show_message = true;
			} else if ( arg == MPT_USTRING("--no-message") ) {
				flags.show_message = false;
			} else if ( arg == MPT_USTRING("--driver") && nextarg != MPT_USTRING("") ) {
				if ( false ) {
					// nothing
				} else if ( nextarg == MPT_USTRING("help") ) {
					string_concat_stream<mpt::ustring> drivers;
					realtime_audio_stream::show_drivers( drivers );
					throw show_help_exception( drivers.str() );
				} else if ( nextarg == MPT_USTRING("default") ) {
					flags.driver = MPT_USTRING("");
				} else {
					flags.driver = nextarg;
				}
				++i;
			} else if ( arg == MPT_USTRING("--device") && nextarg != MPT_USTRING("") ) {
				if ( false ) {
					// nothing
				} else if ( nextarg == MPT_USTRING("help") ) {
					string_concat_stream<mpt::ustring> devices;
					realtime_audio_stream::show_devices( devices, log );
					throw show_help_exception( devices.str() );
				} else if ( nextarg == MPT_USTRING("default") ) {
					flags.device = MPT_USTRING("");
				} else {
					flags.device = nextarg;
				}
				++i;
			} else if ( arg == MPT_USTRING("--buffer") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.buffer, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--period") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.period, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--update") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.ui_redraw_interval, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--stdout") ) {
				flags.use_stdout = true;
			} else if ( ( arg == MPT_USTRING("-o") || arg == MPT_USTRING("--output") ) && nextarg != MPT_USTRING("") ) {
				flags.output_filename = mpt::transcode<mpt::native_path>( nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--force") ) {
				flags.force_overwrite = true;
			} else if ( arg == MPT_USTRING("--output-type") && nextarg != MPT_USTRING("") ) {
				flags.output_extension = mpt::transcode<mpt::native_path>( nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--samplerate") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.samplerate, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--channels") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.channels, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--float") ) {
				flags.use_float = true;
			} else if ( arg == MPT_USTRING("--no-float") ) {
				flags.use_float = false;
			} else if ( arg == MPT_USTRING("--gain") && nextarg != MPT_USTRING("") ) {
				double gain = 0.0;
				mpt::parse_into( gain, nextarg );
				flags.gain = mpt::saturate_round<std::int32_t>( gain * 100.0 );
				++i;
			} else if ( arg == MPT_USTRING("--stereo") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.separation, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--filter") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.filtertaps, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--ramping") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.ramping, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--tempo") && nextarg != MPT_USTRING("") ) {
				flags.tempo = double_to_tempo_flag( mpt::parse_or<double>( nextarg, 1.0 ) );
				++i;
			} else if ( arg == MPT_USTRING("--pitch") && nextarg != MPT_USTRING("") ) {
				flags.pitch = double_to_pitch_flag( mpt::parse_or<double>( nextarg, 1.0 ) );
				++i;
			} else if ( arg == MPT_USTRING("--dither") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.dither, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--playlist") && nextarg != MPT_USTRING("") ) {
				parse_playlist( flags, mpt::transcode<mpt::native_path>( nextarg ), log );
				++i;
			} else if ( arg == MPT_USTRING("--randomize") ) {
				flags.randomize = true;
			} else if ( arg == MPT_USTRING("--no-randomize") ) {
				flags.randomize = false;
			} else if ( arg == MPT_USTRING("--shuffle") ) {
				flags.shuffle = true;
			} else if ( arg == MPT_USTRING("--no-shuffle") ) {
				flags.shuffle = false;
			} else if ( arg == MPT_USTRING("--restart") ) {
				flags.restart = true;
			} else if ( arg == MPT_USTRING("--no-restart") ) {
				flags.restart = false;
			} else if ( arg == MPT_USTRING("--subsong") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.subsong, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--repeat") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.repeatcount, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--ctl") && nextarg != MPT_USTRING("") ) {
				std::string ctl_c_v = mpt::transcode<std::string>( libopenmpt_encoding, nextarg );
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
			} else if ( arg == MPT_USTRING("--seek") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.seek_target, nextarg );
				++i;
			} else if ( arg == MPT_USTRING("--end-time") && nextarg != MPT_USTRING("") ) {
				mpt::parse_into( flags.end_time, nextarg );
				++i;
			} else if ( arg.size() > 0 && arg.substr( 0, 1 ) == MPT_USTRING("-") ) {
				throw args_error_exception();
			}
		}
	}

	if ( return_action ) {
		switch ( *return_action ) {
			case action::help:
				throw show_help_exception();
				break;
			case action::help_keyboard:
				throw show_help_keyboard_exception();
				break;
			case action::man_version:
				throw show_man_version_exception();
				break;
			case action::man_help:
				throw show_man_help_exception();
				break;
			case action::version:
				throw show_version_number_exception();
				break;
			case action::short_version:
				throw show_short_version_number_exception();
				break;
			case action::long_version:
				throw show_long_version_number_exception();
				break;
			case action::credits:
				throw show_credits_exception();
				break;
			case action::license:
				throw show_license_exception();
				break;
		}
	}

}

static mpt::uint8 main( std::vector<mpt::ustring> args ) {

	FILE_mode_guard stdout_text_guard( stdout, FILE_mode::text );
	FILE_mode_guard stderr_text_guard( stderr, FILE_mode::text );

	textout_wrapper<textout_destination::destination_stdout> std_out;
	textout_wrapper<textout_destination::destination_stderr> std_err;

	commandlineflags flags;

	try {

		parse_openmpt123( flags, args, std_err );

		flags.check_and_sanitize();

	} catch ( args_nofiles_exception & ) {
		show_banner( std_out, flags.banner );
		show_help( std_out );
		std_out.writeout();
		return 0;
	} catch ( args_error_exception & ) {
		show_banner( std_out, flags.banner );
		show_help( std_out );
		std_out.writeout();
		if ( args.size() > 1 ) {
			std_err << MPT_USTRING("Error parsing command line.") << lf;
			std_err.writeout();
		}
		return 1;
	} catch ( show_man_help_exception & ) {
		show_banner( std_out, flags.banner );
		show_help( std_out, true, true );
		return 0;
	} catch ( show_man_version_exception & ) {
		show_man_version( std_out );
		return 0;
	} catch ( show_help_exception & e ) {
		show_banner( std_out, flags.banner );
		show_help( std_out, e.longhelp, false, e.message );
		if ( flags.verbose ) {
			show_credits( std_out, verbosity_hidden );
		}
		return 0;
	} catch ( show_help_keyboard_exception & ) {
		show_banner( std_out, flags.banner );
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
		show_credits( std_out, flags.banner );
		return 0;
	} catch ( show_license_exception & ) {
		show_license( std_out, flags.banner );
		return 0;
	} catch ( silent_exit_exception & ) {
		return 0;
	} catch ( exception & e ) {
		std_err << MPT_USTRING("error: ") << mpt::get_exception_text<mpt::ustring>( e ) << lf;
		std_err.writeout();
		return 1;
	} catch ( std::exception & e ) {
		std_err << MPT_USTRING("error: ") << mpt::get_exception_text<mpt::ustring>( e ) << lf;
		std_err.writeout();
		return 1;
	} catch ( ... ) {
		std_err << MPT_USTRING("unknown error") << lf;
		std_err.writeout();
		return 1;
	}

	try {

		const FILE_mode stdin_mode = mpt::contains( flags.filenames, MPT_NATIVE_PATH("-") ) ? FILE_mode::binary : FILE_mode::text;
		const FILE_mode stdout_mode = flags.use_stdout ? FILE_mode::binary : FILE_mode::text;

		[[maybe_unused]] const bool stdin_text = ( stdin_mode == FILE_mode::text );
		[[maybe_unused]] const bool stdin_data = ( stdin_mode == FILE_mode::binary );
		[[maybe_unused]] const bool stdout_text = ( stdout_mode == FILE_mode::text );
		[[maybe_unused]] const bool stdout_data = ( stdout_mode == FILE_mode::binary );

		// set stdin/stdout to binary for data input/output
		[[maybe_unused]] std::optional<FILE_mode_guard> stdin_guard{ stdin_data ? std::make_optional<FILE_mode_guard>( stdin, FILE_mode::binary ) : std::nullopt };
		[[maybe_unused]] std::optional<FILE_mode_guard> stdout_guard{ stdout_data ? std::make_optional<FILE_mode_guard>( stdout, FILE_mode::binary ) : std::nullopt };

		// setup terminal input
		[[maybe_unused]] std::optional<FILE_mode_guard> stdin_text_guard{ stdin_text ? std::make_optional<FILE_mode_guard>( stdin, FILE_mode::text ) : std::nullopt };
		[[maybe_unused]] std::optional<terminal_ui_guard> input_guard{ stdin_text && ( flags.mode == Mode::UI ) ? std::make_optional<terminal_ui_guard>() : std::nullopt };

		// choose text output between quiet/stdout/stderr
		textout_dummy dummy_log;
		textout & log = flags.quiet ? static_cast<textout&>( dummy_log ) : stdout_text ? static_cast<textout&>( std_out ) : static_cast<textout&>( std_err );

		show_banner( log, flags.banner );

		if ( !flags.warnings.empty() ) {
			log << flags.warnings << lf;
		}

		if ( flags.verbose ) {
			log << flags;
		}

		log.writeout();

		mpt::sane_random_device rd;
		mpt::good_engine prng = mpt::make_prng<mpt::good_engine>( rd );
		mpt::crand::reseed( prng );

		switch ( flags.mode ) {
			case Mode::Probe: {
				for ( const auto & filename : flags.filenames ) {
					probe_file( flags, filename, log );
					flags.playlist_index++;
				}
			} break;
			case Mode::Info: {
				void_audio_stream dummy;
				render_files( flags, log, dummy, prng );
			} break;
			case Mode::UI:
			case Mode::Batch: {
				if ( flags.use_stdout ) {
					flags.apply_default_buffer_sizes();
					stdout_stream_raii stdout_audio_stream;
					render_files( flags, log, stdout_audio_stream, prng );
				} else if ( !flags.output_filename.empty() ) {
					flags.apply_default_buffer_sizes();
					file_audio_stream file_audio_stream( flags, flags.output_filename, log );
					render_files( flags, log, file_audio_stream, prng );
				} else {
					realtime_audio_stream audio_stream( flags, log );
					render_files( flags, log, audio_stream, prng );
				}
			} break;
			case Mode::Render: {
				for ( const auto & filename : flags.filenames ) {
					flags.apply_default_buffer_sizes();
					file_audio_stream file_audio_stream( flags, filename + MPT_NATIVE_PATH(".") + flags.output_extension, log );
					render_file( flags, filename, log, file_audio_stream );
					flags.playlist_index++;
				}
			} break;
			case Mode::None:
			break;
		}

	} catch ( args_error_exception & ) {
		show_banner( std_out, flags.banner );
		show_help( std_out );
		std_err << MPT_USTRING("Error parsing command line.") << lf;
		std_err.writeout();
		return 1;
	} catch ( silent_exit_exception & ) {
		return 0;
	} catch ( exception & e ) {
		std_err << MPT_USTRING("error: ") << mpt::get_exception_text<mpt::ustring>( e ) << lf;
		std_err.writeout();
		return 1;
	} catch ( std::exception & e ) {
		std_err << MPT_USTRING("error: ") << mpt::get_exception_text<mpt::ustring>( e ) << lf;
		std_err.writeout();
		return 1;
	} catch ( ... ) {
		std_err << MPT_USTRING("unknown error") << lf;
		std_err.writeout();
		return 1;
	}

	return 0;
}

} // namespace openmpt123


MPT_MAIN_IMPLEMENT_MAIN(openmpt123)
