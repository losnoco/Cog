/*
 * openmpt123_sdl.hpp
 * ------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_SDL_HPP
#define OPENMPT123_SDL_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_SDL)

#include <SDL.h>
#ifdef main
#undef main
#endif
#ifdef SDL_main
#undef SDL_main
#endif

namespace openmpt123 {

struct sdl_exception : public exception {
	sdl_exception( int /*code*/ ) throw() : exception( "SDL error" ) { }
};

class sdl_stream_raii : public write_buffers_blocking_wrapper {
private:
	std::ostream & log;
	std::size_t channels;
protected:
	void check_sdl_error( int e ) {
		if ( e < 0 ) {
			throw sdl_exception( e );
			return;
		}
	}
	std::uint32_t round_up_power2(std::uint32_t x)
	{
		std::uint32_t result = 1;
		while ( result < x ) {
			result *= 2;
		}
		return result;
	}
public:
	sdl_stream_raii( commandlineflags & flags, std::ostream & log_ )
		: write_buffers_blocking_wrapper(flags)
		, log(log_)
		, channels(flags.channels)
	{
		if ( flags.buffer == default_high ) {
			flags.buffer = 160;
		} else if ( flags.buffer == default_low ) {
			flags.buffer = 80;
		}
		if ( flags.period == default_high ) {
			flags.period = 20;
		} else if ( flags.period == default_low ) {
			flags.period = 10;
		}
		flags.apply_default_buffer_sizes();
		check_sdl_error( SDL_Init( SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO ) );
		SDL_AudioSpec audiospec;
		std::memset( &audiospec, 0, sizeof( SDL_AudioSpec ) );
		audiospec.freq = flags.samplerate;
		audiospec.format = AUDIO_S16SYS;
		audiospec.channels = flags.channels;
		audiospec.silence = 0;
		audiospec.samples = round_up_power2( ( flags.buffer * flags.samplerate ) / ( 1000 * 2 ) );
		audiospec.size = audiospec.samples * audiospec.channels * sizeof( std::int16_t );
		audiospec.callback = &sdl_callback_wrapper;
		audiospec.userdata = this;
		if ( flags.verbose ) {
			log << "SDL:" << std::endl;
			log << " latency: " << ( audiospec.samples * 2.0 / flags.samplerate ) << " (2 * " << audiospec.samples << ")" << std::endl;
			log << std::endl;
		}
		set_queue_size_frames( round_up_power2( ( flags.buffer * flags.samplerate ) / ( 1000 * 2 ) ) );
		check_sdl_error( SDL_OpenAudio( &audiospec, NULL ) );
		SDL_PauseAudio( 0 );
	}
	~sdl_stream_raii() {
		SDL_PauseAudio( 1 );
		SDL_CloseAudio();
		SDL_Quit();
	}
private:
	static void sdl_callback_wrapper( void * userdata, Uint8 * stream, int len ) {
		return reinterpret_cast<sdl_stream_raii*>( userdata )->sdl_callback( stream, len );
	}
	void sdl_callback( Uint8 * stream, int len ) {
		std::size_t framesToRender = len / sizeof( std::int16_t ) / channels;
		for ( std::size_t frame = 0; frame < framesToRender; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				std::int16_t sample = pop_queue<std::int16_t>();
				std::memcpy( stream, &sample, sizeof( std::int16_t ) );
				stream += sizeof( std::int16_t );
			}
		}		
	}
public:
	bool pause() {
		SDL_PauseAudio( 1 );
		return true;
	}
	bool unpause() {
		SDL_PauseAudio( 0 );
		return true;
	}
	void lock() {
		SDL_LockAudio();
	}
	void unlock() {
		SDL_UnlockAudio();
	}
	bool sleep( int ms ) {
		SDL_Delay( ms );
		return true;
	}
};

} // namespace openmpt123

#endif // MPT_WITH_SDL

#endif // OPENMPT123_SDL_HPP

