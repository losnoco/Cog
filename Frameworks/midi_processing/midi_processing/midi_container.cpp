#include "midi_container.h"

#include <string.h>

#include <algorithm>

midi_event::midi_event( const midi_event & p_in )
{
    m_timestamp = p_in.m_timestamp;
    m_channel = p_in.m_channel;
    m_type = p_in.m_type;
    m_data_count = p_in.m_data_count;
    memcpy( m_data, p_in.m_data, m_data_count );
    m_ext_data = p_in.m_ext_data;
}

midi_event::midi_event( unsigned long p_timestamp, event_type p_type, unsigned p_channel, const uint8_t * p_data, std::size_t p_data_count )
{
    m_timestamp = p_timestamp;
    m_type = p_type;
    m_channel = p_channel;
    if ( p_data_count <= max_static_data_count )
    {
        m_data_count = p_data_count;
        memcpy( m_data, p_data, p_data_count );
    }
    else
    {
        m_data_count = max_static_data_count;
        memcpy( m_data, p_data, max_static_data_count );
        m_ext_data.assign( p_data + max_static_data_count, p_data + p_data_count );
    }
}

unsigned long midi_event::get_data_count() const
{
    return m_data_count + m_ext_data.size();
}

void midi_event::copy_data( uint8_t * p_out, unsigned long p_offset, unsigned long p_count ) const
{
    unsigned long max_count = m_data_count + m_ext_data.size() - p_offset;
    p_count = std::min( p_count, max_count );
    if ( p_offset < max_static_data_count )
    {
        unsigned long _max_count = max_static_data_count - p_offset;
        unsigned long count = std::min( _max_count, p_count );
        memcpy( p_out, m_data + p_offset, count );
        p_offset -= count;
        p_count -= count;
        p_out += count;
    }
    if ( p_count ) memcpy( p_out, &m_ext_data[0], p_count );
}

midi_track::midi_track(const midi_track & p_in)
{
    m_events = p_in.m_events;
}

void midi_track::add_event( const midi_event & p_event )
{
    auto it = m_events.end();

    if ( m_events.size() )
    {
        midi_event & event = *(it - 1);
        if ( event.m_type == midi_event::extended && event.get_data_count() >= 2 &&
            event.m_data[ 0 ] == 0xFF && event.m_data[ 1 ] == 0x2F )
        {
            --it;
            if ( event.m_timestamp < p_event.m_timestamp )
            {
                event.m_timestamp = p_event.m_timestamp;
            }
        }

        while ( it > m_events.begin() )
        {
            if ( (*( it - 1 )).m_timestamp <= p_event.m_timestamp ) break;
            --it;
        }
    }

    m_events.insert( it, p_event );
}

std::size_t midi_track::get_count() const
{
    return m_events.size();
}

const midi_event & midi_track::operator [] ( std::size_t p_index ) const
{
    return m_events[ p_index ];
}

midi_event & midi_track::operator [] ( std::size_t p_index )
{
    return m_events[ p_index ];
}

void midi_track::remove_event( unsigned long index )
{
    m_events.erase( m_events.begin() + index );
}

tempo_entry::tempo_entry(unsigned long p_timestamp, unsigned p_tempo)
{
    m_timestamp = p_timestamp;
    m_tempo = p_tempo;
}

void tempo_map::add_tempo( unsigned p_tempo, unsigned long p_timestamp )
{
    auto it = m_entries.end();

    while ( it > m_entries.begin() )
    {
        if ( (*( it - 1 )).m_timestamp <= p_timestamp ) break;
        --it;
    }

    if ( it > m_entries.begin() && (*( it - 1 )).m_timestamp == p_timestamp )
    {
        (*( it - 1 )).m_tempo = p_tempo;
    }
    else
    {
        m_entries.insert( it, tempo_entry( p_timestamp, p_tempo ) );
    }
}

unsigned long tempo_map::timestamp_to_ms( unsigned long p_timestamp, unsigned p_dtx ) const
{
    unsigned long timestamp_ms = 0;
    unsigned long timestamp = 0;
    auto tempo_it = m_entries.begin();
    unsigned current_tempo = 500000;

    unsigned half_dtx = p_dtx * 500;
    p_dtx = half_dtx * 2;

    while ( tempo_it < m_entries.end() && timestamp + p_timestamp >= (*tempo_it).m_timestamp )
    {
        unsigned long delta = (*tempo_it).m_timestamp - timestamp;
        timestamp_ms += ((uint64_t)current_tempo * (uint64_t)delta + half_dtx) / p_dtx;
        current_tempo = (*tempo_it).m_tempo;
        ++tempo_it;
        timestamp += delta;
        p_timestamp -= delta;
    }

    timestamp_ms += ((uint64_t)current_tempo * (uint64_t)p_timestamp + half_dtx) / p_dtx;

    return timestamp_ms;
}

std::size_t tempo_map::get_count() const
{
    return m_entries.size();
}

const tempo_entry & tempo_map::operator [] ( std::size_t p_index ) const
{
    return m_entries[ p_index ];
}

tempo_entry & tempo_map::operator [] ( std::size_t p_index )
{
    return m_entries[ p_index ];
}

system_exclusive_entry::system_exclusive_entry(const system_exclusive_entry & p_in)
{
    m_port = p_in.m_port;
    m_offset = p_in.m_offset;
    m_length = p_in.m_length;
}

system_exclusive_entry::system_exclusive_entry(std::size_t p_port, std::size_t p_offset, std::size_t p_length)
{
    m_port = p_port;
    m_offset = p_offset;
    m_length = p_length;
}

unsigned system_exclusive_table::add_entry( const uint8_t * p_data, std::size_t p_size, std::size_t p_port )
{
    for ( auto it = m_entries.begin(); it < m_entries.end(); ++it )
    {
        const system_exclusive_entry & entry = *it;
        if ( p_port == entry.m_port && p_size == entry.m_length && !memcmp( p_data, &m_data[ entry.m_offset ], p_size ) )
            return ((unsigned)(it - m_entries.begin()));
    }
    system_exclusive_entry entry( p_port, m_data.size(), p_size );
    m_data.insert( m_data.end(), p_data, p_data + p_size );
    m_entries.push_back( entry );
    return ((unsigned)(m_entries.size() - 1));
}

