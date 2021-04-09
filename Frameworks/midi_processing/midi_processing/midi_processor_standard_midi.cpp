#include "midi_processor.h"

bool midi_processor::is_standard_midi( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 18 ) return false;
    if ( p_file[ 0 ] != 'M' || p_file[ 1 ] != 'T' || p_file[ 2 ] != 'h' || p_file[ 3 ] != 'd') return false;
    if ( p_file[ 4 ] != 0 || p_file[ 5 ] != 0 || p_file[ 6 ] != 0 || p_file[ 7 ] != 6 ) return false;
    if ( p_file[ 10 ] == 0 && p_file[ 11 ] == 0 ) return false; // no tracks
    if ( p_file[ 12 ] == 0 && p_file[ 13 ] == 0 ) return false; // dtx == 0, will cause division by zero on tempo calculations
    return true;
}

bool midi_processor::process_standard_midi_count( std::vector<uint8_t> const& p_file, size_t & track_count )
{
    track_count = 0;

    if ( p_file[ 0 ] != 'M' || p_file[ 1 ] != 'T' || p_file[ 2 ] != 'h' || p_file[ 3 ] != 'd' ) return false;
    if ( p_file[ 4 ] != 0 || p_file[ 5 ] != 0 || p_file[ 6 ] != 0 || p_file[ 7 ] != 6 ) return false; /*throw exception_io_data("Bad MIDI header size");*/

    std::vector<uint8_t>::const_iterator it = p_file.begin() + 8;

    uint16_t form = ( it[0] << 8 ) | it[1];
    if ( form > 2 ) return false;

    uint16_t track_count_16 = ( it[2] << 8 ) | it[3];

    if ( form == 2 ) track_count = track_count_16;
    else track_count = 1;

    return true;
}

