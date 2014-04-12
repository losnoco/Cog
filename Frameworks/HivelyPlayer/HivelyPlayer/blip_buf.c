/* blip_buf 1.1.0. http://www.slack.net/~ant/ */

#include "blip_buf.h"

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

/* Library Copyright (C) 2003-2009 Shay Green. This library is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#if defined (BLARGG_TEST) && BLARGG_TEST
	#include "blargg_test.h"
#endif

/* Equivalent to ULONG_MAX >= 0xFFFFFFFF00000000.
Avoids constants that don't fit in 32 bits. */
#if ULONG_MAX/0xFFFFFFFF > 0xFFFFFFFF
	typedef unsigned long fixed_t;
	enum { pre_shift = 32 };

#elif defined(ULLONG_MAX)
	typedef unsigned long long fixed_t;
	enum { pre_shift = 32 };

#else
	typedef unsigned fixed_t;
	enum { pre_shift = 0 };

#endif

enum { time_bits = pre_shift + 20 };

static fixed_t const time_unit = (fixed_t) 1 << time_bits;

enum { bass_shift  = 9 }; /* affects high-pass filter breakpoint frequency */
enum { end_frame_extra = 2 }; /* allows deltas slightly after frame length */

enum { half_width  = 8 };
enum { buf_extra   = half_width*2 + end_frame_extra };
enum { phase_bits  = 5 };
enum { phase_count = 1 << phase_bits };
enum { delta_bits  = 15 };
enum { delta_unit  = 1 << delta_bits };
enum { frac_bits = time_bits - pre_shift };

/* We could eliminate avail and encode whole samples in offset, but that would
limit the total buffered samples to blip_max_frame. That could only be
increased by decreasing time_bits, which would reduce resample ratio accuracy.
*/

/** Sample buffer that resamples to output rate and accumulates samples
until they're read out */
struct hvl_blip_t
{
	fixed_t factor;
	fixed_t offset;
	int avail;
	int size;
	fixed_t integrator;
};

typedef int buf_t;

/* probably not totally portable */
#define SAMPLES( buf ) ((buf_t*) ((buf) + 1))

/* Arithmetic (sign-preserving) right shift */
#define ARITH_SHIFT( n, shift ) \
	((n) >> (shift))

enum { max_sample = +32767 };
enum { min_sample = -32768 };

#define CLAMP( n ) \
	{\
		if ( (short) n != n )\
			n = ARITH_SHIFT( n, 16 ) ^ max_sample;\
	}

static void check_assumptions( void )
{
	int n;
	
	#if INT_MAX < 0x7FFFFFFF || UINT_MAX < 0xFFFFFFFF
		#error "int must be at least 32 bits"
	#endif
	
	assert( (-3 >> 1) == -2 ); /* right shift must preserve sign */
	
	n = max_sample * 2;
	CLAMP( n );
	assert( n == max_sample );
	
	n = min_sample * 2;
	CLAMP( n );
	assert( n == min_sample );
	
    assert( hvl_blip_max_ratio <= time_unit );
    assert( hvl_blip_max_frame <= (fixed_t) -1 >> time_bits );
}

size_t hvl_blip_size( int size )
{
	assert( size >= 0 );
    return sizeof (hvl_blip_t) + (size + buf_extra) * sizeof (buf_t);
}

void hvl_blip_new_inplace( hvl_blip_t* m, int size )
{
	assert( size >= 0 );
	
	if ( m )
	{
        m->factor = time_unit / hvl_blip_max_ratio;
		m->size   = size;
        hvl_blip_clear( m );
		check_assumptions();
	}
}

void hvl_blip_set_rates( hvl_blip_t* m, double clock_rate, double sample_rate )
{
	double factor = time_unit * sample_rate / clock_rate;
	m->factor = (fixed_t) factor;
	
	/* Fails if clock_rate exceeds maximum, relative to sample_rate */
	assert( 0 <= factor - m->factor && factor - m->factor < 1 );
	
	/* Avoid requiring math.h. Equivalent to
	m->factor = (int) ceil( factor ) */
	if ( m->factor < factor )
		m->factor++;
	
	/* At this point, factor is most likely rounded up, but could still
	have been rounded down in the floating-point calculation. */
}