void system_exclusive_table::get_entry( unsigned p_index, const uint8_t * & p_data, std::size_t & p_size, std::size_t & p_port ) const
{
    const system_exclusive_entry & entry = m_entries[ p_index ];
    p_data = &m_data[ entry.m_offset ];
    p_size = entry.m_length;
    p_port = entry.m_port;
}

midi_stream_event::midi_stream_event(unsigned long p_timestamp, unsigned p_event)
{
    m_timestamp = p_timestamp;
    m_event = p_event;
}

midi_meta_data_item::midi_meta_data_item(const midi_meta_data_item & p_in)
{
    m_timestamp = p_in.m_timestamp;
    m_name = p_in.m_name;
    m_value = p_in.m_value;
}

midi_meta_data_item::midi_meta_data_item(unsigned long p_timestamp, const char * p_name, const char * p_value)
{
    m_timestamp = p_timestamp;
    m_name = p_name;
    m_value = p_value;
}

void midi_meta_data::add_item( const midi_meta_data_item & p_item )
{
    m_data.push_back( p_item );
}

void midi_meta_data::append( const midi_meta_data & p_data )
{
    m_data.insert( m_data.end(), p_data.m_data.begin(), p_data.m_data.end() );
    m_bitmap = p_data.m_bitmap;
}

bool midi_meta_data::get_item( const char * p_name, midi_meta_data_item & p_out ) const
{
    for ( unsigned i = 0; i < m_data.size(); ++i )
    {
        const midi_meta_data_item & item = m_data[ i ];
        if ( !strcasecmp( p_name, item.m_name.c_str() ) )
        {
            p_out = item;
            return true;
        }
    }
    return false;
}

bool midi_meta_data::get_bitmap( std::vector<uint8_t> & p_out )
{
    p_out = m_bitmap;
    return p_out.size() != 0;
}

void midi_meta_data::assign_bitmap( std::vector<uint8_t>::const_iterator const& begin, std::vector<uint8_t>::const_iterator const& end )
{
    m_bitmap.assign( begin, end );
}

std::size_t midi_meta_data::get_count() const
{
    return m_data.size();
}

const midi_meta_data_item & midi_meta_data::operator [] ( std::size_t p_index ) const
{
    return m_data[ p_index ];
}

void midi_container::encode_delta( std::vector<uint8_t> & p_out, unsigned long delta )
{
    unsigned shift = 7 * 4;
    while ( shift && !( delta >> shift ) )
    {
        shift -= 7;
    }
    while (shift > 0)
    {
        p_out.push_back( (unsigned char)( ( ( delta >> shift ) & 0x7F ) | 0x80 ) );
        shift -= 7;
    }
    p_out.push_back( (unsigned char)( delta & 0x7F ) );
}

unsigned long midi_container::timestamp_to_ms( unsigned long p_timestamp, unsigned long p_subsong ) const
{
    unsigned long timestamp_ms = 0;
    unsigned long timestamp = 0;
    std::size_t tempo_index = 0;
    unsigned current_tempo = 500000;

    unsigned half_dtx = m_dtx * 500;
    unsigned p_dtx = half_dtx * 2;

    unsigned long subsong_count = m_tempo_map.size();

    if ( p_subsong && subsong_count )
    {
        for ( unsigned long i = std::min( p_subsong, subsong_count ); --i; )
        {
            unsigned long count = m_tempo_map[ i ].get_count();
            if ( count )
            {
                current_tempo = m_tempo_map[ i ][ count - 1 ].m_tempo;
                break;
            }
        }
    }

    if ( p_subsong < subsong_count )
    {
        const tempo_map & m_entries = m_tempo_map[ p_subsong ];

        std::size_t tempo_count = m_entries.get_count();

        while ( tempo_index < tempo_count && timestamp + p_timestamp >= m_entries[ tempo_index ].m_timestamp )
        {
            unsigned long delta = m_entries[ tempo_index ].m_timestamp - timestamp;
            timestamp_ms += ((uint64_t)current_tempo * (uint64_t)delta + half_dtx) / p_dtx;
            current_tempo = m_entries[ tempo_index ].m_tempo;
            ++tempo_index;
            timestamp += delta;
            p_timestamp -= delta;
        }
    }

    timestamp_ms += ((uint64_t)current_tempo * (uint64_t)p_timestamp + half_dtx) / p_dtx;

    return timestamp_ms;
}

void midi_container::initialize( unsigned p_form, unsigned p_dtx )
{
    m_form = p_form;
    m_dtx = p_dtx;
    if ( p_form != 2 )
    {
        m_channel_mask.resize( 1 );
        m_channel_mask[ 0 ] = 0;
        m_tempo_map.resize( 1 );
        m_timestamp_end.resize( 1 );
        m_timestamp_end[ 0 ] = 0;
        m_timestamp_loop_start.resize( 1 );
        m_timestamp_loop_end.resize( 1 );
    }
}

