#include "midi_processor.h"

bool midi_processor::is_hmi( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 12 ) return false;
    if ( p_file[ 0 ] != 'H' || p_file[ 1 ] != 'M' || p_file[ 2 ] != 'I' || p_file[ 3 ] != '-' ||
         p_file[ 4 ] != 'M' || p_file[ 5 ] != 'I' || p_file[ 6 ] != 'D' || p_file[ 7 ] != 'I' ||
         p_file[ 8 ] != 'S' || p_file[ 9 ] != 'O' || p_file[ 10 ] != 'N' || p_file[ 11 ] != 'G' ) return false;
    return true;
}

bool midi_processor::process_hmi( std::vector<uint8_t> const& p_file, midi_container & p_out )
{
    std::vector<uint8_t> buffer;

    std::vector<uint8_t>::const_iterator it = p_file.begin() + 0xE4;

    uint32_t track_count        = it[ 0 ] | ( it[ 1 ] << 8 ) | ( it[ 2 ] << 16 ) | ( it[ 3 ] << 24 );
    uint32_t track_table_offset = it[ 4 ] | ( it[ 5 ] << 8 ) | ( it[ 6 ] << 16 ) | ( it[ 7 ] << 24 );

	if ( track_table_offset >= p_file.size() || track_table_offset + track_count * 4 > p_file.size() )
		return false;

    it = p_file.begin() + track_table_offset;

    std::vector<uint32_t> track_offsets;
    track_offsets.resize( track_count );

	for ( unsigned i = 0; i < track_count; ++i )
	{
        track_offsets[ i ] = it[ 0 ] | ( it[ 1 ] << 8 ) | ( it[ 2 ] << 16 ) | ( it[ 3 ] << 24 );
        it += 4;
	}

	p_out.initialize( 1, 0xC0 );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, hmp_default_tempo, _countof( hmp_default_tempo ) ) );
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

	for ( unsigned i = 0; i < track_count; ++i )
	{
		unsigned track_offset = track_offsets[ i ];
		unsigned long track_length;
		if ( i + 1 < track_count )
		{
			track_length = track_offsets[ i + 1 ] - track_offset;
		}
		else
		{
            track_length = p_file.size() - track_offset;
		}
		if ( track_offset >= p_file.size() || track_offset + track_length > p_file.size() )
			return false;

        std::vector<uint8_t>::const_iterator track_body = p_file.begin() + track_offset;
        std::vector<uint8_t>::const_iterator track_end = track_body + track_length;

        if ( track_length < 13 ) return false;
        if ( track_body[ 0 ] != 'H' || track_body[ 1 ] != 'M' || track_body[ 2 ] != 'I' || track_body[ 3 ] != '-' ||
             track_body[ 4 ] != 'M' || track_body[ 5 ] != 'I' || track_body[ 6 ] != 'D' || track_body[ 7 ] != 'I' ||
             track_body[ 8 ] != 'T' || track_body[ 9 ] != 'R' || track_body[ 10 ] != 'A' || track_body[ 11 ] != 'C' ||
             track_body[ 12 ] != 'K' ) return false;

		midi_track track;
		unsigned current_timestamp = 0;
		unsigned char last_event_code = 0xFF;

		unsigned last_event_timestamp = 0;

        if ( track_length < 0x4B + 4 ) return false;

        uint32_t meta_offset = track_body[ 0x4B ] | ( track_body[ 0x4C ] << 8 ) | ( track_body[ 0x4D ] << 16 ) | ( track_body[ 0x4E ] << 24 );
        if ( meta_offset && meta_offset + 1 < track_length )
		{
            buffer.resize( 2 );
            std::copy( track_body + meta_offset, track_body + meta_offset + 2, buffer.begin() );
			unsigned meta_size = buffer[ 1 ];
            if ( meta_offset + 2 + meta_size > track_length ) return false;
            buffer.resize( meta_size + 2 );
            std::copy( track_body + meta_offset + 2, track_body + meta_offset + 2 + meta_size, buffer.begin() + 2 );
			while ( meta_size > 0 && buffer[ meta_size + 1 ] == ' ' ) --meta_size;
            if ( meta_size > 0 )
            {
                buffer[ 0 ] = 0xFF;
                buffer[ 1 ] = 0x01;
                track.add_event( midi_event( 0, midi_event::extended, 0, &buffer[0], meta_size + 2 ) );
            }
		}

        if ( track_length < 0x57 + 4 ) return false;

        uint32_t track_data_offset = track_body[ 0x57 ] | ( track_body[ 0x58 ] << 8 ) | ( track_body[ 0x59 ] << 16 ) | ( track_body[ 0x5A ] << 24 );

        it = track_body + track_data_offset;

        buffer.resize( 3 );

        while ( it != track_end )
		{
            int delta = decode_delta( it, track_end );
			if ( delta > 0xFFFF || delta < 0 )
			{
				current_timestamp = last_event_timestamp;
                /*console::formatter() << "[foo_midi] Large HMI delta detected, shunting.";*/
			}
			else
			{
				current_timestamp += delta;
				if ( current_timestamp > last_event_timestamp )
				{
					last_event_timestamp = current_timestamp;
				}
			}

			if ( it == track_end ) return false;
            buffer[ 0 ] = *it++;
			if ( buffer[ 0 ] == 0xFF )
			{
				last_event_code = 0xFF;
				if ( it == track_end ) return false;
                buffer[ 1 ] = *it++;
                int meta_count = decode_delta( it, track_end );
                if ( meta_count < 0 ) return false; /*throw exception_io_data( "Invalid HMI meta message" );*/
				if ( track_end - it < meta_count ) return false;
                buffer.resize( meta_count + 2 );
                std::copy( it, it + meta_count, buffer.begin() + 2 );
                it += meta_count;
				if ( buffer[ 1 ] == 0x2F && last_event_timestamp > current_timestamp )
				{
					current_timestamp = last_event_timestamp;
				}
                track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], meta_count + 2 ) );
				if ( buffer[ 1 ] == 0x2F ) break;
			}
			else if ( buffer[ 0 ] == 0xF0 )
			{
				last_event_code = 0xFF;
                int system_exclusive_count = decode_delta( it, track_end );
                if ( system_exclusive_count < 0 ) return false; /*throw exception_io_data( "Invalid HMI System Exclusive message" );*/
				if ( track_end - it < system_exclusive_count ) return false;
                buffer.resize( system_exclusive_count + 1 );
                std::copy( it, it + system_exclusive_count, buffer.begin() + 1 );
                it += system_exclusive_count;
                track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], system_exclusive_count + 1 ) );
			}
			else if ( buffer[ 0 ] == 0xFE )
			{
				last_event_code = 0xFF;
				if ( it == track_end ) return false;
                buffer[ 1 ] = *it++;
				if ( buffer[ 1 ] == 0x10 )
				{
					if ( track_end - it < 3 ) return false;
                    it += 2;
                    buffer[ 2 ] = *it++;
					if ( track_end - it < buffer[ 2 ] + 4 ) return false;
                    it += buffer[ 2 ] + 4;
				}
				else if ( buffer[ 1 ] == 0x12 )
				{
					if ( track_end - it < 2 ) return false;
                    it += 2;
				}
				else if ( buffer[ 1 ] == 0x13 )
				{
					if ( track_end - it < 10 ) return false;
                    it += 10;
				}
				else if ( buffer[ 1 ] == 0x14 )
				{
					if ( track_end - it < 2 ) return false;
                    it += 2;
					p_out.add_track_event( 0, midi_event( current_timestamp, midi_event::extended, 0, loop_start, _countof( loop_start ) ) );
				}
				else if ( buffer[ 1 ] == 0x15 )
				{
					if ( track_end - it < 6 ) return false;
                    it += 6;
					p_out.add_track_event( 0, midi_event( current_timestamp, midi_event::extended, 0, loop_end, _countof( loop_end ) ) );
				}
                else return false; /*throw exception_io_data( "Unexpected HMI meta event" );*/
			}
			else if ( buffer[ 0 ] <= 0xEF )
			{
				unsigned bytes_read = 1;
				if ( buffer[ 0 ] >= 0x80 )
				{
					if ( it == track_end ) return false;
                    buffer[ 1 ] = *it++;
					last_event_code = buffer[ 0 ];
				}
				else
				{
                    if ( last_event_code == 0xFF ) return false; /*throw exception_io_data( "HMI used shortened event after Meta or SysEx message" );*/
					buffer[ 1 ] = buffer[ 0 ];
					buffer[ 0 ] = last_event_code;
				}
				midi_event::event_type type = (midi_event::event_type)( ( buffer[ 0 ] >> 4 ) - 8 );
				unsigned channel = buffer[ 0 ] & 0x0F;
				if ( type != midi_event::program_change && type != midi_event::channel_aftertouch )
				{
					if ( it == track_end ) return false;
                    buffer[ 2 ] = *it++;
					bytes_read = 2;
				}
                track.add_event( midi_event( current_timestamp, type, channel, &buffer[ 1 ], bytes_read ) );
				if ( type == midi_event::note_on )
				{
					buffer[ 2 ] = 0x00;
                    int note_length = decode_delta( it, track_end );
                    if ( note_length < 0 ) return false; /*throw exception_io_data( "Invalid HMI note message" );*/
					unsigned note_end_timestamp = current_timestamp + note_length;
					if ( note_end_timestamp > last_event_timestamp ) last_event_timestamp = note_end_timestamp;
                    track.add_event( midi_event( note_end_timestamp, midi_event::note_on, channel, &buffer[1], bytes_read ) );
				}
			}
            else return false; /*throw exception_io_data( "Unexpected HMI status code" );*/
		}

		p_out.add_track( track );
	}

    return true;
}
