/*
 * libopenmpt_impl.cpp
 * -------------------
 * Purpose: libopenmpt private interface implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "common/stdafx.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"

#include "libopenmpt_impl.hpp"

#include <algorithm>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <ostream>

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "common/version.h"
#include "common/misc_util.h"
#include "common/FileReader.h"
#include "common/Logging.h"
#include "common/mptMutex.h"
#include "soundlib/Sndfile.h"
#include "soundlib/mod_specifications.h"
#include "soundlib/AudioReadTarget.h"

OPENMPT_NAMESPACE_BEGIN

#if !defined(MPT_BUILD_SILENCE_LIBOPENMPT_CONFIGURATION_WARNINGS)

#if MPT_OS_WINDOWS && MPT_OS_WINDOWS_WINRT
#if defined(_WIN32_WINNT)
#if (_WIN32_WINNT < 0x0602)
MPT_WARNING("Warning: libopenmpt for WinRT is built with reduced functionality. Please #define _WIN32_WINNT 0x0602.")
#endif // _WIN32_WINNT
#endif // _WIN32_WINNT
#endif // MPT_OS_WINDOWS && MPT_OS_WINDOWS_WINRT

#if defined(MPT_BUILD_MSVC) || defined(MPT_BUILD_VCPKG)
#if MPT_OS_WINDOWS_WINRT
#pragma comment(lib, "ole32.lib")
#else
#pragma comment(lib, "rpcrt4.lib")
#endif
#ifndef NO_DMO
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "strmiids.lib")
#endif // !NO_DMO
#endif // MPT_BUILD_MSVC

#if MPT_PLATFORM_MULTITHREADED && MPT_MUTEX_NONE
MPT_WARNING("Warning: libopenmpt built in non thread-safe mode because mutexes are not supported by the C++ standard library available.")
#endif // MPT_MUTEX_NONE

#if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(_GLIBCXX_HAS_GTHREADS) && !defined(MPT_WITH_MINGWSTDTHREADS)
MPT_WARNING("Warning: Building libopenmpt with MinGW-w64 without std::thread support is not recommended and is deprecated. Please use MinGW-w64 with posix threading model (as opposed to win32 threading model), or build with mingw-std-threads.")
#endif // MINGW

#if MPT_CLANG_AT_LEAST(5,0,0) && defined(__powerpc__) && !defined(__powerpc64__)
MPT_WARNING("Warning: libopenmpt is known to trigger bad code generation with Clang 5 or later on powerpc (32bit) when using -O3. See <https://bugs.llvm.org/show_bug.cgi?id=46683>.")
#endif

#endif // !MPT_BUILD_SILENCE_LIBOPENMPT_CONFIGURATION_WARNINGS

#if defined(MPT_ASSERT_HANDLER_NEEDED) && !defined(ENABLE_TESTS)

MPT_NOINLINE void AssertHandler(const mpt::source_location &loc, const char *expr, const char *msg)
{
	if(msg)
	{
		mpt::log::Logger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetSource, msg) + U_(" (") + mpt::ToUnicode(mpt::CharsetSource, expr) + U_(")")
			);
	} else
	{
		mpt::log::Logger().SendLogMessage(loc, LogError, "ASSERT",
			U_("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetSource, expr)
			);
	}
	#if defined(MPT_BUILD_FATAL_ASSERTS)
		std::abort();
	#endif // MPT_BUILD_FATAL_ASSERTS
}

#endif // MPT_ASSERT_HANDLER_NEEDED && !ENABLE_TESTS

OPENMPT_NAMESPACE_END

using namespace OpenMPT;

namespace openmpt {

namespace version {

std::uint32_t get_library_version() {
	return OPENMPT_API_VERSION;
}

std::uint32_t get_core_version() {
	return Version::Current().GetRawVersion();
}

static std::string get_library_version_string() {
	std::string str;
	const SourceInfo sourceInfo = SourceInfo::Current();
	str += mpt::fmt::val(OPENMPT_API_VERSION_MAJOR);
	str += ".";
	str += mpt::fmt::val(OPENMPT_API_VERSION_MINOR);
	str += ".";
	str += mpt::fmt::val(OPENMPT_API_VERSION_PATCH);
	if ( std::string(OPENMPT_API_VERSION_PREREL).length() > 0 ) {
		str += OPENMPT_API_VERSION_PREREL;
	}
	std::vector<std::string> fields;
	if ( sourceInfo.Revision() ) {
		fields.push_back( "r" + mpt::fmt::val( sourceInfo.Revision() ) );
	}
	if ( sourceInfo.IsDirty() ) {
		fields.push_back( "modified" );
	} else if ( sourceInfo.HasMixedRevisions() ) {
		fields.push_back( "mixed" );
	}
	if ( sourceInfo.IsPackage() ) {
		fields.push_back( "pkg" );
	}
	if ( !fields.empty() ) {
		str += "+";
		str += mpt::String::Combine( fields, std::string(".") );
	}
	return str;
}

static std::string get_library_features_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, mpt::String::Trim(Build::GetBuildFeaturesString()));
}

static std::string get_core_version_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetVersionStringExtended());
}

static std::string get_source_url_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, SourceInfo::Current().GetUrlWithRevision());
}

static std::string get_source_date_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, SourceInfo::Current().Date());
}

static std::string get_source_revision_string() {
	const SourceInfo sourceInfo = SourceInfo::Current();
	return sourceInfo.Revision() ? mpt::fmt::val(sourceInfo.Revision()) : std::string();
}

static std::string get_build_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetBuildDateString());
}

static std::string get_build_compiler_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetBuildCompilerString());
}

static std::string get_credits_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetFullCreditsString());
}

static std::string get_contact_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, U_("Forum: ") +  Build::GetURL(Build::Url::Forum));
}

static std::string get_license_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetLicenseString());
}

static std::string get_url_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetURL(Build::Url::Website));
}

static std::string get_support_forum_url_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetURL(Build::Url::Forum));
}

static std::string get_bugtracker_url_string() {
	return mpt::ToCharset(mpt::Charset::UTF8, Build::GetURL(Build::Url::Bugtracker));
}

std::string get_string( const std::string & key ) {
	if ( key == "" ) {
		return std::string();
	} else if ( key == "library_version" ) {
		return get_library_version_string();
	} else if ( key == "library_version_major" ) {
		return mpt::fmt::val(OPENMPT_API_VERSION_MAJOR);
	} else if ( key == "library_version_minor" ) {
		return mpt::fmt::val(OPENMPT_API_VERSION_MINOR);
	} else if ( key == "library_version_patch" ) {
		return mpt::fmt::val(OPENMPT_API_VERSION_PATCH);
	} else if ( key == "library_version_prerel" ) {
		return mpt::fmt::val(OPENMPT_API_VERSION_PREREL);
	} else if ( key == "library_version_is_release" ) {
		return ( std::string(OPENMPT_API_VERSION_PREREL).length() == 0 ) ? "1" : "0";
	} else if ( key == "library_features" ) {
		return get_library_features_string();
	} else if ( key == "core_version" ) {
		return get_core_version_string();
	} else if ( key == "source_url" ) {
		return get_source_url_string();
	} else if ( key == "source_date" ) {
		return get_source_date_string();
	} else if ( key == "source_revision" ) {
		return get_source_revision_string();
	} else if ( key == "source_is_modified" ) {
		return SourceInfo::Current().IsDirty() ? "1" : "0";
	} else if ( key == "source_has_mixed_revision" ) {
		return SourceInfo::Current().HasMixedRevisions() ? "1" : "0";
	} else if ( key == "source_is_package" ) {
		return SourceInfo::Current().IsPackage() ? "1" : "0";
	} else if ( key == "build" ) {
		return get_build_string();
	} else if ( key == "build_compiler" ) {
		return get_build_compiler_string();
	} else if ( key == "credits" ) {
		return get_credits_string();
	} else if ( key == "contact" ) {
		return get_contact_string();
	} else if ( key == "license" ) {
		return get_license_string();
	} else if ( key == "url" ) {
		return get_url_string();
	} else if ( key == "support_forum_url" ) {
		return get_support_forum_url_string();
	} else if ( key == "bugtracker_url" ) {
		return get_bugtracker_url_string();
	} else {
		return std::string();
	}
}

} // namespace version

log_interface::log_interface() {
	return;
}
log_interface::~log_interface() {
	return;
}

std_ostream_log::std_ostream_log( std::ostream & dst ) : destination(dst) {
	return;
}
std_ostream_log::~std_ostream_log() {
	return;
}
void std_ostream_log::log( const std::string & message ) const {
	destination.flush();
	destination << message << std::endl;
	destination.flush();
}

class log_forwarder : public ILog {
private:
	log_interface & destination;
public:
	log_forwarder( log_interface & dest ) : destination(dest) {
		return;
	}
private:
	void AddToLog( LogLevel level, const mpt::ustring & text ) const override {
		destination.log( mpt::ToCharset( mpt::Charset::UTF8, LogLevelToString( level ) + U_(": ") + text ) );
	}
}; // class log_forwarder

class loader_log : public ILog {
private:
	mutable std::vector<std::pair<LogLevel,std::string> > m_Messages;
public:
	std::vector<std::pair<LogLevel,std::string> > GetMessages() const;
private:
	void AddToLog( LogLevel level, const mpt::ustring & text ) const override;
}; // class loader_log

std::vector<std::pair<LogLevel,std::string> > loader_log::GetMessages() const {
	return m_Messages;
}
void loader_log::AddToLog( LogLevel level, const mpt::ustring & text ) const {
	m_Messages.push_back( std::make_pair( level, mpt::ToCharset( mpt::Charset::UTF8, text ) ) );
}

void module_impl::PushToCSoundFileLog( const std::string & text ) const {
	m_sndFile->AddToLog( LogError, mpt::ToUnicode( mpt::Charset::UTF8, text ) );
}
void module_impl::PushToCSoundFileLog( int loglevel, const std::string & text ) const {
	m_sndFile->AddToLog( static_cast<LogLevel>( loglevel ), mpt::ToUnicode( mpt::Charset::UTF8, text ) );
}

module_impl::subsong_data::subsong_data( double duration, std::int32_t start_row, std::int32_t start_order, std::int32_t sequence )
	: duration(duration)
	, start_row(start_row)
	, start_order(start_order)
	, sequence(sequence)
{
	return;
}

static ResamplingMode filterlength_to_resamplingmode(std::int32_t length) {
	ResamplingMode result = SRCMODE_SINC8LP;
	if ( length == 0 ) {
		result = SRCMODE_SINC8LP;
	} else if ( length >= 8 ) {
		result = SRCMODE_SINC8LP;
	} else if ( length >= 3 ) {
		result = SRCMODE_CUBIC;
	} else if ( length >= 2 ) {
		result = SRCMODE_LINEAR;
	} else if ( length >= 1 ) {
		result = SRCMODE_NEAREST;
	} else {
		throw openmpt::exception("negative filter length");
	}
	return result;
}
static std::int32_t resamplingmode_to_filterlength(ResamplingMode mode) {
	switch ( mode ) {
	case SRCMODE_NEAREST:
		return 1;
		break;
	case SRCMODE_LINEAR:
		return 2;
		break;
	case SRCMODE_CUBIC:
		return 4;
		break;
	case SRCMODE_SINC8:
	case SRCMODE_SINC8LP:
	case SRCMODE_DEFAULT:
		return 8;
	default:
		throw openmpt::exception("unknown interpolation filter length set internally");
		break;
	}
}

static Resampling::AmigaFilter translate_amiga_filter_type( module_impl::amiga_filter_type amiga_type ) {
	switch (amiga_type ) {
		case module_impl::amiga_filter_type::a500:
			return Resampling::AmigaFilter::A500;
		case module_impl::amiga_filter_type::a1200:
		case module_impl::amiga_filter_type::auto_filter:
		default:
			return Resampling::AmigaFilter::A1200;
		case module_impl::amiga_filter_type::unfiltered:
			return Resampling::AmigaFilter::Unfiltered;
	}
}

static void ramping_to_mixersettings( MixerSettings & settings, int ramping ) {
	if ( ramping == -1 ) {
		settings.SetVolumeRampUpMicroseconds( MixerSettings().GetVolumeRampUpMicroseconds() );
		settings.SetVolumeRampDownMicroseconds( MixerSettings().GetVolumeRampDownMicroseconds() );
	} else if ( ramping <= 0 ) {
		settings.SetVolumeRampUpMicroseconds( 0 );
		settings.SetVolumeRampDownMicroseconds( 0 );
	} else {
		settings.SetVolumeRampUpMicroseconds( ramping * 1000 );
		settings.SetVolumeRampDownMicroseconds( ramping * 1000 );
	}
}
static void mixersettings_to_ramping( int & ramping, const MixerSettings & settings ) {
	std::int32_t ramp_us = std::max( settings.GetVolumeRampUpMicroseconds(), settings.GetVolumeRampDownMicroseconds() );
	if ( ( settings.GetVolumeRampUpMicroseconds() == MixerSettings().GetVolumeRampUpMicroseconds() ) && ( settings.GetVolumeRampDownMicroseconds() == MixerSettings().GetVolumeRampDownMicroseconds() ) ) {
		ramping = -1;
	} else if ( ramp_us <= 0 ) {
		ramping = 0;
	} else {
		ramping = ( ramp_us + 500 ) / 1000;
	}
}

std::string module_impl::mod_string_to_utf8( const std::string & encoded ) const {
	return mpt::ToCharset( mpt::Charset::UTF8, m_sndFile->GetCharsetInternal(), encoded );
}
void module_impl::apply_mixer_settings( std::int32_t samplerate, int channels ) {
	bool samplerate_changed = static_cast<std::int32_t>( m_sndFile->m_MixerSettings.gdwMixingFreq ) != samplerate;
	bool channels_changed = static_cast<int>( m_sndFile->m_MixerSettings.gnChannels ) != channels;
	if ( samplerate_changed || channels_changed ) {
		MixerSettings mixersettings = m_sndFile->m_MixerSettings;
		std::int32_t volrampin_us = mixersettings.GetVolumeRampUpMicroseconds();
		std::int32_t volrampout_us = mixersettings.GetVolumeRampDownMicroseconds();
		mixersettings.gdwMixingFreq = samplerate;
		mixersettings.gnChannels = channels;
		mixersettings.SetVolumeRampUpMicroseconds( volrampin_us );
		mixersettings.SetVolumeRampDownMicroseconds( volrampout_us );
		m_sndFile->SetMixerSettings( mixersettings );
	} else if ( !m_mixer_initialized ) {
		m_sndFile->InitPlayer( true );
	}
	if ( samplerate_changed ) {
		m_sndFile->SuspendPlugins();
		m_sndFile->ResumePlugins();
	}
	m_mixer_initialized = true;
}
void module_impl::apply_libopenmpt_defaults() {
	set_render_param( module::RENDER_STEREOSEPARATION_PERCENT, 100 );
	m_sndFile->Order.SetSequence( 0 );
}
module_impl::subsongs_type module_impl::get_subsongs() const {
	std::vector<subsong_data> subsongs;
	if ( m_sndFile->Order.GetNumSequences() == 0 ) {
		throw openmpt::exception("module contains no songs");
	}
	for ( SEQUENCEINDEX seq = 0; seq < m_sndFile->Order.GetNumSequences(); ++seq ) {
		const std::vector<GetLengthType> lengths = m_sndFile->GetLength( eNoAdjust, GetLengthTarget( true ).StartPos( seq, 0, 0 ) );
		for ( const auto & l : lengths ) {
			subsongs.push_back( subsong_data( l.duration, l.startRow, l.startOrder, seq ) );
		}
	}
	return subsongs;
}
void module_impl::init_subsongs( subsongs_type & subsongs ) const {
	subsongs = get_subsongs();
}
bool module_impl::has_subsongs_inited() const {
	return !m_subsongs.empty();
}
void module_impl::ctor( const std::map< std::string, std::string > & ctls ) {
	m_sndFile = std::make_unique<CSoundFile>();
	m_loaded = false;
	m_mixer_initialized = false;
	m_Dither = std::make_unique<Dither>( mpt::global_prng() );
	m_LogForwarder = std::make_unique<log_forwarder>( *m_Log );
	m_sndFile->SetCustomLog( m_LogForwarder.get() );
	m_current_subsong = 0;
	m_currentPositionSeconds = 0.0;
	m_Gain = 1.0f;
	m_ctl_play_at_end = song_end_action::fadeout_song;
	m_ctl_load_skip_samples = false;
	m_ctl_load_skip_patterns = false;
	m_ctl_load_skip_plugins = false;
	m_ctl_load_skip_subsongs_init = false;
	m_ctl_seek_sync_samples = false;
	// init member variables that correspond to ctls
	for ( const auto & ctl : ctls ) {
		ctl_set( ctl.first, ctl.second, false );
	}
}
void module_impl::load( const FileReader & file, const std::map< std::string, std::string > & ctls ) {
	loader_log loaderlog;
	m_sndFile->SetCustomLog( &loaderlog );
	{
		int load_flags = CSoundFile::loadCompleteModule;
		if ( m_ctl_load_skip_samples ) {
			load_flags &= ~CSoundFile::loadSampleData;
		}
		if ( m_ctl_load_skip_patterns ) {
			load_flags &= ~CSoundFile::loadPatternData;
		}
		if ( m_ctl_load_skip_plugins ) {
			load_flags &= ~(CSoundFile::loadPluginData | CSoundFile::loadPluginInstance);
		}
		if ( !m_sndFile->Create( file, static_cast<CSoundFile::ModLoadingFlags>( load_flags ) ) ) {
			throw openmpt::exception("error loading file");
		}
		if ( !m_ctl_load_skip_subsongs_init ) {
			init_subsongs( m_subsongs );
		}
		m_loaded = true;
	}
	m_sndFile->SetCustomLog( m_LogForwarder.get() );
	std::vector<std::pair<LogLevel,std::string> > loaderMessages = loaderlog.GetMessages();
	for ( const auto & msg : loaderMessages ) {
		PushToCSoundFileLog( msg.first, msg.second );
		m_loaderMessages.push_back( mpt::ToCharset( mpt::Charset::UTF8, LogLevelToString( msg.first ) ) + std::string(": ") + msg.second );
	}
	// init CSoundFile state that corresponds to ctls
	for ( const auto & ctl : ctls ) {
		ctl_set( ctl.first, ctl.second, false );
	}
}
bool module_impl::is_loaded() const {
	return m_loaded;
}
std::size_t module_impl::read_wrapper( std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right ) {
	m_sndFile->ResetMixStat();
	m_sndFile->m_bIsRendering = ( m_ctl_play_at_end != song_end_action::fadeout_song );
	std::size_t count_read = 0;
	std::int16_t * const buffers[4] = { left, right, rear_left, rear_right };
	AudioReadTargetGainBuffer<audio_buffer_planar<std::int16_t>> target( audio_buffer_planar<std::int16_t>( buffers, planar_audio_buffer_valid_channels( buffers, std::size( buffers) ), count ), *m_Dither, m_Gain );
	while ( count > 0 ) {
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min( static_cast<std::uint64_t>( count ), static_cast<std::uint64_t>( std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	if ( count_read == 0 && m_ctl_play_at_end == song_end_action::continue_song ) {
		// This is the song end, but allow the song or loop to restart on the next call
		m_sndFile->m_SongFlags.reset(SONG_ENDREACHED);
	}
	return count_read;
}
std::size_t module_impl::read_wrapper( std::size_t count, float * left, float * right, float * rear_left, float * rear_right ) {
	m_sndFile->ResetMixStat();
	m_sndFile->m_bIsRendering = ( m_ctl_play_at_end != song_end_action::fadeout_song );
	std::size_t count_read = 0;
	float * const buffers[4] = { left, right, rear_left, rear_right };
	AudioReadTargetGainBuffer<audio_buffer_planar<float>> target( audio_buffer_planar<float>( buffers, planar_audio_buffer_valid_channels( buffers, std::size( buffers) ), count ), *m_Dither, m_Gain );
	while ( count > 0 ) {
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min( static_cast<std::uint64_t>( count ), static_cast<std::uint64_t>( std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	if ( count_read == 0 && m_ctl_play_at_end == song_end_action::continue_song ) {
		// This is the song end, but allow the song or loop to restart on the next call
		m_sndFile->m_SongFlags.reset(SONG_ENDREACHED);
	}
	return count_read;
}
std::size_t module_impl::read_interleaved_wrapper( std::size_t count, std::size_t channels, std::int16_t * interleaved ) {
	m_sndFile->ResetMixStat();
	m_sndFile->m_bIsRendering = ( m_ctl_play_at_end != song_end_action::fadeout_song );
	std::size_t count_read = 0;
	AudioReadTargetGainBuffer<audio_buffer_interleaved<std::int16_t>> target( audio_buffer_interleaved<std::int16_t>( interleaved, channels, count ), *m_Dither, m_Gain );
	while ( count > 0 ) {
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min( static_cast<std::uint64_t>( count ), static_cast<std::uint64_t>( std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	if ( count_read == 0 && m_ctl_play_at_end == song_end_action::continue_song ) {
		// This is the song end, but allow the song or loop to restart on the next call
		m_sndFile->m_SongFlags.reset(SONG_ENDREACHED);
	}
	return count_read;
}
std::size_t module_impl::read_interleaved_wrapper( std::size_t count, std::size_t channels, float * interleaved ) {
	m_sndFile->ResetMixStat();
	m_sndFile->m_bIsRendering = ( m_ctl_play_at_end != song_end_action::fadeout_song );
	std::size_t count_read = 0;
	AudioReadTargetGainBuffer<audio_buffer_interleaved<float>> target( audio_buffer_interleaved<float>( interleaved, channels, count ), *m_Dither, m_Gain );
	while ( count > 0 ) {
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min( static_cast<std::uint64_t>( count ), static_cast<std::uint64_t>( std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	if ( count_read == 0 && m_ctl_play_at_end == song_end_action::continue_song ) {
		// This is the song end, but allow the song or loop to restart on the next call
		m_sndFile->m_SongFlags.reset(SONG_ENDREACHED);
	}
	return count_read;
}

std::vector<std::string> module_impl::get_supported_extensions() {
	std::vector<std::string> retval;
	std::vector<const char *> extensions = CSoundFile::GetSupportedExtensions( false );
	std::copy( extensions.begin(), extensions.end(), std::back_insert_iterator<std::vector<std::string> >( retval ) );
	return retval;
}
bool module_impl::is_extension_supported( std::string_view extension ) {
	return CSoundFile::IsExtensionSupported( extension );
}
double module_impl::could_open_probability( const OpenMPT::FileReader & file, double effort, std::unique_ptr<log_interface> log ) {
	try {
		if ( effort >= 0.8 ) {
			std::unique_ptr<CSoundFile> sndFile = std::make_unique<CSoundFile>();
			std::unique_ptr<log_forwarder> logForwarder = std::make_unique<log_forwarder>( *log );
			sndFile->SetCustomLog( logForwarder.get() );
			if ( !sndFile->Create( file, CSoundFile::loadCompleteModule ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 1.0;
		} else if ( effort >= 0.6 ) {
			std::unique_ptr<CSoundFile> sndFile = std::make_unique<CSoundFile>();
			std::unique_ptr<log_forwarder> logForwarder = std::make_unique<log_forwarder>( *log );
			sndFile->SetCustomLog( logForwarder.get() );
			if ( !sndFile->Create( file, CSoundFile::loadNoPatternOrPluginData ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 0.8;
		} else if ( effort >= 0.2 ) {
			std::unique_ptr<CSoundFile> sndFile = std::make_unique<CSoundFile>();
			std::unique_ptr<log_forwarder> logForwarder = std::make_unique<log_forwarder>( *log );
			sndFile->SetCustomLog( logForwarder.get() );
			if ( !sndFile->Create( file, CSoundFile::onlyVerifyHeader ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 0.6;
		} else if ( effort >= 0.1 ) {
			FileReader::PinnedRawDataView view = file.GetPinnedRawDataView( probe_file_header_get_recommended_size() );
			int probe_file_header_result = probe_file_header( probe_file_header_flags_default, view.data(), view.size(), file.GetLength() );
			double result = 0.0;
			switch ( probe_file_header_result ) {
				case probe_file_header_result_success:
					result = 0.6;
					break;
				case probe_file_header_result_failure:
					result = 0.0;
					break;
				case probe_file_header_result_wantmoredata:
					result = 0.3;
					break;
				default:
					throw openmpt::exception("");
					break;
			}
			return result;
		} else {
			return 0.2;
		}
	} catch ( ... ) {
		return 0.0;
	}
}
double module_impl::could_open_probability( callback_stream_wrapper stream, double effort, std::unique_ptr<log_interface> log ) {
	CallbackStream fstream;
	fstream.stream = stream.stream;
	fstream.read = stream.read;
	fstream.seek = stream.seek;
	fstream.tell = stream.tell;
	return could_open_probability( make_FileReader( fstream ), effort, std::move(log) );
}
double module_impl::could_open_probability( std::istream & stream, double effort, std::unique_ptr<log_interface> log ) {
	return could_open_probability( make_FileReader( &stream ), effort, std::move(log) );
}

std::size_t module_impl::probe_file_header_get_recommended_size() {
	return CSoundFile::ProbeRecommendedSize;
}
int module_impl::probe_file_header( std::uint64_t flags, const std::byte * data, std::size_t size, std::uint64_t filesize ) {
	int result = 0;
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( data, size ), &filesize ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
int module_impl::probe_file_header( std::uint64_t flags, const std::uint8_t * data, std::size_t size, std::uint64_t filesize ) {
	int result = 0;
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( mpt::byte_cast<const std::byte*>( data ), size ), &filesize ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
int module_impl::probe_file_header( std::uint64_t flags, const void * data, std::size_t size, std::uint64_t filesize ) {
	int result = 0;
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( mpt::void_cast<const std::byte*>( data ), size ), &filesize ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
int module_impl::probe_file_header( std::uint64_t flags, const std::byte * data, std::size_t size ) {
	int result = 0;
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( data, size ), nullptr ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
int module_impl::probe_file_header( std::uint64_t flags, const std::uint8_t * data, std::size_t size ) {
	int result = 0;
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( mpt::byte_cast<const std::byte*>( data ), size ), nullptr ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
int module_impl::probe_file_header( std::uint64_t flags, const void * data, std::size_t size ) {
	int result = 0;
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( mpt::void_cast<const std::byte*>( data ), size ), nullptr ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
int module_impl::probe_file_header( std::uint64_t flags, std::istream & stream ) {
	int result = 0;
	char buffer[ PROBE_RECOMMENDED_SIZE ];
	MemsetZero( buffer );
	std::size_t size_read = 0;
	std::size_t size_toread = CSoundFile::ProbeRecommendedSize;
	if ( stream.bad() ) {
		throw exception("error reading stream");
	}
	const bool seekable = FileDataContainerStdStreamSeekable::IsSeekable( &stream );
	const uint64 filesize = ( seekable ? FileDataContainerStdStreamSeekable::GetLength( &stream ) : 0 );
	while ( ( size_toread > 0 ) && stream ) {
		stream.read( buffer + size_read, size_toread );
		if ( stream.bad() ) {
			throw exception("error reading stream");
		} else if ( stream.eof() ) {
			// normal
		} else if ( stream.fail() ) {
			throw exception("error reading stream");
		} else {
			// normal
		}
		std::size_t read_count = static_cast<std::size_t>( stream.gcount() );
		size_read += read_count;
		size_toread -= read_count;
	}
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( mpt::byte_cast<const std::byte*>( buffer ), size_read ), seekable ? &filesize : nullptr ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
int module_impl::probe_file_header( std::uint64_t flags, callback_stream_wrapper stream ) {
	int result = 0;
	char buffer[ PROBE_RECOMMENDED_SIZE ];
	MemsetZero( buffer );
	std::size_t size_read = 0;
	std::size_t size_toread = CSoundFile::ProbeRecommendedSize;
	if ( !stream.read ) {
		throw exception("error reading stream");
	}
	CallbackStream fstream;
	fstream.stream = stream.stream;
	fstream.read = stream.read;
	fstream.seek = stream.seek;
	fstream.tell = stream.tell;
	const bool seekable = FileDataContainerCallbackStreamSeekable::IsSeekable( fstream );
	const uint64 filesize = ( seekable ? FileDataContainerCallbackStreamSeekable::GetLength( fstream ) : 0 );
	while ( size_toread > 0 ) {
		std::size_t read_count = stream.read( stream.stream, buffer + size_read, size_toread );
		size_read += read_count;
		size_toread -= read_count;
		if ( read_count == 0 ) { // eof
			break;
		}
	}
	switch ( CSoundFile::Probe( static_cast<CSoundFile::ProbeFlags>( flags ), mpt::span<const std::byte>( mpt::byte_cast<const std::byte*>( buffer ), size_read ), seekable ? &filesize : nullptr ) ) {
		case CSoundFile::ProbeSuccess:
			result = probe_file_header_result_success;
			break;
		case CSoundFile::ProbeFailure:
			result = probe_file_header_result_failure;
			break;
		case CSoundFile::ProbeWantMoreData:
			result = probe_file_header_result_wantmoredata;
			break;
		default:
			throw exception("internal error");
			break;
	}
	return result;
}
module_impl::module_impl( callback_stream_wrapper stream, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	CallbackStream fstream;
	fstream.stream = stream.stream;
	fstream.read = stream.read;
	fstream.seek = stream.seek;
	fstream.tell = stream.tell;
	load( make_FileReader( fstream ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( std::istream & stream, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( &stream ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::vector<std::byte> & data, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( mpt::as_span( data ) ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::vector<std::uint8_t> & data, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( mpt::as_span( data ) ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::vector<char> & data, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( mpt::byte_cast< mpt::span< const std::byte > >( mpt::as_span( data ) ) ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::byte * data, std::size_t size, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( mpt::as_span( data, size ) ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::uint8_t * data, std::size_t size, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( mpt::as_span( data, size ) ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const char * data, std::size_t size, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( mpt::byte_cast< mpt::span< const std::byte > >( mpt::as_span( data, size ) ) ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const void * data, std::size_t size, std::unique_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(std::move(log)) {
	ctor( ctls );
	load( make_FileReader( mpt::as_span( mpt::void_cast< const std::byte * >( data ), size ) ), ctls );
	apply_libopenmpt_defaults();
}
module_impl::~module_impl() {
	m_sndFile->Destroy();
}

std::int32_t module_impl::get_render_param( int param ) const {
	std::int32_t result = 0;
	switch ( param ) {
		case module::RENDER_MASTERGAIN_MILLIBEL: {
			result = static_cast<std::int32_t>( 1000.0f * 2.0f * std::log10( m_Gain ) );
		} break;
		case module::RENDER_STEREOSEPARATION_PERCENT: {
			result = m_sndFile->m_MixerSettings.m_nStereoSeparation * 100 / MixerSettings::StereoSeparationScale;
		} break;
		case module::RENDER_INTERPOLATIONFILTER_LENGTH: {
			result = resamplingmode_to_filterlength( m_sndFile->m_Resampler.m_Settings.SrcMode );
		} break;
		case module::RENDER_VOLUMERAMPING_STRENGTH: {
			int ramping = 0;
			mixersettings_to_ramping( ramping, m_sndFile->m_MixerSettings );
			result = ramping;
		} break;
		default: throw openmpt::exception("unknown render param"); break;
	}
	return result;
}
void module_impl::set_render_param( int param, std::int32_t value ) {
	switch ( param ) {
		case module::RENDER_MASTERGAIN_MILLIBEL: {
			m_Gain = static_cast<float>( std::pow( 10.0f, value * 0.001f * 0.5f ) );
		} break;
		case module::RENDER_STEREOSEPARATION_PERCENT: {
			std::int32_t newvalue = value * MixerSettings::StereoSeparationScale / 100;
			if ( newvalue != static_cast<std::int32_t>( m_sndFile->m_MixerSettings.m_nStereoSeparation ) ) {
				MixerSettings settings = m_sndFile->m_MixerSettings;
				settings.m_nStereoSeparation = newvalue;
				m_sndFile->SetMixerSettings( settings );
			}
		} break;
		case module::RENDER_INTERPOLATIONFILTER_LENGTH: {
			CResamplerSettings newsettings = m_sndFile->m_Resampler.m_Settings;
			newsettings.SrcMode = filterlength_to_resamplingmode( value );
			if ( newsettings != m_sndFile->m_Resampler.m_Settings ) {
				m_sndFile->SetResamplerSettings( newsettings );
			}
		} break;
		case module::RENDER_VOLUMERAMPING_STRENGTH: {
			MixerSettings newsettings = m_sndFile->m_MixerSettings;
			ramping_to_mixersettings( newsettings, value );
			if ( m_sndFile->m_MixerSettings.VolumeRampUpMicroseconds != newsettings.VolumeRampUpMicroseconds || m_sndFile->m_MixerSettings.VolumeRampDownMicroseconds != newsettings.VolumeRampDownMicroseconds ) {
				m_sndFile->SetMixerSettings( newsettings );
			}
		} break;
		default: throw openmpt::exception("unknown render param"); break;
	}
}

std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * mono ) {
	if ( !mono ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 1 );
	count = read_wrapper( count, mono, nullptr, nullptr, nullptr );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right ) {
	if ( !left || !right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_wrapper( count, left, right, nullptr, nullptr );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right ) {
	if ( !left || !right || !rear_left || !rear_right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_wrapper( count, left, right, rear_left, rear_right );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * mono ) {
	if ( !mono ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 1 );
	count = read_wrapper( count, mono, nullptr, nullptr, nullptr );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * left, float * right ) {
	if ( !left || !right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_wrapper( count, left, right, nullptr, nullptr );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * rear_left, float * rear_right ) {
	if ( !left || !right || !rear_left || !rear_right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_wrapper( count, left, right, rear_left, rear_right );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_stereo( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_stereo ) {
	if ( !interleaved_stereo ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_interleaved_wrapper( count, 2, interleaved_stereo );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_quad( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_quad ) {
	if ( !interleaved_quad ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_interleaved_wrapper( count, 4, interleaved_quad );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_stereo( std::int32_t samplerate, std::size_t count, float * interleaved_stereo ) {
	if ( !interleaved_stereo ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_interleaved_wrapper( count, 2, interleaved_stereo );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_quad( std::int32_t samplerate, std::size_t count, float * interleaved_quad ) {
	if ( !interleaved_quad ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_interleaved_wrapper( count, 4, interleaved_quad );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}


double module_impl::get_duration_seconds() const {
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::make_unique<subsongs_type>( get_subsongs() );
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	if ( m_current_subsong == all_subsongs ) {
		// Play all subsongs consecutively.
		double total_duration = 0.0;
		for ( const auto & subsong : subsongs ) {
			total_duration += subsong.duration;
		}
		return total_duration;
	}
	return subsongs[m_current_subsong].duration;
}
void module_impl::select_subsong( std::int32_t subsong ) {
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::make_unique<subsongs_type>( get_subsongs() );
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	if ( subsong != all_subsongs && ( subsong < 0 || subsong >= static_cast<std::int32_t>( subsongs.size() ) ) ) {
		throw openmpt::exception("invalid subsong");
	}
	m_current_subsong = subsong;
	m_sndFile->m_SongFlags.set( SONG_PLAYALLSONGS, subsong == all_subsongs );
	if ( subsong == all_subsongs ) {
		subsong = 0;
	}
	m_sndFile->Order.SetSequence( static_cast<SEQUENCEINDEX>( subsongs[subsong].sequence ) );
	set_position_order_row( subsongs[subsong].start_order, subsongs[subsong].start_row );
	m_currentPositionSeconds = 0.0;
}
std::int32_t module_impl::get_selected_subsong() const {
	return m_current_subsong;
}
void module_impl::set_repeat_count( std::int32_t repeat_count ) {
	m_sndFile->SetRepeatCount( repeat_count );
}
std::int32_t module_impl::get_repeat_count() const {
	return m_sndFile->GetRepeatCount();
}
double module_impl::get_position_seconds() const {
	return m_currentPositionSeconds;
}
double module_impl::set_position_seconds( double seconds ) {
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::make_unique<subsongs_type>( get_subsongs() );
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	const subsong_data * subsong = 0;
	double base_seconds = 0.0;
	if ( m_current_subsong == all_subsongs ) {
		// When playing all subsongs, find out which subsong this time would belong to.
		subsong = &subsongs.back();
		for ( std::size_t i = 0; i < subsongs.size(); ++i ) {
			if ( base_seconds + subsongs[i].duration > seconds ) {
				subsong = &subsongs[i];
				break;
			}
			base_seconds += subsong->duration;
		}
		seconds -= base_seconds;
	} else {
		subsong = &subsongs[m_current_subsong];
	}
	GetLengthType t = m_sndFile->GetLength( m_ctl_seek_sync_samples ? eAdjustSamplePositions : eAdjust, GetLengthTarget( seconds ).StartPos( static_cast<SEQUENCEINDEX>( subsong->sequence ), static_cast<ORDERINDEX>( subsong->start_order ), static_cast<ROWINDEX>( subsong->start_row ) ) ).back();
	m_sndFile->m_PlayState.m_nCurrentOrder = t.lastOrder;
	m_sndFile->SetCurrentOrder( t.lastOrder );
	m_sndFile->m_PlayState.m_nNextRow = t.lastRow;
	m_currentPositionSeconds = base_seconds + t.duration;
	return m_currentPositionSeconds;
}
double module_impl::set_position_order_row( std::int32_t order, std::int32_t row ) {
	if ( order < 0 || order >= m_sndFile->Order().GetLengthTailTrimmed() ) {
		return m_currentPositionSeconds;
	}
	PATTERNINDEX pattern = m_sndFile->Order()[order];
	if ( m_sndFile->Patterns.IsValidIndex( pattern ) ) {
		if ( row < 0 || row >= static_cast<std::int32_t>( m_sndFile->Patterns[pattern].GetNumRows() ) ) {
			return m_currentPositionSeconds;
		}
	} else {
		row = 0;
	}
	m_sndFile->m_PlayState.m_nCurrentOrder = static_cast<ORDERINDEX>( order );
	m_sndFile->SetCurrentOrder( static_cast<ORDERINDEX>( order ) );
	m_sndFile->m_PlayState.m_nNextRow = static_cast<ROWINDEX>( row );
	m_currentPositionSeconds = m_sndFile->GetLength( m_ctl_seek_sync_samples ? eAdjustSamplePositions : eAdjust, GetLengthTarget( static_cast<ORDERINDEX>( order ), static_cast<ROWINDEX>( row ) ) ).back().duration;
	return m_currentPositionSeconds;
}
std::vector<std::string> module_impl::get_metadata_keys() const {
	return
	{
		"type",
		"type_long",
		"originaltype",
		"originaltype_long",
		"container",
		"container_long",
		"tracker",
		"artist",
		"title",
		"date",
		"message",
		"message_raw",
		"warnings",
	};
}
std::string module_impl::get_message_instruments() const {
	std::string retval;
	std::string tmp;
	bool valid = false;
	for ( INSTRUMENTINDEX i = 1; i <= m_sndFile->GetNumInstruments(); ++i ) {
		std::string instname = m_sndFile->GetInstrumentName( i );
		if ( !instname.empty() ) {
			valid = true;
		}
		tmp += instname;
		tmp += "\n";
	}
	if ( valid ) {
		retval = tmp;
	}
	return retval;
}
std::string module_impl::get_message_samples() const {
	std::string retval;
	std::string tmp;
	bool valid = false;
	for ( SAMPLEINDEX i = 1; i <= m_sndFile->GetNumSamples(); ++i ) {
		std::string samplename = m_sndFile->GetSampleName( i );
		if ( !samplename.empty() ) {
			valid = true;
		}
		tmp += samplename;
		tmp += "\n";
	}
	if ( valid ) {
		retval = tmp;
	}
	return retval;
}
std::string module_impl::get_metadata( const std::string & key ) const {
	if ( key == std::string("type") ) {
		return mpt::ToCharset(mpt::Charset::UTF8, m_sndFile->m_modFormat.type );
	} else if ( key == std::string("type_long") ) {
		return mpt::ToCharset(mpt::Charset::UTF8, m_sndFile->m_modFormat.formatName );
	} else if ( key == std::string("originaltype") ) {
		return mpt::ToCharset(mpt::Charset::UTF8, m_sndFile->m_modFormat.originalType );
	} else if ( key == std::string("originaltype_long") ) {
		return mpt::ToCharset(mpt::Charset::UTF8, m_sndFile->m_modFormat.originalFormatName );
	} else if ( key == std::string("container") ) {
		return mpt::ToCharset(mpt::Charset::UTF8, CSoundFile::ModContainerTypeToString( m_sndFile->GetContainerType() ) );
	} else if ( key == std::string("container_long") ) {
		return mpt::ToCharset(mpt::Charset::UTF8, CSoundFile::ModContainerTypeToTracker( m_sndFile->GetContainerType() ) );
	} else if ( key == std::string("tracker") ) {
		return mpt::ToCharset(mpt::Charset::UTF8, m_sndFile->m_modFormat.madeWithTracker );
	} else if ( key == std::string("artist") ) {
		return mpt::ToCharset( mpt::Charset::UTF8, m_sndFile->m_songArtist );
	} else if ( key == std::string("title") ) {
		return mod_string_to_utf8( m_sndFile->GetTitle() );
	} else if ( key == std::string("date") ) {
		if ( m_sndFile->GetFileHistory().empty() || !m_sndFile->GetFileHistory().back().HasValidDate() ) {
			return std::string();
		}
		return mpt::ToCharset(mpt::Charset::UTF8, m_sndFile->GetFileHistory().back().AsISO8601() );
	} else if ( key == std::string("message") ) {
		std::string retval = m_sndFile->m_songMessage.GetFormatted( SongMessage::leLF );
		if ( retval.empty() ) {
			switch ( m_sndFile->GetMessageHeuristic() ) {
				case ModMessageHeuristicOrder::Instruments:
					retval = get_message_instruments();
					break;
				case ModMessageHeuristicOrder::Samples:
					retval = get_message_samples();
					break;
				case ModMessageHeuristicOrder::InstrumentsSamples:
					if ( retval.empty() ) {
						retval = get_message_instruments();
					}
					if ( retval.empty() ) {
						retval = get_message_samples();
					}
					break;
				case ModMessageHeuristicOrder::SamplesInstruments:
					if ( retval.empty() ) {
						retval = get_message_samples();
					}
					if ( retval.empty() ) {
						retval = get_message_instruments();
					}
					break;
				case ModMessageHeuristicOrder::BothInstrumentsSamples:
					{
						std::string message_instruments = get_message_instruments();
						std::string message_samples = get_message_samples();
						if ( !message_instruments.empty() ) {
							retval += std::move( message_instruments );
						}
						if ( !message_samples.empty() ) {
							retval += std::move( message_samples );
						}
					}
					break;
				case ModMessageHeuristicOrder::BothSamplesInstruments:
					{
						std::string message_instruments = get_message_instruments();
						std::string message_samples = get_message_samples();
						if ( !message_samples.empty() ) {
							retval += std::move( message_samples );
						}
						if ( !message_instruments.empty() ) {
							retval += std::move( message_instruments );
						}
					}
					break;
			}
		}
		return mod_string_to_utf8( retval );
	} else if ( key == std::string("message_raw") ) {
		std::string retval = m_sndFile->m_songMessage.GetFormatted( SongMessage::leLF );
		return mod_string_to_utf8( retval );
	} else if ( key == std::string("warnings") ) {
		std::string retval;
		bool first = true;
		for ( const auto & msg : m_loaderMessages ) {
			if ( !first ) {
				retval += "\n";
			} else {
				first = false;
			}
			retval += msg;
		}
		return retval;
	}
	return "";
}

double module_impl::get_current_estimated_bpm() const {
	return m_sndFile->GetCurrentBPM();
}
std::int32_t module_impl::get_current_speed() const {
	return m_sndFile->m_PlayState.m_nMusicSpeed;
}
std::int32_t module_impl::get_current_tempo() const {
	return static_cast<std::int32_t>( m_sndFile->m_PlayState.m_nMusicTempo.GetInt() );
}
std::int32_t module_impl::get_current_order() const {
	return m_sndFile->GetCurrentOrder();
}
std::int32_t module_impl::get_current_pattern() const {
	std::int32_t order = m_sndFile->GetCurrentOrder();
	if ( order < 0 || order >= m_sndFile->Order().GetLengthTailTrimmed() ) {
		return m_sndFile->GetCurrentPattern();
	}
	std::int32_t pattern = m_sndFile->Order()[order];
	if ( !m_sndFile->Patterns.IsValidIndex( static_cast<PATTERNINDEX>( pattern ) ) ) {
		return -1;
	}
	return pattern;
}
std::int32_t module_impl::get_current_row() const {
	return m_sndFile->m_PlayState.m_nRow;
}
std::int32_t module_impl::get_current_playing_channels() const {
	return m_sndFile->GetMixStat();
}

float module_impl::get_current_channel_vu_mono( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	const float left = m_sndFile->m_PlayState.Chn[channel].nLeftVU * (1.0f/128.0f);
	const float right = m_sndFile->m_PlayState.Chn[channel].nRightVU * (1.0f/128.0f);
	return std::sqrt(left*left + right*right);
}
float module_impl::get_current_channel_vu_left( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? 0.0f : m_sndFile->m_PlayState.Chn[channel].nLeftVU * (1.0f/128.0f);
}
float module_impl::get_current_channel_vu_right( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? 0.0f : m_sndFile->m_PlayState.Chn[channel].nRightVU * (1.0f/128.0f);
}
float module_impl::get_current_channel_vu_rear_left( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? m_sndFile->m_PlayState.Chn[channel].nLeftVU * (1.0f/128.0f) : 0.0f;
}
float module_impl::get_current_channel_vu_rear_right( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? m_sndFile->m_PlayState.Chn[channel].nRightVU * (1.0f/128.0f) : 0.0f;
}

std::int32_t module_impl::get_num_subsongs() const {
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::make_unique<subsongs_type>( get_subsongs() );
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	return static_cast<std::int32_t>( subsongs.size() );
}
std::int32_t module_impl::get_num_channels() const {
	return m_sndFile->GetNumChannels();
}
std::int32_t module_impl::get_num_orders() const {
	return m_sndFile->Order().GetLengthTailTrimmed();
}
std::int32_t module_impl::get_num_patterns() const {
	return m_sndFile->Patterns.GetNumPatterns();
}
std::int32_t module_impl::get_num_instruments() const {
	return m_sndFile->GetNumInstruments();
}
std::int32_t module_impl::get_num_samples() const {
	return m_sndFile->GetNumSamples();
}

std::vector<std::string> module_impl::get_subsong_names() const {
	std::vector<std::string> retval;
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::make_unique<subsongs_type>( get_subsongs() );
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	retval.reserve( subsongs.size() );
	for ( const auto & subsong : subsongs ) {
		const auto & order = m_sndFile->Order( static_cast<SEQUENCEINDEX>( subsong.sequence ) );
		retval.push_back( mpt::ToCharset( mpt::Charset::UTF8, order.GetName() ) );
		if ( retval.back().empty() ) {
			// use first pattern name instead
			if ( order.IsValidPat( static_cast<SEQUENCEINDEX>( subsong.start_order ) ) )
				retval.back() = mpt::ToCharset( mpt::Charset::UTF8, m_sndFile->GetCharsetInternal(), m_sndFile->Patterns[ order[ subsong.start_order ] ].GetName() );
		}
	}
	return retval;
}
std::vector<std::string> module_impl::get_channel_names() const {
	std::vector<std::string> retval;
	for ( CHANNELINDEX i = 0; i < m_sndFile->GetNumChannels(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->ChnSettings[i].szName ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_order_names() const {
	std::vector<std::string> retval;
	ORDERINDEX num_orders = m_sndFile->Order().GetLengthTailTrimmed();
	retval.reserve( num_orders );
	for ( ORDERINDEX i = 0; i < num_orders; ++i ) {
		PATTERNINDEX pat = m_sndFile->Order()[i];
		if ( m_sndFile->Patterns.IsValidIndex( pat ) ) {
			retval.push_back( mod_string_to_utf8( m_sndFile->Patterns[ m_sndFile->Order()[i] ].GetName() ) );
		} else {
			if ( pat == m_sndFile->Order.GetIgnoreIndex() ) {
				retval.push_back( "+++ skip" );
			} else if ( pat == m_sndFile->Order.GetInvalidPatIndex() ) {
				retval.push_back( "--- stop" );
			} else {
				retval.push_back( "???" );
			}
		}
	}
	return retval;
}
std::vector<std::string> module_impl::get_pattern_names() const {
	std::vector<std::string> retval;
	retval.reserve( m_sndFile->Patterns.GetNumPatterns() );
	for ( PATTERNINDEX i = 0; i < m_sndFile->Patterns.GetNumPatterns(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->Patterns[i].GetName() ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_instrument_names() const {
	std::vector<std::string> retval;
	retval.reserve( m_sndFile->GetNumInstruments() );
	for ( INSTRUMENTINDEX i = 1; i <= m_sndFile->GetNumInstruments(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->GetInstrumentName( i ) ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_sample_names() const {
	std::vector<std::string> retval;
	retval.reserve( m_sndFile->GetNumSamples() );
	for ( SAMPLEINDEX i = 1; i <= m_sndFile->GetNumSamples(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->GetSampleName( i ) ) );
	}
	return retval;
}

std::int32_t module_impl::get_order_pattern( std::int32_t o ) const {
	if ( o < 0 || o >= m_sndFile->Order().GetLengthTailTrimmed() ) {
		return -1;
	}
	return m_sndFile->Order()[o];
}
std::int32_t module_impl::get_pattern_num_rows( std::int32_t p ) const {
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return 0;
	}
	return m_sndFile->Patterns[p].GetNumRows();
}

std::uint8_t module_impl::get_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return 0;
	}
	const CPattern &pattern = m_sndFile->Patterns[p];
	if ( r < 0 || r >= static_cast<std::int32_t>( pattern.GetNumRows() ) ) {
		return 0;
	}
	if ( c < 0 || c >= m_sndFile->GetNumChannels() ) {
		return 0;
	}
	if ( cmd < module::command_note || cmd > module::command_parameter ) {
		return 0;
	}
	const ModCommand & cell = *pattern.GetpModCommand( static_cast<ROWINDEX>( r ), static_cast<CHANNELINDEX>( c ) );
	switch ( cmd ) {
		case module::command_note: return cell.note; break;
		case module::command_instrument: return cell.instr; break;
		case module::command_volumeffect: return cell.volcmd; break;
		case module::command_effect: return cell.command; break;
		case module::command_volume: return cell.vol; break;
		case module::command_parameter: return cell.param; break;
	}
	return 0;
}

/*

highlight chars explained:

  : empty/space
. : empty/dot
n : generic note
m : special note
i : generic instrument
u : generic volume column effect
v : generic volume column parameter
e : generic effect column effect
f : generic effect column parameter

*/