void midi_container::add_track( const midi_track & p_track )
{
    unsigned i;
    unsigned long port_number = 0;

    std::vector<uint8_t> data;
    std::string device_name;

    m_tracks.push_back( p_track );

    for ( i = 0; i < p_track.get_count(); ++i )
    {
        const midi_event & event = p_track[ i ];
        if ( event.m_type == midi_event::extended && event.get_data_count() >= 5 &&
            event.m_data[ 0 ] == 0xFF && event.m_data[ 1 ] == 0x51 )
        {
            unsigned tempo = ( event.m_data[ 2 ] << 16 ) + ( event.m_data[ 3 ] << 8 ) + event.m_data[ 4 ];
            if ( m_form != 2 ) m_tempo_map[ 0 ].add_tempo( tempo, event.m_timestamp );
            else
            {
                m_tempo_map.resize( m_tracks.size() );
                m_tempo_map[ m_tracks.size() - 1 ].add_tempo( tempo, event.m_timestamp );
            }
        }
        else if ( event.m_type == midi_event::extended && event.get_data_count() >= 3 &&
            event.m_data[ 0 ] == 0xFF )
        {
            if ( event.m_data[ 1 ] == 4 || event.m_data[1] == 9 )
            {
                unsigned long data_count = event.get_data_count() - 2;
                data.resize( data_count );
                event.copy_data( &data[0], 2, data_count );
                device_name.assign( data.begin(), data.begin() + data_count );
                std::transform( device_name.begin(), device_name.end(), device_name.begin(), ::tolower );
            }
            else if ( event.m_data[ 1 ] == 0x21 )
            {
                port_number = event.m_data[ 2 ];
                limit_port_number( port_number );
                device_name.clear();
            }
        }
        else if ( event.m_type == midi_event::note_on || event.m_type == midi_event::note_off )
        {
            unsigned channel = event.m_channel;
            if ( device_name.length() )
            {
                unsigned long j, k;
                for ( j = 0, k = m_device_names[ channel ].size(); j < k; ++j )
                {
                    if ( !strcmp( m_device_names[ channel ][ j ].c_str(), device_name.c_str() ) ) break;
                }
                if ( j < k ) port_number = j;
                else
                {
                    m_device_names[ channel ].push_back( device_name );
                    port_number = k;
                }
                device_name.clear();
                limit_port_number( port_number );
            }

            channel += 16 * port_number;
            channel %= 48;
            if ( m_form != 2 ) m_channel_mask[ 0 ] |= 1ULL << channel;
            else
            {
                m_channel_mask.resize( m_tracks.size(), 0 );
                m_channel_mask[ m_tracks.size() - 1 ] |= 1ULL << channel;
            }
        }
    }

    if ( i && m_form != 2 && p_track[ i - 1 ].m_timestamp > m_timestamp_end[ 0 ] )
        m_timestamp_end[ 0 ] = p_track[ i - 1 ].m_timestamp;
    else if ( m_form == 2 )
    {
        if ( i )
            m_timestamp_end.push_back( p_track[ i - 1 ].m_timestamp );
        else
            m_timestamp_end.push_back( (unsigned)0 );
    }
}

void midi_container::add_track_event( std::size_t p_track_index, const midi_event & p_event )
{
    midi_track & track = m_tracks[ p_track_index ];

    track.add_event( p_event );

    if ( p_event.m_type == midi_event::extended && p_event.get_data_count() >= 5 &&
        p_event.m_data[ 0 ] == 0xFF && p_event.m_data[ 1 ] == 0x51 )
    {
        unsigned tempo = ( p_event.m_data[ 2 ] << 16 ) + ( p_event.m_data[ 3 ] << 8 ) + p_event.m_data[ 4 ];
        if ( m_form != 2 ) m_tempo_map[ 0 ].add_tempo( tempo, p_event.m_timestamp );
        else
        {
            m_tempo_map.resize( m_tracks.size() );
            m_tempo_map[ p_track_index ].add_tempo( tempo, p_event.m_timestamp );
        }
    }
    else if ( p_event.m_type == midi_event::note_on || p_event.m_type == midi_event::note_off )
    {
        if ( m_form != 2 ) m_channel_mask[ 0 ] |= 1ULL << p_event.m_channel;
        else
        {
            m_channel_mask.resize( m_tracks.size(), 0 );
            m_channel_mask[ p_track_index ] |= 1ULL << p_event.m_channel;
        }
    }

    if ( m_form != 2 && p_event.m_timestamp > m_timestamp_end[ 0 ] )
    {
        m_timestamp_end[ 0 ] = p_event.m_timestamp;
    }
    else if ( m_form == 2 && p_event.m_timestamp > m_timestamp_end[ p_track_index ] )
    {
        m_timestamp_end[ p_track_index ] = p_event.m_timestamp;
    }
}

void midi_container::merge_tracks( const midi_container & p_source )
{
    for ( unsigned i = 0; i < p_source.m_tracks.size(); i++ )
    {
        add_track( p_source.m_tracks[ i ] );
    }
}

void midi_container::set_track_count( unsigned count )
{
    m_tracks.resize( count );
}

void midi_container::set_extra_meta_data( const midi_meta_data & p_data )
{
    m_extra_meta_data = p_data;
}

void midi_container::apply_hackfix( unsigned hack )
{
    switch (hack)
    {
        case 0:
            for (unsigned i = 0; i < m_tracks.size(); ++i)
            {
                midi_track & t = m_tracks[ i ];
                for ( unsigned j = 0; j < t.get_count(); )
                {
                    if ( t[ j ].m_type != midi_event::extended &&
                        t[ j ].m_channel == 16 )
                    {
                        t.remove_event( j );
                    }
                    else
                    {
                        ++j;
                    }
                }
            }
            break;

        case 1:
            for (unsigned i = 0; i < m_tracks.size(); ++i)
            {
                midi_track & t = m_tracks[ i ];
                for ( unsigned j = 0; j < t.get_count(); )
                {
                    if ( t[ j ].m_type != midi_event::extended &&
                        ( t[ j ].m_channel - 10 < 6 ) )
                    {
                        t.remove_event( j );
                    }
                    else
                    {
                        ++j;
                    }
                }
            }
            break;
    }
}

