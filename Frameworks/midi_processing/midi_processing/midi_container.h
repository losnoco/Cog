#ifndef _MIDI_CONTAINER_H_
#define _MIDI_CONTAINER_H_

#include <stdint.h>
#include <string>
#include <vector>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define snprintf sprintf_s
#endif

struct midi_event
{
    enum
    {
        max_static_data_count = 16
    };

    enum event_type
    {
        note_off = 0,
        note_on,
        polyphonic_aftertouch,
        control_change,
        program_change,
        channel_aftertouch,
        pitch_wheel,
        extended
    };

    unsigned long m_timestamp;

    event_type m_type;
    unsigned m_channel;
    unsigned long m_data_count;
    uint8_t m_data[max_static_data_count];
    std::vector<uint8_t> m_ext_data;

    midi_event() : m_timestamp(0), m_type(note_off), m_channel(0), m_data_count(0) { }
    midi_event( const midi_event & p_in );
    midi_event( unsigned long p_timestamp, event_type p_type, unsigned p_channel, const uint8_t * p_data, std::size_t p_data_count );

    unsigned long get_data_count() const;
    void copy_data( uint8_t * p_out, unsigned long p_offset, unsigned long p_count ) const;
};

class midi_track
{
    std::vector<midi_event> m_events;

public:
    midi_track() { }
    midi_track(const midi_track & p_in);

    void add_event( const midi_event & p_event );
    std::size_t get_count() const;
    const midi_event & operator [] ( std::size_t p_index ) const;
    midi_event & operator [] ( std::size_t p_index );

    void remove_event( unsigned long index );
};

struct tempo_entry
{
    unsigned long m_timestamp;
    unsigned m_tempo;

    tempo_entry() : m_timestamp(0), m_tempo(0) { }
    tempo_entry(unsigned long p_timestamp, unsigned p_tempo);
};

class tempo_map
{
    std::vector<tempo_entry> m_entries;

public:
    void add_tempo( unsigned p_tempo, unsigned long p_timestamp );
    unsigned long timestamp_to_ms( unsigned long p_timestamp, unsigned p_dtx ) const;

    std::size_t get_count() const;
    const tempo_entry & operator [] ( std::size_t p_index ) const;
    tempo_entry & operator [] ( std::size_t p_index );
};

struct system_exclusive_entry
{
    std::size_t m_port;
    std::size_t m_offset;
    std::size_t m_length;
    system_exclusive_entry() : m_port(0), m_offset(0), m_length(0) { }
    system_exclusive_entry(const system_exclusive_entry & p_in);
    system_exclusive_entry(std::size_t p_port, std::size_t p_offset, std::size_t p_length);
};

class system_exclusive_table
{
    std::vector<uint8_t> m_data;
    std::vector<system_exclusive_entry> m_entries;

public:
    unsigned add_entry( const uint8_t * p_data, std::size_t p_size, std::size_t p_port );
    void get_entry( unsigned p_index, const uint8_t * & p_data, std::size_t & p_size, std::size_t & p_port ) const;
};

struct midi_stream_event
{
    unsigned long m_timestamp;
    uint32_t m_event;

    midi_stream_event() : m_timestamp(0), m_event(0) { }
    midi_stream_event(unsigned long p_timestamp, uint32_t p_event);
};

struct midi_meta_data_item
{
    unsigned long m_timestamp;
    std::string m_name;
    std::string m_value;

    midi_meta_data_item() : m_timestamp(0) { }
    midi_meta_data_item(const midi_meta_data_item & p_in);
    midi_meta_data_item(unsigned long p_timestamp, const char * p_name, const char * p_value);
};

class midi_meta_data
{
    std::vector<midi_meta_data_item> m_data;
    std::vector<uint8_t> m_bitmap;

public:
    midi_meta_data() { }

    void add_item( const midi_meta_data_item & p_item );

    void append( const midi_meta_data & p_data );

    bool get_item( const char * p_name, midi_meta_data_item & p_out ) const;

    bool get_bitmap( std::vector<uint8_t> & p_out );

