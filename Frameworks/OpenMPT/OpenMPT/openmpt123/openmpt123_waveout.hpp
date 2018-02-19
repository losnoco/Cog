/*
 * openmpt123_waveout.hpp
 * ------------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_WAVEOUT_HPP
#define OPENMPT123_WAVEOUT_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(WIN32)

namespace openmpt123 {

struct waveout_exception : public exception {
	waveout_exception() throw() : exception( "waveout" ) { }
};

class waveout_stream_raii : public write_buffers_interface {
private:
	HWAVEOUT waveout;
	std::size_t num_channels;
	std::size_t num_chunks;
	std::size_t frames_per_chunk;
	std::size_t bytes_per_chunk;
	std::vector<WAVEHDR> waveheaders;
	std::vector<std::vector<char> > wavebuffers;
	std::deque<char> byte_queue;
public:
	waveout_stream_raii( commandlineflags & flags )
		: waveout(NULL)
		, num_channels(0)
		, num_chunks(0)
		, frames_per_chunk(0)
		, bytes_per_chunk(0)
	{
		if ( flags.buffer == default_high ) {
			flags.buffer = 150;
		} else if ( flags.buffer == default_low ) {
			flags.buffer = 50;
		}
		if ( flags.period == default_high ) {
			flags.period = 30;
		} else if ( flags.period == default_low ) {
			flags.period = 10;
		}
		flags.apply_default_buffer_sizes();
		WAVEFORMATEX wfx;
		ZeroMemory( &wfx, sizeof( wfx ) );
		wfx.wFormatTag = flags.use_float ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
		wfx.nChannels = flags.channels;
		wfx.nSamplesPerSec = flags.samplerate;
		wfx.wBitsPerSample = flags.use_float ? 32 : 16;
		wfx.nBlockAlign = ( wfx.wBitsPerSample / 8 ) * wfx.nChannels;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		wfx.cbSize = 0;
		std::istringstream device_string( flags.device );
		int device = -1;
		device_string >> device;
		waveOutOpen( &waveout, device == -1 ? WAVE_MAPPER : device, &wfx, 0, 0, CALLBACK_NULL );
		num_channels = flags.channels;
		std::size_t frames_per_buffer = flags.samplerate * flags.buffer / 1000;
		num_chunks = ( flags.buffer + flags.period - 1 ) / flags.period;
		if ( num_chunks < 2 ) {
			num_chunks = 2;
		}
		frames_per_chunk = ( frames_per_buffer + num_chunks - 1 ) / num_chunks;
		bytes_per_chunk = wfx.nBlockAlign * frames_per_chunk;
		waveheaders.resize( num_chunks );
		wavebuffers.resize( num_chunks );
		for ( std::size_t i = 0; i < num_chunks; ++i ) {
			wavebuffers[i].resize( bytes_per_chunk );
			waveheaders[i] = WAVEHDR();
			waveheaders[i].lpData = wavebuffers[i].data();
			waveheaders[i].dwBufferLength = static_cast<DWORD>( wavebuffers[i].size() );
			waveheaders[i].dwFlags = 0;
			waveOutPrepareHeader( waveout, &waveheaders[i], sizeof( WAVEHDR ) );
		}
	}
	~waveout_stream_raii() {
		if ( waveout ) {
			write_or_wait( true );
			drain();
			waveOutReset( waveout );
			for ( std::size_t i = 0; i < num_chunks; ++i ) {
				waveheaders[i].dwBufferLength = static_cast<DWORD>( wavebuffers[i].size() );
				waveOutUnprepareHeader( waveout, &waveheaders[i], sizeof( WAVEHDR ) );
			}
			wavebuffers.clear();
			waveheaders.clear();
			frames_per_chunk = 0;
			num_chunks = 0;
			waveOutClose( waveout );
			waveout = NULL;
		}
	}
private:
	void drain() {
		std::size_t empty_chunks = 0;
		while ( empty_chunks != num_chunks ) {
			empty_chunks = 0;
			for ( std::size_t chunk = 0; chunk < num_chunks; ++chunk ) {
				DWORD flags = waveheaders[chunk].dwFlags;
				if ( !(flags & WHDR_INQUEUE) || (flags & WHDR_DONE) ) {
					empty_chunks++;
				}
			}
			if ( empty_chunks != num_chunks ) {
				Sleep( 1 );
			}
		}
	}
	std::size_t wait_for_empty_chunk() {
		while ( true ) {
			for ( std::size_t chunk = 0; chunk < num_chunks; ++chunk ) {
				DWORD flags = waveheaders[chunk].dwFlags;
				if ( !(flags & WHDR_INQUEUE) || (flags & WHDR_DONE) ) {
					return chunk;
				}
			}
			Sleep( 1 );
		}
	}
	void write_chunk() {
		std::size_t chunk = wait_for_empty_chunk();
		std::size_t chunk_bytes = std::min( byte_queue.size(), bytes_per_chunk );
		waveheaders[chunk].dwBufferLength = static_cast<DWORD>( chunk_bytes );
		for ( std::size_t byte = 0; byte < chunk_bytes; ++byte ) {
			wavebuffers[chunk][byte] = byte_queue.front();
			byte_queue.pop_front();
		}
		waveOutWrite( waveout, &waveheaders[chunk], sizeof( WAVEHDR ) );
	}
	void write_or_wait( bool flush = false ) {
		while ( byte_queue.size() >= bytes_per_chunk ) {
			write_chunk();
		}
		if ( flush && !byte_queue.empty() ) {
			write_chunk();
		}
	}
	template < typename Tsample >
	void write_buffers( const std::vector<Tsample*> buffers, std::size_t frames ) {
		for ( std::size_t frame = 0; frame < frames; ++frame ) {
			for ( std::size_t channel = 0; channel < buffers.size(); ++channel ) {
				Tsample val = buffers[channel][frame];
				char buf[ sizeof( Tsample ) ];
				std::memcpy( buf, &val, sizeof( Tsample ) );
				std::copy( buf, buf + sizeof( Tsample ), std::back_inserter( byte_queue ) );
			}
		}
		write_or_wait();
	}
public:
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		write_buffers( buffers, frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
		write_buffers( buffers, frames );
	}
	bool pause() {
		waveOutPause( waveout );
		return true;
	}
	bool unpause() {
		waveOutRestart( waveout );
		return true;
	}
	bool sleep( int ms ) {
		Sleep( ms );
		return true;
	}
};

static std::string show_waveout_devices( std::ostream & /*log*/ ) {
	std::ostringstream devices;
	devices << " waveout:" << std::endl;
	for ( UINT i = 0; i < waveOutGetNumDevs(); ++i ) {
		devices << "    " << i << ": ";
		WAVEOUTCAPSW caps;
		ZeroMemory( &caps, sizeof( caps ) );
		waveOutGetDevCapsW( i, &caps, sizeof( caps ) );
		devices << wstring_to_utf8( caps.szPname );
		devices << std::endl;
	}
	return devices.str();
}

} // namespace openmpt123

#endif // WIN32

#endif // OPENMPT123_WAVEOUT_HPP