void midi_container::serialize_as_stream( unsigned long subsong,
                                          std::vector<midi_stream_event> & p_stream,
                                          system_exclusive_table & p_system_exclusive,
                                          unsigned long & loop_start,
                                          unsigned long & loop_end,
                                          unsigned clean_flags ) const
{
    std::vector<uint8_t> data;
    std::vector<std::size_t> track_positions;
    std::vector<uint8_t> port_numbers;
    std::vector<std::string> device_names;
    std::size_t track_count = m_tracks.size();

    unsigned long tick_loop_start = get_timestamp_loop_start(subsong);
    unsigned long tick_loop_end = get_timestamp_loop_end(subsong);
    unsigned long local_loop_start = ~0UL;
    unsigned long local_loop_end = ~0UL;

    track_positions.resize( track_count, 0 );
    port_numbers.resize( track_count, 0 );
    device_names.resize( track_count );

    bool clean_emidi = !!( clean_flags & clean_flag_emidi );
    bool clean_instruments = !!( clean_flags & clean_flag_instruments );
    bool clean_banks = !!( clean_flags & clean_flag_banks );

    if ( clean_emidi )
    {
        for ( unsigned i = 0; i < track_count; ++i )
        {
            bool skip_track = false;
            const midi_track & track = m_tracks[ i ];
            for ( unsigned j = 0; j < track.get_count(); ++j )
            {
                const midi_event & event = track[ j ];
                if ( event.m_type == midi_event::control_change &&
                     event.m_data[ 0 ] == 110 )
                {
                    if ( event.m_data[ 1 ] != 0 && event.m_data[ 1 ] != 1 && event.m_data[ 1 ] != 127 )
                    {
                        skip_track = true;
                        break;
                    }
                }
            }
            if ( skip_track )
            {
                track_positions[ i ] = track.get_count();
            }
        }
    }

    if ( m_form == 2 )
    {
        for ( unsigned long i = 0; i < track_count; ++i )
        {
            if ( i != subsong ) track_positions[ i ] = m_tracks[ i ].get_count();
        }
    }

    for (;;)
    {
        unsigned long next_timestamp = ~0UL;
        std::size_t next_track = 0;
        for ( unsigned i = 0; i < track_count; ++i )
        {
            if ( track_positions[ i ] >= m_tracks[ i ].get_count() ) continue;
            if ( m_tracks[ i ][ track_positions[ i ] ].m_timestamp < next_timestamp )
            {
                next_timestamp = m_tracks[ i ][ track_positions[ i ] ].m_timestamp;
                next_track = i;
            }
        }
        if ( next_timestamp == ~0UL ) break;

        bool filtered = false;

        if ( clean_instruments || clean_banks )
        {
            const midi_event & event = m_tracks[ next_track ][ track_positions[ next_track ] ];
            if ( clean_instruments && event.m_type == midi_event::program_change ) filtered = true;
            else if ( clean_banks && event.m_type == midi_event::control_change &&
                ( event.m_data[ 0 ] == 0x00 || event.m_data[ 0 ] == 0x20 ) ) filtered = true;
        }

        if ( !filtered )
        {
            unsigned long tempo_track = 0;
            if ( m_form == 2 && subsong ) tempo_track = subsong;

            const midi_event & event = m_tracks[ next_track ][ track_positions[ next_track ] ];

            if ( local_loop_start == ~0UL && event.m_timestamp >= tick_loop_start )
                local_loop_start = p_stream.size();
            if ( local_loop_end == ~0UL && event.m_timestamp > tick_loop_end )
                local_loop_end = p_stream.size();

            unsigned long timestamp_ms = timestamp_to_ms( event.m_timestamp, tempo_track );
            if ( event.m_type != midi_event::extended )
            {
                if ( device_names[ next_track ].length() )
                {
                    unsigned long i, j;
                    for ( i = 0, j = m_device_names[ event.m_channel ].size(); i < j; ++i )
                    {
                        if ( !strcmp( m_device_names[ event.m_channel ][ i ].c_str(), device_names[ next_track ].c_str() ) ) break;
                    }
                    port_numbers[ next_track ] = (uint8_t) i;
                    device_names[ next_track ].clear();
                    limit_port_number( port_numbers[ next_track ] );
                }

                uint32_t event_code = ( ( event.m_type + 8 ) << 4 ) + event.m_channel;
                if ( event.m_data_count >= 1 ) event_code += event.m_data[ 0 ] << 8;
                if ( event.m_data_count >= 2 ) event_code += event.m_data[ 1 ] << 16;
                event_code += port_numbers[ next_track ] << 24;
                p_stream.push_back( midi_stream_event( timestamp_ms, event_code ) );
            }
            else
            {
                std::size_t data_count = event.get_data_count();
                if ( data_count >= 3 && event.m_data[ 0 ] == 0xF0 )
                {
                    if ( device_names[ next_track ].length() )
                    {
                        unsigned long i, j;
                        for ( i = 0, j = m_device_names[ event.m_channel ].size(); i < j; ++i )
                        {
                            if ( !strcmp( m_device_names[ event.m_channel ][ i ].c_str(), device_names[ next_track ].c_str() ) ) break;
                        }
                        port_numbers[ next_track ] = (uint8_t) i;
                        device_names[ next_track ].clear();
                        limit_port_number( port_numbers[ next_track ] );
                    }

                    data.resize( data_count );
                    event.copy_data( &data[0], 0, data_count );
                    if ( data[ data_count - 1 ] == 0xF7 )
                    {
                        uint32_t system_exclusive_index = p_system_exclusive.add_entry( &data[0], data_count, port_numbers[ next_track ] );
                        p_stream.push_back( midi_stream_event( timestamp_ms, system_exclusive_index | 0x80000000 ) );
                    }
                }
                else if ( data_count >= 3 && event.m_data[ 0 ] == 0xFF )
                {
                    if ( event.m_data[ 1 ] == 4 || event.m_data[ 1 ] == 9 )
                    {
                        unsigned long _data_count = event.get_data_count() - 2;
                        data.resize( _data_count );
                        event.copy_data( &data[0], 2, _data_count );
                        device_names[ next_track ].clear();
                        device_names[ next_track ].assign( data.begin(), data.begin() + _data_count );
                        std::transform( device_names[ next_track ].begin(), device_names[ next_track ].end(), device_names[ next_track ].begin(), ::tolower );
                    }
                    else if ( event.m_data[ 1 ] == 0x21 )
                    {
                        port_numbers[ next_track ] = event.m_data[ 2 ];
                        device_names[ next_track ].clear();
                        limit_port_number( port_numbers[ next_track ] );
                    }
                }
                else if ( data_count == 1 && event.m_data[ 0 ] >= 0xF8 )
                {
                    if ( device_names[ next_track ].length() )
                    {
                        unsigned long i, j;
                        for ( i = 0, j = m_device_names[ event.m_channel ].size(); i < j; ++i )
                        {
                            if ( !strcmp( m_device_names[ event.m_channel ][ i ].c_str(), device_names[ next_track ].c_str() ) ) break;
                        }
                        port_numbers[ next_track ] = (uint8_t) i;
                        device_names[ next_track ].clear();
                        limit_port_number( port_numbers[ next_track ] );
                    }

                    uint32_t event_code = port_numbers[ next_track ] << 24;
                    event_code += event.m_data[ 0 ];
                    p_stream.push_back( midi_stream_event( timestamp_ms, event_code ) );
                }
            }
        }

        track_positions[ next_track ]++;
    }

    loop_start = local_loop_start;
    loop_end = local_loop_end;
}

