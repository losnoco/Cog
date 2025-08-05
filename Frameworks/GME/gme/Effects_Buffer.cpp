// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/

#include "Effects_Buffer.h"

#include <string.h>
#include <algorithm>

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
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

#ifdef BLARGG_ENABLE_OPTIMIZER
	#include BLARGG_ENABLE_OPTIMIZER
#endif

typedef int32_t fixed_t;

using std::min;
using std::max;

#define TO_FIXED( f )   fixed_t ((f) * (1L << 15) + 0.5)
#define FMUL( x, y )    (((x) * (y)) >> 15)

static const unsigned echo_size = 4096;
static const unsigned echo_mask = echo_size - 1;
static_assert( (echo_size & echo_mask) == 0, "echo_size must be a power of 2" );

static const unsigned reverb_size = 8192 * 2;
static const unsigned reverb_mask = reverb_size - 1;
static_assert( (reverb_size & reverb_mask) == 0, "reverb_size must be a power of 2" );

Effects_Buffer::config_t::config_t()
{
	pan_1           = -0.15f;
	pan_2           =  0.15f;
	reverb_delay    = 88.0f;
	reverb_level    = 0.12f;
	echo_delay      = 61.0f;
	echo_level      = 0.10f;
	delay_variance  = 18.0f;
	effects_enabled = false;
}

void Effects_Buffer::set_depth( double d )
{
	float f = (float) d;
	config_t c;
	c.pan_1             = -0.6f * f;
	c.pan_2             =  0.6f * f;
	c.reverb_delay      = 880 * 0.1f;
	c.echo_delay        = 610 * 0.1f;
	if ( f > 0.5 )
		f = 0.5; // TODO: more linear reduction of extreme reverb/echo
	c.reverb_level      = 0.5f * f;
	c.echo_level        = 0.30f * f;
	c.delay_variance    = 180 * 0.1f;
	c.effects_enabled   = (d > 0.0f);
	config( c );
}

Effects_Buffer::Effects_Buffer( int num_voices, bool center_only )
	: Multi_Buffer( 2*num_voices )
	, max_voices(num_voices)
	, bufs(max_voices * (center_only ? (max_buf_count - 4) : max_buf_count))
	, chan_types(max_voices * chan_types_count)
	, stereo_remain(0)
	, effect_remain(0)
	// TODO: Reorder buf_count to be initialized before bufs to factor out channel sizing
	, buf_count(max_voices * (center_only ? (max_buf_count - 4) : max_buf_count))
	, effects_enabled(false)
	, reverb_buf(max_voices, std::vector<blip_sample_t>(reverb_size))
	, echo_buf(max_voices, std::vector<blip_sample_t>(echo_size))
	, reverb_pos(max_voices)
	, echo_pos(max_voices)
{
	set_depth( 0 );
}

Effects_Buffer::~Effects_Buffer()
{}

blargg_err_t Effects_Buffer::set_sample_rate( long rate, int msec ) noexcept
{
	for(int i=0; i<max_voices; i++)
	{
		if ( !echo_buf[i].size() )
		{
			echo_buf[i].resize( echo_size );
		}

		if ( !reverb_buf[i].size() )
		{
			reverb_buf[i].resize( reverb_size );
		}

		if ( !echo_buf[i].size() || !reverb_buf[i].size() )
		{
			return "Out of memory";
		}
	}

	for ( int i = 0; i < buf_count; i++ )
		RETURN_ERR( bufs [i].set_sample_rate( rate, msec ) );

	config( config_ );
	clear();

	return Multi_Buffer::set_sample_rate( bufs [0].sample_rate(), bufs [0].length() );
}

void Effects_Buffer::clock_rate( long rate )
{
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].clock_rate( rate );
}

void Effects_Buffer::bass_freq( int freq )
{
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].bass_freq( freq );
}

void Effects_Buffer::clear()
{
	stereo_remain = 0;
	effect_remain = 0;

	for(int i=0; i<max_voices; i++)
	{
		if ( echo_buf[i].size() )
			memset( &echo_buf[i][0], 0, echo_size * sizeof echo_buf[i][0] );

		if ( reverb_buf[i].size() )
			memset( &reverb_buf[i][0], 0, reverb_size * sizeof reverb_buf[i][0] );
	}

	for ( int i = 0; i < buf_count; i++ )
		bufs [i].clear();
}

inline int pin_range( int n, int max, int min = 0 )
{
	if ( n < min )
		return min;
	if ( n > max )
		return max;
	return n;
}

