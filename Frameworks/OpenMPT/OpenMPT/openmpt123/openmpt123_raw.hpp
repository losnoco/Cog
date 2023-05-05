/*
 * openmpt123_raw.hpp
 * ------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_RAW_HPP
#define OPENMPT123_RAW_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#include <fstream>

namespace openmpt123 {

class raw_stream_raii : public file_audio_stream_base {
private:
	commandlineflags flags;
	mpt::IO::ofstream file;
	std::vector<float> interleaved_float_buffer;
	std::vector<std::int16_t> interleaved_int_buffer;
public:
	raw_stream_raii( const mpt::native_path & filename, const commandlineflags & flags_, concat_stream<mpt::ustring> & /*log*/ ) : flags(flags_), file(filename, std::ios::binary) {
		return;
	}
	~raw_stream_raii() {
		return;
	}
	void write_metadata( std::map<mpt::ustring, mpt::ustring> /* metadata */ ) override {
		return;
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		interleaved_float_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_float_buffer.push_back( buffers[channel][frame] );
			}
		}
		file.write( reinterpret_cast<const char *>( interleaved_float_buffer.data() ), frames * buffers.size() * sizeof( float ) );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		interleaved_int_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_int_buffer.push_back( buffers[channel][frame] );
			}
		}
		file.write( reinterpret_cast<const char *>( interleaved_int_buffer.data() ), frames * buffers.size() * sizeof( std::int16_t ) );
	}
};

} // namespace openmpt123

#endif // OPENMPT123_RAW_HPP