void midi_container::serialize_as_standard_midi_file( std::vector<uint8_t> & p_midi_file ) const
{
    if ( !m_tracks.size() ) return;

    std::vector<uint8_t> data;

    const char signature[] = "MThd";
    p_midi_file.insert( p_midi_file.end(), signature, signature + 4 );
    p_midi_file.push_back( 0 );
    p_midi_file.push_back( 0 );
    p_midi_file.push_back( 0 );
    p_midi_file.push_back( 6 );
    p_midi_file.push_back( 0 );
    p_midi_file.push_back( m_form );
    p_midi_file.push_back( (uint8_t) (m_tracks.size() >> 8) );
    p_midi_file.push_back( (uint8_t) m_tracks.size() );
    p_midi_file.push_back( (m_dtx >> 8) );
    p_midi_file.push_back( m_dtx );

    for ( unsigned i = 0; i < m_tracks.size(); ++i )
    {
        const midi_track & track = m_tracks[ i ];
        unsigned long last_timestamp = 0;
        unsigned char last_event_code = 0xFF;
        std::size_t length_offset;

        const char _signature[] = "MTrk";
        p_midi_file.insert( p_midi_file.end(), _signature, _signature + 4 );

        length_offset = p_midi_file.size();
        p_midi_file.push_back( 0 );
        p_midi_file.push_back( 0 );
        p_midi_file.push_back( 0 );
        p_midi_file.push_back( 0 );

        for ( unsigned j = 0; j < track.get_count(); ++j )
        {
            const midi_event & event = track[ j ];
            encode_delta( p_midi_file, event.m_timestamp - last_timestamp );
            last_timestamp = event.m_timestamp;
            if ( event.m_type != midi_event::extended )
            {
                const unsigned char event_code = ( ( event.m_type + 8 ) << 4 ) + event.m_channel;
                if ( event_code != last_event_code )
                {
                    p_midi_file.push_back( event_code );
                    last_event_code = event_code;
                }
                p_midi_file.insert( p_midi_file.end(), event.m_data, event.m_data + event.m_data_count );
            }
            else
            {
                std::size_t data_count = event.get_data_count();
                if ( data_count >= 1 )
                {
                    if ( event.m_data[ 0 ] == 0xF0 )
                    {
                        --data_count;
                        p_midi_file.push_back( 0xF0 );
                        encode_delta( p_midi_file, data_count );
                        if ( data_count )
                        {
                            data.resize( data_count );
                            event.copy_data( &data[0], 1, data_count );
                            p_midi_file.insert( p_midi_file.end(), data.begin(), data.begin() + data_count );
                        }
                    }
                    else if ( event.m_data[ 0 ] == 0xFF && data_count >= 2 )
                    {
                        data_count -= 2;
                        p_midi_file.push_back( 0xFF );
                        p_midi_file.push_back( event.m_data[ 1 ] );
                        encode_delta( p_midi_file, data_count );
                        if ( data_count )
                        {
                            data.resize( data_count );
                            event.copy_data( &data[0], 2, data_count );
                            p_midi_file.insert( p_midi_file.end(), data.begin(), data.begin() + data_count );
                        }
                    }
                    else
                    {
                        data.resize( data_count );
                        event.copy_data( &data[0], 1, data_count );
                        p_midi_file.insert( p_midi_file.end(), data.begin(), data.begin() + data_count );
                    }
                }
            }
        }

        std::size_t track_length = p_midi_file.size() - length_offset - 4;
        p_midi_file[ length_offset + 0 ] = (unsigned char)( track_length >> 24 );
        p_midi_file[ length_offset + 1 ] = (unsigned char)( track_length >> 16 );
        p_midi_file[ length_offset + 2 ] = (unsigned char)( track_length >> 8 );
        p_midi_file[ length_offset + 3 ] = (unsigned char)track_length;
    }
}

void midi_container::promote_to_type1()
{
    if ( m_form == 0 && m_tracks.size() <= 2 )
    {
        bool meter_track_present = false;
        midi_track new_tracks[17];
        midi_track original_data_track = m_tracks[ m_tracks.size() - 1 ];
        if ( m_tracks.size() > 1 )
        {
            new_tracks[0] = m_tracks[0];
            meter_track_present = true;
        }

        m_tracks.resize( 0 );

        for ( std::size_t i = 0; i < original_data_track.get_count(); ++i )
        {
            const midi_event & event = original_data_track[ i ];

            if ( event.m_type != midi_event::extended )
            {
                new_tracks[ 1 + event.m_channel ].add_event( event );
            }
            else
            {
                if ( event.m_data[0] != 0xFF || event.get_data_count() < 2 || event.m_data[1] != 0x2F )
                {
                    new_tracks[ 0 ].add_event( event );
                }
                else
                {
                    if ( !meter_track_present )
                        new_tracks[ 0 ].add_event( event );
                    for ( std::size_t j = 1; j < 17; ++j )
                    {
                        new_tracks[ j ].add_event( event );
                    }
                }
            }
        }

        for ( std::size_t i = 0; i < 17; ++i )
        {
            if ( new_tracks[ i ].get_count() > 1 )
                add_track( new_tracks[ i ] );
        }

        m_form = 1;
    }
}

unsigned long midi_container::get_subsong_count() const
{
    unsigned long subsong_count = 0;
    for ( unsigned i = 0; i < m_channel_mask.size(); ++i )
    {
        if ( m_channel_mask[ i ] ) ++subsong_count;
    }
    return subsong_count;
}

unsigned long midi_container::get_subsong( unsigned long p_index ) const
{
    for ( unsigned i = 0; i < m_channel_mask.size(); ++i )
    {
        if ( m_channel_mask[ i ] )
        {
            if ( p_index ) --p_index;
            else return i;
        }
    }
    return 0;
}

unsigned long midi_container::get_timestamp_end(unsigned long subsong, bool ms /* = false */) const
{
    unsigned long tempo_track = 0;
    unsigned long timestamp = m_timestamp_end[ 0 ];
    if ( m_form == 2 && subsong )
    {
        tempo_track = subsong;
        timestamp = m_timestamp_end[ subsong ];
    }
    if ( !ms ) return timestamp;
    else return timestamp_to_ms( timestamp, tempo_track );
}

unsigned midi_container::get_format() const
{
    return m_form;
}

