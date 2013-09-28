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
    RETURN_ERR( apu.init() );
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
    apu.mute_voices( m );
}

blargg_err_t Sfm_Emu::load_mem_( byte const in [], int size )
{
    set_voice_count( Spc_Dsp::voice_count );
    if ( size < Sfm_Emu::sfm_min_file_size )
        return blargg_err_file_type;

    static const char* const names [Spc_Dsp::voice_count] = {
        "DSP 1", "DSP 2", "DSP 3", "DSP 4", "DSP 5", "DSP 6", "DSP 7", "DSP 8"
    };
    set_voice_names( names );

    return check_sfm_header( in );
}

// Emulation

void Sfm_Emu::set_tempo_( double t )
{
    apu.set_tempo( (int) (t * Snes_Spc::tempo_unit) );
}

// (n ? n : 256)
#define IF_0_THEN_256( n ) ((uint8_t) ((n) - 1) + 1)

#define META_ENUM_INT(n) (value = metadata.enumValue(n), value ? strtoul(value, &end, 10) : 0)

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
    int metadata_size = get_le32(ptr + 4);
    if ( file_size() < metadata_size + Sfm_Emu::sfm_min_file_size )
        return "SFM file too small";
    char * temp = new char[metadata_size + 1];
    temp[metadata_size] = '\0';
    memcpy(temp, ptr + 8, metadata_size);
    metadata.parseDocument(temp);
    delete [] temp;

    apu.init_rom( ipl_rom );

    apu.reset();

    memcpy( apu.m.ram.ram, ptr + 8 + metadata_size, 65536 );

    memcpy( apu.dsp.m.regs, ptr + 8 + metadata_size + 65536, 128 );

    apu.set_sfm_queue( ptr + 8 + metadata_size + 65536 + 128, ptr + file_size() );

    byte regs[Snes_Spc::reg_count] = {0};

    char * end;
    const char * value;

    regs[Snes_Spc::r_test] = META_ENUM_INT("smp:test");
    regs[Snes_Spc::r_control] |= META_ENUM_INT("smp:iplrom") ? 0x80 : 0;
    regs[Snes_Spc::r_dspaddr] = META_ENUM_INT("smp:dspaddr");

    value = metadata.enumValue("smp:ram");
    if (value)
    {
        regs[Snes_Spc::r_f8] = strtoul(value, &end, 10);
        if (*end)
        {
            value = end + 1;
            regs[Snes_Spc::r_f9] = strtoul(value, &end, 10);
        }
    }

    char temp_path[256];
    for (int i = 0; i < 3; ++i)
    {
        sprintf(temp_path, "smp:timer[%u]:", i);
        size_t length = strlen(temp_path);
        strcpy(temp_path + length, "enable");
        value = metadata.enumValue(temp_path);
        if (value)
        {
            regs[Snes_Spc::r_control] |= strtoul(value, &end, 10) ? 1 << i : 0;
        }
        strcpy(temp_path + length, "target");
        value = metadata.enumValue(temp_path);
        if (value)
        {
            regs[Snes_Spc::r_t0target + i] = strtoul(value, &end, 10);
        }
        strcpy(temp_path + length, "stage");
        value = metadata.enumValue(temp_path);
        if (value)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (value) value = strchr(value, ',');
                if (value) ++value;
            }
            if (value)
            {
                regs[Snes_Spc::r_t0out + i] = strtoul(value, &end, 10);
            }
        }
    }

    apu.load_regs( regs );
    apu.m.rom_enabled = 0;
    apu.regs_loaded();

    for (int i = 0; i < 3; ++i)
    {
        sprintf(temp_path, "smp:timer[%u]:", i);
        size_t length = strlen(temp_path);
        strcpy(temp_path + length, "stage");
        value = metadata.enumValue(temp_path);
        if (value)
        {
            const char * stage = value;
            apu.m.timers[i].next_time = strtoul(stage, &end, 10) + 1;
            for (int j = 0; j < 2; ++j)
            {
                if (stage) stage = strchr(stage, ',');
                if (stage) ++stage;
            }
            if (stage)
            {
                apu.m.timers[i].divider = strtoul(value, &end, 10);
            }
        }
    }

    apu.dsp.m.echo_hist_pos = &apu.dsp.m.echo_hist[META_ENUM_INT("dsp:echohistaddr")];

    value = metadata.enumValue("dsp:echohistdata");
    if (value)
    {
        for (int i = 0; i < 8; ++i)
        {
            apu.dsp.m.echo_hist[i][0] = strtoul(value, &end, 10);
            value = strchr(value, ',');
            if (!value) break;
            ++value;
            apu.dsp.m.echo_hist[i][1] = strtoul(value, &end, 10);
            value = strchr(value, ',');
            if (!value) break;
            ++value;
        }
    }

    apu.dsp.m.phase = META_ENUM_INT("dsp:sample");
    apu.dsp.m.kon = META_ENUM_INT("dsp:kon");
    apu.dsp.m.noise = META_ENUM_INT("dsp:noise");
    apu.dsp.m.counter = META_ENUM_INT("dsp:counter");
    apu.dsp.m.echo_offset = META_ENUM_INT("dsp:echooffset");
    apu.dsp.m.echo_length = META_ENUM_INT("dsp:echolength");
    apu.dsp.m.new_kon = META_ENUM_INT("dsp:koncache");
    apu.dsp.m.endx_buf = META_ENUM_INT("dsp:endx");
    apu.dsp.m.envx_buf = META_ENUM_INT("dsp:envx");
    apu.dsp.m.outx_buf = META_ENUM_INT("dsp:outx");
    apu.dsp.m.t_pmon = META_ENUM_INT("dsp:pmon");
    apu.dsp.m.t_non = META_ENUM_INT("dsp:non");
    apu.dsp.m.t_eon = META_ENUM_INT("dsp:eon");
    apu.dsp.m.t_dir = META_ENUM_INT("dsp:dir");
    apu.dsp.m.t_koff = META_ENUM_INT("dsp:koff");
    apu.dsp.m.t_brr_next_addr = META_ENUM_INT("dsp:brrnext");
    apu.dsp.m.t_adsr0 = META_ENUM_INT("dsp:adsr0");
    apu.dsp.m.t_brr_header = META_ENUM_INT("dsp:brrheader");
    apu.dsp.m.t_brr_byte = META_ENUM_INT("dsp:brrdata");
    apu.dsp.m.t_srcn = META_ENUM_INT("dsp:srcn");
    apu.dsp.m.t_esa = META_ENUM_INT("dsp:esa");
    apu.dsp.m.t_echo_enabled = !META_ENUM_INT("dsp:echodisable");
    apu.dsp.m.t_dir_addr = META_ENUM_INT("dsp:diraddr");
    apu.dsp.m.t_pitch = META_ENUM_INT("dsp:pitch");
    apu.dsp.m.t_output = META_ENUM_INT("dsp:output");
    apu.dsp.m.t_looped = META_ENUM_INT("dsp:looped");
    apu.dsp.m.t_echo_ptr = META_ENUM_INT("dsp:echoaddr");


