#include "midi_processor.h"

bool midi_processor::is_syx( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 2 ) return false;
    if ( p_file[ 0 ] != 0xF0 || p_file[ p_file.size() - 1 ] != 0xF7 ) return false;
    return true;
}

bool midi_processor::process_syx( std::vector<uint8_t> const& p_file, midi_container & p_out )
{
    const size_t size = p_file.size();
    size_t ptr = 0;

    p_out.initialize( 0, 1 );

    midi_track track;

    while ( ptr < size )
    {
        size_t msg_length = 1;

        if ( p_file[ptr] != 0xF0 ) return false;

        while ( p_file[ptr + msg_length++] != 0xF7 );

        track.add_event( midi_event( 0, midi_event::extended, 0, &p_file[ptr], msg_length ) );

        ptr += msg_length;
    }

    p_out.add_track( track );

    return true;
}
