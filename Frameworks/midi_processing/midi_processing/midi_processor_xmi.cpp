#include "midi_processor.h"

#include <string.h>

bool midi_processor::is_xmi( std::vector<uint8_t> const& p_file )
{
    if ( p_file.size() < 0x22 ) return false;
    if ( p_file[ 0 ] != 'F' || p_file[ 1 ] != 'O' || p_file[ 2 ] != 'R' || p_file[ 3 ] != 'M' ||
         p_file[ 8 ] != 'X' || p_file[ 9 ] != 'D' || p_file[ 10 ] != 'I' || p_file[ 11 ] != 'R' ||
         p_file[ 0x1E ] != 'X' || p_file[ 0x1F ] != 'M' || p_file[ 0x20 ] != 'I' || p_file[ 0x21 ] != 'D' ) return false;
    return true;
}

const uint8_t midi_processor::xmi_default_tempo[5] = {0xFF, 0x51, 0x07, 0xA1, 0x20};

unsigned midi_processor::decode_xmi_delta( std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end )
{
	unsigned delta = 0;
	if ( it == end ) return 0;
    uint8_t byte = *it++;
	if ( !( byte & 0x80 ) )
	{
		do
		{
			delta += byte;
			if ( it == end ) break;
            byte = *it++;
		}
        while ( !( byte & 0x80 ) && it != end );
	}
    --it;
	return delta;
}

struct iff_chunk
{
    uint8_t m_id[4];
    uint8_t m_type[4];
    std::vector<uint8_t> m_data;
    std::vector<iff_chunk> m_sub_chunks;

	iff_chunk()
	{
		memset( m_id, 0, sizeof( m_id ) );
		memset( m_type, 0, sizeof( m_type ) );
	}

	iff_chunk( const iff_chunk & p_in )
	{
		memcpy( m_id, p_in.m_id, sizeof( m_id ) );
		memcpy( m_type, p_in.m_type, sizeof( m_type ) );
		m_data = p_in.m_data;
		m_sub_chunks = p_in.m_sub_chunks;
	}

	const iff_chunk & find_sub_chunk( const char * p_id, unsigned index = 0 ) const
	{
        for ( std::size_t i = 0; i < m_sub_chunks.size(); ++i )
		{
			if ( !memcmp( p_id, m_sub_chunks[ i ].m_id, 4 ) )
			{
				if ( index ) --index;
				if ( !index ) return m_sub_chunks[ i ];
			}
		}
        /*throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );*/
        return *this;
	}

	unsigned get_chunk_count( const char * p_id ) const
	{
		unsigned chunk_count = 0;
        for ( std::size_t i = 0; i < m_sub_chunks.size(); ++i )
        {
			if ( !memcmp( p_id, m_sub_chunks[ i ].m_id, 4 ) )
			{
				++chunk_count;
			}
		}
		return chunk_count;
	}
};

struct iff_stream
{
    std::vector<iff_chunk> m_chunks;

    iff_chunk fail;

	const iff_chunk & find_chunk( const char * p_id ) const
	{
        for ( std::size_t i = 0; i < m_chunks.size(); ++i )
		{
			if ( !memcmp( p_id, m_chunks[ i ].m_id, 4 ) )
			{
				return m_chunks[ i ];
			}
		}
        /*throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );*/
        return fail;
	}
};

static bool read_iff_chunk( std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, iff_chunk & p_out, bool first_chunk )
{
	if ( end - it < 8 ) return false;
    std::copy( it, it + 4, p_out.m_id );
    it += 4;
    uint32_t chunk_size = ( it[ 0 ] << 24 ) | ( it[ 1 ] << 16 ) | ( it[ 2 ] << 8 ) | it[ 3 ];
	if ( (unsigned long)(end - it) < chunk_size ) return false;
    it += 4;
	bool is_cat_chunk = !memcmp( p_out.m_id, "CAT ", 4 );
	bool is_form_chunk = !memcmp( p_out.m_id, "FORM", 4 );
    std::size_t chunk_size_limit = end - it;
	if ( chunk_size > chunk_size_limit ) chunk_size = (uint32_t) chunk_size_limit;
	if ( ( first_chunk && is_form_chunk ) || ( !first_chunk && is_cat_chunk ) )
	{
		if ( end - it < 4 ) return false;
        std::vector<uint8_t>::const_iterator chunk_end = it + chunk_size;
        std::copy( it, it + 4, p_out.m_type );
        it += 4;
        while ( it < chunk_end )
		{
			iff_chunk chunk;
            if ( !read_iff_chunk( it, chunk_end, chunk, is_cat_chunk ) ) return false;
            p_out.m_sub_chunks.push_back( chunk );
		}
        it = chunk_end;
        if ( chunk_size & 1 && it != end ) ++it;
	}
	else if ( !is_form_chunk && !is_cat_chunk )
	{
        p_out.m_data.assign( it, it + chunk_size );
        it += chunk_size;
        if ( chunk_size & 1 && it != end ) ++it;
	}
	else
	{
        /*if ( first_chunk ) throw exception_io_data( pfc::string_formatter() << "Found " << pfc::string8( (const char *)p_out.m_id, 4 ) << " chunk instead of FORM" );
        else throw exception_io_data( "Found multiple FORM chunks" );*/
        return false;
	}
    return true;
}

static bool read_iff_stream( std::vector<uint8_t> const& p_file, iff_stream & p_out )
{
    std::vector<uint8_t>::const_iterator it = p_file.begin(), end = p_file.end();
	bool first_chunk = true;
    while ( it != end )
	{
		iff_chunk chunk;
        if ( !read_iff_chunk( it, end, chunk, first_chunk ) ) return false;
        p_out.m_chunks.push_back( chunk );
		first_chunk = false;
	}
    return true;
}

