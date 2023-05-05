/*
 * openmpt123_portaudio.hpp
 * ------------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_PORTAUDIO_HPP
#define OPENMPT123_PORTAUDIO_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#include "mpt/base/detect.hpp"

#if defined(MPT_WITH_PORTAUDIO)

#include <portaudio.h>
#if defined(MPT_BUILD_MSVC) && MPT_COMPILER_MSVC && MPT_ARCH_X86
extern "C" {
void PaUtil_InitializeX86PlainConverters(void);
}
#endif


namespace openmpt123 {

inline constexpr auto portaudio_encoding = mpt::common_encoding::utf8;

struct portaudio_exception : public exception {
	portaudio_exception( PaError code ) : exception( mpt::transcode<mpt::ustring>( portaudio_encoding, Pa_GetErrorText( code ) ) ) { }
};

typedef void (*PaUtilLogCallback ) (const char *log);
#ifdef _MSC_VER
extern "C" void PaUtil_SetDebugPrintFunction(PaUtilLogCallback  cb);
#endif

class portaudio_raii {
private:
	concat_stream<mpt::ustring> & log;
	bool log_set;
	bool portaudio_initialized;
	static concat_stream<mpt::ustring> * portaudio_log_stream;
private:
	static void portaudio_log_function( const char * log ) {
		if ( portaudio_log_stream ) {
			*portaudio_log_stream << MPT_USTRING("PortAudio: ") << mpt::transcode<mpt::ustring>( portaudio_encoding, log );
		}
	}
protected:
	void check_portaudio_error( PaError e ) {
		if ( e > 0 ) {
			return;
		}
		if ( e == paNoError ) {
			return;
		}
		if ( e == paOutputUnderflowed ) {
			log << MPT_USTRING("PortAudio warning: ") << mpt::transcode<mpt::ustring>( portaudio_encoding, Pa_GetErrorText( e ) ) << lf;
			return;
		}
		throw portaudio_exception( e );
	}
public:
	portaudio_raii( bool verbose, concat_stream<mpt::ustring> & log ) : log(log), log_set(false), portaudio_initialized(false) {
		if ( verbose ) {
			portaudio_log_stream = &log;
		} else {
			portaudio_log_stream = 0;
		}
#ifdef _MSC_VER
		PaUtil_SetDebugPrintFunction( portaudio_log_function );
		log_set = true;
#endif
#if defined(MPT_BUILD_MSVC) && MPT_COMPILER_MSVC && MPT_ARCH_X86
		PaUtil_InitializeX86PlainConverters();
#endif
		check_portaudio_error( Pa_Initialize() );
		portaudio_initialized = true;
		if ( verbose ) {
			*portaudio_log_stream << lf;
		}
	}
	~portaudio_raii() {
		if ( portaudio_initialized ) {
			check_portaudio_error( Pa_Terminate() );
			portaudio_initialized = false;
		}
		if ( log_set ) {
#ifdef _MSC_VER
			PaUtil_SetDebugPrintFunction( NULL );
			log_set = false;
#endif
		}
		portaudio_log_stream = 0;
	}
};

concat_stream<mpt::ustring> * portaudio_raii::portaudio_log_stream = 0;

class portaudio_stream_blocking_raii : public portaudio_raii, public write_buffers_interface {
private:
	PaStream * stream;
	bool interleaved;
	std::size_t channels;
	std::vector<float> sampleBufFloat;
	std::vector<std::int16_t> sampleBufInt;
public:
	portaudio_stream_blocking_raii( commandlineflags & flags, concat_stream<mpt::ustring> & log )
		: portaudio_raii(flags.verbose, log)
		, stream(NULL)
		, interleaved(false)
		, channels(flags.channels)
	{
		PaStreamParameters streamparameters;
		std::memset( &streamparameters, 0, sizeof(PaStreamParameters) );
		const int device = mpt::parse_or<int>( flags.device, -1 );
		streamparameters.device = ( device == -1 ) ? Pa_GetDefaultOutputDevice() : device;
		streamparameters.channelCount = flags.channels;
		streamparameters.sampleFormat = ( flags.use_float ? paFloat32 : paInt16 ) | paNonInterleaved;
		if ( flags.buffer == default_high ) {
			streamparameters.suggestedLatency = Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency;
			flags.buffer = static_cast<std::int32_t>( Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency * 1000.0 );
		} else if ( flags.buffer == default_low ) {
			streamparameters.suggestedLatency = Pa_GetDeviceInfo( streamparameters.device )->defaultLowOutputLatency;
			flags.buffer = static_cast<std::int32_t>( Pa_GetDeviceInfo( streamparameters.device )->defaultLowOutputLatency * 1000.0 );
		} else {
			streamparameters.suggestedLatency = flags.buffer * 0.001;
		}
		unsigned long framesperbuffer = 0;
		if ( flags.mode != Mode::UI ) {
			framesperbuffer = paFramesPerBufferUnspecified;
			flags.period = 50;
			flags.period = std::min( flags.period, flags.buffer / 3 );
		} else if ( flags.period == default_high ) {
			framesperbuffer = paFramesPerBufferUnspecified;
			flags.period = 50;
			flags.period = std::min( flags.period, flags.buffer / 3 );
		} else if ( flags.period == default_low ) {
			framesperbuffer = paFramesPerBufferUnspecified;
			flags.period = 10;
			flags.period = std::min( flags.period, flags.buffer / 3 );
		} else {
			framesperbuffer = flags.period * flags.samplerate / 1000;
		}
		if ( flags.period <= 0 ) {
			flags.period = 1;
		}
		flags.apply_default_buffer_sizes();
		if ( flags.verbose ) {
			log << MPT_USTRING("PortAudio:") << lf;
			log << MPT_USTRING(" device: ")
				<< streamparameters.device
				<< MPT_USTRING(" [ ") << mpt::transcode<mpt::ustring>( portaudio_encoding, Pa_GetHostApiInfo( Pa_GetDeviceInfo( streamparameters.device )->hostApi )->name ) << MPT_USTRING(" / ") << mpt::transcode<mpt::ustring>( portaudio_encoding, Pa_GetDeviceInfo( streamparameters.device )->name ) << MPT_USTRING(" ] ")
				<< lf;
			log << MPT_USTRING(" low latency: ") << Pa_GetDeviceInfo( streamparameters.device )->defaultLowOutputLatency << lf;
			log << MPT_USTRING(" high latency: ") << Pa_GetDeviceInfo( streamparameters.device )->defaultHighOutputLatency << lf;
			log << MPT_USTRING(" suggested latency: ") << streamparameters.suggestedLatency << lf;
			log << MPT_USTRING(" frames per buffer: ") << framesperbuffer << lf;
			log << MPT_USTRING(" ui redraw: ") << flags.period << lf;
		}
		PaError e = PaError();
		e = Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, framesperbuffer, ( flags.dither > 0 ) ? paNoFlag : paDitherOff, NULL, NULL );
		if ( e != paNoError ) {
			// Non-interleaved failed, try interleaved next.
			// This might help broken portaudio on MacOS X.
			streamparameters.sampleFormat &= ~paNonInterleaved;
			e = Pa_OpenStream( &stream, NULL, &streamparameters, flags.samplerate, framesperbuffer, ( flags.dither > 0 ) ? paNoFlag : paDitherOff, NULL, NULL );
			if ( e == paNoError ) {
				interleaved = true;
			}
			check_portaudio_error( e );
		}
		check_portaudio_error( Pa_StartStream( stream ) );
		if ( flags.verbose ) {
			log << MPT_USTRING(" channels: ") << streamparameters.channelCount << lf;
			log << MPT_USTRING(" sampleformat: ") << ( ( ( streamparameters.sampleFormat & ~paNonInterleaved ) == paFloat32 ) ? MPT_USTRING("paFloat32") : MPT_USTRING("paInt16") ) << lf;
			log << MPT_USTRING(" latency: ") << Pa_GetStreamInfo( stream )->outputLatency << lf;
			log << MPT_USTRING(" samplerate: ") << Pa_GetStreamInfo( stream )->sampleRate << lf;
			log << lf;
		}
	}
	~portaudio_stream_blocking_raii() {
		if ( stream ) {
			PaError stopped = Pa_IsStreamStopped( stream );
			check_portaudio_error( stopped );
			if ( !stopped ) {
				check_portaudio_error( Pa_StopStream( stream ) );
			}
			check_portaudio_error( Pa_CloseStream( stream ) );
			stream = NULL;
		}
	}
private:
	template<typename Tsample>
	void write_frames( const Tsample * buffer, std::size_t frames ) {
		while ( frames > 0 ) {
			unsigned long chunk_frames = static_cast<unsigned long>( std::min( static_cast<std::uint64_t>( frames ), static_cast<std::uint64_t>( std::numeric_limits<unsigned long>::max() ) ) );
			check_portaudio_error( Pa_WriteStream( stream, buffer, chunk_frames ) );
			buffer += chunk_frames * channels;
			frames -= chunk_frames;
		}
	}
	template<typename Tsample>
	void write_frames( std::vector<Tsample*> buffers, std::size_t frames ) {
		while ( frames > 0 ) {
			unsigned long chunk_frames = static_cast<unsigned long>( std::min( static_cast<std::uint64_t>( frames ), static_cast<std::uint64_t>( std::numeric_limits<unsigned long>::max() ) ) );
			check_portaudio_error( Pa_WriteStream( stream, buffers.data(), chunk_frames ) );
			for ( std::size_t channel = 0; channel < channels; ++channel ) {
				buffers[channel] += chunk_frames;
			}
			frames -= chunk_frames;
		}
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		if ( interleaved ) {
			sampleBufFloat.clear();
			for ( std::size_t frame = 0; frame < frames; ++frame ) {
				for ( std::size_t channel = 0; channel < channels; ++channel ) {
					sampleBufFloat.push_back( buffers[channel][frame] );
				}
			}
			write_frames( sampleBufFloat.data(), frames );
		} else {
			write_frames( buffers, frames );
		}
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		if ( interleaved ) {
			sampleBufInt.clear();
			for ( std::size_t frame = 0; frame < frames; ++frame ) {
				for ( std::size_t channel = 0; channel < channels; ++channel ) {
					sampleBufInt.push_back( buffers[channel][frame] );
				}
			}
			write_frames( sampleBufInt.data(), frames );
		} else {
			write_frames( buffers, frames );
		}
	}
	bool unpause() override {
		check_portaudio_error( Pa_StartStream( stream ) );
		return true;
	}
	bool pause() override {
		check_portaudio_error( Pa_StopStream( stream ) );
		return true;
	}
	bool sleep( int ms ) override {
		Pa_Sleep( ms );
		return true;
	}
};

#define portaudio_stream_raii portaudio_stream_blocking_raii

static mpt::ustring show_portaudio_devices( concat_stream<mpt::ustring> & log ) {
	string_concat_stream<mpt::ustring> devices;
	devices << MPT_USTRING(" portaudio:") << lf;
	portaudio_raii portaudio( false, log );
	for ( PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); ++i ) {
		if ( Pa_GetDeviceInfo( i ) && Pa_GetDeviceInfo( i )->maxOutputChannels > 0 ) {
			devices << MPT_USTRING("    ") << i << MPT_USTRING(": ");
			if ( Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi ) && Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->name ) {
				devices << mpt::transcode<mpt::ustring>( portaudio_encoding, Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->name );
			} else {
				devices << MPT_USTRING("Host API ") << Pa_GetDeviceInfo( i )->hostApi;
			}
			if ( Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi ) ) {
				if ( i == Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->defaultOutputDevice ) {
					devices << MPT_USTRING(" (default)");
				}
			}
			devices << MPT_USTRING(" - ");
			if ( Pa_GetDeviceInfo( i )->name ) {
				devices << mpt::transcode<mpt::ustring>( portaudio_encoding, Pa_GetDeviceInfo( i )->name );
			} else {
				devices << MPT_USTRING("Device ") << i;
			}
			devices << MPT_USTRING(" (");
			devices << MPT_USTRING("high latency: ") << Pa_GetDeviceInfo( i )->defaultHighOutputLatency;
			devices << MPT_USTRING(", ");
			devices << MPT_USTRING("low latency: ") << Pa_GetDeviceInfo( i )->defaultLowOutputLatency;
			devices << MPT_USTRING(")");
			devices << lf;
		}
	}
	return devices.str();
}

} // namespace openmpt123

#endif // MPT_WITH_PORTAUDIO

#endif // OPENMPT123_PORTAUDIO_HPP