unsigned midi_container::get_track_count() const
{
    return (unsigned) m_tracks.size();
}

unsigned midi_container::get_channel_count( unsigned long subsong ) const
{
    unsigned count = 0;
    uint64_t j = 1;
    for (unsigned i = 0; i < 48; ++i, j <<= 1)
    {
        if ( m_channel_mask[ subsong ] & j ) ++count;
    }
    return count;
}

unsigned long midi_container::get_timestamp_loop_start( unsigned long subsong, bool ms /* = false */ ) const
{
    unsigned long tempo_track = 0;
    unsigned long timestamp = m_timestamp_loop_start[ 0 ];
    if ( m_form == 2 && subsong )
    {
        tempo_track = subsong;
        timestamp = m_timestamp_loop_start[ subsong ];
    }
    if ( !ms ) return timestamp;
    else if ( timestamp != ~0UL ) return timestamp_to_ms( timestamp, tempo_track );
    else return ~0UL;
}

unsigned long midi_container::get_timestamp_loop_end( unsigned long subsong, bool ms /* = false */ ) const
{
    unsigned long tempo_track = 0;
    unsigned long timestamp = m_timestamp_loop_end[ 0 ];
    if ( m_form == 2 && subsong )
    {
        tempo_track = subsong;
        timestamp = m_timestamp_loop_end[ subsong ];
    }
    if ( !ms ) return timestamp;
    else if ( timestamp != ~0UL ) return timestamp_to_ms( timestamp, tempo_track );
    else return ~0UL;
}

/* TODO: Use iconv or libintl or something to probe for code pages and convert some mess to UTF-8 */
static void convert_mess_to_utf8( const char * p_src, std::size_t p_src_len, std::string & p_dst )
{
    p_dst.assign( p_src, p_src + p_src_len );
}

void midi_container::get_meta_data( unsigned long subsong, midi_meta_data & p_out )
{
    char temp[32];
    std::string convert;

    std::vector<uint8_t> data;

    bool type_found = false;
    bool type_non_gm_found = false;

    for ( unsigned long i = 0; i < m_tracks.size(); ++i )
    {
        if ( m_form == 2 && i != subsong ) continue;

        unsigned long tempo_track = 0;
        if ( m_form == 2 ) tempo_track = i;

        const midi_track & track = m_tracks[ i ];
        for ( unsigned j = 0; j < track.get_count(); ++j )
        {
            const midi_event & event = track[ j ];
            if ( event.m_type == midi_event::extended )
            {
                std::size_t data_count = event.get_data_count();
                if ( !type_non_gm_found && data_count >= 1 && event.m_data[ 0 ] == 0xF0 )
                {
                    unsigned char test = 0;
                    unsigned char test2 = 0;
                    if ( data_count > 1 ) test  = event.m_data[ 1 ];
                    if ( data_count > 3 ) test2 = event.m_data[ 3 ];

                    const char * type = NULL;

                    switch( test )
                    {
                    case 0x7E:
                        type_found = true;
                        break;
                    case 0x43:
                        type = "XG";
                        break;
                    case 0x42:
                        type = "X5";
                        break;
                    case 0x41:
                        if ( test2 == 0x42 ) type = "GS";
                        else if ( test2 == 0x16 ) type = "MT-32";
                        else if ( test2 == 0x14 ) type = "D-50";
                    }

                    if ( type )
                    {
                        type_found = true;
                        type_non_gm_found = true;
                        p_out.add_item( midi_meta_data_item( timestamp_to_ms( event.m_timestamp, tempo_track ), "type", type ) );
                    }
                }
                else if ( data_count >= 2 && event.m_data[ 0 ] == 0xFF )
                {
                    data_count -= 2;
                    if ( !data_count ) continue;
                    switch ( event.m_data[ 1 ] )
                    {
                    case 6:
                        data.resize( data_count );
                        event.copy_data( &data[0], 2, data_count );
                        convert_mess_to_utf8( ( const char * ) &data[0], data_count, convert );
                        p_out.add_item( midi_meta_data_item( timestamp_to_ms( event.m_timestamp, tempo_track ), "track_marker", convert.c_str() ) );
                        break;

                    case 2:
                        data.resize( data_count );
                        event.copy_data( &data[0], 2, data_count );
                        convert_mess_to_utf8( ( const char * ) &data[0], data_count, convert );
                        p_out.add_item( midi_meta_data_item( timestamp_to_ms( event.m_timestamp, tempo_track ), "copyright", convert.c_str() ) );
                        break;

                    case 1:
                        data.resize( data_count );
                        event.copy_data( &data[0], 2, data_count );
                        convert_mess_to_utf8( ( const char * ) &data[0], data_count, convert );
                        snprintf(temp, 31, "track_text_%02lu", i);
                        p_out.add_item( midi_meta_data_item( timestamp_to_ms( event.m_timestamp, tempo_track ), temp, convert.c_str() ) );
                        break;

                    case 3:
                    case 4:
                        data.resize( data_count );
                        event.copy_data( &data[0], 2, data_count );
                        convert_mess_to_utf8( ( const char * ) &data[0], data_count, convert );
                        snprintf(temp, 31, "track_name_%02lu", i);
                        p_out.add_item( midi_meta_data_item( timestamp_to_ms( event.m_timestamp, tempo_track ), temp, convert.c_str() ) );
                        break;
                    }
                }
            }
        }
    }

    if ( type_found && !type_non_gm_found )
    {
        p_out.add_item( midi_meta_data_item( 0, "type", "GM" ) );
    }

    p_out.append( m_extra_meta_data );
}

void midi_container::trim_tempo_map( unsigned long p_index, unsigned long base_timestamp )
{
    if ( p_index < m_tempo_map.size() )
    {
        tempo_map & map = m_tempo_map[ p_index ];

        for ( unsigned long i = 0, j = map.get_count(); i < j; ++i )
        {
            tempo_entry & entry = map[ i ];
            if ( entry.m_timestamp >= base_timestamp )
                entry.m_timestamp -= base_timestamp;
            else
                entry.m_timestamp = 0;
        }
    }
}

