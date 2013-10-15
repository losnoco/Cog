#include "midi_processor.h"

const uint8_t midi_processor::hmp_default_tempo[5] = {0xFF, 0x51, 0x18, 0x80, 0x00};

bool midi_processor::is_hmp( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 8 ) return false;
    if ( p_file[ 0 ] != 'H' || p_file[ 1 ] != 'M' || p_file[ 2 ] != 'I' || p_file[ 3 ] != 'M' ||
         p_file[ 4 ] != 'I' || p_file[ 5 ] != 'D' || p_file[ 6 ] != 'I' ||
         ( p_file[ 7 ] != 'P' && p_file[ 7 ] != 'R' ) ) return false;
    return true;
}

unsigned midi_processor::decode_hmp_delta( std::vector<uint8_t>::const_iterator & it )
{
	unsigned delta = 0;
	unsigned shift = 0;
	unsigned char byte;
	do
    {
        byte = *it++;
		delta = delta + ( ( byte & 0x7F ) << shift );
		shift += 7;
	}
	while ( !( byte & 0x80 ) );
	return delta;
}

bool midi_processor::process_hmp( std::vector<uint8_t> const& p_file, midi_container & p_out )
{
    bool is_funky = p_file[ 7 ] == 'R';

    uint8_t track_count_8;
    uint16_t dtx = 0xC0;

    std::vector<uint8_t>::const_iterator it = p_file.begin() + ( is_funky ? 0x1A : 0x30 );

    track_count_8 = *it;

	if ( is_funky )
    {
        dtx = ( p_file[ 0x4C ] << 16 ) | p_file[ 0x4D ];
	}

	p_out.initialize( 1, dtx );

    {
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, hmp_default_tempo, _countof( hmp_default_tempo ) ) );
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

    uint8_t buffer[ 4 ];

    buffer[ 0 ] = *it++;

    while ( it < p_file.end() )
	{
		if ( buffer[ 0 ] != 0xFF )
		{
            buffer[ 0 ] = *it++;
			continue;
		}
        buffer[ 1 ] = *it++;
		if ( buffer[ 1 ] != 0x2F )
		{
			buffer[ 0 ] = buffer[ 1 ];
			continue;
		}
		break;
	}

    it += ( is_funky ? 3 : 5 );

	unsigned track_count = track_count_8;

	for ( unsigned i = 1; i < track_count; ++i )
	{
        uint16_t track_size_16;
        uint32_t track_size_32;

		if ( is_funky )
		{
            track_size_16 = it[ 0 ] | ( it[ 1 ] << 8 );
            it += 2;
            track_size_32 = track_size_16 - 4;
            if ( p_file.end() - it < track_size_32 + 2 ) break;
            it += 2;
		}
		else
		{
            track_size_32 = it[ 0 ] | ( it[ 1 ] << 8 ) | ( it[ 2 ] << 16 ) | ( it[ 3 ] << 24 );
            it += 4;
			track_size_32 -= 12;
            if ( p_file.end() - it < track_size_32 + 8 ) break;
            it += 4;
		}

		midi_track track;

		unsigned current_timestamp = 0;

        std::vector<uint8_t> buffer;
        buffer.resize( 3 );

        std::vector<uint8_t>::const_iterator track_end = it + track_size_32;

        while ( it < track_end )
		{
            unsigned delta = decode_hmp_delta( it );
			current_timestamp += delta;
            buffer[ 0 ] = *it++;
			if ( buffer[ 0 ] == 0xFF )
			{
                buffer[ 1 ] = *it++;
                int meta_count = decode_delta( it );
                if ( meta_count < 0 ) return false; /*throw exception_io_data( "Invalid HMP meta message" );*/
                buffer.resize( meta_count + 2 );
                std::copy( it, it + meta_count, buffer.begin() + 2 );
                it += meta_count;
                track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], meta_count + 2 ) );
				if ( buffer[ 1 ] == 0x2F ) break;
			}
			else if ( buffer[ 0 ] >= 0x80 && buffer[ 0 ] <= 0xEF )
			{
				unsigned bytes_read = 2;
				switch ( buffer[ 0 ] & 0xF0 )
				{
				case 0xC0:
				case 0xD0:
					bytes_read = 1;
                }
                std::copy( it, it + bytes_read, buffer.begin() + 1 );
                it += bytes_read;
                track.add_event( midi_event( current_timestamp, (midi_event::event_type)( ( buffer[ 0 ] >> 4 ) - 8 ), buffer[ 0 ] & 0x0F, &buffer[1], bytes_read ) );
			}
            else return false; /*throw exception_io_data( "Unexpected status code in HMP track" );*/
		}

        it = track_end + ( is_funky ? 0 : 4 );

		p_out.add_track( track );
	}

    return true;
}