std::pair< std::string, std::string > module_impl::format_and_highlight_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return std::make_pair( std::string(), std::string() );
	}
	const CPattern &pattern = m_sndFile->Patterns[p];
	if ( r < 0 || r >= static_cast<std::int32_t>( pattern.GetNumRows() ) ) {
		return std::make_pair( std::string(), std::string() );
	}
	if ( c < 0 || c >= m_sndFile->GetNumChannels() ) {
		return std::make_pair( std::string(), std::string() );
	}
	if ( cmd < module::command_note || cmd > module::command_parameter ) {
		return std::make_pair( std::string(), std::string() );
	}
	const ModCommand & cell = *pattern.GetpModCommand( static_cast<ROWINDEX>( r ), static_cast<CHANNELINDEX>( c ) );
	switch ( cmd ) {
		case module::command_note:
			return std::make_pair(
					( cell.IsNote() || cell.IsSpecialNote() ) ? mpt::ToCharset( mpt::Charset::UTF8, m_sndFile->GetNoteName( cell.note, cell.instr ) ) : std::string("...")
				,
					( cell.IsNote() ) ? std::string("nnn") : cell.IsSpecialNote() ? std::string("mmm") : std::string("...")
				);
			break;
		case module::command_instrument:
			return std::make_pair(
					cell.instr ? mpt::fmt::HEX0<2>( cell.instr ) : std::string("..")
				,
					cell.instr ? std::string("ii") : std::string("..")
				);
			break;
		case module::command_volumeffect:
			return std::make_pair(
					cell.IsPcNote() ? std::string(" ") : cell.volcmd != VOLCMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetVolEffectLetter( cell.volcmd ) ) : std::string(" ")
				,
					cell.IsPcNote() ? std::string(" ") : cell.volcmd != VOLCMD_NONE ? std::string("u") : std::string(" ")
				);
			break;
		case module::command_volume:
			return std::make_pair(
					cell.IsPcNote() ? mpt::fmt::HEX0<2>( cell.GetValueVolCol() & 0xff ) : cell.volcmd != VOLCMD_NONE ? mpt::fmt::HEX0<2>( cell.vol ) : std::string("..")
				,
					cell.IsPcNote() ? std::string("vv") : cell.volcmd != VOLCMD_NONE ? std::string("vv") : std::string("..")
				);
			break;
		case module::command_effect:
			return std::make_pair(
					cell.IsPcNote() ? mpt::fmt::HEX0<1>( ( cell.GetValueEffectCol() & 0x0f00 ) > 16 ) : cell.command != CMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetEffectLetter( cell.command ) ) : std::string(".")
				,
					cell.IsPcNote() ? std::string("e") : cell.command != CMD_NONE ? std::string("e") : std::string(".")
				);
			break;
		case module::command_parameter:
			return std::make_pair(
					cell.IsPcNote() ? mpt::fmt::HEX0<2>( cell.GetValueEffectCol() & 0x00ff ) : cell.command != CMD_NONE ? mpt::fmt::HEX0<2>( cell.param ) : std::string("..")
				,
					cell.IsPcNote() ? std::string("ff") : cell.command != CMD_NONE ? std::string("ff") : std::string("..")
				);
			break;
	}
	return std::make_pair( std::string(), std::string() );
}
std::string module_impl::format_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	return format_and_highlight_pattern_row_channel_command( p, r, c, cmd ).first;
}
std::string module_impl::highlight_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	return format_and_highlight_pattern_row_channel_command( p, r, c, cmd ).second;
}

