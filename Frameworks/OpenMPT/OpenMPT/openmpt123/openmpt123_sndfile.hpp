/*
 * openmpt123_sndfile.hpp
 * ----------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_SNDFILE_HPP
#define OPENMPT123_SNDFILE_HPP

#include "openmpt123_config.hpp"
#include "openmpt123.hpp"

#if defined(MPT_WITH_SNDFILE)

#include "mpt/base/detect.hpp"

#include <sndfile.h>

namespace openmpt123 {

inline constexpr auto sndfile_encoding = mpt::common_encoding::utf8;
	
class sndfile_stream_raii : public file_audio_stream_base {
private:
	commandlineflags flags;
	concat_stream<mpt::ustring> & log;
	SNDFILE * sndfile;
	std::vector<float> interleaved_float_buffer;
	std::vector<std::int16_t> interleaved_int_buffer;
private:
	enum match_mode_enum {
		match_print,
		match_recurse,
		match_exact,
		match_better,
		match_any
	};
	mpt::ustring match_mode_to_string( match_mode_enum match_mode ) {
		switch ( match_mode ) {
			case match_print  : return MPT_USTRING("print")  ; break;
			case match_recurse: return MPT_USTRING("recurse"); break;
			case match_exact  : return MPT_USTRING("exact")  ; break;
			case match_better : return MPT_USTRING("better") ; break;
			case match_any    : return MPT_USTRING("any")    ; break;
		}
		return MPT_USTRING("");
	}
	int matched_result( int format, const SF_FORMAT_INFO & format_info, const SF_FORMAT_INFO & subformat_info, match_mode_enum match_mode ) {
		if ( flags.verbose ) {
			log << MPT_USTRING("sndfile: using format '")
			    << mpt::transcode<mpt::ustring>( sndfile_encoding, format_info.name ) << MPT_USTRING(" (") << mpt::transcode<mpt::ustring>( sndfile_encoding, format_info.extension ) << MPT_USTRING(")") << MPT_USTRING(" / ") << mpt::transcode<mpt::ustring>( sndfile_encoding, subformat_info.name )
			    << MPT_USTRING("', ")
			    << MPT_USTRING("match: ") << match_mode_to_string( match_mode )
			    << lf;
		}
		return format;
	}
	int find_format( const mpt::native_path & extension, match_mode_enum match_mode ) {

		if ( match_mode == match_recurse ) {
			int result = 0;
			result = find_format( extension, match_exact );
			if ( result ) {
				return result;
			}
			result = find_format( extension, match_better );
			if ( result ) {
				return result;
			}
			result = find_format( extension, match_any );
			if ( result ) {
				return result;
			}
			if ( result ) {
				return result;
			}
			return 0;
		}

		int major_count;
		sf_command( 0, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof( int ) );
		for ( int m = 0; m < major_count; m++ ) {

			SF_FORMAT_INFO format_info;
			format_info.format = m;
			sf_command( 0, SFC_GET_FORMAT_MAJOR, &format_info, sizeof( SF_FORMAT_INFO ) );

			int subtype_count;
			sf_command( 0, SFC_GET_FORMAT_SUBTYPE_COUNT, &subtype_count, sizeof( int ) );
			for ( int s = 0; s < subtype_count; s++ ) {

				SF_FORMAT_INFO subformat_info;
				subformat_info.format = s;
				sf_command( 0, SFC_GET_FORMAT_SUBTYPE, &subformat_info, sizeof( SF_FORMAT_INFO ) );
				int format = ( format_info.format & SF_FORMAT_TYPEMASK ) | ( subformat_info.format & SF_FORMAT_SUBMASK );

				SF_INFO sfinfo;
				std::memset( &sfinfo, 0, sizeof( SF_INFO ) );
				sfinfo.channels = flags.channels;
				sfinfo.format = format;
				if ( sf_format_check( &sfinfo ) ) {

					switch ( match_mode ) {
					case match_print:
						log << MPT_USTRING("sndfile: ")
						    << mpt::transcode<mpt::ustring>( sndfile_encoding, ( format_info.name ? format_info.name : "" ) ) << MPT_USTRING(" (.") << mpt::transcode<mpt::ustring>( sndfile_encoding, ( format_info.extension ? format_info.extension : "" ) ) << MPT_USTRING(")")
						    << MPT_USTRING(" / ")
						    << mpt::transcode<mpt::ustring>( sndfile_encoding, ( subformat_info.name ? subformat_info.name : "" ) )
						    << MPT_USTRING(" [")
						    << mpt::format<mpt::ustring>::hex0<8>( format )
						    << MPT_USTRING("]")
						    << lf;
						break;
					case match_recurse:
						break;
					case match_exact:
						if ( mpt::transcode<std::string>( sndfile_encoding, extension ) == format_info.extension ) {
							if ( flags.use_float && ( subformat_info.format == SF_FORMAT_FLOAT ) ) {
								return matched_result( format, format_info, subformat_info, match_mode );
							} else if ( !flags.use_float && ( subformat_info.format == SF_FORMAT_PCM_16 ) ) {
								return matched_result( format, format_info, subformat_info, match_mode );
							}
						}
						break;
					case match_better:
						if ( mpt::transcode<std::string>( sndfile_encoding, extension ) == format_info.extension ) {
							if ( flags.use_float && ( subformat_info.format == SF_FORMAT_FLOAT || subformat_info.format == SF_FORMAT_DOUBLE ) ) {
								return matched_result( format, format_info, subformat_info, match_mode );
							} else if ( !flags.use_float && ( subformat_info.format & ( subformat_info.format == SF_FORMAT_PCM_16 || subformat_info.format == SF_FORMAT_PCM_24 || subformat_info.format == SF_FORMAT_PCM_32 ) ) ) {
								return matched_result( format, format_info, subformat_info, match_mode );
							}
						}
						break;
					case match_any:
						if ( mpt::transcode<std::string>( sndfile_encoding, extension ) == format_info.extension ) {
							return matched_result( format, format_info, subformat_info, match_mode );
						}
						break;
					}

				}
			}
		}

		return 0;

	}
	void write_metadata_field( int str_type, const std::string & str ) {
		if ( !str.empty() ) {
			sf_set_string( sndfile, str_type, str.c_str() );
		}
	}
public:
	sndfile_stream_raii( const mpt::native_path & filename, const commandlineflags & flags_, concat_stream<mpt::ustring> & log_ ) : flags(flags_), log(log_), sndfile(0) {
		if ( flags.verbose ) {
			find_format( MPT_NATIVE_PATH(""), match_print );
			log << lf;
		}
		int format = find_format( flags.output_extension, match_recurse );
		if ( !format ) {
			throw exception( MPT_USTRING("unknown file type") );
		}
		SF_INFO info;
		std::memset( &info, 0, sizeof( SF_INFO ) );
		info.samplerate = flags.samplerate;
		info.channels = flags.channels;
		info.format = format;
#if MPT_OS_WINDOWS && defined(UNICODE)
		sndfile = sf_wchar_open( filename.AsNative().c_str(), SFM_WRITE, &info );
#else
		sndfile = sf_open( filename.AsNative().c_str(), SFM_WRITE, &info );
#endif
	}
	~sndfile_stream_raii() {
		sf_close( sndfile );
		sndfile = 0;
	}
	void write_metadata( std::map<mpt::ustring, mpt::ustring> metadata ) override {
		write_metadata_field( SF_STR_TITLE, mpt::transcode<std::string>( sndfile_encoding, metadata[ MPT_USTRING("title") ] ) );
		write_metadata_field( SF_STR_ARTIST, mpt::transcode<std::string>( sndfile_encoding, metadata[ MPT_USTRING("artist") ] ) );
		write_metadata_field( SF_STR_DATE, mpt::transcode<std::string>( sndfile_encoding, metadata[ MPT_USTRING("date") ] ) );
		write_metadata_field( SF_STR_COMMENT, mpt::transcode<std::string>( sndfile_encoding, metadata[ MPT_USTRING("message") ] ) );
		write_metadata_field( SF_STR_SOFTWARE, mpt::transcode<std::string>( sndfile_encoding, append_software_tag( metadata[ MPT_USTRING("tracker") ] ) ) );
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) override {
		interleaved_float_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_float_buffer.push_back( buffers[channel][frame] );
			}
		}
		sf_writef_float( sndfile, interleaved_float_buffer.data(), frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) override {
		interleaved_int_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_int_buffer.push_back( buffers[channel][frame] );
			}
		}
		sf_writef_short( sndfile, interleaved_int_buffer.data(), frames );
	}
};

} // namespace openmpt123

#endif // MPT_WITH_SNDFILE

#endif // OPENMPT123_SNDFILE_HPP
