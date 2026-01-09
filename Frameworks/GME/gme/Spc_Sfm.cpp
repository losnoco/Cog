// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/

#include "Spc_Sfm.h"

#include "blargg_endian.h"

#include <stdio.h>

/* Copyright (C) 2013-2026 Christopher Snowhill. This module is free
software; you can redistribute it and/or modify it under the terms of
the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2.1 of the License, or (at your option) any
later version. This module is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
General Public License for more details. You should have received a copy
of the GNU Lesser General Public License along with this module; if not,
write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth
Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

#ifndef _countof
#define _countof(x) (sizeof((x))/sizeof((x)[0]))
#endif

// TODO: support Spc_Filter's bass

Sfm_Emu::Sfm_Emu()
{
	set_type( gme_sfm_type );
	set_voice_count( SuperFamicom::SPC_DSP::voice_count );
	static const char* const names [SuperFamicom::SPC_DSP::voice_count] = {
		"DSP 1", "DSP 2", "DSP 3", "DSP 4", "DSP 5", "DSP 6", "DSP 7", "DSP 8"
	};
	set_voice_names( names );
	set_gain( 1.4 );
	set_max_initial_silence( 30 );
	set_silence_lookahead( 30 ); // Some SFMs may have a lot of initialization code
	enable_echo( true );
}

Sfm_Emu::~Sfm_Emu() { }

// Track info

static void copy_field( char* out, size_t size, const Bml_Parser& in, char const* in_path )
{
	const char * value = in.enumValue( in_path );
	if ( value ) strncpy( out, value, size - 1 ), out[ size - 1 ] = 0;
	else out[ 0 ] = 0;
}

static void copy_info( track_info_t* out, const Bml_Parser& in )
{
	copy_field( out->song, sizeof(out->song), in, "information:title" );
	copy_field( out->game, sizeof(out->game), in, "information:game" );
	copy_field( out->author, sizeof(out->author), in, "information:author" );
	copy_field( out->composer, sizeof(out->composer), in, "information:composer" );
	copy_field( out->copyright, sizeof(out->copyright), in, "information:copyright" );
	copy_field( out->date, sizeof(out->date), in, "information:date" );
	copy_field( out->track, sizeof(out->track), in, "information:track" );
	copy_field( out->disc, sizeof(out->disc), in, "information:disc" );
	copy_field( out->dumper, sizeof(out->dumper), in, "information:dumper" );
	
	char * end;
	const char * value = in.enumValue( "timing:length" );
	if ( value )
		out->length = strtoul( value, &end, 10 );
	else
		out->length = 0;
	
	value = in.enumValue( "timing:fade" );
	if ( value )
		out->fade_length = strtoul( value, &end, 10 );
	else
		out->fade_length = -1;
}

blargg_err_t Sfm_Emu::track_info_( track_info_t* out, int ) const
{
	copy_info( out, metadata );
	return 0;
}

static void set_track_info( const track_info_t* in, Bml_Parser& out )
{
	out.setValue( "information:title", in->song );
	out.setValue( "information:game", in->game );
	out.setValue( "information:author", in->author );
	out.setValue( "information:composer", in->composer );
	out.setValue( "information:copyright", in->copyright );
	out.setValue( "information:date", in->date );
	out.setValue( "information:track", in->track );
	out.setValue( "information:disc", in->disc );
	out.setValue( "information:dumper", in->dumper );
	
	out.setValue( "timing:length", in->length );
	out.setValue( "timing:fade", in->fade_length );
}

blargg_err_t Sfm_Emu::set_track_info_( const track_info_t* in, int )
{
	::set_track_info(in, metadata);
	
	return 0;
}

static blargg_err_t check_sfm_header( void const* header )
{
	if ( memcmp( header, "SFM1", 4 ) )
		return gme_wrong_file_type;
	return 0;
}

struct Sfm_File : Gme_Info_
{
	blargg_vector<byte> data;
	Bml_Parser metadata;
	unsigned long original_metadata_size;
	
	Sfm_File() { set_type( gme_sfm_type ); }
	
	blargg_err_t load_( Data_Reader& in )
	{
		long file_size = in.remain();
		if ( file_size < Sfm_Emu::sfm_min_file_size )
			return gme_wrong_file_type;
		RETURN_ERR( data.resize( file_size ) );
		RETURN_ERR( in.read( data.begin(), data.end() - data.begin() ) );
		RETURN_ERR( check_sfm_header( data.begin() ) );
		if ( file_size < 8 )
			return "SFM file too small";
		int metadata_size = get_le32( data.begin() + 4 );
		metadata.parseDocument( (const char *)data.begin() + 8, metadata_size );
		original_metadata_size = metadata_size;
		return 0;
	}
	
	blargg_err_t track_info_( track_info_t* out, int ) const
	{
		copy_info( out, metadata );
		return 0;
	}
	
	blargg_err_t set_track_info_( const track_info_t* in, int )
	{
		::set_track_info( in, metadata );
		return 0;
	}
};

static Music_Emu* new_sfm_emu () { return BLARGG_NEW Sfm_Emu ; }
static Music_Emu* new_sfm_file() { return BLARGG_NEW Sfm_File; }

static gme_type_t_ const gme_sfm_type_ = { "Super Nintendo with log", 1, &new_sfm_emu, &new_sfm_file, "SFM", 0 };
extern gme_type_t const gme_sfm_type = &gme_sfm_type_;

// Setup

blargg_err_t Sfm_Emu::set_sample_rate_( long sample_rate )
{
	smp.power();
	if ( sample_rate != native_sample_rate )
	{
		RETURN_ERR( resampler.buffer_size( native_sample_rate / 20 * 2 ) );
		resampler.time_ratio( (double) native_sample_rate / sample_rate, 0.9965 );
	}
	return 0;
}

void Sfm_Emu::enable_accuracy_( bool b )
{
        Music_Emu::enable_accuracy_( b );
        filter.enable( b );
	if ( b ) enable_echo();
}

void Sfm_Emu::mute_voices_( int m )
{
	Music_Emu::mute_voices_( m );
	for ( int i = 0, j = 1; i < 8; ++i, j <<= 1 )
		smp.dsp.channel_enable( i, !( m & j ) );
}

blargg_err_t Sfm_Emu::load_mem_( byte const in [], long size )
{
	if ( size < Sfm_Emu::sfm_min_file_size )
		return gme_wrong_file_type;
	
	file_data = in;
	file_size = size;
	
	RETURN_ERR( check_sfm_header( in ) );

	const byte * ptr = file_data;
	int metadata_size = get_le32(ptr + 4);
	if ( file_size < metadata_size + Sfm_Emu::sfm_min_file_size )
		return "SFM file too small";
	metadata.parseDocument((const char *) ptr + 8, metadata_size);
	
	return 0;
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
	const byte * ptr = file_data;
	int metadata_size = get_le32(ptr + 4);
	
	memcpy( smp.iplrom, ipl_rom, 64 );
	
	smp.reset();
	
	memcpy( smp.apuram, ptr + 8 + metadata_size, 65536 );
	memcpy( smp.dsp.spc_dsp.m.echo_ram, ptr + 8 + metadata_size, 65536 );
	
	memcpy( smp.dsp.spc_dsp.m.regs, ptr + 8 + metadata_size + 65536, 128 );
	
	const uint8_t* log_begin = ptr + 8 + metadata_size + 65536 + 128;
	const uint8_t* log_end = ptr + file_size;
	size_t loop_begin = log_end - log_begin;
	
	char * end;
	const char * value;
	
	loop_begin = META_ENUM_INT("timing:loopstart", loop_begin);
	
	smp.set_sfm_queue( log_begin, log_end, log_begin + loop_begin );
	
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
		for (unsigned long i = 0; i < _countof(smp.sfm_last); i++)
		{
			smp.sfm_last[i] = strtol(value, &end, 10);
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
	
	filter.set_gain( (int) (gain() * SPC_Filter::gain_unit) );
	return 0;
}

#undef META_ENUM_INT

blargg_err_t Sfm_Emu::play_and_filter( long count, sample_t out [] )
{
	smp.render( out, count );
	filter.run( out, count );
	return 0;
}

blargg_err_t Sfm_Emu::skip_( long count )
{
	if ( sample_rate() != native_sample_rate )
	{
		count = long (count * resampler.ratio()) & ~1;
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
		const long resampler_latency = 64;
		sample_t buf [resampler_latency];
		return play_( resampler_latency, buf );
	}
	
	return 0;
}

blargg_err_t Sfm_Emu::play_( long count, sample_t out [] )
{
	if ( sample_rate() == native_sample_rate )
		return play_and_filter( count, out );
	
	long remain = count;
	while ( remain > 0 )
	{
		remain -= resampler.read( &out [count - remain], remain );
		if ( remain > 0 )
		{
			int n = resampler.max_write();
			RETURN_ERR( play_and_filter( n, resampler.buffer() ) );
			resampler.write( n );
		}
	}
	check( remain == 0 );
	return 0;
}