std::pair< std::string, std::string > module_impl::format_and_highlight_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const {
	std::string text = pad ? std::string( width, ' ' ) : std::string();
	std::string high = pad ? std::string( width, ' ' ) : std::string();
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return std::make_pair( text, high );
	}
	const CPattern &pattern = m_sndFile->Patterns[p];
	if ( r < 0 || r >= static_cast<std::int32_t>( pattern.GetNumRows() ) ) {
		return std::make_pair( text, high );
	}
	if ( c < 0 || c >= m_sndFile->GetNumChannels() ) {
		return std::make_pair( text, high );
	}
	//  0000000001111
	//  1234567890123
	// "NNN IIvVV EFF"
	const ModCommand & cell = *pattern.GetpModCommand( static_cast<ROWINDEX>( r ), static_cast<CHANNELINDEX>( c ) );
	text.clear();
	high.clear();
	text += ( cell.IsNote() || cell.IsSpecialNote() ) ? mpt::ToCharset( mpt::Charset::UTF8, m_sndFile->GetNoteName( cell.note, cell.instr ) ) : std::string("...");
	high += ( cell.IsNote() ) ? std::string("nnn") : cell.IsSpecialNote() ? std::string("mmm") : std::string("...");
	if ( ( width == 0 ) || ( width >= 6 ) ) {
		text += std::string(" ");
		high += std::string(" ");
		text += cell.instr ? mpt::fmt::HEX0<2>( cell.instr ) : std::string("..");
		high += cell.instr ? std::string("ii") : std::string("..");
	}
	if ( ( width == 0 ) || ( width >= 9 ) ) {
		text += cell.IsPcNote() ? std::string(" ") + mpt::fmt::HEX0<2>( cell.GetValueVolCol() & 0xff ) : cell.volcmd != VOLCMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetVolEffectLetter( cell.volcmd ) ) + mpt::fmt::HEX0<2>( cell.vol ) : std::string(" ..");
		high += cell.IsPcNote() ? std::string(" vv") : cell.volcmd != VOLCMD_NONE ? std::string("uvv") : std::string(" ..");
	}
	if ( ( width == 0 ) || ( width >= 13 ) ) {
		text += std::string(" ");
		high += std::string(" ");
		text += cell.IsPcNote() ? mpt::fmt::HEX0<3>( cell.GetValueEffectCol() & 0x0fff ) : cell.command != CMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetEffectLetter( cell.command ) ) + mpt::fmt::HEX0<2>( cell.param ) : std::string("...");
		high += cell.IsPcNote() ? std::string("eff") : cell.command != CMD_NONE ? std::string("eff") : std::string("...");
	}
	if ( ( width != 0 ) && ( text.length() > width ) ) {
		text = text.substr( 0, width );
	} else if ( ( width != 0 ) && pad ) {
		text += std::string( width - text.length(), ' ' );
	}
	if ( ( width != 0 ) && ( high.length() > width ) ) {
		high = high.substr( 0, width );
	} else if ( ( width != 0 ) && pad ) {
		high += std::string( width - high.length(), ' ' );
	}
	return std::make_pair( text, high );
}
std::string module_impl::format_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const {
	return format_and_highlight_pattern_row_channel( p, r, c, width, pad ).first;
}
std::string module_impl::highlight_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const {
	return format_and_highlight_pattern_row_channel( p, r, c, width, pad ).second;
}