    void assign_bitmap( std::vector<uint8_t>::const_iterator const& begin, std::vector<uint8_t>::const_iterator const& end );

    std::size_t get_count() const;

    const midi_meta_data_item & operator [] ( std::size_t p_index ) const;
};

class midi_container
{
public:
    enum
    {
        clean_flag_emidi       = 1 << 0,
        clean_flag_instruments = 1 << 1,
        clean_flag_banks       = 1 << 2,
    };

private:
    unsigned m_form;
    unsigned m_dtx;
    std::vector<uint64_t> m_channel_mask;
    std::vector<tempo_map> m_tempo_map;
    std::vector<midi_track> m_tracks;

    std::vector<uint8_t> m_port_numbers;

    std::vector< std::vector< std::string > > m_device_names;

    midi_meta_data m_extra_meta_data;

    std::vector<unsigned long> m_timestamp_end;

    std::vector<unsigned long> m_timestamp_loop_start;
    std::vector<unsigned long> m_timestamp_loop_end;

    unsigned long timestamp_to_ms( unsigned long p_timestamp, unsigned long p_subsong ) const;

    /*
     * Normalize port numbers properly
     */
    template <typename T> void limit_port_number(T & number)
    {
        for ( unsigned i = 0; i < m_port_numbers.size(); i++ )
        {
            if ( m_port_numbers[ i ] == number )
            {
                number = i;
                return;
            }
        }
        m_port_numbers.push_back( (const uint8_t) number );
        number = m_port_numbers.size() - 1;
    }

    template <typename T> void limit_port_number(T & number) const
    {
        for ( unsigned i = 0; i < m_port_numbers.size(); i++ )
        {
            if ( m_port_numbers[ i ] == number )
            {
                number = i;
                return;
            }
        }
    }

public:
    midi_container() { m_device_names.resize( 16 ); }

    void initialize( unsigned p_form, unsigned p_dtx );

    void add_track( const midi_track & p_track );

    void add_track_event( std::size_t p_track_index, const midi_event & p_event );

    /*
     * These functions are really only designed to merge and later remove System Exclusive message dumps
     */
    void merge_tracks( const midi_container & p_source );
    void set_track_count( unsigned count );
    void set_extra_meta_data( const midi_meta_data & p_data );

    /*
     * Blah.
     * Hack 0: Remove channel 16
     * Hack 1: Remove channels 11-16
     */
    void apply_hackfix( unsigned hack );

    void serialize_as_stream( unsigned long subsong, std::vector<midi_stream_event> & p_stream, system_exclusive_table & p_system_exclusive, unsigned long & loop_start, unsigned long & loop_end, unsigned clean_flags ) const;

    void serialize_as_standard_midi_file( std::vector<uint8_t> & p_midi_file ) const;

    void promote_to_type1();

    void trim_start();

private:
    void trim_range_of_tracks(unsigned long start, unsigned long end);
    void trim_tempo_map(unsigned long p_index, unsigned long base_timestamp);

public:
    typedef std::string(*split_callback)(uint8_t bank_msb, uint8_t bank_lsb, uint8_t instrument);

    void split_by_instrument_changes(split_callback cb = NULL);

    unsigned long get_subsong_count() const;
    unsigned long get_subsong( unsigned long p_index ) const;

    unsigned long get_timestamp_end(unsigned long subsong, bool ms = false) const;

    unsigned get_format() const;
    unsigned get_track_count() const;
    unsigned get_channel_count(unsigned long subsong) const;

    unsigned get_port_mask(unsigned long subsong) const;
    static unsigned get_port_mask(const std::vector<midi_stream_event> & p_stream, const system_exclusive_table & p_sysex);

    unsigned long get_timestamp_loop_start(unsigned long subsong, bool ms = false) const;
    unsigned long get_timestamp_loop_end(unsigned long subsong, bool ms = false) const;

    void get_meta_data( unsigned long subsong, midi_meta_data & p_out );

    void scan_for_loops( bool p_xmi_loops, bool p_marker_loops, bool p_rpgmaker_loops, bool p_touhou_loops );

    static void encode_delta( std::vector<uint8_t> & p_out, unsigned long delta );
};

#endif
