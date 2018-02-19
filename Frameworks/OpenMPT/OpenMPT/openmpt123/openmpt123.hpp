/*
 * openmpt123.hpp
 * --------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_HPP
#define OPENMPT123_HPP

#include "openmpt123_config.hpp"

namespace openmpt123 {

struct exception : public openmpt::exception {
	exception( const std::string & text ) throw() : openmpt::exception(text) { }
};

struct show_help_exception {
	std::string message;
	bool longhelp;
	show_help_exception( const std::string & msg = "", bool longhelp_ = true ) : message(msg), longhelp(longhelp_) { }
};

struct args_error_exception {
	args_error_exception() { }
};

struct show_help_keyboard_exception { };

#if defined(WIN32)

std::string wstring_to_utf8( const std::wstring & unicode_string ) {
	int required_size = WideCharToMultiByte( CP_UTF8, 0, unicode_string.c_str(), -1, NULL, 0, NULL, NULL );
	if ( required_size <= 0 ) {
		return std::string();
	}
	std::vector<char> utf8_buf( required_size );
	WideCharToMultiByte( CP_UTF8, 0, unicode_string.c_str(), -1, &utf8_buf[0], required_size, NULL, NULL );
	return &utf8_buf[0];
}

std::wstring utf8_to_wstring( const std::string & utf8_string ) {
	int required_size = MultiByteToWideChar( CP_UTF8, 0, utf8_string.c_str(), -1, NULL, 0 );
	if ( required_size <= 0 ) {
		return std::wstring();
	}
	std::vector<wchar_t> unicode_buf( required_size );
	MultiByteToWideChar( CP_UTF8, 0, utf8_string.data(), -1, &unicode_buf[0], required_size );
	return &unicode_buf[0];
}

std::string wstring_to_locale( const std::wstring & unicode_string ) {
	int required_size = WideCharToMultiByte( CP_ACP, 0, unicode_string.c_str(), -1, NULL, 0, NULL, NULL );
	if ( required_size <= 0 ) {
		return std::string();
	}
	std::vector<char> locale_buf( required_size );
	WideCharToMultiByte( CP_ACP, 0, unicode_string.c_str(), -1, &locale_buf[0], required_size, NULL, NULL );
	return &locale_buf[0];
}

std::wstring locale_to_wstring( const std::string & locale_string ) {
	int required_size = MultiByteToWideChar( CP_ACP, 0, locale_string.c_str(), -1, NULL, 0 );
	if ( required_size <= 0 ) {
		return std::wstring();
	}
	std::vector<wchar_t> unicode_buf( required_size );
	MultiByteToWideChar( CP_ACP, 0, locale_string.data(), -1, &unicode_buf[0], required_size );
	return &unicode_buf[0];
}

#endif // WIN32

#if defined(MPT_NEEDS_THREADS)

#if defined(WIN32)

class mutex {
private:
	CRITICAL_SECTION impl;
public:
	mutex() { InitializeCriticalSection(&impl); }
	~mutex() { DeleteCriticalSection(&impl); }
	void lock() { EnterCriticalSection(&impl); }
	void unlock() { LeaveCriticalSection(&impl); }
};

#else

class mutex {
private:
	pthread_mutex_t impl;
public:
	mutex() {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init( &attr );
		pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_NORMAL );
		pthread_mutex_init( &impl, &attr );
		pthread_mutexattr_destroy( &attr );
	}
	~mutex() { pthread_mutex_destroy( &impl ); }
	void lock() { pthread_mutex_lock( &impl ); }
	void unlock() { pthread_mutex_unlock( &impl ); }
};

#endif

#endif

struct field {
	std::string key;
	std::string val;
	field( const std::string & key )
		: key(key)
	{
		return;
	}
};

class textout : public std::ostringstream {
public:
	textout() {
		return;
	}
	virtual ~textout() {
		return;
	}
public:
	virtual void write( const std::string & text ) = 0;
	virtual void writeout() {
		write( str() );
		str(std::string());
	}
};

class textout_dummy : public textout {
public:
	textout_dummy() {
		return;
	}
	virtual ~textout_dummy() {
		return;
	}
public:
	virtual void write( const std::string & /* text */ ) {
		return;
	}
};