std::pair<const module_impl::ctl_info *, const module_impl::ctl_info *> module_impl::get_ctl_infos() const {
	static constexpr ctl_info ctl_infos[] = {
		{ "load.skip_samples", ctl_type::boolean },
		{ "load.skip_patterns", ctl_type::boolean },
		{ "load.skip_plugins", ctl_type::boolean },
		{ "load.skip_subsongs_init", ctl_type::boolean },
		{ "seek.sync_samples", ctl_type::boolean },
		{ "subsong", ctl_type::integer },
		{ "play.tempo_factor", ctl_type::floatingpoint },
		{ "play.pitch_factor", ctl_type::floatingpoint },
		{ "play.at_end", ctl_type::text },
		{ "render.resampler.emulate_amiga", ctl_type::boolean },
		{ "render.resampler.emulate_amiga_type", ctl_type::text },
		{ "render.opl.volume_factor", ctl_type::floatingpoint },
		{ "dither", ctl_type::integer }
	};
	return std::make_pair(std::begin(ctl_infos), std::end(ctl_infos));
}

std::vector<std::string> module_impl::get_ctls() const {
	std::vector<std::string> result;
	auto ctl_infos = get_ctl_infos();
	result.reserve(std::distance(ctl_infos.first, ctl_infos.second));
	for ( std::ptrdiff_t i = 0; i < std::distance(ctl_infos.first, ctl_infos.second); ++i ) {
		result.push_back(ctl_infos.first[i].name);
	}
	return result;
}

