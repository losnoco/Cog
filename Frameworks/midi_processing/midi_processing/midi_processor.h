#ifndef _MIDI_PROCESSORS_H_
#define _MIDI_PROCESSORS_H_

#include "midi_container.h"

#ifndef _countof
template <typename T, size_t N>
char ( &_ArraySizeHelper( T (&array)[N] ))[N];
#define _countof( array ) (sizeof( _ArraySizeHelper( array ) ))
#endif

class midi_processor
{
    static const uint8_t end_of_track[2];
    static const uint8_t loop_start[11];
    static const uint8_t loop_end[9];

    static const uint8_t hmp_default_tempo[5];

    static const uint8_t xmi_default_tempo[5];

    static const uint8_t mus_default_tempo[5];
    static const uint8_t mus_controllers[15];

    static const uint8_t lds_default_tempo[5];

    static int decode_delta( std::vector<uint8_t>::const_iterator & it );
    static unsigned decode_hmp_delta( std::vector<uint8_t>::const_iterator & it );
    static unsigned decode_xmi_delta( std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end );

    static bool is_standard_midi( std::vector<uint8_t> const& p_file );
    static bool is_riff_midi( std::vector<uint8_t> const& p_file );
    static bool is_hmp( std::vector<uint8_t> const& p_file );
    static bool is_hmi( std::vector<uint8_t> const& p_file );
    static bool is_xmi( std::vector<uint8_t> const& p_file );
    static bool is_mus( std::vector<uint8_t> const& p_file );
    static bool is_mids( std::vector<uint8_t> const& p_file );
    static bool is_lds( std::vector<uint8_t> const& p_file, const char * p_extension );
    static bool is_gmf( std::vector<uint8_t> const& p_file );
    static bool is_syx( std::vector<uint8_t> const& p_file );

    static bool process_standard_midi_track( std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, midi_container & p_out, bool needs_end_marker );

    static bool process_standard_midi( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_riff_midi( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_hmp( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_hmi( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_xmi( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_mus( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_mids( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_lds( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_gmf( std::vector<uint8_t> const& p_file, midi_container & p_out );
    static bool process_syx( std::vector<uint8_t> const& p_file, midi_container & p_out );

public:
    static bool process_file( std::vector<uint8_t> const& p_file, const char * p_extension, midi_container & p_out );

    static bool process_syx_file( std::vector<uint8_t> const& p_file, midi_container & p_out );
};

#endif
