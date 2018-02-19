/*
 * openmpt123_stdout.hpp
 * ---------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_STDOUT_HPP
#define OPENMPT123_STDOUT_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

namespace openmpt123 {

class stdout_stream_raii : public write_buffers_interface {
private:
	std::vector<float> interleaved_float_buffer;
	std::vector<std::int16_t> interleaved_int_buffer;
public:
	stdout_stream_raii() {
		return;
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		interleaved_float_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_float_buffer.push_back( buffers[channel][frame] );
			}
		}
		std::cout.write( reinterpret_cast<const char *>( interleaved_float_buffer.data() ), interleaved_float_buffer.size() * sizeof( float ) );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		interleaved_int_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_int_buffer.push_back( buffers[channel][frame] );
			}
		}
		std::cout.write( reinterpret_cast<const char *>( interleaved_int_buffer.data() ), interleaved_int_buffer.size() * sizeof( std::int16_t ) );
	}
};

} // namespace openmpt123

#endif // OPENMPT123_STDOUT_HPP
