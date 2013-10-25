#include "midi_processor.h"

#include <string.h>

bool midi_processor::is_riff_midi( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 20 ) return false;
    if ( p_file[ 0 ] != 'R' || p_file[ 1 ] != 'I' || p_file[ 2 ] != 'F' || p_file[ 3 ] != 'F' ) return false;
    uint32_t riff_size = p_file[ 4 ] | ( p_file[ 5 ] << 8 ) | ( p_file[ 6 ] << 16 ) | ( p_file[ 7 ] << 24 );
    if ( riff_size < 12 || ( p_file.size() < riff_size + 8 ) ) return false;
    if ( p_file[ 8 ] != 'R' || p_file[ 9 ] != 'M' || p_file[ 10 ] != 'I' || p_file[ 11 ] != 'D' ||
         p_file[ 12 ] != 'd' || p_file[ 13 ] != 'a' || p_file[ 14 ] != 't' || p_file[ 15 ] != 'a' ) return false;
    uint32_t data_size = p_file[ 16 ] | ( p_file[ 17 ] << 8 ) | ( p_file[ 18 ] << 16 ) | ( p_file[ 19 ] << 24 );
    if ( data_size < 18 || p_file.size() < data_size + 20 || riff_size < data_size + 12 ) return false;
    std::vector<uint8_t> test;
    test.assign( p_file.begin() + 20, p_file.begin() + 20 + 18 );
    return is_standard_midi( test );
}

static const char * riff_tag_mappings[][2] = 
{
	{ "IALB", "album" },
	{ "IARL", "archival_location" },
	{ "IART", "artist" },
	{ "ITRK", "tracknumber" },
	{ "ICMS", "commissioned" },
	{ "ICMP", "composer" },
	{ "ICMT", "comment" },
	{ "ICOP", "copyright" },
	{ "ICRD", "creation_date" },
	{ "IENG", "engineer" },
	{ "IGNR", "genre" },
	{ "IKEY", "keywords" },
	{ "IMED", "medium" },
	{ "INAM", "title" },
	{ "IPRD", "product" },
	{ "ISBJ", "subject" },
	{ "ISFT", "software" },
	{ "ISRC", "source" },
	{ "ISRF", "source_form" },
	{ "ITCH", "technician" }
};

bool midi_processor::process_riff_midi( std::vector<uint8_t> const& p_file, midi_container & p_out )
{
    uint32_t file_size = p_file[ 4 ] | ( p_file[ 5 ] << 8 ) | ( p_file[ 6 ] << 16 ) | ( p_file[ 7 ] << 24 );

    std::vector<uint8_t>::const_iterator it = p_file.begin() + 12;

    std::vector<uint8_t>::const_iterator body_end = p_file.begin() + 8 + file_size;

	bool found_data = false;
	bool found_info = false;

	midi_meta_data meta_data;

    std::vector<uint8_t> extra_buffer;

    while ( it != body_end )
	{
		if ( body_end - it < 8 ) return false;
        uint32_t chunk_size = it[ 4 ] | ( it[ 5 ] << 8 ) | ( it[ 6 ] << 16 ) | ( it[ 7 ] << 24 );
		if ( (unsigned long)(body_end - it) < chunk_size ) return false;
        if ( it[ 0 ] == 'd' && it[ 1 ] == 'a' && it[ 2 ] == 't' && it[ 3 ] == 'a' )
		{
			if ( !found_data )
			{
                std::vector<uint8_t> midi_file;
                midi_file.assign( it + 8, it + 8 + chunk_size );
                if ( !process_standard_midi( midi_file, p_out ) ) return false;
				found_data = true;
            }
            else return false; /*throw exception_io_data( "Multiple RIFF data chunks found" );*/
            it += 8 + chunk_size;
            if ( chunk_size & 1 && it != body_end ) ++it;
		}
        else if ( it[ 0 ] == 'D' && it[ 1 ] == 'I' && it[ 2 ] == 'S' && it[ 3 ] == 'P' )
		{
            uint32_t type = it[ 8 ] | ( it[ 9 ] << 8 ) | ( it[ 10 ] << 16 ) | ( it[ 11 ] << 24 );
			if ( type == 1 )
			{
                extra_buffer.resize( chunk_size - 4 );
                std::copy( it + 12, it + 8 + chunk_size, extra_buffer.begin() );
                meta_data.add_item( midi_meta_data_item( 0, "display_name", (const char *) &extra_buffer[0] ) );
			}
            it += 8 + chunk_size;
            if ( chunk_size & 1 && it != body_end ) ++it;
		}
        else if ( it[ 0 ] == 'L' && it[ 1 ] == 'I' && it[ 2 ] == 'S' && it[ 3 ] == 'T' )
		{
            std::vector<uint8_t>::const_iterator chunk_end = it + 8 + chunk_size;
            if ( it[ 8 ] == 'I' && it[ 9 ] == 'N' && it[ 10 ] == 'F' && it[ 11 ] == 'O' )
			{
				if ( !found_info )
				{
					if ( chunk_end - it < 12 ) return false;
                    it += 12;
                    while ( it != chunk_end )
					{
						if ( chunk_end - it < 4 ) return false;
                        uint32_t field_size = it[ 4 ] | ( it[ 5 ] << 8 ) | ( it[ 6 ] << 16 ) | ( it[ 7 ] << 24 );
						if ( (unsigned long)(chunk_end - it) < 8 + field_size ) return false;
                        std::string field;
                        field.assign( it, it + 4 );
						for ( unsigned i = 0; i < _countof(riff_tag_mappings); ++i )
						{
                            if ( !memcmp( &it[0], riff_tag_mappings[ i ][ 0 ], 4 ) )
							{
								field = riff_tag_mappings[ i ][ 1 ];
								break;
							}
						}
                        extra_buffer.resize( field_size );
                        std::copy( it + 8, it + 8 + field_size, extra_buffer.begin() );
                        it += 8 + field_size;
                        meta_data.add_item( midi_meta_data_item( 0, field.c_str(), ( const char * ) &extra_buffer[0] ) );
                        if ( field_size & 1 && it != chunk_end ) ++it;
					}
					found_info = true;
				}
                else return false; /*throw exception_io_data( "Multiple RIFF LIST INFO chunks found" );*/
			}
            else return false; /* unknown LIST chunk */
            it = chunk_end;
            if ( chunk_size & 1 && it != body_end ) ++it;
        }
        else
        {
            it += chunk_size;
            if ( chunk_size & 1 && it != body_end ) ++it;
        }

		if ( found_data && found_info ) break;
	}

	p_out.set_extra_meta_data( meta_data );

    return true;
}