class textout_ostream : public textout {
private:
	std::ostream & s;
public:
	textout_ostream( std::ostream & s_ )
		: s(s_)
	{
		return;
	}
	virtual ~textout_ostream() {
		writeout();
	}
public:
	virtual void write( const std::string & text ) {
		s << text;
	}
	virtual void writeout() {
		textout::writeout();
		s.flush();
	}
};

#if defined(WIN32)

class textout_console : public textout {
private:
	HANDLE handle;
public:
	textout_console( HANDLE handle_ )
		: handle(handle_)
	{
		return;
	}
	virtual ~textout_console() {
		writeout();
	}
public:
	virtual void write( const std::string & text ) {
		#if defined(UNICODE)
			std::wstring wtext = utf8_to_wstring( text );
			WriteConsole( handle, wtext.data(), static_cast<DWORD>( wtext.size() ), NULL, NULL );
		#else
			WriteConsole( handle, text.data(), static_cast<DWORD>( text.size() ), NULL, NULL );
		#endif
	}
};

#endif // WIN32

static inline float mpt_round( float val ) {
	if ( val >= 0.0f ) {
		return std::floor( val + 0.5f );
	} else {
		return std::ceil( val - 0.5f );
	}
}

static inline long mpt_lround( float val ) {
	return static_cast< long >( mpt_round( val ) );
}

static inline std::string append_software_tag( std::string software ) {
	std::string openmpt123 = std::string() + "openmpt123 " + OPENMPT123_VERSION_STRING + " (libopenmpt " + openmpt::string::get( "library_version" ) + ", OpenMPT " + openmpt::string::get( "core_version" ) + ")";
	if ( software.empty() ) {
		software = openmpt123;
	} else {
		software += " (via " + openmpt123 + ")";
	}
	return software;
}

static inline std::string get_encoder_tag() {
	return std::string() + "openmpt123 " + OPENMPT123_VERSION_STRING + " (libopenmpt " + openmpt::string::get( "library_version" ) + ", OpenMPT " + openmpt::string::get( "core_version" ) + ")";
}

static inline std::string get_extension( std::string filename ) {
	if ( filename.find_last_of( "." ) != std::string::npos ) {
		return filename.substr( filename.find_last_of( "." ) + 1 );
	}
	return "";
}

bool IsTerminal( int fd );

enum Mode {
	ModeNone,
	ModeProbe,
	ModeInfo,
	ModeUI,
	ModeBatch,
	ModeRender
};

static inline std::string mode_to_string( Mode mode ) {
	switch ( mode ) {
		case ModeNone:   return "none"; break;
		case ModeProbe:  return "probe"; break;
		case ModeInfo:   return "info"; break;
		case ModeUI:     return "ui"; break;
		case ModeBatch:  return "batch"; break;
		case ModeRender: return "render"; break;
	}
	return "";
}

static const std::int32_t default_low = -2;
static const std::int32_t default_high = -1;

