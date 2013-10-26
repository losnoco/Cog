// Game_Music_Emu $vers. http://www.slack.net/~ant/

#include "Spc_Sfm.h"

#include "blargg_endian.h"

#include <stdio.h>

/* Copyright (C) 2004-2013 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

// TODO: support Spc_Filter's bass

Sfm_Emu::Sfm_Emu()
{
    set_type( gme_sfm_type );
    set_gain( 1.4 );
    set_max_initial_silence( 30 );
	set_silence_lookahead( 30 ); // Some SFMs may have a lot of initialization code
}

Sfm_Emu::~Sfm_Emu() { }

// Track info

static void hash_sfm_file( byte const* data, int data_size, Music_Emu::Hash_Function& out )
{
    out.hash_( data, data_size );
}

blargg_err_t Sfm_Emu::track_info_( track_info_t* out, int ) const
{
    const char * title = metadata.enumValue("information:title");
    if (title) strncpy( out->song, title, 255 );
    else out->song[0] = 0;
    out->song[255] = '\0';
    return blargg_ok;
}

static blargg_err_t check_sfm_header( void const* header )
{
    if ( memcmp( header, "SFM1", 4 ) )
        return blargg_err_file_type;
    return blargg_ok;
}

struct Sfm_File : Gme_Info_
{
    blargg_vector<byte> data;
    Bml_Parser metadata;

    Sfm_File() { set_type( gme_sfm_type ); }

    blargg_err_t load_( Data_Reader& in )
    {
        int file_size = in.remain();
        if ( file_size < Sfm_Emu::sfm_min_file_size )
            return blargg_err_file_type;
        RETURN_ERR( data.resize( file_size ) );
        RETURN_ERR( in.read( data.begin(), data.end() - data.begin() ) );
        RETURN_ERR( check_sfm_header( data.begin() ) );
        if ( file_size < 8 )
            return "SFM file too small";
        int metadata_size = get_le32( data.begin() + 4 );
        byte temp = data[ 8 + metadata_size ];
        data[ 8 + metadata_size ] = '\0';
        metadata.parseDocument( (const char *)data.begin() + 8 );
        data[ 8 + metadata_size ] = temp;
        return blargg_ok;
    }

    blargg_err_t track_info_( track_info_t* out, int ) const
    {
        const char * title = metadata.enumValue("information:title");
        if (title) strncpy( out->song, title, 255 );
        else out->song[0] = 0;
        out->song[255] = '\0';
        return blargg_ok;
    }

    blargg_err_t hash_( Hash_Function& out ) const
    {
        hash_sfm_file( data.begin(), data.end() - data.begin(), out );
        return blargg_ok;
    }
};

static Music_Emu* new_sfm_emu () { return BLARGG_NEW Sfm_Emu ; }
static Music_Emu* new_sfm_file() { return BLARGG_NEW Sfm_File; }

gme_type_t_ const gme_sfm_type [1] = {{ "Super Nintendo with log", 1, &new_sfm_emu, &new_sfm_file, "SFM", 0 }};

// Setup

blargg_err_t Sfm_Emu::set_sample_rate_( int sample_rate )
{
    smp.power();
    if ( sample_rate != native_sample_rate )
    {
        RETURN_ERR( resampler.resize_buffer( native_sample_rate / 20 * 2 ) );
        RETURN_ERR( resampler.set_rate( (double) native_sample_rate / sample_rate ) ); // 0.9965 rolloff
    }
    return blargg_ok;
}

void Sfm_Emu::mute_voices_( int m )
{
    Music_Emu::mute_voices_( m );
    for ( int i = 0, j = 1; i < 8; ++i, j <<= 1 )
        smp.dsp.channel_enable( i, !( m & j ) );
}

blargg_err_t Sfm_Emu::load_mem_( byte const in [], int size )
{
    set_voice_count( 8 );
    if ( size < Sfm_Emu::sfm_min_file_size )
        return blargg_err_file_type;

    static const char* const names [ 8 ] = {
        "DSP 1", "DSP 2", "DSP 3", "DSP 4", "DSP 5", "DSP 6", "DSP 7", "DSP 8"
    };
    set_voice_names( names );

    return check_sfm_header( in );
}

// Emulation

void Sfm_Emu::set_tempo_( double t )
{
    smp.set_tempo( t );
}

// (n ? n : 256)
#define IF_0_THEN_256( n ) ((uint8_t) ((n) - 1) + 1)

#define META_ENUM_INT(n,d) (value = metadata.enumValue(n), value ? strtol(value, &end, 10) : (d))

static const byte ipl_rom[0x40] =
{
    0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0,
    0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
    0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4,
    0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
    0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB,
    0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
    0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD,
    0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF
};

blargg_err_t Sfm_Emu::start_track_( int track )
{
    RETURN_ERR( Music_Emu::start_track_( track ) );
    resampler.clear();
    filter.clear();
    const byte * ptr = file_begin();
    if ( file_size() < Sfm_Emu::sfm_min_file_size )
        return "SFM file too small";
    int metadata_size = get_le32(ptr + 4);
    if ( file_size() < metadata_size + Sfm_Emu::sfm_min_file_size )
        return "SFM file too small";
    metadata.parseDocument((const char *) ptr + 8, metadata_size);

    memcpy( smp.iplrom, ipl_rom, 64 );

    smp.reset();

    memcpy( smp.apuram, ptr + 8 + metadata_size, 65536 );

    memcpy( smp.dsp.spc_dsp.m.regs, ptr + 8 + metadata_size + 65536, 128 );

    smp.set_sfm_queue( ptr + 8 + metadata_size + 65536 + 128, ptr + file_size() );

    char * end;
    const char * value;

    uint32_t test = META_ENUM_INT("smp:test", 0);
    smp.status.clock_speed = (test >> 6) & 3;
    smp.status.timer_speed = (test >> 4) & 3;
    smp.status.timers_enable = test & 0x08;
    smp.status.ram_disable = test & 0x04;
    smp.status.ram_writable = test & 0x02;
    smp.status.timers_disable = test & 0x01;

    smp.status.iplrom_enable = META_ENUM_INT("smp:iplrom",1);
    smp.status.dsp_addr = META_ENUM_INT("smp:dspaddr",0);

    value = metadata.enumValue("smp:ram");
    if (value)
    {
        smp.status.ram00f8 = strtol(value, &end, 10);
        if (*end)
        {
            value = end + 1;
            smp.status.ram00f9 = strtol(value, &end, 10);
        }
    }

    std::string name;
    std::ostringstream oss;
    
    name = "smp:regs:";
    smp.regs.pc = META_ENUM_INT(name + "pc", 0xffc0);
    smp.regs.a = META_ENUM_INT(name + "a", 0x00);
    smp.regs.x = META_ENUM_INT(name + "x", 0x00);
    smp.regs.y = META_ENUM_INT(name + "y", 0x00);
    smp.regs.s = META_ENUM_INT(name + "s", 0xef);
    smp.regs.p = META_ENUM_INT(name + "psw", 0x02);

    value = metadata.enumValue("smp:ports");
    if (value)
    {
        for (auto &n : smp.sfm_last)
        {
            n = strtol(value, &end, 10);
            if (*end == ',')
                value = end + 1;
            else
                break;
        }
    }
    
    for (int i = 0; i < 3; ++i)
    {
        SuperFamicom::SMP::Timer<192> &t = (i == 0 ? smp.timer0 : (i == 1 ? smp.timer1 : *(SuperFamicom::SMP::Timer<192>*)&smp.timer2));
        oss.str("");
        oss.clear();
        oss << "smp:timer[" << i << "]:";
        name = oss.str();
        value = metadata.enumValue(name + "enable");
        if (value)
        {
            t.enable = !!strtol(value, &end, 10);
        }
        value = metadata.enumValue(name + "target");
        if (value)
        {
            t.target = strtol(value, &end, 10);
        }
        value = metadata.enumValue(name + "stage");
        if (value)
        {
            t.stage0_ticks = strtol(value, &end, 10);
            if (*end != ',') break;
            value = end + 1;
            t.stage1_ticks = strtol(value, &end, 10);
            if (*end != ',') break;
            value = end + 1;
            t.stage2_ticks = strtol(value, &end, 10);
            if (*end != ',') break;
            value = end + 1;
            t.stage3_ticks = strtol(value, &end, 10);
        }
        value = metadata.enumValue(name + "line");
        if (value)
        {
            t.current_line = !!strtol(value, &end, 10);
        }
    }

    smp.dsp.clock = META_ENUM_INT("dsp:clock", 0) * 4096;
    
    smp.dsp.spc_dsp.m.echo_hist_pos = &smp.dsp.spc_dsp.m.echo_hist[META_ENUM_INT("dsp:echohistaddr", 0)];

    value = metadata.enumValue("dsp:echohistdata");
    if (value)
    {
        for (int i = 0; i < 8; ++i)
        {
            smp.dsp.spc_dsp.m.echo_hist[i][0] = strtol(value, &end, 10);
            value = strchr(value, ',');
            if (!value) break;
            ++value;
            smp.dsp.spc_dsp.m.echo_hist[i][1] = strtol(value, &end, 10);
            value = strchr(value, ',');
            if (!value) break;
            ++value;
        }
    }

    smp.dsp.spc_dsp.m.phase = META_ENUM_INT("dsp:sample", 0);
    smp.dsp.spc_dsp.m.kon = META_ENUM_INT("dsp:kon", 0);
    smp.dsp.spc_dsp.m.noise = META_ENUM_INT("dsp:noise", 0);
    smp.dsp.spc_dsp.m.counter = META_ENUM_INT("dsp:counter", 0);
    smp.dsp.spc_dsp.m.echo_offset = META_ENUM_INT("dsp:echooffset", 0);
    smp.dsp.spc_dsp.m.echo_length = META_ENUM_INT("dsp:echolength", 0);
    smp.dsp.spc_dsp.m.new_kon = META_ENUM_INT("dsp:koncache", 0);
    smp.dsp.spc_dsp.m.endx_buf = META_ENUM_INT("dsp:endx", 0);
    smp.dsp.spc_dsp.m.envx_buf = META_ENUM_INT("dsp:envx", 0);
    smp.dsp.spc_dsp.m.outx_buf = META_ENUM_INT("dsp:outx", 0);
    smp.dsp.spc_dsp.m.t_pmon = META_ENUM_INT("dsp:pmon", 0);
    smp.dsp.spc_dsp.m.t_non = META_ENUM_INT("dsp:non", 0);
    smp.dsp.spc_dsp.m.t_eon = META_ENUM_INT("dsp:eon", 0);
    smp.dsp.spc_dsp.m.t_dir = META_ENUM_INT("dsp:dir", 0);
    smp.dsp.spc_dsp.m.t_koff = META_ENUM_INT("dsp:koff", 0);
    smp.dsp.spc_dsp.m.t_brr_next_addr = META_ENUM_INT("dsp:brrnext", 0);
    smp.dsp.spc_dsp.m.t_adsr0 = META_ENUM_INT("dsp:adsr0", 0);
    smp.dsp.spc_dsp.m.t_brr_header = META_ENUM_INT("dsp:brrheader", 0);
    smp.dsp.spc_dsp.m.t_brr_byte = META_ENUM_INT("dsp:brrdata", 0);
    smp.dsp.spc_dsp.m.t_srcn = META_ENUM_INT("dsp:srcn", 0);
    smp.dsp.spc_dsp.m.t_esa = META_ENUM_INT("dsp:esa", 0);
    smp.dsp.spc_dsp.m.t_echo_enabled = !META_ENUM_INT("dsp:echodisable", 0);
    smp.dsp.spc_dsp.m.t_dir_addr = META_ENUM_INT("dsp:diraddr", 0);
    smp.dsp.spc_dsp.m.t_pitch = META_ENUM_INT("dsp:pitch", 0);
    smp.dsp.spc_dsp.m.t_output = META_ENUM_INT("dsp:output", 0);
    smp.dsp.spc_dsp.m.t_looped = META_ENUM_INT("dsp:looped", 0);
    smp.dsp.spc_dsp.m.t_echo_ptr = META_ENUM_INT("dsp:echoaddr", 0);


#define META_ENUM_LEVELS(n, o) \
    value = metadata.enumValue(n); \
    if (value) \
    { \
        (o)[0] = strtol(value, &end, 10); \
        if (*end) \
        { \
            value = end + 1; \
            (o)[1] = strtol(value, &end, 10); \
        } \
    }

    META_ENUM_LEVELS("dsp:mainout", smp.dsp.spc_dsp.m.t_main_out);
    META_ENUM_LEVELS("dsp:echoout", smp.dsp.spc_dsp.m.t_echo_out);
    META_ENUM_LEVELS("dsp:echoin", smp.dsp.spc_dsp.m.t_echo_in);

#undef META_ENUM_LEVELS

    for (int i = 0; i < 8; ++i)
    {
        oss.str("");
        oss.clear();
        oss << "dsp:voice[" << i << "]:";
        name = oss.str();
        SuperFamicom::SPC_DSP::voice_t & voice = smp.dsp.spc_dsp.m.voices[i];
        value = metadata.enumValue(name + "brrhistaddr");
        if (value)
        {
            voice.buf_pos = strtol(value, &end, 10);
        }
        value = metadata.enumValue(name + "brrhistdata");
        if (value)
        {
            for (int j = 0; j < SuperFamicom::SPC_DSP::brr_buf_size; ++j)
            {
                voice.buf[j] = voice.buf[j + SuperFamicom::SPC_DSP::brr_buf_size] = strtol(value, &end, 10);
                if (!*end) break;
                value = end + 1;
            }
        }
        voice.interp_pos = META_ENUM_INT(name + "interpaddr",0);
        voice.brr_addr = META_ENUM_INT(name + "brraddr",0);
        voice.brr_offset = META_ENUM_INT(name + "brroffset",0);
        voice.vbit = META_ENUM_INT(name + "vbit",0);
        voice.regs = &smp.dsp.spc_dsp.m.regs[META_ENUM_INT(name + "vidx",0)];
        voice.kon_delay = META_ENUM_INT(name + "kondelay", 0);
        voice.env_mode = (SuperFamicom::SPC_DSP::env_mode_t) META_ENUM_INT(name + "envmode", 0);
        voice.env = META_ENUM_INT(name + "env", 0);
        voice.t_envx_out = META_ENUM_INT(name + "envxout", 0);
        voice.hidden_env = META_ENUM_INT(name + "envcache", 0);
    }

    filter.set_gain( (int) (gain() * Spc_Filter::gain_unit) );
    return blargg_ok;
}

#undef META_ENUM_INT

blargg_err_t Sfm_Emu::save( gme_writer_t writer, void* your_data ) const
{
    bool first;
    std::string name;
    std::ostringstream oss;
    Bml_Parser metadata;
    const byte * ptr = file_begin();
    int metadata_size = get_le32(ptr + 4);
    metadata.parseDocument((const char *) ptr + 8, metadata_size);

    metadata.setValue( "smp:test", (smp.status.clock_speed << 6) | (smp.status.timer_speed << 4) | (smp.status.timers_enable << 3) | (smp.status.ram_disable << 2) | (smp.status.ram_writable << 1) | (smp.status.timers_disable << 0) );
    metadata.setValue( "smp:iplrom", smp.status.iplrom_enable );
    metadata.setValue( "smp:dspaddr", smp.status.dsp_addr );
    
    oss.str("");
    oss.clear();
    oss << smp.status.ram00f8 << "," << smp.status.ram00f9;
    metadata.setValue( "smp:ram", oss.str().c_str() );
    
    name = "smp:regs:";
    metadata.setValue( name + "pc", smp.regs.pc );
    metadata.setValue( name + "a", smp.regs.a );
    metadata.setValue( name + "x", smp.regs.x );
    metadata.setValue( name + "y", smp.regs.y );
    metadata.setValue( name + "s", smp.regs.s );
    metadata.setValue( name + "psw", smp.regs.p );

    oss.str("");
    oss.clear();
    first = true;
    for (auto n : smp.sfm_last)
    {
        if (!first) oss << ",";
        oss << (unsigned long)n;
        first = false;
    }
    metadata.setValue("smp:ports", oss.str().c_str());
    
    for (int i = 0; i < 3; ++i)
    {
        SuperFamicom::SMP::Timer<192> const& t = (i == 0 ? smp.timer0 : (i == 1 ? smp.timer1 : *(SuperFamicom::SMP::Timer<192>*)&smp.timer2));
        oss.str("");
        oss.clear();
        oss << "smp:timer[" << i << "]:";
        name = oss.str();
        metadata.setValue( name + "enable", t.enable );
        metadata.setValue( name + "target", t.target );
        oss.str("");
        oss.clear();
        oss << (unsigned long)t.stage0_ticks << "," << (unsigned long)t.stage1_ticks << ","
            << (unsigned long)t.stage2_ticks << "," << (unsigned long)t.stage3_ticks;
        metadata.setValue( name + "stage", oss.str().c_str() );
        metadata.setValue( name + "line", t.current_line );
    }
    
    metadata.setValue( "dsp:clock", smp.dsp.clock / 4096 );

    metadata.setValue( "dsp:echohistaddr", smp.dsp.spc_dsp.m.echo_hist_pos - smp.dsp.spc_dsp.m.echo_hist );
    
    oss.str("");
    oss.clear();
    for (int i = 0; i < 8; ++i)
    {
        oss << smp.dsp.spc_dsp.m.echo_hist[i][0] << ","
            << smp.dsp.spc_dsp.m.echo_hist[i][1];
        if ( i != 7 ) oss << ",";
    }
    metadata.setValue( "dsp:echohistdata", oss.str().c_str() );
    
    metadata.setValue( "dsp:sample", smp.dsp.spc_dsp.m.phase );
    metadata.setValue( "dsp:kon", smp.dsp.spc_dsp.m.kon );
    metadata.setValue( "dsp:noise", smp.dsp.spc_dsp.m.noise );
    metadata.setValue( "dsp:counter", smp.dsp.spc_dsp.m.counter );
    metadata.setValue( "dsp:echooffset", smp.dsp.spc_dsp.m.echo_offset );
    metadata.setValue( "dsp:echolength", smp.dsp.spc_dsp.m.echo_length );
    metadata.setValue( "dsp:koncache", smp.dsp.spc_dsp.m.new_kon );
    metadata.setValue( "dsp:endx", smp.dsp.spc_dsp.m.endx_buf );
    metadata.setValue( "dsp:envx", smp.dsp.spc_dsp.m.envx_buf );
    metadata.setValue( "dsp:outx", smp.dsp.spc_dsp.m.outx_buf );
    metadata.setValue( "dsp:pmon", smp.dsp.spc_dsp.m.t_pmon );
    metadata.setValue( "dsp:non", smp.dsp.spc_dsp.m.t_non );
    metadata.setValue( "dsp:eon", smp.dsp.spc_dsp.m.t_eon );
    metadata.setValue( "dsp:dir", smp.dsp.spc_dsp.m.t_dir );
    metadata.setValue( "dsp:koff", smp.dsp.spc_dsp.m.t_koff );
    metadata.setValue( "dsp:brrnext", smp.dsp.spc_dsp.m.t_brr_next_addr );
    metadata.setValue( "dsp:adsr0", smp.dsp.spc_dsp.m.t_adsr0 );
    metadata.setValue( "dsp:brrheader", smp.dsp.spc_dsp.m.t_brr_header );
    metadata.setValue( "dsp:brrdata", smp.dsp.spc_dsp.m.t_brr_byte );
    metadata.setValue( "dsp:srcn", smp.dsp.spc_dsp.m.t_srcn );
    metadata.setValue( "dsp:esa", smp.dsp.spc_dsp.m.t_esa );
    metadata.setValue( "dsp:echodisable", !smp.dsp.spc_dsp.m.t_echo_enabled );
    metadata.setValue( "dsp:diraddr", smp.dsp.spc_dsp.m.t_dir_addr );
    metadata.setValue( "dsp:pitch", smp.dsp.spc_dsp.m.t_pitch );
    metadata.setValue( "dsp:output", smp.dsp.spc_dsp.m.t_output );
    metadata.setValue( "dsp:looped", smp.dsp.spc_dsp.m.t_looped );
    metadata.setValue( "dsp:echoaddr", smp.dsp.spc_dsp.m.t_echo_ptr );

#define META_WRITE_LEVELS(n, o) \
    oss.str(""); \
    oss.clear(); \
    oss << (o)[0] << "," << (o)[1]; \
    metadata.setValue((n), oss.str().c_str());
    
    META_WRITE_LEVELS("dsp:mainout", smp.dsp.spc_dsp.m.t_main_out);
    META_WRITE_LEVELS("dsp:echoout", smp.dsp.spc_dsp.m.t_echo_out);
    META_WRITE_LEVELS("dsp:echoin", smp.dsp.spc_dsp.m.t_echo_in);
    
#undef META_WRITE_LEVELS
    
    for (int i = 0; i < 8; ++i)
    {
        oss.str("");
        oss.clear();
        oss << "dsp:voice[" << i << "]:";
        name = oss.str();
        SuperFamicom::SPC_DSP::voice_t const& voice = smp.dsp.spc_dsp.m.voices[i];
        metadata.setValue( name + "brrhistaddr", voice.buf_pos );
        oss.str("");
        oss.clear();
        for (int j = 0; j < SuperFamicom::SPC_DSP::brr_buf_size; ++j)
        {
            oss << voice.buf[j];
            if ( j != SuperFamicom::SPC_DSP::brr_buf_size - 1 )
                oss << ",";
        }
        metadata.setValue( name + "brrhistdata", oss.str().c_str() );
        metadata.setValue( name + "interpaddr", voice.interp_pos );
        metadata.setValue( name + "brraddr", voice.brr_addr );
        metadata.setValue( name + "brroffset", voice.brr_offset );
        metadata.setValue( name + "vbit", voice.vbit );
        metadata.setValue( name + "vidx", voice.regs - smp.dsp.spc_dsp.m.regs);
        metadata.setValue( name + "kondelay", voice.kon_delay );
        metadata.setValue( name + "envmode", voice.env_mode );
        metadata.setValue( name + "env", voice.env );
        metadata.setValue( name + "envxout", voice.t_envx_out );
        metadata.setValue( name + "envcache", voice.hidden_env );
    }
    
    metadata.serialize( name );

    RETURN_ERR( writer( your_data, "SFM1", 4 ) );

    uint8_t temp[4];
    uint32_t meta_length = (uint32_t) name.length();
    set_le32( temp, meta_length );
    RETURN_ERR( writer( your_data, temp, 4 ) );
    
    RETURN_ERR( writer( your_data, name.c_str(), meta_length ) );

    RETURN_ERR( writer( your_data, smp.apuram, 65536 ) );

    RETURN_ERR( writer( your_data, smp.dsp.spc_dsp.m.regs, 128 ) );
    
    if ( smp.get_sfm_queue_remain() )
        RETURN_ERR( writer( your_data, smp.get_sfm_queue(), smp.get_sfm_queue_remain() ) );
    
    return blargg_ok;
}

blargg_err_t Sfm_Emu::play_and_filter( int count, sample_t out [] )
{
    smp.render( out, count );
    filter.run( out, count );
    return blargg_ok;
}

blargg_err_t Sfm_Emu::skip_( int count )
{
    if ( sample_rate() != native_sample_rate )
    {
        count = (int) (count * resampler.rate()) & ~1;
        count -= resampler.skip_input( count );
    }

    // TODO: shouldn't skip be adjusted for the 64 samples read afterwards?

    if ( count > 0 )
    {
        smp.skip( count );
        filter.clear();
    }

	if ( sample_rate() != native_sample_rate )
	{
		// eliminate pop due to resampler
		const int resampler_latency = 64;
		sample_t buf [resampler_latency];
		return play_( resampler_latency, buf );
	}

	return blargg_ok;
}

blargg_err_t Sfm_Emu::play_( int count, sample_t out [] )
{
    if ( sample_rate() == native_sample_rate )
        return play_and_filter( count, out );

    int remain = count;
    while ( remain > 0 )
    {
        remain -= resampler.read( &out [count - remain], remain );
        if ( remain > 0 )
        {
            int n = resampler.buffer_free();
            RETURN_ERR( play_and_filter( n, resampler.buffer() ) );
            resampler.write( n );
        }
    }
    check( remain == 0 );
    return blargg_ok;
}

blargg_err_t Sfm_Emu::hash_( Hash_Function& out ) const
{
    hash_sfm_file( file_begin(), file_size(), out );
    return blargg_ok;
}

