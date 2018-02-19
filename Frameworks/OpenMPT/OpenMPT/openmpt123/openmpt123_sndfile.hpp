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

#include <sndfile.h>

namespace openmpt123 {
	
class sndfile_stream_raii : public file_audio_stream_base {
private:
	commandlineflags flags;
	std::ostream & log;
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
	std::string match_mode_to_string( match_mode_enum match_mode ) {
		switch ( match_mode ) {
			case match_print  : return "print"  ; break;
			case match_recurse: return "recurse"; break;
			case match_exact  : return "exact"  ; break;
			case match_better : return "better" ; break;
			case match_any    : return "any"    ; break;
		}
		return "";
	}
	int matched_result( const SF_FORMAT_INFO & format_info, const SF_FORMAT_INFO & subformat_info, match_mode_enum match_mode ) {
		if ( flags.verbose ) {
			log << "sndfile: using format '"
			    << format_info.name << " (" << format_info.extension << ")" << " / " << subformat_info.name
			    << "', "
			    << "match: " << match_mode_to_string( match_mode )
			    << std::endl;
		}
		return ( format_info.format & SF_FORMAT_TYPEMASK ) | subformat_info.format;
	}
	int find_format( const std::string & extension, match_mode_enum match_mode ) {

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

		int format = 0;
		int major_count;
		sf_command( 0, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof( int ) );
		for ( int m = 0; m < major_count; m++ ) {

			SF_FORMAT_INFO format_info;
			format_info.format = m;
			sf_command( 0, SFC_GET_FORMAT_MAJOR, &format_info, sizeof( SF_FORMAT_INFO ) );
			format = format_info.format;

			int subtype_count;
			sf_command( 0, SFC_GET_FORMAT_SUBTYPE_COUNT, &subtype_count, sizeof( int ) );
			for ( int s = 0; s < subtype_count; s++ ) {

				SF_FORMAT_INFO subformat_info;
				subformat_info.format = s;
				sf_command( 0, SFC_GET_FORMAT_SUBTYPE, &subformat_info, sizeof( SF_FORMAT_INFO ) );
				format = ( format & SF_FORMAT_TYPEMASK ) | subformat_info.format;

				SF_INFO sfinfo;
				std::memset( &sfinfo, 0, sizeof( SF_INFO ) );
				sfinfo.channels = flags.channels;
				sfinfo.format = format;
				if ( sf_format_check( &sfinfo ) ) {

					switch ( match_mode ) {
					case match_print:
						log << "sndfile: "
						    << ( format_info.name ? format_info.name : "" ) << " (" << ( format_info.extension ? format_info.extension : "" ) << ")"
						    << " / "
						    << ( subformat_info.name ? subformat_info.name : "" )
						    << " ["
						    << std::setbase(16) << std::setw(8) << std::setfill('0') << format_info.format
						    << "|"
						    << std::setbase(16) << std::setw(8) << std::setfill('0') << subformat_info.format
						    << "]"
						    << std::endl;
						break;
					case match_recurse:
						break;
					case match_exact:
						if ( extension == format_info.extension ) {
							if ( flags.use_float && ( subformat_info.format == SF_FORMAT_FLOAT ) ) {
								return matched_result( format_info, subformat_info, match_mode );
							} else if ( !flags.use_float && ( subformat_info.format == SF_FORMAT_PCM_16 ) ) {
								return matched_result( format_info, subformat_info, match_mode );
							}
						}
						break;
					case match_better:
						if ( extension == format_info.extension ) {
							if ( flags.use_float && ( subformat_info.format == SF_FORMAT_FLOAT || subformat_info.format == SF_FORMAT_DOUBLE ) ) {
								return matched_result( format_info, subformat_info, match_mode );
							} else if ( !flags.use_float && ( subformat_info.format & ( subformat_info.format == SF_FORMAT_PCM_16 || subformat_info.format == SF_FORMAT_PCM_24 || subformat_info.format == SF_FORMAT_PCM_32 ) ) ) {
								return matched_result( format_info, subformat_info, match_mode );
							}
						}
						break;
					case match_any:
						if ( extension == format_info.extension ) {
							return matched_result( format_info, subformat_info, match_mode );
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
	sndfile_stream_raii( const std::string & filename, const commandlineflags & flags_, std::ostream & log_ ) : flags(flags_), log(log_), sndfile(0) {
		if ( flags.verbose ) {
			find_format( "", match_print );
			log << std::endl;
		}
		int format = find_format( flags.output_extension, match_recurse );
		if ( !format ) {
			throw exception( "unknown file type" );
		}
		SF_INFO info;
		std::memset( &info, 0, sizeof( SF_INFO ) );
		info.samplerate = flags.samplerate;
		info.channels = flags.channels;
		info.format = format;
		sndfile = sf_open( filename.c_str(), SFM_WRITE, &info );
	}
	~sndfile_stream_raii() {
		sf_close( sndfile );
		sndfile = 0;
	}
	void write_metadata( std::map<std::string,std::string> metadata ) {
		write_metadata_field( SF_STR_TITLE, metadata[ "title" ] );
		write_metadata_field( SF_STR_ARTIST, metadata[ "artist" ] );
		write_metadata_field( SF_STR_DATE, metadata[ "date" ] );
		write_metadata_field( SF_STR_COMMENT, metadata[ "message" ] );
		write_metadata_field( SF_STR_SOFTWARE, append_software_tag( metadata[ "tracker" ] ) );
	}
	void write( const std::vector<float*> buffers, std::size_t frames ) {
		interleaved_float_buffer.clear();
		for ( std::size_t frame = 0; frame < frames; frame++ ) {
			for ( std::size_t channel = 0; channel < buffers.size(); channel++ ) {
				interleaved_float_buffer.push_back( buffers[channel][frame] );
			}
		}
		sf_writef_float( sndfile, interleaved_float_buffer.data(), frames );
	}
	void write( const std::vector<std::int16_t*> buffers, std::size_t frames ) {
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