struct commandlineflags {
	Mode mode;
	bool canUI;
	std::int32_t ui_redraw_interval;
	bool canProgress;
	std::string driver;
	std::string device;
	std::int32_t buffer;
	std::int32_t period;
	std::int32_t samplerate;
	std::int32_t channels;
	std::int32_t gain;
	std::int32_t separation;
	std::int32_t filtertaps;
	std::int32_t ramping; // ramping strength : -1:default 0:off 1 2 3 4 5 // roughly milliseconds
	std::int32_t tempo;
	std::int32_t pitch;
	std::int32_t dither;
	std::int32_t repeatcount;
	std::int32_t subsong;
	std::map<std::string, std::string> ctls;
	double seek_target;
	double end_time;
	bool quiet;
	bool verbose;
	int terminal_width;
	int terminal_height;
	bool show_details;
	bool show_message;
	bool show_ui;
	bool show_progress;
	bool show_meters;
	bool show_channel_meters;
	bool show_pattern;
	bool use_float;
	bool use_stdout;
	bool randomize;
	bool shuffle;
	bool restart;
	std::size_t playlist_index;
	std::vector<std::string> filenames;
	std::string output_filename;
	std::string output_extension;
	bool force_overwrite;
	bool paused;
	std::string warnings;
	void apply_default_buffer_sizes() {
		if ( ui_redraw_interval == default_high ) {
			ui_redraw_interval = 50;
		} else if ( ui_redraw_interval == default_low ) {
			ui_redraw_interval = 10;
		}
		if ( buffer == default_high ) {
			buffer = 250;
		} else if ( buffer == default_low ) {
			buffer = 50;
		}
		if ( period == default_high ) {
			period = 50;
		} else if ( period == default_low ) {
			period = 10;
		}
	}
	commandlineflags() {
		mode = ModeUI;
		ui_redraw_interval = default_high;
		driver = "";
		device = "";
		buffer = default_high;
		period = default_high;
		samplerate = 48000;
		channels = 2;
		use_float = true;
		gain = 0;
		separation = 100;
		filtertaps = 8;
		ramping = -1;
		tempo = 0;
		pitch = 0;
		dither = 1;
		repeatcount = 0;
		subsong = -1;
		seek_target = 0.0;
		end_time = 0.0;
		quiet = false;
		verbose = false;
		terminal_width = 72;
		terminal_height = 23;
#if defined(WIN32)
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		ZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
		GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &csbi );
		terminal_width = csbi.dwSize.X - 1;
		terminal_height = 23; //csbi.dwSize.Y - 1;
#else // WIN32
		if ( isatty( STDERR_FILENO ) ) {
			if ( std::getenv( "COLUMNS" ) ) {
				std::istringstream istr( std::getenv( "COLUMNS" ) );
				int tmp = 0;
				istr >> tmp;
				if ( tmp > 0 ) {
					terminal_width = tmp;
				}
			}
			if ( std::getenv( "ROWS" ) ) {
				std::istringstream istr( std::getenv( "ROWS" ) );
				int tmp = 0;
				istr >> tmp;
				if ( tmp > 0 ) {
					terminal_height = tmp;
				}
			}
			#if defined(TIOCGWINSZ)
				struct winsize ts;
				if ( ioctl( STDERR_FILENO, TIOCGWINSZ, &ts ) >= 0 ) {
					terminal_width = ts.ws_col;
					terminal_height = ts.ws_row;
				}
			#elif defined(TIOCGSIZE)
				struct ttysize ts;
				if ( ioctl( STDERR_FILENO, TIOCGSIZE, &ts ) >= 0 ) {
					terminal_width = ts.ts_cols;
					terminal_height = ts.ts_rows;
				}
			#endif
		}
#endif
		show_details = true;
		show_message = false;
#if defined(WIN32)
		canUI = IsTerminal( 0 ) ? true : false;
		canProgress = IsTerminal( 2 ) ? true : false;
#else // !WIN32
		canUI = isatty( STDIN_FILENO ) ? true : false;
		canProgress = isatty( STDERR_FILENO ) ? true : false;
#endif // WIN32
		show_ui = canUI;
		show_progress = canProgress;
		show_meters = canUI && canProgress;
		show_channel_meters = false;
		show_pattern = false;
		use_stdout = false;
		randomize = false;
		shuffle = false;
		restart = false;
		playlist_index = 0;
		output_extension = "wav";
		force_overwrite = false;
		paused = false;
	}
	void check_and_sanitize() {
		if ( filenames.size() == 0 ) {
			throw args_error_exception();
		}
		if ( use_stdout && ( device != commandlineflags().device || !output_filename.empty() ) ) {
			throw args_error_exception();
		}
		if ( !output_filename.empty() && ( device != commandlineflags().device || use_stdout ) ) {
			throw args_error_exception();
		}
		for ( std::vector<std::string>::iterator i = filenames.begin(); i != filenames.end(); ++i ) {
			if ( *i == "-" ) {
				canUI = false;
			}
		}
		show_ui = canUI;
		if ( mode == ModeNone ) {
			if ( canUI ) {
				mode = ModeUI;
			} else {
				mode = ModeBatch;
			}
		}
		if ( mode == ModeUI && !canUI ) {
			throw args_error_exception();
		}
		if ( show_progress && !canProgress ) {
			throw args_error_exception();
		}
		switch ( mode ) {
			case ModeNone:
				throw args_error_exception();
			break;
			case ModeProbe:
				show_ui = false;
				show_progress = false;
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case ModeInfo:
				show_ui = false;
				show_progress = false;
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case ModeUI:
			break;
			case ModeBatch:
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case ModeRender:
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
				show_ui = false;
			break;
		}
		if ( quiet ) {
			verbose = false;
			show_ui = false;
			show_details = false;
			show_progress = false;
			show_channel_meters = false;
		}
		if ( verbose ) {
			show_details = true;
		}
		if ( channels != 1 && channels != 2 && channels != 4 ) {
			channels = commandlineflags().channels;
		}
		if ( samplerate < 0 ) {
			samplerate = commandlineflags().samplerate;
		}
		if ( mode == ModeRender && !output_filename.empty() ) {
			std::ostringstream warning;
			warning << "Warning: --output is deprecated in --render mode. Use --output-type instead." << std::endl;
			warnings += warning.str();
		}
		if ( mode != ModeRender && output_extension != "wav" ) {
			std::ostringstream warning;
			warning << "Warning: --output-type is deprecated in modes other than --render. Use --output instead." << std::endl;
			warnings += warning.str();
		}
		if ( !output_filename.empty() ) {
			output_extension = get_extension( output_filename );
		}
		if ( mode == ModeRender && output_extension.empty() ) {
			throw args_error_exception();
		}
	}
};