bool midi_processor::process_xmi( std::vector<uint8_t> const& p_file, midi_container & p_out )
{
	iff_stream xmi_file;
    if ( !read_iff_stream( p_file, xmi_file ) ) return false;

	const iff_chunk & form_chunk = xmi_file.find_chunk( "FORM" );
    if ( memcmp( form_chunk.m_type, "XDIR", 4 ) ) return false; /*throw exception_io_data( "XMI IFF not XDIR type" );*/

	const iff_chunk & cat_chunk = xmi_file.find_chunk( "CAT " );
    if ( memcmp( cat_chunk.m_type, "XMID", 4 ) ) return false; /*throw exception_io_data( "XMI CAT chunk not XMID type" );*/

	unsigned track_count = cat_chunk.get_chunk_count( "FORM" );

	p_out.initialize( track_count > 1 ? 2 : 0, 60 );

	for ( unsigned i = 0; i < track_count; ++i )
	{
		const iff_chunk & xmid_form_chunk = cat_chunk.find_sub_chunk( "FORM", i );
        if ( memcmp( xmid_form_chunk.m_type, "XMID", 4 ) ) return false; /*throw exception_io_data( "XMI nested FORM chunk not XMID type" );*/

		const iff_chunk & event_chunk = xmid_form_chunk.find_sub_chunk( "EVNT" );
        if ( memcmp( event_chunk.m_id, "EVNT", 4 ) ) return false; /* EVNT chunk not found */
        std::vector<uint8_t> const& event_body = event_chunk.m_data;

		midi_track track;

		bool initial_tempo = false;

		unsigned current_timestamp = 0;

		unsigned last_event_timestamp = 0;

        std::vector<uint8_t> buffer;
        buffer.resize( 3 );

        std::vector<uint8_t>::const_iterator it = event_body.begin(), end = event_body.end();

        while ( it != end )
		{
            unsigned delta = decode_xmi_delta( it, end );
			current_timestamp += delta;

			if ( current_timestamp > last_event_timestamp )
			{
				last_event_timestamp = current_timestamp;
			}

			if ( it == end ) return false;
            buffer[ 0 ] = *it++;
			if ( buffer[ 0 ] == 0xFF )
			{
				if ( it == end ) return false;
                buffer[ 1 ] = *it++;
				long meta_count;
				if ( buffer[ 1 ] == 0x2F )
				{
					meta_count = 0;
				}
				else
				{
                    meta_count = decode_delta( it, end );
                    if ( meta_count < 0 ) return false; /*throw exception_io_data( "Invalid XMI meta message" );*/
					if ( end - it < meta_count ) return false;
                    buffer.resize( meta_count + 2 );
                    std::copy( it, it + meta_count, buffer.begin() + 2 );
                    it += meta_count;
				}
				if ( buffer[ 1 ] == 0x2F && current_timestamp < last_event_timestamp )
				{
					current_timestamp = last_event_timestamp;
				}
				if ( buffer[ 1 ] == 0x51 && meta_count == 3 )
				{
					unsigned tempo = buffer[ 2 ] * 0x10000 + buffer[ 3 ] * 0x100 + buffer[ 4 ];
					unsigned ppqn = ( tempo * 3 ) / 25000;
					tempo = tempo * 60 / ppqn;
					buffer[ 2 ] = tempo / 0x10000;
					buffer[ 3 ] = tempo / 0x100;
					buffer[ 4 ] = tempo;
					if ( current_timestamp == 0 ) initial_tempo = true;
				}
                track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], meta_count + 2 ) );
				if ( buffer[ 1 ] == 0x2F ) break;
			}
			else if ( buffer[ 0 ] == 0xF0 )
			{
                long system_exclusive_count = decode_delta( it, end );
                if ( system_exclusive_count < 0 ) return false; /*throw exception_io_data( "Invalid XMI System Exclusive message" );*/
				if ( end - it < system_exclusive_count ) return false;
                buffer.resize( system_exclusive_count + 1 );
                std::copy( it, it + system_exclusive_count, buffer.begin() + 1 );
                it += system_exclusive_count;
                track.add_event( midi_event( current_timestamp, midi_event::extended, 0, &buffer[0], system_exclusive_count + 1 ) );
			}
			else if ( buffer[ 0 ] >= 0x80 && buffer[ 0 ] <= 0xEF )
			{
				unsigned bytes_read = 1;
				if ( it == end ) return false;
                buffer[ 1 ] = *it++;
				midi_event::event_type type = (midi_event::event_type)( ( buffer[ 0 ] >> 4 ) - 8 );
				unsigned channel = buffer[ 0 ] & 0x0F;
				if ( type != midi_event::program_change && type != midi_event::channel_aftertouch )
				{
					if ( it == end ) return false;
                    buffer[ 2 ] = *it++;
					bytes_read = 2;
				}
                track.add_event( midi_event( current_timestamp, type, channel, &buffer[1], bytes_read ) );
				if ( type == midi_event::note_on )
				{
					buffer[ 2 ] = 0x00;
                    int note_length = decode_delta( it, end );
                    if ( note_length < 0 ) return false; /*throw exception_io_data( "Invalid XMI note message" );*/
					unsigned note_end_timestamp = current_timestamp + note_length;
					if ( note_end_timestamp > last_event_timestamp ) last_event_timestamp = note_end_timestamp;
                    track.add_event( midi_event( note_end_timestamp, type, channel, &buffer[1], bytes_read ) );
				}
			}
            else return false; /*throw exception_io_data( "Unexpected XMI status code" );*/
		}

		if ( !initial_tempo )
			track.add_event( midi_event( 0, midi_event::extended, 0, xmi_default_tempo, _countof( xmi_default_tempo ) ) );

		p_out.add_track( track );
	}

    return true;
}
