/*
 * openmpt123_pulseaudio.hpp
 * -------------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_PULSEAUDIO_HPP
#define OPENMPT123_PULSEAUDIO_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_PULSEAUDIO)

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

namespace openmpt123 {

struct pulseaudio_exception : public exception {
	static std::string error_to_string( int error ) {
		try {
			if ( error == 0 ) {
				return std::string();
			}
			std::ostringstream e;
			const char * str = pa_strerror( error );
			if ( !str ) {
				e << "error=" << error;
				return e.str();
			}
			if ( std::strlen(str) == 0 ) {
				e << "error=" << error;
				return e.str();
			}
			e << str << " (error=" << error << ")";
			return e.str();
		} catch ( const std::bad_alloc & ) {
			return std::string();
		}
	}
	pulseaudio_exception( int error ) throw() : exception( error_to_string( error ) ) { }
};

class pulseaudio_stream_raii : public write_buffers_interface {
private:
	pa_simple * stream;
	std::size_t channels;
	std::size_t sampleSize;
	std::vector<float> sampleBufFloat;
	std::vector<std::int16_t> sampleBufInt;
public:
	pulseaudio_stream_raii( commandlineflags & flags, std::ostream & /* log */ )
		: stream(NULL)
		, channels(flags.channels)
		, sampleSize(flags.use_float ? sizeof( float ) : sizeof( std::int16_t ))
	{
		int error = 0;
		pa_sample_spec ss;
		std::memset( &ss, 0, sizeof( pa_sample_spec ) );
		ss.format = ( flags.use_float ? PA_SAMPLE_FLOAT32 : PA_SAMPLE_S16NE );
		ss.rate = flags.samplerate;
		ss.channels = flags.channels;
		pa_buffer_attr ba;
		std::memset( &ba, 0, sizeof( pa_buffer_attr ) );
		bool use_ba = false;
		if ( flags.buffer != default_high && flags.buffer != default_low ) {
			use_ba = true;
			ba.maxlength = channels * sampleSize * ( flags.buffer * flags.samplerate / 1000 );
		} else {
			ba.maxlength = static_cast<std::uint32_t>(-1);
		}
		if ( flags.period != default_high && flags.period != default_low ) {
			use_ba = true;
			ba.minreq = channels * sampleSize * ( flags.period * flags.samplerate / 1000 );
			if ( ba.maxlength != static_cast<std::uint32_t>(-1) ) {
				ba.tlength = ba.maxlength - ba.minreq;
				ba.prebuf = ba.tlength;
			} else {
				ba.tlength = static_cast<std::uint32_t>(-1);
				ba.prebuf = static_cast<std::uint32_t>(-1);
			}
		} else {
			ba.minreq = static_cast<std::uint32_t>(-1);
			ba.tlength = static_cast<std::uint32_t>(-1);
			ba.prebuf = static_cast<std::uint32_t>(-1);
		}
		ba.fragsize = 0;
		flags.apply_default_buffer_sizes();
		sampleBufFloat.resize( channels * ( flags.ui_redraw_interval * flags.samplerate / 1000 ) );
		sampleBufInt.resize( channels * ( flags.ui_redraw_interval * flags.samplerate / 1000 ) );
		stream = pa_simple_new( NULL, "openmpt123", PA_STREAM_PLAYBACK, NULL, "openmpt123", &ss, NULL, ( use_ba ? &ba : NULL ), &error );
		if ( !stream ) {
			throw pulseaudio_exception( error );
		}
	}
	~pulseaudio_stream_raii() {
		int error = 0;
		if ( stream ) {
			error = 0;
			if ( pa_simple_drain( stream, &error ) < 0 ) {
				// throw pulseaudio_exception( error );
			}
			pa_simple_free( stream );
			stream = NULL;
		}
	}
private:
	template<typename Tsample>
	void write_frames( const Tsample * buffer, std::size_t frames ) {
		int error = 0;
		if ( pa_simple_write( stream, buffer, frames * channels * sampleSize, &error ) < 0 ) {
			throw pulseaudio_exception( error );
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		sampleBufFloat.clear();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleBufFloat.push_back( buffers[channel][frame] );
			}
		}
		write_frames( sampleBufFloat.data(), frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		sampleBufInt.clear();
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				sampleBufInt.push_back( buffers[channel][frame] );
			}
		}
		write_frames( sampleBufInt.data(), frames );
	}
	bool unpause() {
		return true;
	}
	bool pause() {
		int error = 0;
		error = 0;
		if ( pa_simple_drain( stream, &error ) < 0 ) {
			throw pulseaudio_exception( error );
		}
		return true;
	}
	bool sleep( int ms ) {
		pa_msleep( ms );
		return true;
	}
};

static std::string show_pulseaudio_devices( std::ostream & /* log */ ) {
	std::ostringstream devices;
	devices << " pulseaudio:" << std::endl;
	devices << "    " << "0" << ": Default Device" << std::endl;
	return devices.str();
}

} // namespace openmpt123

#endif // MPT_WITH_PULSEAUDIO

#endif // OPENMPT123_PULSEAUDIO_HPP