void midi_container::trim_range_of_tracks(unsigned long start, unsigned long end)
{
    unsigned long timestamp_first_note = ~0UL;

    for (unsigned long i = start; i <= end; ++i)
    {
        unsigned long j, k;

        const midi_track & track = m_tracks[ i ];

        for (j = 0, k = track.get_count(); j < k; ++j)
        {
            const midi_event & event = track[ j ];

            if ( event.m_type == midi_event::note_on && event.m_data[ 0 ] )
                break;
        }

        if ( j < k )
        {
            if ( track[ j ].m_timestamp < timestamp_first_note )
                timestamp_first_note = track[ j ].m_timestamp;
        }
    }

    if ( timestamp_first_note < ~0UL && timestamp_first_note > 0 )
    {
        for (unsigned long i = start; i <= end; ++i)
        {
            midi_track & track = m_tracks[ i ];

            for (unsigned long j = 0, k = track.get_count(); j < k; ++j)
            {
                midi_event & event = track[ j ];
                if ( event.m_timestamp >= timestamp_first_note )
                    event.m_timestamp -= timestamp_first_note;
                else
                    event.m_timestamp = 0;
            }
        }

        if ( start == end )
        {
            trim_tempo_map( start, timestamp_first_note );

            m_timestamp_end[ start ] -= timestamp_first_note;

            if ( m_timestamp_loop_end[ start ] != ~0UL )
                m_timestamp_loop_end[ start ] -= timestamp_first_note;
            if ( m_timestamp_loop_start[ start ] != ~0UL )
            {
                if ( m_timestamp_loop_start[ start ] > timestamp_first_note )
                    m_timestamp_loop_start[ start ] -= timestamp_first_note;
                else
                    m_timestamp_loop_start[ start ] = 0;
            }
        }
        else
        {
            trim_tempo_map( 0, timestamp_first_note );

            m_timestamp_end[ 0 ] -= timestamp_first_note;

            if ( m_timestamp_loop_end[ 0 ] != ~0UL )
                m_timestamp_loop_end[ 0 ] -= timestamp_first_note;
            if ( m_timestamp_loop_start[ 0 ] != ~0UL )
            {
                if ( m_timestamp_loop_start[ 0 ] > timestamp_first_note )
                    m_timestamp_loop_start[ 0 ] -= timestamp_first_note;
                else
                    m_timestamp_loop_start[ 0 ] = 0;
            }
        }
    }
}

void midi_container::trim_start()
{
    if (m_form == 2)
    {
        for (unsigned long i = 0, j = m_tracks.size(); i < j; ++i)
        {
            trim_range_of_tracks(i, i);
        }
    }
    else
    {
        trim_range_of_tracks(0, m_tracks.size() - 1);
    }
}

void midi_container::split_by_instrument_changes(split_callback cb)
{
    if (m_form != 1) /* This would literally die on anything else */
        return;

    for (unsigned long i = 0, j = m_tracks.size(); i < j; ++i)
    {
        midi_track source_track = m_tracks[0];

        m_tracks.erase(m_tracks.begin());

        midi_track output_track;
        midi_track program_change;

        for (unsigned long k = 0, l = source_track.get_count(); k < l; ++k)
        {
            const midi_event & event = source_track[ k ];
            if ( event.m_type == midi_event::program_change ||
               ( event.m_type == midi_event::control_change &&
                 (event.m_data[0] == 0 || event.m_data[0] == 0x20)))
            {
                program_change.add_event( event );
            }
            else
            {
                if (program_change.get_count())
                {
                    if (output_track.get_count())
                        m_tracks.push_back( output_track );
                    output_track = program_change;
                    if (cb)
                    {
                        unsigned long timestamp = 0;
                        uint8_t bank_msb = 0, bank_lsb = 0, instrument = 0;
                        for (int i = 0, j = program_change.get_count(); i < j; ++i)
                        {
                            const midi_event & ev = program_change[i];
                            if (ev.m_type == midi_event::program_change)
                                instrument = ev.m_data[0];
                            else if (ev.m_data[0] == 0)
                                bank_msb = ev.m_data[1];
                            else
                                bank_lsb = ev.m_data[1];
                            if (ev.m_timestamp > timestamp)
                                timestamp = ev.m_timestamp;
                        }

                        std::string name = cb(bank_msb, bank_lsb, instrument);

                        std::vector<uint8_t> data;

                        data.resize(name.length() + 2);

                        data[0] = 0xFF;
                        data[1] = 0x03;

                        std::copy(name.begin(), name.end(), data.begin() + 2);

                        output_track.add_event(midi_event(timestamp, midi_event::extended, 0, &data[0], data.size()));
                    }
                    program_change = midi_track();
                }
                output_track.add_event( event );
            }
        }

        if (output_track.get_count())
            m_tracks.push_back(output_track);
    }
}

