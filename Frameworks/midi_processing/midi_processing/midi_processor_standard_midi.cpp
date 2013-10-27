#include "midi_processor.h"

bool midi_processor::is_standard_midi( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 18 ) return false;
    if ( p_file[ 0 ] != 'M' || p_file[ 1 ] != 'T' || p_file[ 2 ] != 'h' || p_file[ 3 ] != 'd') return false;
    if ( p_file[ 4 ] != 0 || p_file[ 5 ] != 0 || p_file[ 6 ] != 0 || p_file[ 7 ] != 6 ) return false;
    if ( p_file[ 14 ] != 'M' || p_file[ 15 ] != 'T' || p_file[ 16 ] != 'r' || p_file[ 17 ] != 'k' ) return false;
    return true;
}

bool midi_processor::process_standard_midi_track( std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, midi_container & p_out, bool needs_end_marker )
{
	midi_track track;
	unsigned current_timestamp = 0;
	unsigned char last_event_code = 0xFF;

    std::vector<uint8_t> buffer;
    buffer.resize( 3 );

    for (;;)
    {
        if ( !needs_end_marker && it == end ) break;
        int delta = decode_delta( it, end );
        if ( !needs_end_marker && it == end ) break;

        if ( delta < 0 )
        {
            /*"MIDI processor encountered negative delta: " << delta << "; flipping sign.";*/
            delta = -delta;
        }

        current_timestamp += delta;
        if ( it == end ) return false;
        unsigned char event_code = *it++;
        unsigned data_bytes_read = 0;
        if ( event_code < 0x80 )
        {
            if ( last_event_code == 0xFF ) return false; /*throw exception_io_data("First MIDI track event short encoded");*/
            buffer[ data_bytes_read++ ] = event_code;
            event_code = last_event_code;
        }
        if ( event_code < 0xF0 )
        {
            last_event_code = event_code;
            if ( !needs_end_marker && ( event_code & 0xF0 ) == 0xE0 ) continue;
            if ( data_bytes_read < 1 )
            {
				if ( it == end ) return false;
                buffer[ 0 ] = *it++;
                ++data_bytes_read;
            }
            switch ( event_code & 0xF0 )
            {
            case 0xC0:
            case 0xD0:
                break;
            default:
				if ( it == end ) return false;
                buffer[ data_bytes_read ] = *it++;
                ++data_bytes_read;
            }
            track.add_event( midi_event( current_timestamp, (midi_event::event_type)(( event_code >> 4 ) - 8), event_code & 0x0F, &buffer[0], data_bytes_read ) );
        }
        else if ( event_code == 0xF0 )
        {
            int data_count = decode_delta( it, end );
            if ( data_count < 0 ) return false; /*throw exception_io_data( "Invalid System Exclusive message" );*/
			if ( end - it < data_count ) return false;
            buffer.resize( data_count + 1 );
            buffer[ 0 ] = 0xF0;
            std::copy( it, it + data_count, buffer.begin() + 1 );
            it += data_count;
            track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], data_count + 1 ) );
        }
        else if ( event_code == 0xFF )
        {
			if ( it == end ) return false;
            unsigned char meta_type = *it++;
            int data_count = decode_delta( it, end );
            if ( data_count < 0 ) return false; /*throw exception_io_data( "Invalid meta message" );*/
			if ( end - it < data_count ) return false;
            buffer.resize( data_count + 2 );
            buffer[ 0 ] = 0xFF;
            buffer[ 1 ] = meta_type;
            std::copy( it, it + data_count, buffer.begin() + 2 );
            it += data_count;
            track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], data_count + 2 ) );

            if ( meta_type == 0x2F )
            {
                needs_end_marker = true;
                break;
            }
        }
        else if ( event_code == 0xFB || event_code == 0xFC )
        {
            /*console::formatter() << "MIDI " << ( ( event_code == 0xFC ) ? "stop" : "start" ) << " status code ignored";*/
        }
        else return false; /*throw exception_io_data("Unhandled MIDI status code");*/
    }

    if ( !needs_end_marker )
	{
		buffer[ 0 ] = 0xFF;
		buffer[ 1 ] = 0x2F;
        track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], 2 ) );
	}

	p_out.add_track( track );

    return true;
}

bool midi_processor::process_standard_midi( std::vector<uint8_t> const& p_file, midi_container & p_out )
{
    if ( p_file[ 0 ] != 'M' || p_file[ 1 ] != 'T' || p_file[ 2 ] != 'h' || p_file[ 3 ] != 'd' ) return false;
    if ( p_file[ 4 ] != 0 || p_file[ 5 ] != 0 || p_file[ 6 ] != 0 || p_file[ 7 ] != 6 ) return false; /*throw exception_io_data("Bad MIDI header size");*/

    std::vector<uint8_t>::const_iterator it = p_file.begin() + 8;
	std::vector<uint8_t>::const_iterator end = p_file.end();

    uint16_t form = ( it[0] << 8 ) | it[1];
    if ( form > 2 ) return false;

    uint16_t track_count_16 = ( it[2] << 8 ) | it[3];
    uint16_t dtx = ( it[4] << 8 ) | it[5];

    it += 6;

    std::size_t track_count = track_count_16;

	p_out.initialize( form, dtx );

    for ( std::size_t i = 0; i < track_count; ++i )
	{
		if ( end - it < 8 ) return false;
        if ( it[0] != 'M' || it[1] != 'T' || it[2] != 'r' || it[3] != 'k' ) return false;

        uint32_t track_size = ( it[4] << 24 ) | ( it[5] << 16 ) | ( it[6] << 8 ) | it[7];
		if ( (unsigned long)(end - it) < track_size ) return false;

        it += 8;

        intptr_t track_data_offset = it - p_file.begin();

        if ( !process_standard_midi_track( it, it + track_size, p_out, true ) ) return false;

		track_data_offset += track_size;
        if ( it - p_file.begin() != track_data_offset )
		{
            it = p_file.begin() + track_data_offset;
		}
	}

    return true;
}