void midi_processor::process_standard_midi_track( std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, midi_container & p_out)
{
    bool needs_added_end_marker = true;
    
    midi_track track;
    midi_event event;
    bool event_ready = false;
    unsigned current_timestamp = 0;
    unsigned next_timestamp;
    unsigned char last_event_code = 0xFF;

    unsigned last_sysex_length = 0;
    unsigned last_sysex_timestamp = 0;

    std::vector<uint8_t> buffer;
    buffer.resize( 3 );

    for (;;)
    {
        if ( needs_added_end_marker && it == end ) goto trackerror;
        int delta = decode_delta( it, end );
        if ( needs_added_end_marker && it == end ) goto trackerror;

        if ( delta < 0 )
        {
            /*"MIDI processor encountered negative delta: " << delta << "; flipping sign.";*/
            delta = -delta;
        }
        
        if ( delta > 1000000 )
            break;

        next_timestamp = current_timestamp + delta;
        if ( it == end ) goto trackerror;
        
        if ( event_ready )
        {
            track.add_event( event );
            event_ready = false;
        }
        
        unsigned char event_code = *it++;
        unsigned data_bytes_read = 0;
        if ( event_code < 0x80 )
        {
            if ( last_event_code == 0xFF ) goto trackerror; /*throw exception_io_data("First MIDI track event short encoded");*/
            buffer[ data_bytes_read++ ] = event_code;
            event_code = last_event_code;
        }
        if ( event_code < 0xF0 )
        {
            if ( last_sysex_length )
            {
                event = midi_event( last_sysex_timestamp, midi_event::extended, 0, &buffer[0], last_sysex_length );
                event_ready = true;
                last_sysex_length = 0;
            }

            last_event_code = event_code;
            if ( needs_added_end_marker && ( event_code & 0xF0 ) == 0xE0 ) continue;
            if ( data_bytes_read < 1 )
            {
                if ( it == end ) goto trackerror;
                buffer[ 0 ] = *it++;
                ++data_bytes_read;
            }
            switch ( event_code & 0xF0 )
            {
            case 0xC0:
            case 0xD0:
                break;
            default:
                if ( it == end ) goto trackerror;
                buffer[ data_bytes_read ] = *it++;
                ++data_bytes_read;
            }
            current_timestamp = next_timestamp;
            if ( event_ready )
                track.add_event( event );
            event = midi_event( current_timestamp, (midi_event::event_type)(( event_code >> 4 ) - 8), event_code & 0x0F, &buffer[0], data_bytes_read );
            event_ready = true;
        }
        else if ( event_code == 0xF0 )
        {
            if ( last_sysex_length )
            {
                event = midi_event( last_sysex_timestamp, midi_event::extended, 0, &buffer[0], last_sysex_length );
                event_ready = true;
                last_sysex_length = 0;
            }

            int data_count = decode_delta( it, end );
            if ( data_count < 0 ) goto trackerror; /*throw exception_io_data( "Invalid System Exclusive message" );*/
            if ( end - it < data_count ) goto trackerror;
            buffer.resize( data_count + 1 );
            buffer[ 0 ] = 0xF0;
            std::copy( it, it + data_count, buffer.begin() + 1 );
            it += data_count;
            last_sysex_length = data_count + 1;
            current_timestamp = next_timestamp;
            last_sysex_timestamp = current_timestamp;
        }
        else if ( event_code == 0xF7 ) // SysEx continuation
        {
            if ( !last_sysex_length ) goto trackerror;
            int data_count = decode_delta( it, end );
            if ( data_count < 0 ) goto trackerror;
            if ( end - it < data_count ) goto trackerror;
            buffer.resize( last_sysex_length + data_count );
            std::copy( it, it + data_count, buffer.begin() + last_sysex_length );
            it += data_count;
            last_sysex_length += data_count;
        }
        else if ( event_code == 0xFF )
        {
            if ( last_sysex_length )
            {
                event = midi_event( last_sysex_timestamp, midi_event::extended, 0, &buffer[0], last_sysex_length );
                event_ready = true;
                last_sysex_length = 0;
            }

            if ( it == end ) goto trackerror;
            unsigned char meta_type = *it++;
            int data_count = decode_delta( it, end );
            if ( data_count < 0 ) break; /*throw exception_io_data( "Invalid meta message" );*/
            if ( end - it < data_count ) goto trackerror;
            buffer.resize( data_count + 2 );
            buffer[ 0 ] = 0xFF;
            buffer[ 1 ] = meta_type;
            std::copy( it, it + data_count, buffer.begin() + 2 );
            it += data_count;
            current_timestamp = next_timestamp;
            if ( event_ready )
                track.add_event( event );
            event = midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], data_count + 2 );
            event_ready = true;

            if ( meta_type == 0x2F )
            {
                needs_added_end_marker = false;
                break;
            }
        }
        else if ( event_code >= 0xF8 && event_code <= 0xFE )
        {
            /* Sequencer specific events, single byte */
            buffer[ 0 ] = event_code;
            current_timestamp = next_timestamp;
            event = midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], 1 );
            event_ready = true;
        }
        else break; /*throw exception_io_data("Unhandled MIDI status code");*/
    }
    
    if ( event_ready )
        track.add_event( event );
    
trackerror:

    if ( needs_added_end_marker )
    {
        buffer[ 0 ] = 0xFF;
        buffer[ 1 ] = 0x2F;
        track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], 2 ) );
    }

    p_out.add_track( track );
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

    if ( !track_count_16 || !dtx )
        return false;

    it += 6;

    std::size_t track_count = track_count_16;

    p_out.initialize( form, dtx );

    for ( std::size_t i = 0; i < track_count; ++i )
    {
        if ( end - it < 8 ) break;
        while ( it[0] != 'M' || it[1] != 'T' || it[2] != 'r' || it[3] != 'k' ) {
            uint32_t chunk_size = ( it[4] << 24 ) | ( it[5] << 16 ) | ( it[6] << 8 ) | it[7];
            if ( (unsigned long)(end - it) < 8 + chunk_size )
                break;
            it += 8 + chunk_size;
            if ( end - it < 8 ) break;
        }
        
        if ( end - it < 8 ) break;

        uint32_t track_size = ( it[4] << 24 ) | ( it[5] << 16 ) | ( it[6] << 8 ) | it[7];

        it += 8;

        intptr_t track_data_offset = it - p_file.begin();

        process_standard_midi_track( it, it + track_size, p_out );

        track_data_offset += track_size;
        size_t messup_offset = it - p_file.begin();
        if ( messup_offset != track_data_offset )
        {
            /* Assume messed up track, attempt to skip it. */
            if ( end - p_file.begin() >= track_data_offset )
                it = p_file.begin() + track_data_offset;
            else
                break;
        }
    }

    return !!p_out.get_track_count();
}