void midi_container::scan_for_loops( bool p_xmi_loops, bool p_marker_loops, bool p_rpgmaker_loops, bool p_touhou_loops )
{
    std::vector<uint8_t> data;

    unsigned long subsong_count = m_form == 2 ? m_tracks.size() : 1;

    m_timestamp_loop_start.resize( subsong_count );
    m_timestamp_loop_end.resize( subsong_count );

    for ( unsigned long i = 0; i < subsong_count; ++i )
    {
        m_timestamp_loop_start[ i ] = ~0UL;
        m_timestamp_loop_end[ i ] = ~0UL;
    }

    if ( p_touhou_loops && m_form == 0 )
    {
        bool loop_start_found = false;
        bool loop_end_found = false;
        bool errored = false;

        for ( unsigned long i = 0; !errored && i < m_tracks.size(); ++i )
        {
            const midi_track & track = m_tracks[ i ];
            for ( unsigned long j = 0; !errored && j < track.get_count(); ++j )
            {
                const midi_event & event = track[ j ];
                if ( event.m_type == midi_event::control_change )
                {
                    if ( event.m_data[ 0 ] == 2 )
                    {
                        if ( event.m_data[ 1 ] != 0 )
                        {
                            errored = true;
                            break;
                        }
                        m_timestamp_loop_start[ 0 ] = event.m_timestamp;
                        loop_start_found = true;
                    }
                    if ( event.m_data[ 0 ] == 4 )
                    {
                        if ( event.m_data[ 1 ] != 0 )
                        {
                            errored = true;
                            break;
                        }
                        m_timestamp_loop_end[ 0 ] = event.m_timestamp;
                        loop_end_found = true;
                    }
                }
            }
        }

        if ( errored )
        {
            m_timestamp_loop_start[ 0 ] = ~0UL;
            m_timestamp_loop_end[ 0 ] = ~0UL;
        }
    }

    if ( p_rpgmaker_loops )
    {
        bool emidi_commands_found = false;

        for ( unsigned long i = 0; i < m_tracks.size(); ++i )
        {
            unsigned long subsong = 0;
            if ( m_form == 2 ) subsong = i;

            const midi_track & track = m_tracks[ i ];
            for ( unsigned long j = 0; j < track.get_count(); ++j )
            {
                const midi_event & event = track[ j ];
                if ( event.m_type == midi_event::control_change &&
                    ( event.m_data[ 0 ] == 110 || event.m_data[ 0 ] == 111 ) )
                {
                    if ( event.m_data[ 0 ] == 110 )
                    {
                        emidi_commands_found = true;
                        break;
                    }
                    {
                        if ( m_timestamp_loop_start[ subsong ] == ~0UL || m_timestamp_loop_start[ subsong ] > event.m_timestamp )
                        {
                            m_timestamp_loop_start[ subsong ] = event.m_timestamp;
                        }
                    }
                }
            }

            if ( emidi_commands_found )
            {
                m_timestamp_loop_start[ subsong ] = ~0UL;
                m_timestamp_loop_end[ subsong ] = ~0UL;
                break;
            }
        }
    }

    if ( p_xmi_loops )
    {
        for ( unsigned long i = 0; i < m_tracks.size(); ++i )
        {
            unsigned long subsong = 0;
            if ( m_form == 2 ) subsong = i;

            const midi_track & track = m_tracks[ i ];
            for ( unsigned long j = 0; j < track.get_count(); ++j )
            {
                const midi_event & event = track[ j ];
                if ( event.m_type == midi_event::control_change &&
                    ( event.m_data[ 0 ] >= 0x74 && event.m_data[ 0 ] <= 0x77 ) )
                {
                    if ( event.m_data[ 0 ] == 0x74 || event.m_data[ 0 ] == 0x76 )
                    {
                        if ( m_timestamp_loop_start[ subsong ] == ~0UL || m_timestamp_loop_start[ subsong ] > event.m_timestamp )
                        {
                            m_timestamp_loop_start[ subsong ] = event.m_timestamp;
                        }
                    }
                    else
                    {
                        if ( m_timestamp_loop_end[ subsong ] == ~0UL || m_timestamp_loop_end[ subsong ] < event.m_timestamp )
                        {
                            m_timestamp_loop_end[ subsong ] = event.m_timestamp;
                        }
                    }
                }
            }
        }
    }

    if ( p_marker_loops )
    {
        for ( unsigned long i = 0; i < m_tracks.size(); ++i )
        {
            unsigned long subsong = 0;
            if ( m_form == 2 ) subsong = i;

            const midi_track & track = m_tracks[ i ];
            for ( unsigned long j = 0; j < track.get_count(); ++j )
            {
                const midi_event & event = track[ j ];
                if ( event.m_type == midi_event::extended &&
                    event.get_data_count() >= 9 &&
                    event.m_data[ 0 ] == 0xFF && event.m_data[ 1 ] == 0x06 )
                {
                    unsigned long data_count = event.get_data_count() - 2;
                    data.resize( data_count );
                    event.copy_data( &data[0], 2, data_count );

                    if ( data_count == 9 && !strncasecmp( (const char *) &data[0], "loopStart", 9 ) )
                    {
                        if ( m_timestamp_loop_start[ subsong ] == ~0UL || m_timestamp_loop_start[ subsong ] > event.m_timestamp )
                        {
                            m_timestamp_loop_start[ subsong ] = event.m_timestamp;
                        }
                    }
                    else if ( data_count == 7 && !strncasecmp( (const char *) &data[0], "loopEnd", 7 ) )
                    {
                        if ( m_timestamp_loop_end[ subsong ] == ~0UL || m_timestamp_loop_end[ subsong ] < event.m_timestamp )
                        {
                            m_timestamp_loop_end[ subsong ] = event.m_timestamp;
                        }
                    }
                }
            }
        }
    }

    // Sanity

    for ( unsigned long i = 0; i < subsong_count; ++i )
    {
        unsigned long timestamp_song_end;
        if ( m_form == 2 )
            timestamp_song_end = m_tracks[i][m_tracks[i].get_count()-1].m_timestamp;
        else
        {
            timestamp_song_end = 0;
            for (unsigned long j = 0; j < m_tracks.size(); ++j)
            {
                const midi_track & track = m_tracks[j];
                unsigned long timestamp = track[track.get_count()-1].m_timestamp;
                if (timestamp > timestamp_song_end)
                    timestamp_song_end = timestamp;
            }
        }
        if ( m_timestamp_loop_start[ i ] != ~0UL && ( ( m_timestamp_loop_start[ i ] == m_timestamp_loop_end[ i ] ) || ( m_timestamp_loop_start[ i ] == timestamp_song_end ) ) )
        {
            m_timestamp_loop_start[ i ] = ~0UL;
            m_timestamp_loop_end[ i ] = ~0UL;
        }
    }
}

unsigned midi_container::get_port_mask(unsigned long subsong) const
{
    std::vector<midi_stream_event> stream;
    system_exclusive_table system_exclusive;
    unsigned long loop_start, loop_end;
    unsigned clean_flags = clean_flag_emidi;

    serialize_as_stream(subsong, stream, system_exclusive, loop_start, loop_end, clean_flags);

    return get_port_mask(stream, system_exclusive);
}

unsigned midi_container::get_port_mask(const std::vector<midi_stream_event> & stream, const system_exclusive_table & system_exclusive)
{
    unsigned mask = 0;

    for(auto it = stream.begin(); it != stream.end(); ++it)
    {
        uint32_t event = it->m_event;
        size_t port;
        if(!(event & 0x80000000)) {
            port = (event & 0x7f000000) >> 24;
        } else {
            const uint8_t *data;
            size_t size;
            system_exclusive.get_entry(event & 0x7fffffff, data, size, port);
        }
        if(port > 2)
            port = 0;
        mask |= 1 << port;
    }

    return mask;
}
