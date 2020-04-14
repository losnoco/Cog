#include "midi_processor.h"

#include <string.h>

static inline bool it_equal( std::vector<uint8_t>::const_iterator it1, const char *it2, size_t length )
{
    for ( size_t i = 0; i < length; i++ )
    {
        if ( it1[ i ] != it2[ i ] )
            return false;
    }
    return true;
}

static inline uint32_t toInt32LE( const uint8_t *in )
{
    return static_cast<uint32_t>( in[ 0 ] ) |
           static_cast<uint32_t>( in[ 1 ] << 8 ) |
           static_cast<uint32_t>( in[ 2 ] << 16 ) |
           static_cast<uint32_t>( in[ 3 ] << 24 );
}

static inline uint32_t toInt32LE( std::vector<uint8_t>::const_iterator in )
{
    return static_cast<uint32_t>( in[ 0 ] ) |
           static_cast<uint32_t>( in[ 1 ] << 8 ) |
           static_cast<uint32_t>( in[ 2 ] << 16 ) |
           static_cast<uint32_t>( in[ 3 ] << 24 );
}


bool midi_processor::is_riff_midi( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 20 ) return false;
    if ( memcmp( &p_file[ 0 ], "RIFF", 4 ) != 0 ) return false;
    uint32_t riff_size = p_file[ 4 ] | ( p_file[ 5 ] << 8 ) | ( p_file[ 6 ] << 16 ) | ( p_file[ 7 ] << 24 );
    if ( riff_size < 12 || ( p_file.size() < riff_size + 8 ) ) return false;
    if ( memcmp( &p_file[ 8 ], "RMID", 4 ) != 0 ||
         memcmp( &p_file[ 12 ], "data", 4 ) != 0 ) return false;
    uint32_t data_size = toInt32LE(&p_file[ 16 ]);
    if ( data_size < 18 || p_file.size() < data_size + 20 || riff_size < data_size + 12 ) return false;
    std::vector<uint8_t> test;
    test.assign( p_file.begin() + 20, p_file.begin() + 20 + 18 );
    return is_standard_midi( test );
}

bool midi_processor::process_riff_midi_count( std::vector<uint8_t> const& p_file, size_t & track_count )
{
    track_count = 0;

    uint32_t file_size = p_file[ 4 ] | ( p_file[ 5 ] << 8 ) | ( p_file[ 6 ] << 16 ) | ( p_file[ 7 ] << 24 );

    std::vector<uint8_t>::const_iterator it = p_file.begin() + 12;

    std::vector<uint8_t>::const_iterator body_end = p_file.begin() + 8 + file_size;

    std::vector<uint8_t> extra_buffer;

    while ( it != body_end )
    {
        if ( body_end - it < 8 ) return false;
        uint32_t chunk_size = it[ 4 ] | ( it[ 5 ] << 8 ) | ( it[ 6 ] << 16 ) | ( it[ 7 ] << 24 );
        if ( (unsigned long)(body_end - it) < chunk_size ) return false;
        if ( it_equal(it, "data", 4) )
        {
            std::vector<uint8_t> midi_file;
            midi_file.assign( it + 8, it + 8 + chunk_size );
            return process_standard_midi_count( midi_file, track_count );
        }
        else
        {
            it += chunk_size;
            if ( chunk_size & 1 && it != body_end ) ++it;
        }
    }

    return false;
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
        uint32_t chunk_size = toInt32LE( it + 4 );
        if ( (unsigned long)(body_end - it) < chunk_size ) return false;
        if ( it_equal( it, "data", 4 ) )
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
        else if ( it_equal( it, "DISP", 4 ) )
        {
            uint32_t type = toInt32LE( it + 8 );
            if ( type == 1 )
            {
                extra_buffer.resize( chunk_size - 4 + 1 );
                std::copy( it + 12, it + 8 + chunk_size, extra_buffer.begin() );
				extra_buffer[ chunk_size - 4 ] = '\0';
                meta_data.add_item( midi_meta_data_item( 0, "display_name", (const char *) &extra_buffer[0] ) );
            }
            it += 8 + chunk_size;
            if ( chunk_size & 1 && it != body_end ) ++it;
        }
        else if ( it_equal( it, "LIST", 4 ) )
        {
            std::vector<uint8_t>::const_iterator chunk_end = it + 8 + chunk_size;
            if ( it_equal( it + 8, "INFO", 4 ) )
            {
                if ( !found_info )
                {
                    if ( chunk_end - it < 12 ) return false;
                    it += 12;
                    while ( it != chunk_end )
                    {
                        if ( chunk_end - it < 4 ) return false;
                        uint32_t field_size = toInt32LE( it + 4 );
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
                        extra_buffer.resize( field_size + 1 );
                        std::copy( it + 8, it + 8 + field_size, extra_buffer.begin() );
						extra_buffer[ field_size ] = '\0';
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