std::string module_impl::ctl_get( std::string ctl, bool throw_if_unknown ) const {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl");
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + ctl);
		} else {
			return std::string();
		}
	}
	std::string result;
	switch ( found_ctl->type ) {
		case ctl_type::boolean:
			return mpt::fmt::val( ctl_get_boolean( ctl, throw_if_unknown ) );
			break;
		case ctl_type::integer:
			return mpt::fmt::val( ctl_get_integer( ctl, throw_if_unknown ) );
			break;
		case ctl_type::floatingpoint:
			return mpt::fmt::val( ctl_get_floatingpoint( ctl, throw_if_unknown ) );
			break;
		case ctl_type::text:
			return ctl_get_text( ctl, throw_if_unknown );
			break;
	}
	return result;
}
bool module_impl::ctl_get_boolean( std::string_view ctl, bool throw_if_unknown ) const {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl");
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl));
		} else {
			return false;
		}
	}
	if ( found_ctl->type != ctl_type::boolean ) {
		throw openmpt::exception("wrong ctl value type");
	}
	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl");
	} else if ( ctl == "load.skip_samples" || ctl == "load_skip_samples" ) {
		return m_ctl_load_skip_samples;
	} else if ( ctl == "load.skip_patterns" || ctl == "load_skip_patterns" ) {
		return m_ctl_load_skip_patterns;
	} else if ( ctl == "load.skip_plugins" ) {
		return m_ctl_load_skip_plugins;
	} else if ( ctl == "load.skip_subsongs_init" ) {
		return m_ctl_load_skip_subsongs_init;
	} else if ( ctl == "seek.sync_samples" ) {
		return m_ctl_seek_sync_samples;
	} else if ( ctl == "render.resampler.emulate_amiga" ) {
		return ( m_sndFile->m_Resampler.m_Settings.emulateAmiga != Resampling::AmigaFilter::Off );
	} else {
		MPT_ASSERT_NOTREACHED();
		return false;
	}
}
std::int64_t module_impl::ctl_get_integer( std::string_view ctl, bool throw_if_unknown ) const {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl");
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl));
		} else {
			return 0;
		}
	}
	if ( found_ctl->type != ctl_type::integer ) {
		throw openmpt::exception("wrong ctl value type");
	}
	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl");
	} else if ( ctl == "subsong" ) {
		return get_selected_subsong();
	} else if ( ctl == "dither" ) {
		return static_cast<int>( m_Dither->GetMode() );
	} else {
		MPT_ASSERT_NOTREACHED();
		return 0;
	}
}
double module_impl::ctl_get_floatingpoint( std::string_view ctl, bool throw_if_unknown ) const {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl");
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl));
		} else {
			return 0.0;
		}
	}
	if ( found_ctl->type != ctl_type::floatingpoint ) {
		throw openmpt::exception("wrong ctl value type");
	}
	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl");
	} else if ( ctl == "play.tempo_factor" ) {
		if ( !is_loaded() ) {
			return 1.0;
		}
		return 65536.0 / m_sndFile->m_nTempoFactor;
	} else if ( ctl == "play.pitch_factor" ) {
		if ( !is_loaded() ) {
			return 1.0;
		}
		return m_sndFile->m_nFreqFactor / 65536.0;
	} else if ( ctl == "render.opl.volume_factor" ) {
		return static_cast<double>( m_sndFile->m_OPLVolumeFactor ) / static_cast<double>( m_sndFile->m_OPLVolumeFactorScale );
	} else {
		MPT_ASSERT_NOTREACHED();
		return 0.0;
	}
}
std::string module_impl::ctl_get_text( std::string_view ctl, bool throw_if_unknown ) const {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl");
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl));
		} else {
			return std::string();
		}
	}
	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl");
	} else if ( ctl == "play.at_end" ) {
		switch ( m_ctl_play_at_end )
		{
		case song_end_action::fadeout_song:
			return "fadeout";
		case song_end_action::continue_song:
			return "continue";
		case song_end_action::stop_song:
			return "stop";
		default:
			return std::string();
		}
	} else if ( ctl == "render.resampler.emulate_amiga_type" ) {
		switch ( m_ctl_render_resampler_emulate_amiga_type ) {
			case amiga_filter_type::a500:
				return "a500";
			case amiga_filter_type::a1200:
				return "a1200";
			case amiga_filter_type::unfiltered:
				return "unfiltered";
			case amiga_filter_type::auto_filter:
				return "auto";
			default:
				return std::string();
		}
	} else {
		MPT_ASSERT_NOTREACHED();
		return std::string();
	}
}