void Effects_Buffer::config( const config_t& cfg )
{
	channels_changed();

	// clear echo and reverb buffers
	// ensure the echo/reverb buffers have already been allocated, so this method can be
	// called before set_sample_rate is called
	if ( !config_.effects_enabled && cfg.effects_enabled && echo_buf[0].size() )
	{
		for(int i=0; i<max_voices; i++)
		{
			memset( &echo_buf[i][0], 0, echo_size * sizeof echo_buf[i][0] );
			memset( &reverb_buf[i][0], 0, reverb_size * sizeof reverb_buf[i][0] );
		}
	}

	config_ = cfg;

	if ( config_.effects_enabled )
	{
		// convert to internal format

		chans.pan_1_levels [0] = TO_FIXED( 1 ) - TO_FIXED( config_.pan_1 );
		chans.pan_1_levels [1] = TO_FIXED( 2 ) - chans.pan_1_levels [0];

		chans.pan_2_levels [0] = TO_FIXED( 1 ) - TO_FIXED( config_.pan_2 );
		chans.pan_2_levels [1] = TO_FIXED( 2 ) - chans.pan_2_levels [0];

		chans.reverb_level = TO_FIXED( config_.reverb_level );
		chans.echo_level = TO_FIXED( config_.echo_level );

		int delay_offset = int (1.0 / 2000 * config_.delay_variance * sample_rate());

		int reverb_sample_delay = int (1.0 / 1000 * config_.reverb_delay * sample_rate());
		chans.reverb_delay_l = pin_range( reverb_size -
				(reverb_sample_delay - delay_offset) * 2, reverb_size - 2, 0 );
		chans.reverb_delay_r = pin_range( reverb_size + 1 -
				(reverb_sample_delay + delay_offset) * 2, reverb_size - 1, 1 );

		int echo_sample_delay = int (1.0 / 1000 * config_.echo_delay * sample_rate());
		chans.echo_delay_l = pin_range( echo_size - 1 - (echo_sample_delay - delay_offset),
				echo_size - 1 );
		chans.echo_delay_r = pin_range( echo_size - 1 - (echo_sample_delay + delay_offset),
				echo_size - 1 );

		for(int i=0; i<max_voices; i++)
		{
			chan_types [i*chan_types_count+0].center = &bufs [i*max_buf_count+0];
			chan_types [i*chan_types_count+0].left   = &bufs [i*max_buf_count+3];
			chan_types [i*chan_types_count+0].right  = &bufs [i*max_buf_count+4];

			chan_types [i*chan_types_count+1].center = &bufs [i*max_buf_count+1];
			chan_types [i*chan_types_count+1].left   = &bufs [i*max_buf_count+3];
			chan_types [i*chan_types_count+1].right  = &bufs [i*max_buf_count+4];

			chan_types [i*chan_types_count+2].center = &bufs [i*max_buf_count+2];
			chan_types [i*chan_types_count+2].left   = &bufs [i*max_buf_count+5];
			chan_types [i*chan_types_count+2].right  = &bufs [i*max_buf_count+6];
		}
		blaarg_static_assert( chan_types_count >= 3, "Need at least three audio channels for effects processing" );
	}
	else
	{
		for(int i=0; i<max_voices; i++)
		{
			// set up outputs
			for ( int j = 0; j < chan_types_count; j++ )
			{
				channel_t& c = chan_types [i*chan_types_count+j];
				c.center = &bufs [i*max_buf_count+0];
				c.left   = &bufs [i*max_buf_count+1];
				c.right  = &bufs [i*max_buf_count+2];
			}
		}
	}

	if ( buf_count < max_buf_count ) // if center_only
	{
		for(int i=0; i<max_voices; i++)
		{
			for ( int j = 0; j < chan_types_count; j++ )
			{
				channel_t& c = chan_types [i*chan_types_count+j];
				c.left   = c.center;
				c.right  = c.center;
			}
		}
	}
}

Effects_Buffer::channel_t Effects_Buffer::channel( int i, int type )
{
	int out = chan_types_count-1;
	if ( !type )
	{
		out = i % 5;
		if ( out > chan_types_count-1 )
			out = chan_types_count-1;
	}
	else if ( !(type & noise_type) && (type & type_index_mask) % 3 != 0 )
	{
		out = type & 1;
	}
	return chan_types [(i%max_voices)*chan_types_count+out];
}