void hvl_blip_clear( hvl_blip_t* m )
{
	/* We could set offset to 0, factor/2, or factor-1. 0 is suitable if
	factor is rounded up. factor-1 is suitable if factor is rounded down.
	Since we don't know rounding direction, factor/2 accommodates either,
	with the slight loss of showing an error in half the time. Since for
	a 64-bit factor this is years, the halving isn't a problem. */
	
	m->offset     = m->factor / 2;
	m->avail      = 0;
	m->integrator = 0;
	memset( SAMPLES( m ), 0, (m->size + buf_extra) * sizeof (buf_t) );
}

int hvl_blip_clocks_needed( const hvl_blip_t* m, int samples )
{
	fixed_t needed;
	
	/* Fails if buffer can't hold that many more samples */
	assert( samples >= 0 && m->avail + samples <= m->size );
	
	needed = (fixed_t) samples * time_unit;
	if ( needed < m->offset )
		return 0;
	
	return (needed - m->offset + m->factor - 1) / m->factor;
}

void hvl_blip_end_frame( hvl_blip_t* m, unsigned t )
{
	fixed_t off = t * m->factor + m->offset;
	m->avail += off >> time_bits;
	m->offset = off & (time_unit - 1);
	
	/* Fails if buffer size was exceeded */
	assert( m->avail <= m->size );
}

int hvl_blip_samples_avail( const hvl_blip_t* m )
{
	return m->avail;
}

static void remove_samples( hvl_blip_t* m, int count )
{
	buf_t* buf = SAMPLES( m );
	int remain = m->avail + buf_extra - count;
	m->avail -= count;
	
	memmove( &buf [0], &buf [count], remain * sizeof buf [0] );
	memset( &buf [remain], 0, count * sizeof buf [0] );
}

int hvl_blip_read_samples( hvl_blip_t* m, int out [], int count, int gain )
{
	assert( count >= 0 );
	
	if ( count > m->avail )
		count = m->avail;
	
	if ( count )
	{
		buf_t const* in  = SAMPLES( m );
		buf_t const* end = in + count;
		fixed_t sum = m->integrator;
		do
		{
			/* Eliminate fraction */
			int s = ARITH_SHIFT( sum, delta_bits );
			
			sum += *in++;
			
			*out = s * gain;
			out += 2;
			
			/* High-pass filter */
			sum -= s << (delta_bits - bass_shift);
		}
		while ( in != end );
		m->integrator = sum;
		
		remove_samples( m, count );
	}
	
	return count;
}

/* Things that didn't help performance on x86:
	__attribute__((aligned(128)))
	#define short int
	restrict
*/

static int const bl_step [phase_count + 1] [half_width] =
{
{    0,    0,    0,    0,    0,    0,    0,32768},
{   -1,    9,  -30,   79, -178,  380, -923,32713},
{   -2,   17,  -58,  153, -346,  739,-1775,32549},
{   -3,   24,  -83,  221, -503, 1073,-2555,32277},
{   -4,   30, -107,  284, -647, 1382,-3259,31898},
{   -5,   36, -127,  340, -778, 1662,-3887,31415},
{   -5,   40, -145,  390, -895, 1913,-4439,30832},
{   -6,   44, -160,  433, -998, 2133,-4914,30151},
{   -6,   47, -172,  469,-1085, 2322,-5313,29377},
{   -6,   49, -181,  499,-1158, 2479,-5636,28515},
{   -6,   50, -188,  521,-1215, 2604,-5885,27570},
{   -5,   51, -193,  537,-1257, 2697,-6063,26548},
{   -5,   51, -195,  547,-1285, 2760,-6172,25455},
{   -5,   50, -195,  550,-1298, 2792,-6214,24298},
{   -4,   49, -192,  548,-1298, 2795,-6193,23084},
{   -4,   47, -188,  540,-1284, 2770,-6112,21820},
{   -3,   45, -182,  526,-1258, 2719,-5976,20513},
{   -3,   42, -175,  508,-1221, 2643,-5788,19172},
{   -2,   39, -166,  486,-1173, 2544,-5554,17805},
{   -2,   36, -156,  460,-1116, 2425,-5277,16418},
{   -1,   33, -145,  431,-1050, 2287,-4963,15020},
{   -1,   30, -133,  399, -977, 2132,-4615,13618},
{   -1,   26, -120,  365, -898, 1963,-4240,12221},
{    0,   23, -107,  329, -813, 1783,-3843,10836},
{    0,   20,  -94,  292, -725, 1593,-3427, 9470},
{    0,   17,  -81,  254, -633, 1396,-2998, 8131},
{    0,   14,  -68,  215, -540, 1194,-2560, 6824},
{    0,   11,  -56,  177, -446,  989,-2119, 5556},
{    0,    8,  -43,  139, -353,  784,-1678, 4334},
{    0,    6,  -31,  102, -260,  581,-1242, 3162},
{    0,    3,  -20,   66, -170,  381, -814, 2046},
{    0,    1,   -9,   32,  -83,  187, -399,  991},
{    0,    0,    0,    0,    0,    0,    0,    0}
};