#define META_ENUM_LEVELS(n, o) \
    value = metadata.enumValue(n); \
    if (value) \
    { \
        (o)[0] = strtoul(value, &end, 10); \
        if (*end) \
        { \
            value = end + 1; \
            (o)[1] = strtoul(value, &end, 10); \
        } \
    }

    META_ENUM_LEVELS("dsp:mainout", apu.dsp.m.t_main_out);
    META_ENUM_LEVELS("dsp:echoout", apu.dsp.m.t_echo_out);
    META_ENUM_LEVELS("dsp:echoin", apu.dsp.m.t_echo_in);

#undef META_ENUM_LEVELS

    for (int i = 0; i < 8; ++i)
    {
        sprintf(temp_path, "dsp:voice[%u]:", i);
        size_t length = strlen(temp_path);
        Spc_Dsp::voice_t & voice = apu.dsp.m.voices[i];
        strcpy(temp_path + length, "brrhistaddr");
        value = metadata.enumValue(temp_path);
        if (value)
        {
            voice.buf_pos = strtoul(value, &end, 10);
        }
        strcpy(temp_path + length, "brrhistdata");
        value = metadata.enumValue(temp_path);
        if (value)
        {
            for (int j = 0; j < Spc_Dsp::brr_buf_size; ++j)
            {
                voice.buf[j] = voice.buf[j + Spc_Dsp::brr_buf_size] = strtoul(value, &end, 10);
                if (!*end) break;
                value = end + 1;
            }
        }
        strcpy(temp_path + length, "interpaddr");
        voice.interp_pos = META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "brraddr");
        voice.brr_addr = META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "brroffset");
        voice.brr_offset = META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "vbit");
        voice.vbit = META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "vidx");
        voice.regs = &apu.dsp.m.regs[META_ENUM_INT(temp_path)];
        strcpy(temp_path + length, "kondelay");
        voice.kon_delay = META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "envmode");
        voice.env_mode = (Spc_Dsp::env_mode_t) META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "env");
        voice.env = META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "envxout");
        voice.t_envx_out = META_ENUM_INT(temp_path);
        strcpy(temp_path + length, "envcache");
        voice.hidden_env = META_ENUM_INT(temp_path);
    }

    filter.set_gain( (int) (gain() * Spc_Filter::gain_unit) );
    apu.clear_echo( true );
    return blargg_ok;
}

#undef META_ENUM_INT

blargg_err_t Sfm_Emu::play_and_filter( int count, sample_t out [] )
{
    RETURN_ERR( apu.play( count, out ) );
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
        RETURN_ERR( apu.skip( count ) );
        filter.clear();
    }

    // eliminate pop due to resampler
    const int resampler_latency = 64;
    sample_t buf [resampler_latency];
    return play_( resampler_latency, buf );
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