void Effects_Buffer::end_frame( blip_time_t clock_count )
{
	int bufs_used = 0;
	int stereo_mask = (config_.effects_enabled ? 0x78 : 0x06);

	const int buf_count_per_voice = buf_count/max_voices;
	for ( int v = 0; v < max_voices; v++ ) // foreach voice
	{
		for ( int i = 0; i < buf_count_per_voice; i++) // foreach buffer of that voice
		{
			bufs_used |= bufs [v*buf_count_per_voice + i].clear_modified() << i;
			bufs [v*buf_count_per_voice + i].end_frame( clock_count );

			if ( (bufs_used & stereo_mask) && buf_count == max_voices*max_buf_count )
				stereo_remain = max(stereo_remain, bufs [v*buf_count_per_voice + i].samples_avail() + bufs [v*buf_count_per_voice + i].output_latency());
			if ( effects_enabled || config_.effects_enabled )
				effect_remain = max(effect_remain, bufs [v*buf_count_per_voice + i].samples_avail() + bufs [v*buf_count_per_voice + i].output_latency());
		}
		bufs_used = 0;
	}

	effects_enabled = config_.effects_enabled;
}

long Effects_Buffer::samples_avail() const
{
	return bufs [0].samples_avail() * 2;
}

long Effects_Buffer::read_samples( blip_sample_t* out, long total_samples )
{
	const int n_channels = max_voices * 2;
	const int buf_count_per_voice = buf_count/max_voices;

	require( total_samples % n_channels == 0 ); // as many items needed to fill at least one frame

	long remain = bufs [0].samples_avail();
	total_samples = remain = min( remain, total_samples/n_channels );

	while ( remain )
	{
		int active_bufs = buf_count_per_voice;
		long count = remain;

		// optimizing mixing to skip any channels which had nothing added
		if ( effect_remain )
		{
			if ( count > effect_remain )
				count = effect_remain;

			if ( stereo_remain )
			{
				mix_enhanced( out, count );
			}
			else
			{
				mix_mono_enhanced( out, count );
				active_bufs = 3;
			}
		}
		else if ( stereo_remain )
		{
			mix_stereo( out, count );
			active_bufs = 3;
		}
		else
		{
			mix_mono( out, count );
			active_bufs = 1;
		}

		out += count * n_channels;
		remain -= count;

		stereo_remain -= count;
		if ( stereo_remain < 0 )
			stereo_remain = 0;

		effect_remain -= count;
		if ( effect_remain < 0 )
			effect_remain = 0;

		// skip the output from any buffers that didn't contribute to the sound output
		// during this frame (e.g. if we only render mono then only the very first buf
		// is 'active')
		for ( int v = 0; v < max_voices; v++ ) // foreach voice
		{
			for ( int i = 0; i < buf_count_per_voice; i++) // foreach buffer of that voice
			{
				if ( i < active_bufs )
					bufs [v*buf_count_per_voice + i].remove_samples( count );
				else // keep time synchronized
					bufs [v*buf_count_per_voice + i].remove_silence( count );
			}
		}
	}

	return total_samples * n_channels;
}

void Effects_Buffer::mix_mono( blip_sample_t* out_, int32_t count )
{
    for(int i=0; i<max_voices; i++)
    {
	blip_sample_t* BLIP_RESTRICT out = out_;
	int const bass = BLIP_READER_BASS( bufs [i*max_buf_count+0] );
	BLIP_READER_BEGIN( c, bufs [i*max_buf_count+0] );

	// unrolled loop
	for ( int32_t n = count >> 1; n; --n )
	{
		int32_t cs0 = BLIP_READER_READ( c );
		BLIP_READER_NEXT( c, bass );

		int32_t cs1 = BLIP_READER_READ( c );
		BLIP_READER_NEXT( c, bass );

		if ( (int16_t) cs0 != cs0 )
			cs0 = 0x7FFF - (cs0 >> 24);
		((uint32_t*) out) [i] = ((uint16_t) cs0) | (uint16_t(cs0) << 16);

		if ( (int16_t) cs1 != cs1 )
			cs1 = 0x7FFF - (cs1 >> 24);
		((uint32_t*) out) [i+max_voices] = ((uint16_t) cs1) | (uint16_t(cs1) << 16);
		out += max_voices*4;
	}

	if ( count & 1 )
	{
		int s = BLIP_READER_READ( c );
		BLIP_READER_NEXT( c, bass );
		if ( (int16_t) s != s )
			s = 0x7FFF - (s >> 24);
		out [i*2+0] = s;
		out [i*2+1] = s;
	}

	BLIP_READER_END( c, bufs [i*max_buf_count+0] );
    }
}

