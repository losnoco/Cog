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

#include "mpt/base/compiletime_warning.hpp"
#include "mpt/base/floatingpoint.hpp"
#include "mpt/base/preprocessor.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <string>

namespace openmpt123 {

struct exception : public openmpt::exception {
	exception( const std::string & text ) : openmpt::exception(text) { }
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
bool IsConsole( DWORD stdHandle );
#endif
bool IsTerminal( int fd );



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
protected:
	std::string pop() {
		std::string text = str();
		str(std::string());
		return text;
	}
public:
	virtual void writeout() = 0;
	virtual void cursor_up( std::size_t lines ) {
		static_cast<void>( lines );
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
	void writeout() override {
		static_cast<void>( pop() );
	}
};

class textout_ostream : public textout {
private:
	std::ostream & s;
#if defined(__DJGPP__)
	mpt::common_encoding codepage;
#endif
public:
	textout_ostream( std::ostream & s_ )
		: s(s_)
#if defined(__DJGPP__)
		, codepage(mpt::common_encoding::cp437)
#endif
	{
		#if defined(__DJGPP__)
			codepage = mpt::djgpp_get_locale_encoding();
		#endif
		return;
	}
	virtual ~textout_ostream() {
		writeout_impl();
	}
private:
	void writeout_impl() {
		std::string text = pop();
		if ( text.length() > 0 ) {
			#if defined(__DJGPP__)
				s << mpt::transcode<std::string>( codepage, mpt::common_encoding::utf8, text );
			#elif defined(__EMSCRIPTEN__)
				s << text;
			#else
				s << mpt::transcode<std::string>( mpt::logical_encoding::locale, mpt::common_encoding::utf8, text );
		#endif
			s.flush();
		}	
	}
public:
	void writeout() override {
		writeout_impl();
	}
	void cursor_up( std::size_t lines ) override {
		s.flush();
		for ( std::size_t line = 0; line < lines; ++line ) {
			*this << "\x1b[1A";
		}
	}
};

#if defined(WIN32)

class textout_ostream_console : public textout {
private:
#if defined(UNICODE)
	std::wostream & s;
#else
	std::ostream & s;
#endif
	HANDLE handle;
	bool console;
public:
#if defined(UNICODE)
	textout_ostream_console( std::wostream & s_, DWORD stdHandle_ )
#else
	textout_ostream_console( std::ostream & s_, DWORD stdHandle_ )
#endif
		: s(s_)
		, handle(GetStdHandle( stdHandle_ ))
		, console(IsConsole( stdHandle_ ))
	{
		return;
	}
	virtual ~textout_ostream_console() {
		writeout_impl();
	}
private:
	void writeout_impl() {
		std::string text = pop();
		if ( text.length() > 0 ) {
			if ( console ) {
				#if defined(UNICODE)
					std::wstring wtext = mpt::transcode<std::wstring>( mpt::common_encoding::utf8, text );
					WriteConsole( handle, wtext.data(), static_cast<DWORD>( wtext.size() ), NULL, NULL );
				#else
					std::string ltext = mpt::transcode<std::string>( mpt::logical_encoding::locale, mpt::common_encoding::utf8, text );
					WriteConsole( handle, ltext.data(), static_cast<DWORD>( ltext.size() ), NULL, NULL );
				#endif
			} else {
				#if defined(UNICODE)
					s << mpt::transcode<std::wstring>( mpt::common_encoding::utf8, text );
				#else
					s << mpt::transcode<std::string>( mpt::logical_encoding::locale, mpt::common_encoding::utf8, text );
				#endif
				s.flush();
			}
		}
	}
public:
	void writeout() override {
		writeout_impl();
	}
	void cursor_up( std::size_t lines ) override {
		if ( console ) {
			s.flush();
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			ZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
			COORD coord_cursor = COORD();
			if ( GetConsoleScreenBufferInfo( handle, &csbi ) != FALSE ) {
				coord_cursor = csbi.dwCursorPosition;
				coord_cursor.X = 1;
				coord_cursor.Y -= static_cast<SHORT>( lines );
				SetConsoleCursorPosition( handle, coord_cursor );
			}
		}
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

enum class Mode {
	None,
	Probe,
	Info,
	UI,
	Batch,
	Render
};

static inline std::string mode_to_string( Mode mode ) {
	switch ( mode ) {
		case Mode::None:   return "none"; break;
		case Mode::Probe:  return "probe"; break;
		case Mode::Info:   return "info"; break;
		case Mode::UI:     return "ui"; break;
		case Mode::Batch:  return "batch"; break;
		case Mode::Render: return "render"; break;
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
		mode = Mode::UI;
		ui_redraw_interval = default_high;
		driver = "";
		device = "";
		buffer = default_high;
		period = default_high;
#if defined(__DJGPP__)
		samplerate = 44100;
		channels = 2;
		use_float = false;
#else
		samplerate = 48000;
		channels = 2;
		use_float = mpt::float_traits<float>::is_hard && mpt::float_traits<float>::is_ieee754_binary;
#endif
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
#if defined(__DJGPP__)
		terminal_width = 80;
		terminal_height = 25;
#else
		terminal_width = 72;
		terminal_height = 23;
#endif
#if defined(WIN32)
		terminal_width = 72;
		terminal_height = 23;
		HANDLE hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
		if ( ( hStdOutput != NULL ) && ( hStdOutput != INVALID_HANDLE_VALUE ) ) {
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			ZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
			if ( GetConsoleScreenBufferInfo( hStdOutput, &csbi ) != FALSE ) {
				terminal_width = std::min( static_cast<int>( 1 + csbi.srWindow.Right - csbi.srWindow.Left ), static_cast<int>( csbi.dwSize.X ) );
				terminal_height = std::min( static_cast<int>( 1 + csbi.srWindow.Bottom - csbi.srWindow.Top ), static_cast<int>( csbi.dwSize.Y ) );
			}
		}
#else // WIN32
		if ( isatty( STDERR_FILENO ) ) {
			const char * env_columns = std::getenv( "COLUMNS" );
			if ( env_columns ) {
				std::istringstream istr( env_columns );
				int tmp = 0;
				istr >> tmp;
				if ( tmp > 0 ) {
					terminal_width = tmp;
				}
			}
			const char * env_rows = std::getenv( "ROWS" );
			if ( env_rows ) {
				std::istringstream istr( env_rows );
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
		output_extension = "auto";
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
		for ( const auto & filename : filenames ) {
			if ( filename == "-" ) {
				canUI = false;
			}
		}
		show_ui = canUI;
		if ( mode == Mode::None ) {
			if ( canUI ) {
				mode = Mode::UI;
			} else {
				mode = Mode::Batch;
			}
		}
		if ( mode == Mode::UI && !canUI ) {
			throw args_error_exception();
		}
		if ( show_progress && !canProgress ) {
			throw args_error_exception();
		}
		switch ( mode ) {
			case Mode::None:
				throw args_error_exception();
			break;
			case Mode::Probe:
				show_ui = false;
				show_progress = false;
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case Mode::Info:
				show_ui = false;
				show_progress = false;
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case Mode::UI:
			break;
			case Mode::Batch:
				show_meters = false;
				show_channel_meters = false;
				show_pattern = false;
			break;
			case Mode::Render:
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
		if ( output_extension == "auto" ) {
			output_extension = "";
		}
		if ( mode != Mode::Render && !output_extension.empty() ) {
			throw args_error_exception();
		}
		if ( mode == Mode::Render && !output_filename.empty() ) {
			throw args_error_exception();
		}
		if ( mode != Mode::Render && !output_filename.empty() ) {
			output_extension = get_extension( output_filename );
		}
		if ( output_extension.empty() ) {
			output_extension = "wav";
		}
	}
};

template < typename Tsample > Tsample convert_sample_to( float val );
template < > float convert_sample_to( float val ) {
	return val;
}
template < > std::int16_t convert_sample_to( float val ) {
	std::int32_t tmp = static_cast<std::int32_t>( val * 32768.0f );
	tmp = std::min( tmp, std::int32_t( 32767 ) );
	tmp = std::max( tmp, std::int32_t( -32768 ) );
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

class write_buffers_polling_wrapper : public write_buffers_interface {
protected:
	std::size_t channels;
	std::size_t sampleQueueMaxFrames;
	std::deque<float> sampleQueue;
protected:
	virtual ~write_buffers_polling_wrapper() {
		return;
	}
protected:
	write_buffers_polling_wrapper( const commandlineflags & flags )
		: channels(flags.channels)
		, sampleQueueMaxFrames(0)
	{
		return;
	}
	void set_queue_size_frames( std::size_t frames ) {
		sampleQueueMaxFrames = frames;
	}
	template < typename Tsample >
	Tsample pop_queue() {
		float val = 0.0f;
		if ( !sampleQueue.empty() ) {
			val = sampleQueue.front();
			sampleQueue.pop_front();
		}
		return convert_sample_to<Tsample>( val );
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( buffers[channel][frame] );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( buffers[channel][frame] * (1.0f/32768.0f) );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	virtual bool forward_queue() = 0;
	bool sleep( int ms ) override = 0;
};

class write_buffers_polling_wrapper_int : public write_buffers_interface {
protected:
	std::size_t channels;
	std::size_t sampleQueueMaxFrames;
	std::deque<std::int16_t> sampleQueue;
protected:
	virtual ~write_buffers_polling_wrapper_int() {
		return;
	}
protected:
	write_buffers_polling_wrapper_int( const commandlineflags & flags )
		: channels(flags.channels)
		, sampleQueueMaxFrames(0)
	{
		return;
	}
	void set_queue_size_frames( std::size_t frames ) {
		sampleQueueMaxFrames = frames;
	}
	std::int16_t pop_queue() {
		std::int16_t val = 0;
		if ( !sampleQueue.empty() ) {
			val = sampleQueue.front();
			sampleQueue.pop_front();
		}
		return val;
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( convert_sample_to<std::int16_t>( buffers[channel][frame] ) );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleQueue.push_back( buffers[channel][frame] );
			}
			while ( sampleQueue.size() >= sampleQueueMaxFrames * channels ) {
				while ( !forward_queue() ) {
					sleep( 1 );
				}
			}
		}
	}
	virtual bool forward_queue() = 0;
	bool sleep( int ms ) override = 0;
};

class void_audio_stream : public write_buffers_interface {
public:
	virtual ~void_audio_stream() {
		return;
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		(void)buffers;
		(void)frames;
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		(void)buffers;
		(void)frames;
	}
	bool is_dummy() const override {
		return true;
	}
};

class file_audio_stream_base : public write_buffers_interface {
protected:
	file_audio_stream_base() {
		return;
	}
public:
	void write_metadata( std::map<std::string,std::string> metadata ) override {
		(void)metadata;
		return;
	}
	void write_updated_metadata( std::map<std::string,std::string> metadata ) override {
		(void)metadata;
		return;
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) override = 0;
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override = 0;
	virtual ~file_audio_stream_base() {
		return;
	}
};

} // namespace openmpt123

#endif // OPENMPT123_HPP
