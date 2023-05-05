/*
 * openmpt123_flac.hpp
 * -------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_FLAC_HPP
#define OPENMPT123_FLAC_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_FLAC)

#include "mpt/base/detect.hpp"
#include "mpt/base/saturate_round.hpp"

#if defined(_MSC_VER) && defined(__clang__) && defined(__c2__)
#include <sys/types.h>
#if __STDC__
typedef _off_t off_t;
#endif
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <FLAC/metadata.h>
#include <FLAC/format.h>
#include <FLAC/stream_encoder.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace openmpt123 {

inline constexpr auto flac_encoding = mpt::common_encoding::utf8;

class flac_stream_raii : public file_audio_stream_base {
private:
	commandlineflags flags;
	mpt::native_path filename;
	bool called_init;
	std::vector< std::pair< std::string, std::string > > tags;
	FLAC__StreamMetadata * flac_metadata[1];
	FLAC__StreamEncoder * encoder;
	std::vector<FLAC__int32> interleaved_buffer;
	void add_vorbiscomment_field( FLAC__StreamMetadata * vorbiscomment, const std::string & field, const std::string & value ) {
		if ( !value.empty() ) {
			FLAC__StreamMetadata_VorbisComment_Entry entry;
			FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair( &entry, field.c_str(), value.c_str() );
			FLAC__metadata_object_vorbiscomment_append_comment( vorbiscomment, entry, false );
		}
	}
public:
	flac_stream_raii( const mpt::native_path & filename_, const commandlineflags & flags_, concat_stream<mpt::ustring> & /*log*/ ) : flags(flags_), filename(filename_), called_init(false), encoder(0) {
		flac_metadata[0] = 0;
		encoder = FLAC__stream_encoder_new();
		if ( !encoder ) {
			throw exception( MPT_USTRING("error creating flac encoder") );
		}
		FLAC__stream_encoder_set_channels( encoder, flags.channels );
		FLAC__stream_encoder_set_bits_per_sample( encoder, flags.use_float ? 24 : 16 );
		FLAC__stream_encoder_set_sample_rate( encoder, flags.samplerate );
		FLAC__stream_encoder_set_compression_level( encoder, 8 );
	}
	~flac_stream_raii() {
		if ( encoder ) {
			 FLAC__stream_encoder_finish( encoder );
			 FLAC__stream_encoder_delete( encoder );
			 encoder = 0;
		}
		if ( flac_metadata[0] ) {
			FLAC__metadata_object_delete( flac_metadata[0] );
			flac_metadata[0] = 0;
		}
	}
	void write_metadata( std::map<mpt::ustring, mpt::ustring> metadata ) override {
		if ( called_init ) {
			return;
		}
		tags.clear();
		tags.push_back( std::make_pair( "TITLE", mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("title") ] ) ) );
		tags.push_back( std::make_pair( "ARTIST", mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("artist") ] ) ) );
		tags.push_back( std::make_pair( "DATE", mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("date") ] ) ) );
		tags.push_back( std::make_pair( "COMMENT", mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("message") ] ) ) );
		if ( !metadata[ MPT_USTRING("type") ].empty() && !metadata[ MPT_USTRING("tracker") ].empty() ) {
			tags.push_back( std::make_pair( "SOURCEMEDIA", std::string() + "'" + mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("type") ] ) + "' tracked music file, made with '" + mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("tracker") ] ) + "', rendered with '" + mpt::transcode<std::string>( flac_encoding, get_encoder_tag() ) + "'" ) );
		} else if ( !metadata[ MPT_USTRING("type_long") ].empty() ) {
			tags.push_back( std::make_pair( "SOURCEMEDIA", std::string() + "'" + mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("type") ] ) + "' tracked music file, rendered with '" + mpt::transcode<std::string>( flac_encoding, get_encoder_tag() ) + "'" ) );
		} else if ( !metadata[ MPT_USTRING("tracker") ].empty() ) {
			tags.push_back( std::make_pair( "SOURCEMEDIA", std::string() + "tracked music file, made with '" + mpt::transcode<std::string>( flac_encoding, metadata[ MPT_USTRING("tracker") ] ) + "', rendered with '" + mpt::transcode<std::string>( flac_encoding, get_encoder_tag() ) + "'" ) );
		} else {
			tags.push_back( std::make_pair( "SOURCEMEDIA", std::string() + "tracked music file, rendered with '" + mpt::transcode<std::string>( flac_encoding, get_encoder_tag() ) + "'" ) );
		}
		tags.push_back( std::make_pair( "ENCODER", mpt::transcode<std::string>( flac_encoding, get_encoder_tag() ) ) );
		flac_metadata[0] = FLAC__metadata_object_new( FLAC__METADATA_TYPE_VORBIS_COMMENT );
		for ( std::vector< std::pair< std::string, std::string > >::iterator tag = tags.begin(); tag != tags.end(); ++tag ) {
			add_vorbiscomment_field( flac_metadata[0], tag->first, tag->second );
		}
		FLAC__stream_encoder_set_metadata( encoder, flac_metadata, 1 );
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		if ( !called_init ) {
#if MPT_OS_WINDOWS
			FLAC__stream_encoder_init_file( encoder, mpt::transcode<std::string>( flac_encoding, filename ).c_str(), NULL, 0 );
#else
			FLAC__stream_encoder_init_file( encoder, filename.AsNative().c_str(), NULL, 0 );
#endif
			called_init = true;
		}
		interleaved_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				float in = buffers[channel][frame];
				if ( in <= -1.0f ) {
					in = -1.0f;
				} else if ( in >= 1.0f ) {
					in = 1.0f;
				}
				FLAC__int32 out = mpt::saturate_round<FLAC__int32>( in * (1<<23) );
				out = std::max( 0 - (1<<23), out );
				out = std::min( out, 0 + (1<<23) - 1 );
				interleaved_buffer.push_back( out );
			}
		}
		FLAC__stream_encoder_process_interleaved( encoder, interleaved_buffer.data(), static_cast<unsigned int>( frames ) );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		if ( !called_init ) {
#if MPT_OS_WINDOWS
			FLAC__stream_encoder_init_file( encoder, mpt::transcode<std::string>( flac_encoding, filename ).c_str(), NULL, 0 );
#else
			FLAC__stream_encoder_init_file( encoder, filename.AsNative().c_str(), NULL, 0 );
#endif
			called_init = true;
		}
		interleaved_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_buffer.push_back( buffers[channel][frame] );
			}
		}
		FLAC__stream_encoder_process_interleaved( encoder, interleaved_buffer.data(), static_cast<unsigned int>( frames ) );
	}
};

} // namespace openmpt123

#endif // MPT_WITH_FLAC

#endif // OPENMPT123_FLAC_HPP
