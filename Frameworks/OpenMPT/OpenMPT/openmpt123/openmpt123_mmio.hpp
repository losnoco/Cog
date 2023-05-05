/*
 * openmpt123_mmio.hpp
 * -------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_MMIO_HPP
#define OPENMPT123_MMIO_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#include "mpt/base/detect.hpp"

#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT

namespace openmpt123 {

class mmio_stream_raii : public file_audio_stream_base {
private:
	concat_stream<mpt::ustring> & log;
	commandlineflags flags;
	WAVEFORMATEX waveformatex;
	HMMIO mmio;
	MMCKINFO WAVE_chunk;
	MMCKINFO fmt__chunk;
	MMCKINFO data_chunk;
	MMIOINFO data_info;
private:
	void CHECKED( HRESULT err ) {
		if ( err != 0 ) {
			throw exception( MPT_USTRING("error writing wave file") );
		}
	}
	void UNCHECKED( HRESULT err ) {
		if ( err != 0 ) {
			log << MPT_USTRING("error writing wave file") << lf;
		}
	}
public:
	mmio_stream_raii( const mpt::native_path & filename, const commandlineflags & flags_, concat_stream<mpt::ustring> & log_ ) : log(log_), flags(flags_), mmio(NULL) {

		ZeroMemory( &waveformatex, sizeof( WAVEFORMATEX ) );
		waveformatex.cbSize = 0;
		waveformatex.wFormatTag = flags.use_float ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
		waveformatex.nChannels = static_cast<WORD>( flags.channels );
		waveformatex.nSamplesPerSec = flags.samplerate;
		waveformatex.wBitsPerSample = flags.use_float ? 32 : 16;
		waveformatex.nBlockAlign = static_cast<WORD>( flags.channels * ( waveformatex.wBitsPerSample / 8 ) );
		waveformatex.nAvgBytesPerSec = waveformatex.nSamplesPerSec * waveformatex.nBlockAlign;

		#if defined(UNICODE)
			wchar_t * tmp = _wcsdup( filename.AsNative().c_str() );
			mmio = mmioOpen( tmp, NULL, MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE );
			free( tmp );
			tmp = 0;
		#else
			char * tmp = strdup( filename.AsNative().c_str() );
			mmio = mmioOpen( tmp, NULL, MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE );
			free( tmp );
			tmp = 0;
		#endif
		
		ZeroMemory( &WAVE_chunk, sizeof( MMCKINFO ) );
		WAVE_chunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		CHECKED(mmioCreateChunk( mmio, &WAVE_chunk, MMIO_CREATERIFF ));

			ZeroMemory( &fmt__chunk, sizeof( MMCKINFO ) );
			fmt__chunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
			fmt__chunk.cksize = sizeof( WAVEFORMATEX );
			CHECKED(mmioCreateChunk( mmio, &fmt__chunk, 0 ));

				mmioWrite( mmio, (const char*)&waveformatex, sizeof( WAVEFORMATEX ) );

			CHECKED(mmioAscend( mmio, &fmt__chunk, 0 ));

			ZeroMemory( &data_chunk, sizeof( MMCKINFO ) );
			data_chunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
			data_chunk.cksize = 0;
			CHECKED(mmioCreateChunk( mmio, &data_chunk, 0 ));

				ZeroMemory( &data_info, sizeof( MMIOINFO ) );
				CHECKED(mmioGetInfo( mmio, &data_info, 0 ));

	}
				
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				if ( data_info.pchEndWrite - data_info.pchNext < static_cast<long>( sizeof( float ) ) ) {
					data_info.dwFlags |= MMIO_DIRTY;
					CHECKED(mmioAdvance( mmio, &data_info, MMIO_WRITE ));
				}
				std::memcpy( data_info.pchNext, &( buffers[channel][frame] ), sizeof( float ) );
				data_info.pchNext += sizeof( float );
			}
		}
	}

	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				if ( data_info.pchEndWrite - data_info.pchNext < static_cast<long>( sizeof( std::int16_t ) ) ) {
					data_info.dwFlags |= MMIO_DIRTY;
					CHECKED(mmioAdvance( mmio, &data_info, MMIO_WRITE ));
				}
				std::memcpy( data_info.pchNext, &( buffers[channel][frame] ), sizeof( std::int16_t ) );
				data_info.pchNext += sizeof( std::int16_t );
			}
		}
	}

	~mmio_stream_raii() {

				data_info.dwFlags |= MMIO_DIRTY;
				UNCHECKED(mmioSetInfo( mmio, &data_info, 0 ));

			UNCHECKED(mmioAscend( mmio, &data_chunk, 0 ));

		UNCHECKED(mmioAscend( mmio, &WAVE_chunk, 0 ));

		UNCHECKED(mmioClose( mmio, 0 ));
		mmio = NULL;

	}
};

} // namespace openmpt123

#endif // MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT

#endif // OPENMPT123_MMIO_HPP