void module_impl::ctl_set( std::string ctl, const std::string & value, bool throw_if_unknown ) {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl: := " + value);
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + ctl + " := " + value);
		} else {
			return;
		}
	}
	switch ( found_ctl->type ) {
		case ctl_type::boolean:
			ctl_set_boolean( ctl, ConvertStrTo<bool>( value ), throw_if_unknown );
			break;
		case ctl_type::integer:
			ctl_set_integer( ctl, ConvertStrTo<int64>( value ), throw_if_unknown );
			break;
		case ctl_type::floatingpoint:
			ctl_set_floatingpoint( ctl, ConvertStrTo<double>( value ), throw_if_unknown );
			break;
		case ctl_type::text:
			ctl_set_text( ctl, value, throw_if_unknown );
			break;
	}
}
void module_impl::ctl_set_boolean( std::string_view ctl, bool value, bool throw_if_unknown ) {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl: := " + mpt::fmt::val( value ) );
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl) + " := " + mpt::fmt::val(value));
		} else {
			return;
		}
	}
	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl: := " + mpt::fmt::val( value ) );
	} else if ( ctl == "load.skip_samples" || ctl == "load_skip_samples" ) {
		m_ctl_load_skip_samples = value;
	} else if ( ctl == "load.skip_patterns" || ctl == "load_skip_patterns" ) {
		m_ctl_load_skip_patterns = value;
	} else if ( ctl == "load.skip_plugins" ) {
		m_ctl_load_skip_plugins = value;
	} else if ( ctl == "load.skip_subsongs_init" ) {
		m_ctl_load_skip_subsongs_init = value;
	} else if ( ctl == "seek.sync_samples" ) {
		m_ctl_seek_sync_samples = value;
	} else if ( ctl == "render.resampler.emulate_amiga" ) {
		CResamplerSettings newsettings = m_sndFile->m_Resampler.m_Settings;
		const bool enabled = value;
		if ( enabled )
			newsettings.emulateAmiga = translate_amiga_filter_type( m_ctl_render_resampler_emulate_amiga_type );
		else
			newsettings.emulateAmiga = Resampling::AmigaFilter::Off;
		if ( newsettings != m_sndFile->m_Resampler.m_Settings ) {
			m_sndFile->SetResamplerSettings( newsettings );
		}
	} else {
		MPT_ASSERT_NOTREACHED();
	}
}
void module_impl::ctl_set_integer( std::string_view ctl, std::int64_t value, bool throw_if_unknown ) {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl: := " + mpt::fmt::val( value ) );
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl) + " := " + mpt::fmt::val(value));
		} else {
			return;
		}
	}

	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl: := " + mpt::fmt::val( value ) );
	} else if ( ctl == "subsong" ) {
		select_subsong( mpt::saturate_cast<int32>( value ) );
	} else if ( ctl == "dither" ) {
		int dither = mpt::saturate_cast<int>( value );
		if ( dither < 0 || dither >= NumDitherModes ) {
			dither = DitherDefault;
		}
		m_Dither->SetMode( static_cast<DitherMode>( dither ) );
	} else {
		MPT_ASSERT_NOTREACHED();
	}
}
void module_impl::ctl_set_floatingpoint( std::string_view ctl, double value, bool throw_if_unknown ) {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl: := " + mpt::fmt::val( value ) );
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl) + " := " + mpt::fmt::val(value));
		} else {
			return;
		}
	}

	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl: := " + mpt::fmt::val( value ) );
	} else if ( ctl == "play.tempo_factor" ) {
		if ( !is_loaded() ) {
			return;
		}
		double factor = value;
		if ( factor <= 0.0 || factor > 4.0 ) {
			throw openmpt::exception("invalid tempo factor");
		}
		m_sndFile->m_nTempoFactor = mpt::saturate_round<uint32_t>( 65536.0 / factor );
		m_sndFile->RecalculateSamplesPerTick();
	} else if ( ctl == "play.pitch_factor" ) {
		if ( !is_loaded() ) {
			return;
		}
		double factor = value;
		if ( factor <= 0.0 || factor > 4.0 ) {
			throw openmpt::exception("invalid pitch factor");
		}
		m_sndFile->m_nFreqFactor = mpt::saturate_round<uint32_t>( 65536.0 * factor );
		m_sndFile->RecalculateSamplesPerTick();
	} else if ( ctl == "render.opl.volume_factor" ) {
		m_sndFile->m_OPLVolumeFactor = mpt::saturate_round<int32>( value * static_cast<double>( m_sndFile->m_OPLVolumeFactorScale ) );
	} else {
		MPT_ASSERT_NOTREACHED();
	}
}
void module_impl::ctl_set_text( std::string_view ctl, std::string_view value, bool throw_if_unknown ) {
	if ( !ctl.empty() ) {
		// cppcheck false-positive
		// cppcheck-suppress containerOutOfBounds
		char rightmost = ctl.back();
		if ( rightmost == '!' || rightmost == '?' ) {
			if ( rightmost == '!' ) {
				throw_if_unknown = true;
			} else if ( rightmost == '?' ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	auto found_ctl = std::find_if(get_ctl_infos().first, get_ctl_infos().second, [&](const ctl_info & info) -> bool { return info.name == ctl; });
	if ( found_ctl == get_ctl_infos().second ) {
		if ( ctl == "" ) {
			throw openmpt::exception("empty ctl: := " + std::string( value ) );
		} else if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + std::string(ctl) + " := " + std::string(value));
		} else {
			return;
		}
	}

	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl: := " + std::string( value ) );
	} else if ( ctl == "play.at_end" ) {
		if ( value == "fadeout" ) {
			m_ctl_play_at_end = song_end_action::fadeout_song;
		} else if(value == "continue") {
			m_ctl_play_at_end = song_end_action::continue_song;
		} else if(value == "stop") {
			m_ctl_play_at_end = song_end_action::stop_song;
		} else {
			throw openmpt::exception("unknown song end action:" + std::string(value));
		}
	} else if ( ctl == "render.resampler.emulate_amiga_type" ) {
		if ( value == "a500" ) {
			m_ctl_render_resampler_emulate_amiga_type = amiga_filter_type::a500;
		} else if ( value == "a1200" ) {
			m_ctl_render_resampler_emulate_amiga_type = amiga_filter_type::a1200;
		} else if ( value == "unfiltered" ) {
			m_ctl_render_resampler_emulate_amiga_type = amiga_filter_type::unfiltered;
		} else if ( value == "auto" ) {
			m_ctl_render_resampler_emulate_amiga_type = amiga_filter_type::auto_filter;
		} else {
			throw openmpt::exception( "invalid amiga filter type" );
		}
		if ( m_sndFile->m_Resampler.m_Settings.emulateAmiga != Resampling::AmigaFilter::Off ) {
			CResamplerSettings newsettings = m_sndFile->m_Resampler.m_Settings;
			newsettings.emulateAmiga = translate_amiga_filter_type( m_ctl_render_resampler_emulate_amiga_type );
			if ( newsettings != m_sndFile->m_Resampler.m_Settings ) {
				m_sndFile->SetResamplerSettings( newsettings );
			}
		}
	} else {
		MPT_ASSERT_NOTREACHED();
	}
}

} // namespace openmpt
