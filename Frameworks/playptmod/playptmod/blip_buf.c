/* blip_buf 1.1.0. http://www.slack.net/~ant/ */

#include "blip_buf.h"

#include <assert.h>
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

/* probably not totally portable */
#define SAMPLES( buf ) (buf->samples)

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

void ptm_blip_clear( blip_t* m )
{
	/* We could set offset to 0, factor/2, or factor-1. 0 is suitable if
	factor is rounded up. factor-1 is suitable if factor is rounded down.
	Since we don't know rounding direction, factor/2 accommodates either,
	with the slight loss of showing an error in half the time. Since for
	a 64-bit factor this is years, the halving isn't a problem. */
	
    m->offset     = time_unit / 2;
    m->index      = 0;
	m->integrator = 0;
    m->last_value = 0;
    memset( SAMPLES( m ), 0, 128 * sizeof (buf_t) );
}

int ptm_blip_read_sample( blip_t* m )
{
    int retval;

	{
        buf_t * in  = SAMPLES( m ) + m->index;
		int sum = m->integrator;
		{
			/* Eliminate fraction */
            int s = ARITH_SHIFT( sum, delta_bits );
			
            sum += *in;
			
            retval = s;
		}
		m->integrator = sum;

        *in = 0;

        m->index = ( m->index + 1 ) % 128;
	}
	
    return retval;
}

/* Things that didn't help performance on x86:
	__attribute__((aligned(128)))
	#define short int
	restrict
*/

/* Sinc_Generator( 0.9, 0.55, 4.5 ) */
static short const bl_step [phase_count + 1] [half_width] =
{
{   43, -115,  350, -488, 1136, -914, 5861,21022},
{   44, -118,  348, -473, 1076, -799, 5274,21001},
{   45, -121,  344, -454, 1011, -677, 4706,20936},
{   46, -122,  336, -431,  942, -549, 4156,20829},
{   47, -123,  327, -404,  868, -418, 3629,20679},
{   47, -122,  316, -375,  792, -285, 3124,20488},
{   47, -120,  303, -344,  714, -151, 2644,20256},
{   46, -117,  289, -310,  634,  -17, 2188,19985},
{   46, -114,  273, -275,  553,  117, 1758,19675},
{   44, -108,  255, -237,  471,  247, 1356,19327},
{   43, -103,  237, -199,  390,  373,  981,18944},
{   42,  -98,  218, -160,  310,  495,  633,18527},
{   40,  -91,  198, -121,  231,  611,  314,18078},
{   38,  -84,  178,  -81,  153,  722,   22,17599},
{   36,  -76,  157,  -43,   80,  824, -241,17092},
{   34,  -68,  135,   -3,    8,  919, -476,16558},
{   32,  -61,  115,   34,  -60, 1006, -683,16001},
{   29,  -52,   94,   70, -123, 1083, -862,15422},
{   27,  -44,   73,  106, -184, 1152,-1015,14824},
{   25,  -36,   53,  139, -239, 1211,-1142,14210},
{   22,  -27,   34,  170, -290, 1261,-1244,13582},
{   20,  -20,   16,  199, -335, 1301,-1322,12942},
{   18,  -12,   -3,  226, -375, 1331,-1376,12293},
{   15,   -4,  -19,  250, -410, 1351,-1408,11638},
{   13,    3,  -35,  272, -439, 1361,-1419,10979},
{   11,    9,  -49,  292, -464, 1362,-1410,10319},
{    9,   16,  -63,  309, -483, 1354,-1383, 9660},
{    7,   22,  -75,  322, -496, 1337,-1339, 9005},
{    6,   26,  -85,  333, -504, 1312,-1280, 8355},
{    4,   31,  -94,  341, -507, 1278,-1205, 7713},
{    3,   35, -102,  347, -506, 1238,-1119, 7082},
{    1,   40, -110,  350, -499, 1190,-1021, 6464},
{    0,   43, -115,  350, -488, 1136, -914, 5861}
};

/* Shifting by pre_shift allows calculation using unsigned int rather than
possibly-wider fixed_t. On 32-bit platforms, this is likely more efficient.
And by having pre_shift 32, a 32-bit platform can easily do the shift by
simply ignoring the low half. */

void ptm_blip_add_delta( blip_t* m, float time, int delta )
{
    unsigned fixed = (unsigned) ((int)(time * time_unit + m->offset) >> pre_shift);
    buf_t* out = SAMPLES( m ) + (m->index + (fixed >> frac_bits)) % 128;
    buf_t* end = SAMPLES( m ) + 128;
	
	int const phase_shift = frac_bits - phase_bits;
	int phase = fixed >> phase_shift & (phase_count - 1);
	short const* in  = bl_step [phase];
	short const* rev = bl_step [phase_count - phase];
	
	int interp = fixed >> (phase_shift - delta_bits) & (delta_unit - 1);
    int delta2 = (delta * interp) >> delta_bits;
	delta -= delta2;
	
    *out++ += in[0]*delta + in[half_width+0]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[1]*delta + in[half_width+1]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[2]*delta + in[half_width+2]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[3]*delta + in[half_width+3]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[4]*delta + in[half_width+4]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[5]*delta + in[half_width+5]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[6]*delta + in[half_width+6]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[7]*delta + in[half_width+7]*delta2;
    if (out >= end) out = SAMPLES( m );

	in = rev;
    *out++ += in[7]*delta + in[7-half_width]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[6]*delta + in[6-half_width]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[5]*delta + in[5-half_width]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[4]*delta + in[4-half_width]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[3]*delta + in[3-half_width]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[2]*delta + in[2-half_width]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[1]*delta + in[1-half_width]*delta2;
    if (out >= end) out = SAMPLES( m );
    *out++ += in[0]*delta + in[0-half_width]*delta2;
}