template < typename Tsample > Tsample convert_sample_to( float val );
template < > float convert_sample_to( float val ) {
	return val;
}
template < > std::int16_t convert_sample_to( float val ) {
	std::int32_t tmp = static_cast<std::int32_t>( val * 32768.0f );
	tmp = std::min( tmp, 32767 );
	tmp = std::max( tmp, -32768 );
	return static_cast<std::int16_t>( tmp );
}

class write_buffers_interface {
protected:
	virtual ~write_buffers_interface() {
		return;
	}
public:
	virtual void write_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write_updated_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) = 0;
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) = 0;
	virtual bool pause() {
		return false;
	}
	virtual bool unpause() {
		return false;
	}
	virtual bool sleep( int /*ms*/ ) {
		return false;
	}
	virtual bool is_dummy() const {
		return false;
	}
};

class write_buffers_blocking_wrapper : public write_buffers_interface {
protected:
	std::size_t channels;
	std::size_t sampleQueueMaxFrames;
	std::deque<float> sampleQueue;
protected:
	virtual ~write_buffers_blocking_wrapper() {
		return;
	}
protected:
	write_buffers_blocking_wrapper( const commandlineflags & flags )
		: channels(flags.channels)
		, sampleQueueMaxFrames(0)
	{
		return;
	}
	void set_queue_size_frames( std::size_t frames ) {
		sampleQueueMaxFrames = frames;
	}
	template < typename Tsample >
	float pop_queue() {
		float val = 0.0f;
		if ( !sampleQueue.empty() ) {
			val = sampleQueue.front();
			sampleQueue.pop_front();
		}
		return convert_sample_to<Tsample>( val );
	}
	template < typename Tsample >
	void fill_buffer( Tsample * buf, std::size_t framesToRender ) {
		for ( std::size_t frame = 0; frame < framesToRender; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				*buf = pop_queue<Tsample>();
				buf++;
			}
		}
	}
private:
	void wait_for_queue_space() {
		while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
			unlock();
			sleep( 1 );
			lock();
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		lock();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				wait_for_queue_space();
				sampleQueue.push_back( buffers[channel][frame] );
			}
		}
		unlock();
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		lock();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				wait_for_queue_space();
				sampleQueue.push_back( buffers[channel][frame] * (1.0f/32768.0f) );
			}
		}
		unlock();
	}
	virtual void lock() = 0;
	virtual void unlock() = 0;
	virtual bool sleep( int ms ) = 0;
};

class void_audio_stream : public write_buffers_interface {
public:
	virtual ~void_audio_stream() {
		return;
	}
public:
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) {
		(void)buffers;
		(void)frames;
	}
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		(void)buffers;
		(void)frames;
	}
	virtual bool is_dummy() const {
		return true;
	}
};

class file_audio_stream_base : public write_buffers_interface {
protected:
	file_audio_stream_base() {
		return;
	}
public:
	virtual void write_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write_updated_metadata( std::map<std::string,std::string> metadata ) {
		(void)metadata;
		return;
	}
	virtual void write( const std::vector<float*> buffers, std::size_t frames ) = 0;
	virtual void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) = 0;
	virtual ~file_audio_stream_base() {
		return;
	}
};

} // namespace openmpt123

#endif // OPENMPT123_HPP
