/*
 * openmpt123_allegro42.hpp
 * ------------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_ALLEGRO42_HPP
#define OPENMPT123_ALLEGRO42_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_ALLEGRO42)

#if defined(__GNUC__) && !defined(__clang__) && !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif
#include <allegro.h>
#if defined(__GNUC__) && !defined(__clang__) && !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif

namespace openmpt123 {

inline constexpr auto allegro42_encoding = mpt::logical_encoding::locale;

struct allegro42_exception : public exception {
	static mpt::ustring error_to_string() {
		try {
			return mpt::transcode<mpt::ustring>( allegro42_encoding, std::string( allegro_error ) );
		} catch ( const std::bad_alloc & ) {
			return mpt::ustring();
		}
	}
	allegro42_exception()
		: exception( error_to_string() )
	{
	}
};

class allegro42_raii {
public:
	allegro42_raii() {
		if ( allegro_init() != 0 ) {
			throw allegro42_exception();
		}
	}
	~allegro42_raii() {
		allegro_exit();
	}
};

class allegro42_sound_raii {
public:
	allegro42_sound_raii() {
		if ( install_sound( DIGI_AUTODETECT, MIDI_NONE, NULL ) != 0 ) {
			throw allegro42_exception();
		}
		if ( digi_card == DIGI_NONE ) {
			remove_sound();
			throw exception( MPT_USTRING("no audio device found") );
		}
	}
	~allegro42_sound_raii() {
		remove_sound();
	}
};

class allegro42_stream_raii : public write_buffers_polling_wrapper_int {
private:
	allegro42_raii allegro;
	allegro42_sound_raii allegro_sound;
	AUDIOSTREAM * stream;
	std::size_t bits;
	std::size_t channels;
	std::uint32_t period_frames;
private:
	std::uint32_t round_up_power2(std::uint32_t x)
	{
		std::uint32_t result = 1;
		while ( result < x ) {
			result *= 2;
		}
		return result;
	}
public:
	allegro42_stream_raii( commandlineflags & flags, concat_stream<mpt::ustring> & log )
		: write_buffers_polling_wrapper_int(flags)
		, stream(NULL)
		, bits(16)
		, channels(flags.channels)
		, period_frames(1024)
	{
		if ( flags.use_float ) {
			throw exception( MPT_USTRING("floating point is unsupported") );
		}
		if ( ( flags.channels != 1 ) && ( flags.channels != 2 ) ) {
			throw exception( MPT_USTRING("only mono or stereo supported") );
		}
		if ( flags.buffer == default_high ) {
			flags.buffer = 1024 * 2 * 1000 / flags.samplerate;
		} else if ( flags.buffer == default_low ) {
			flags.buffer = 512 * 2 * 1000 / flags.samplerate;
		}
		if ( flags.period == default_high ) {
			flags.period = 1024 / 2 * 1000 / flags.samplerate;
		} else if ( flags.period == default_low ) {
			flags.period = 512 / 2 * 1000 / flags.samplerate;
		}
		flags.apply_default_buffer_sizes();
		period_frames = round_up_power2( ( flags.buffer * flags.samplerate ) / ( 1000 * 2 ) );
		set_queue_size_frames( period_frames );
		if ( flags.verbose ) {
			log << MPT_USTRING("Allegro-4.2:") << lf;
			log << MPT_USTRING(" allegro samplerate: ") << get_mixer_frequency() << lf;
			log << MPT_USTRING(" latency: ") << flags.buffer << lf;
			log << MPT_USTRING(" period: ") << flags.period << lf;
			log << MPT_USTRING(" frames per buffer: ") << period_frames << lf;
			log << MPT_USTRING(" ui redraw: ") << flags.ui_redraw_interval << lf;
		}
		stream = play_audio_stream( period_frames, 16, ( flags.channels > 1 ) ? TRUE : FALSE, flags.samplerate, 255, 128 );
		if ( !stream ) {
			bits = 8;
			stream = play_audio_stream( period_frames, 8, ( flags.channels > 1 ) ? TRUE : FALSE, flags.samplerate, 255, 128 );
			if ( !stream ) {
				throw allegro42_exception();
			}
		}
	}
	~allegro42_stream_raii() {
		if ( stream ) {
			stop_audio_stream( stream );
			stream = NULL;
		}
	}
public:
	bool forward_queue() override {
		void * p = get_audio_stream_buffer( stream );
		if ( !p ) {
			return false;
		}
		for ( std::size_t frame = 0; frame < period_frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				std::int16_t sample = pop_queue();
				if ( bits == 8 ) {
					std::uint8_t u8sample = ( static_cast<std::uint16_t>( sample ) + 0x8000u ) >> 8;
					std::memcpy( reinterpret_cast<unsigned char *>( p ) + ( ( ( frame * channels ) + channel ) * sizeof( std::uint8_t ) ), &u8sample, sizeof( std::uint8_t ) );
				} else {
					std::uint16_t u16sample = static_cast<std::uint16_t>( sample ) + 0x8000u;
					std::memcpy( reinterpret_cast<unsigned char *>( p ) + ( ( ( frame * channels ) + channel ) * sizeof( std::uint16_t ) ), &u16sample, sizeof( std::uint16_t ) );
				}
			}
		}		
		free_audio_stream_buffer( stream );
		return true;
	}
	bool unpause() override {
		voice_start( stream->voice );
		return true;
	}
	bool pause() override {
		voice_stop( stream->voice );
		return true;
	}
	bool sleep( int ms ) override {
		rest( ms );
		return true;
	}
};

static mpt::ustring show_allegro42_devices( concat_stream<mpt::ustring> & /* log */ ) {
	string_concat_stream<mpt::ustring> devices;
	devices << MPT_USTRING(" allegro42:") << lf;
	devices << MPT_USTRING("    ") << MPT_USTRING("0") << MPT_USTRING(": Default Device") << lf;
	return devices.str();
}

} // namespace openmpt123

#endif // MPT_WITH_ALLEGRO42

#endif // OPENMPT123_ALLEGRO42_HPP
