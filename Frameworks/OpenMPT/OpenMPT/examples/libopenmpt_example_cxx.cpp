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

#include <portaudiocpp/PortAudioCpp.hxx>

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
		const std::size_t buffersize = 480;
#if defined( LIBOPENMPT_QUIRK_NO_CSTDINT )
		const openmpt::std::int32_t samplerate = 48000;
#else
		const std::int32_t samplerate = 48000;
#endif
		std::vector<float> left( buffersize );
		std::vector<float> right( buffersize );
		const float * const buffers[2] = { left.data(), right.data() };
		std::ifstream file( argv[1], std::ios::binary );
		openmpt::module mod( file );
		portaudio::AutoSystem portaudio_initializer;
		portaudio::System & portaudio = portaudio::System::instance();
		portaudio::DirectionSpecificStreamParameters outputstream_parameters( portaudio.defaultOutputDevice(), 2, portaudio::FLOAT32, false, portaudio.defaultOutputDevice().defaultHighOutputLatency(), 0 );
		portaudio::StreamParameters stream_parameters( portaudio::DirectionSpecificStreamParameters::null(), outputstream_parameters, samplerate, paFramesPerBufferUnspecified, paNoFlag );
		portaudio::BlockingStream stream( stream_parameters );
		stream.start();
		while ( true ) {
			std::size_t count = mod.read( samplerate, buffersize, left.data(), right.data() );
			if ( count == 0 ) {
				break;
			}
			try {
				stream.write( buffers, static_cast<unsigned long>( count ) );
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
