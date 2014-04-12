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

void ptm_blip_add_delta( blip_t* m, float time, int delta )
{
    unsigned fixed = (unsigned) ((int)(time * time_unit + m->offset) >> pre_shift);
    buf_t* out = SAMPLES( m ) + (m->index + (fixed >> frac_bits)) % 128;
    buf_t* end = SAMPLES( m ) + 128;
	
	int const phase_shift = frac_bits - phase_bits;
	int phase = fixed >> phase_shift & (phase_count - 1);
	int const* in  = bl_step [phase];
	int const* rev = bl_step [phase_count - phase];
	
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