void Effects_Buffer::mix_stereo( blip_sample_t* out_, int32_t frames )
{
    for(int i=0; i<max_voices; i++)
    {
	blip_sample_t* BLIP_RESTRICT out = out_;
	int const bass = BLIP_READER_BASS( bufs [i*max_buf_count+0] );
	BLIP_READER_BEGIN( c, bufs [i*max_buf_count+0] );
	BLIP_READER_BEGIN( l, bufs [i*max_buf_count+1] );
	BLIP_READER_BEGIN( r, bufs [i*max_buf_count+2] );

	int count = frames;
	while ( count-- )
	{
		int cs = BLIP_READER_READ( c );
		BLIP_READER_NEXT( c, bass );
		int left = cs + BLIP_READER_READ( l );
		int right = cs + BLIP_READER_READ( r );
		BLIP_READER_NEXT( l, bass );
		BLIP_READER_NEXT( r, bass );

		if ( (int16_t) left != left )
			left = 0x7FFF - (left >> 24);

		if ( (int16_t) right != right )
			right = 0x7FFF - (right >> 24);

		out [i*2+0] = left;
		out [i*2+1] = right;

		out += max_voices*2;

	}

	BLIP_READER_END( r, bufs [i*max_buf_count+2] );
	BLIP_READER_END( l, bufs [i*max_buf_count+1] );
	BLIP_READER_END( c, bufs [i*max_buf_count+0] );
    }
}

void Effects_Buffer::mix_mono_enhanced( blip_sample_t* out_, int32_t frames )
{
	for(int i=0; i<max_voices; i++)
	{
	blip_sample_t* BLIP_RESTRICT out = out_;
	int const bass = BLIP_READER_BASS( bufs [i*max_buf_count+2] );
	BLIP_READER_BEGIN( center, bufs [i*max_buf_count+2] );
	BLIP_READER_BEGIN( sq1, bufs [i*max_buf_count+0] );
	BLIP_READER_BEGIN( sq2, bufs [i*max_buf_count+1] );

	blip_sample_t* const reverb_buf = &this->reverb_buf[i][0];
	blip_sample_t* const echo_buf = &this->echo_buf[i][0];
	int echo_pos = this->echo_pos[i];
	int reverb_pos = this->reverb_pos[i];

	int count = frames;
	while ( count-- )
	{
		int sum1_s = BLIP_READER_READ( sq1 );
		int sum2_s = BLIP_READER_READ( sq2 );

		BLIP_READER_NEXT( sq1, bass );
		BLIP_READER_NEXT( sq2, bass );

		int new_reverb_l = FMUL( sum1_s, chans.pan_1_levels [0] ) +
				FMUL( sum2_s, chans.pan_2_levels [0] ) +
				reverb_buf [(reverb_pos + chans.reverb_delay_l) & reverb_mask];

		int new_reverb_r = FMUL( sum1_s, chans.pan_1_levels [1] ) +
				FMUL( sum2_s, chans.pan_2_levels [1] ) +
				reverb_buf [(reverb_pos + chans.reverb_delay_r) & reverb_mask];

		fixed_t reverb_level = chans.reverb_level;
		reverb_buf [reverb_pos] = (blip_sample_t) FMUL( new_reverb_l, reverb_level );
		reverb_buf [reverb_pos + 1] = (blip_sample_t) FMUL( new_reverb_r, reverb_level );
		reverb_pos = (reverb_pos + 2) & reverb_mask;

		int sum3_s = BLIP_READER_READ( center );
		BLIP_READER_NEXT( center, bass );

		int left = new_reverb_l + sum3_s + FMUL( chans.echo_level,
				echo_buf [(echo_pos + chans.echo_delay_l) & echo_mask] );
		int right = new_reverb_r + sum3_s + FMUL( chans.echo_level,
				echo_buf [(echo_pos + chans.echo_delay_r) & echo_mask] );

		echo_buf [echo_pos] = sum3_s;
		echo_pos = (echo_pos + 1) & echo_mask;

		if ( (int16_t) left != left )
			left = 0x7FFF - (left >> 24);

		if ( (int16_t) right != right )
			right = 0x7FFF - (right >> 24);

		out [i*2+0] = left;
		out [i*2+1] = right;
		out += max_voices*2;
	}
	this->reverb_pos[i] = reverb_pos;
	this->echo_pos[i] = echo_pos;

	BLIP_READER_END( sq1, bufs [i*max_buf_count+0] );
	BLIP_READER_END( sq2, bufs [i*max_buf_count+1] );
	BLIP_READER_END( center, bufs [i*max_buf_count+2] );
    }
}