/* Shifting by pre_shift allows calculation using unsigned int rather than
possibly-wider fixed_t. On 32-bit platforms, this is likely more efficient.
And by having pre_shift 32, a 32-bit platform can easily do the shift by
simply ignoring the low half. */

void hvl_blip_add_delta( hvl_blip_t* m, unsigned time, int delta )
{
	unsigned fixed = (unsigned) ((time * m->factor + m->offset) >> pre_shift);
	buf_t* out = SAMPLES( m ) + m->avail + (fixed >> frac_bits);
	
	int const phase_shift = frac_bits - phase_bits;
	int phase = fixed >> phase_shift & (phase_count - 1);
	int const* in  = bl_step [phase];
	int const* rev = bl_step [phase_count - phase];
	
	int interp = fixed >> (phase_shift - delta_bits) & (delta_unit - 1);
	int delta2 = (delta * interp) >> delta_bits;
	delta -= delta2;
	
	/* Fails if buffer size was exceeded */
	assert( out <= &SAMPLES( m ) [m->size + end_frame_extra] );
	
	out [0] += in[0]*delta + in[half_width+0]*delta2;
	out [1] += in[1]*delta + in[half_width+1]*delta2;
	out [2] += in[2]*delta + in[half_width+2]*delta2;
	out [3] += in[3]*delta + in[half_width+3]*delta2;
	out [4] += in[4]*delta + in[half_width+4]*delta2;
	out [5] += in[5]*delta + in[half_width+5]*delta2;
	out [6] += in[6]*delta + in[half_width+6]*delta2;
	out [7] += in[7]*delta + in[half_width+7]*delta2;
	
	in = rev;
	out [ 8] += in[7]*delta + in[7-half_width]*delta2;
	out [ 9] += in[6]*delta + in[6-half_width]*delta2;
	out [10] += in[5]*delta + in[5-half_width]*delta2;
	out [11] += in[4]*delta + in[4-half_width]*delta2;
	out [12] += in[3]*delta + in[3-half_width]*delta2;
	out [13] += in[2]*delta + in[2-half_width]*delta2;
	out [14] += in[1]*delta + in[1-half_width]*delta2;
	out [15] += in[0]*delta + in[0-half_width]*delta2;
}

void hvl_blip_add_delta_fast( hvl_blip_t* m, unsigned time, int delta )
{
	unsigned fixed = (unsigned) ((time * m->factor + m->offset) >> pre_shift);
	buf_t* out = SAMPLES( m ) + m->avail + (fixed >> frac_bits);
	
	int interp = fixed >> (frac_bits - delta_bits) & (delta_unit - 1);
	int delta2 = delta * interp;
	
	/* Fails if buffer size was exceeded */
	assert( out <= &SAMPLES( m ) [m->size + end_frame_extra] );
	
	out [7] += delta * delta_unit - delta2;
	out [8] += delta2;
}
