/*
 * libopenmpt_example_cxx.cpp
 * --------------------------
 * Purpose: libopenmpt C++ API example
 * Notes  : PortAudio C++ is used for sound output.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_example_cxx SOMEMODULE
 */

#include <exception>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>

#include <libopenmpt/libopenmpt.hpp>

#if defined(__clang__)
#if ((__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= 40000)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif
#endif
#include <portaudiocpp/PortAudioCpp.hxx>
#if defined(__clang__)
#if ((__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= 40000)
#pragma clang diagnostic pop
#endif
#endif

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
#if defined( __GNUC__ )
// mingw-w64 g++ does only default to special C linkage for "main", but not for "wmain" (see <https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/>).
extern "C" int wmain( int argc, wchar_t * argv[] ) {
#else
int wmain( int argc, wchar_t * argv[] ) {
#endif
#else
int main( int argc, char * argv[] ) {
#endif
	try {
		if ( argc != 2 ) {
			throw std::runtime_error( "Usage: libopenmpt_example_cxx SOMEMODULE" );
		}
		constexpr std::size_t buffersize = 480;
		constexpr std::int32_t samplerate = 48000;
		std::ifstream file( argv[1], std::ios::binary );
		openmpt::module mod( file );
		portaudio::AutoSystem portaudio_initializer;
		portaudio::System & portaudio = portaudio::System::instance();
		std::vector<float> left( buffersize );
		std::vector<float> right( buffersize );
		std::vector<float> interleaved_buffer( buffersize * 2 );
		bool is_interleaved = false;
#if defined(_MSC_VER) && defined(_PREFAST_)
		// work-around bug in VS2019 MSVC 16.5.5 static analyzer
		is_interleaved = false;
		portaudio::DirectionSpecificStreamParameters outputstream_parameters( portaudio.defaultOutputDevice(), 2, portaudio::FLOAT32, false, portaudio.defaultOutputDevice().defaultHighOutputLatency(), 0 );
		portaudio::StreamParameters stream_parameters( portaudio::DirectionSpecificStreamParameters::null(), outputstream_parameters, samplerate, paFramesPerBufferUnspecified, paNoFlag );
		portaudio::BlockingStream stream( stream_parameters );
#else
		portaudio::BlockingStream stream = [&]() {
			try {
				is_interleaved = false;
				portaudio::DirectionSpecificStreamParameters outputstream_parameters( portaudio.defaultOutputDevice(), 2, portaudio::FLOAT32, false, portaudio.defaultOutputDevice().defaultHighOutputLatency(), 0 );
				portaudio::StreamParameters stream_parameters( portaudio::DirectionSpecificStreamParameters::null(), outputstream_parameters, samplerate, paFramesPerBufferUnspecified, paNoFlag );
				return portaudio::BlockingStream( stream_parameters );
			} catch ( const portaudio::PaException & e ) {
				if ( e.paError() != paSampleFormatNotSupported ) {
					throw;
				}
				is_interleaved = true;
				portaudio::DirectionSpecificStreamParameters outputstream_parameters( portaudio.defaultOutputDevice(), 2, portaudio::FLOAT32, true, portaudio.defaultOutputDevice().defaultHighOutputLatency(), 0 );
				portaudio::StreamParameters stream_parameters( portaudio::DirectionSpecificStreamParameters::null(), outputstream_parameters, samplerate, paFramesPerBufferUnspecified, paNoFlag );
				return portaudio::BlockingStream( stream_parameters );
			}
		}();
#endif
		stream.start();
		while ( true ) {
			std::size_t count = is_interleaved ? mod.read_interleaved_stereo( samplerate, buffersize, interleaved_buffer.data() ) : mod.read( samplerate, buffersize, left.data(), right.data() );
			if ( count == 0 ) {
				break;
			}
			try {
				if ( is_interleaved ) {
					stream.write( interleaved_buffer.data(), static_cast<unsigned long>( count ) );
				} else {
					const float * const buffers[2] = { left.data(), right.data() };
					stream.write( buffers, static_cast<unsigned long>( count ) );
				}
			} catch ( const portaudio::PaException & pa_exception ) {
				if ( pa_exception.paError() != paOutputUnderflowed ) {
					throw;
				}
			}
		}
		stream.stop();
	} catch ( const std::bad_alloc & ) {
		std::cerr << "Error: " << std::string( "out of memory" ) << std::endl;
		return 1;
	} catch ( const std::exception & e ) {
		std::cerr << "Error: " << std::string( e.what() ? e.what() : "unknown error" ) << std::endl;
		return 1;
	}
	return 0;
}