void Effects_Buffer::mix_enhanced( blip_sample_t* out_, int32_t frames )
{
    for(int i=0; i<max_voices; i++)
    {
	blip_sample_t* BLIP_RESTRICT out = out_;
	int const bass = BLIP_READER_BASS( bufs [i*max_buf_count+2] );
	BLIP_READER_BEGIN( center, bufs [i*max_buf_count+2] );
	BLIP_READER_BEGIN( l1, bufs [i*max_buf_count+3] );
	BLIP_READER_BEGIN( r1, bufs [i*max_buf_count+4] );
	BLIP_READER_BEGIN( l2, bufs [i*max_buf_count+5] );
	BLIP_READER_BEGIN( r2, bufs [i*max_buf_count+6] );
	BLIP_READER_BEGIN( sq1, bufs [i*max_buf_count+0] );
	BLIP_READER_BEGIN( sq2, bufs [i*max_buf_count+1] );

	blip_sample_t* const reverb_buf = &this->reverb_buf[i][0];
	blip_sample_t* const echo_buf = &this->echo_buf[i][0];
	int echo_pos = this->echo_pos[i];
	int reverb_pos = this->reverb_pos[i];

	int count = frames;
	while ( count-- )
	{
		int sum1_s = BLIP_READER_READ( sq1 );
		int sum2_s = BLIP_READER_READ( sq2 );

		BLIP_READER_NEXT( sq1, bass );
		BLIP_READER_NEXT( sq2, bass );

		int new_reverb_l = FMUL( sum1_s, chans.pan_1_levels [0] ) +
				FMUL( sum2_s, chans.pan_2_levels [0] ) + BLIP_READER_READ( l1 ) +
				reverb_buf [(reverb_pos + chans.reverb_delay_l) & reverb_mask];

		int new_reverb_r = FMUL( sum1_s, chans.pan_1_levels [1] ) +
				FMUL( sum2_s, chans.pan_2_levels [1] ) + BLIP_READER_READ( r1 ) +
				reverb_buf [(reverb_pos + chans.reverb_delay_r) & reverb_mask];

		BLIP_READER_NEXT( l1, bass );
		BLIP_READER_NEXT( r1, bass );

		fixed_t reverb_level = chans.reverb_level;
		reverb_buf [reverb_pos] = (blip_sample_t) FMUL( new_reverb_l, reverb_level );
		reverb_buf [reverb_pos + 1] = (blip_sample_t) FMUL( new_reverb_r, reverb_level );
		reverb_pos = (reverb_pos + 2) & reverb_mask;

		int sum3_s = BLIP_READER_READ( center );
		BLIP_READER_NEXT( center, bass );

		int left = new_reverb_l + sum3_s + BLIP_READER_READ( l2 ) + FMUL( chans.echo_level,
				echo_buf [(echo_pos + chans.echo_delay_l) & echo_mask] );
		int right = new_reverb_r + sum3_s + BLIP_READER_READ( r2 ) + FMUL( chans.echo_level,
				echo_buf [(echo_pos + chans.echo_delay_r) & echo_mask] );

		BLIP_READER_NEXT( l2, bass );
		BLIP_READER_NEXT( r2, bass );

		echo_buf [echo_pos] = sum3_s;
		echo_pos = (echo_pos + 1) & echo_mask;

		if ( (int16_t) left != left )
			left = 0x7FFF - (left >> 24);

		if ( (int16_t) right != right )
			right = 0x7FFF - (right >> 24);

		out [i*2+0] = left;
		out [i*2+1] = right;

		out += max_voices*2;
	}
	this->reverb_pos[i] = reverb_pos;
	this->echo_pos[i] = echo_pos;

	BLIP_READER_END( l1, bufs [i*max_buf_count+3] );
	BLIP_READER_END( r1, bufs [i*max_buf_count+4] );
	BLIP_READER_END( l2, bufs [i*max_buf_count+5] );
	BLIP_READER_END( r2, bufs [i*max_buf_count+6] );
	BLIP_READER_END( sq1, bufs [i*max_buf_count+0] );
	BLIP_READER_END( sq2, bufs [i*max_buf_count+1] );
	BLIP_READER_END( center, bufs [i*max_buf_count+2] );
    }
}

