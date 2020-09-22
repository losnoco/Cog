/*
 * openmpt123_sdl2.hpp
 * -------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_SDL2_HPP
#define OPENMPT123_SDL2_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_SDL2)

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif // __clang__
#include <SDL.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
#ifdef main
#undef main
#endif
#ifdef SDL_main
#undef SDL_main
#endif
#if (SDL_COMPILEDVERSION < SDL_VERSIONNUM(2, 0, 4))
MPT_WARNING("Support for SDL2 < 2.0.4 has been deprecated and will be removed in a future openmpt123 version.")
#endif

namespace openmpt123 {

struct sdl2_exception : public exception {
private:
	static std::string text_from_code( int code ) {
		std::ostringstream s;
		s << code;
		return s.str();
	}
public:
	sdl2_exception( int code, const char * error ) : exception( text_from_code( code ) + " (" + ( error ? std::string(error) : std::string("NULL") ) + ")" ) { }
};

static void check_sdl2_error( int e ) {
	if ( e < 0 ) {
		throw sdl2_exception( e, SDL_GetError() );
	}
}

class sdl2_raii {
public:
	sdl2_raii( Uint32 flags ) {
		check_sdl2_error( SDL_Init( flags ) );
	}
	~sdl2_raii() {
		SDL_Quit();
	}
};

class sdl2_stream_raii : public write_buffers_interface {
private:
	std::ostream & log;
	sdl2_raii sdl2;
	int dev;
	std::size_t channels;
	bool use_float;
	std::size_t sampleQueueMaxFrames;
	std::vector<float> sampleBufFloat;
	std::vector<std::int16_t> sampleBufInt;
protected:
	std::uint32_t round_up_power2(std::uint32_t x)
	{
		std::uint32_t result = 1;
		while ( result < x ) {
			result *= 2;
		}
		return result;
	}
public:
	sdl2_stream_raii( commandlineflags & flags, std::ostream & log_ )
		: log(log_)
		, sdl2( SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_AUDIO )
		, dev(-1)
		, channels(flags.channels)
		, use_float(flags.use_float)
		, sampleQueueMaxFrames(0)
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
		SDL_AudioSpec audiospec;
		std::memset( &audiospec, 0, sizeof( SDL_AudioSpec ) );
		audiospec.freq = flags.samplerate;
		audiospec.format = ( flags.use_float ? AUDIO_F32SYS : AUDIO_S16SYS );
		audiospec.channels = flags.channels;
		audiospec.silence = 0;
		audiospec.samples = round_up_power2( ( flags.buffer * flags.samplerate ) / ( 1000 * 2 ) );
		audiospec.size = audiospec.samples * audiospec.channels * ( flags.use_float ? sizeof( float ) : sizeof( std::int16_t ) );
		audiospec.callback = NULL;
		audiospec.userdata = NULL;
		if ( flags.verbose ) {
			log << "SDL2:" << std::endl;
			log << " latency: " << ( audiospec.samples * 2.0 / flags.samplerate ) << " (2 * " << audiospec.samples << ")" << std::endl;
			log << std::endl;
		}
		sampleQueueMaxFrames = round_up_power2( ( flags.buffer * flags.samplerate ) / ( 1000 * 2 ) );
		SDL_AudioSpec audiospec_obtained;
		std::memset( &audiospec_obtained, 0, sizeof( SDL_AudioSpec ) );
		std::memcpy( &audiospec_obtained, &audiospec, sizeof( SDL_AudioSpec ) );
		dev = SDL_OpenAudioDevice( NULL, 0, &audiospec, &audiospec_obtained, 0 );
		if ( dev < 0 ) {
			check_sdl2_error( dev );
		} else if ( dev == 0 ) {
			check_sdl2_error( -1 );
		}
		SDL_PauseAudioDevice( dev, 0 );
	}
	~sdl2_stream_raii() {
		SDL_PauseAudioDevice( dev, 1 );
		SDL_CloseAudioDevice( dev );
	}
private:
	std::size_t get_num_writeable_frames() {
		std::size_t num_queued_frames = SDL_GetQueuedAudioSize( dev ) / ( use_float ? sizeof( float ) : sizeof( std::int16_t ) ) / channels;
		if ( num_queued_frames > sampleQueueMaxFrames ) {
			return 0;
		}
		return sampleQueueMaxFrames - num_queued_frames;
	}
	template<typename Tsample>
	void write_frames( const Tsample * buffer, std::size_t frames ) {
		while ( frames > 0 ) {
			std::size_t chunk_frames = std::min( frames, get_num_writeable_frames() );
			if ( chunk_frames > 0 ) {
				check_sdl2_error( SDL_QueueAudio( dev, buffer, chunk_frames * channels * ( use_float ? sizeof( float ) : sizeof( std::int16_t ) ) ) );
				frames -= chunk_frames;
				buffer += chunk_frames * channels;
			} else {
				SDL_Delay( 1 );
			}
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		sampleBufFloat.clear();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleBufFloat.push_back( buffers[channel][frame] );
			}
		}
		write_frames( sampleBufFloat.data(), frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		sampleBufInt.clear();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleBufInt.push_back( buffers[channel][frame] );
			}
		}
		write_frames( sampleBufInt.data(), frames );
	}
	bool pause() override {
		SDL_PauseAudioDevice( dev, 1 );
		return true;
	}
	bool unpause() override {
		SDL_PauseAudioDevice( dev, 0 );
		return true;
	}
	bool sleep( int ms ) override {
		SDL_Delay( ms );
		return true;
	}
};

static std::string show_sdl2_devices( std::ostream & /* log */ ) {
	std::ostringstream devices;
	std::size_t device_index = 0;
	devices << " SDL2:" << std::endl;
	sdl2_raii sdl2( SDL_INIT_NOPARACHUTE | SDL_INIT_AUDIO );
	for ( int driver = 0; driver < SDL_GetNumAudioDrivers(); ++driver ) {
		const char * driver_name = SDL_GetAudioDriver( driver );
		if ( !driver_name ) {
			continue;
		}
		if ( std::string( driver_name ).empty() ) {
			continue;
		}
		if ( SDL_AudioInit( driver_name ) < 0 ) {
			continue;
		}
		for ( int device = 0; device < SDL_GetNumAudioDevices( 0 ); ++device ) {
			const char * device_name = SDL_GetAudioDeviceName( device, 0 );
			if ( !device_name ) {
				continue;
			}
			if ( std::string( device_name ).empty() ) {
				continue;
			}
			devices << "    " << device_index << ": " << driver_name << " - " << device_name << std::endl;
			device_index++;
		}
		SDL_AudioQuit();
	}
	return devices.str();
}

} // namespace openmpt123

#endif // MPT_WITH_SDL2

#endif // OPENMPT123_SDL2_HPP

