#include "resampler.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Copyright (C) 2004-2008 Shay Green.
   Copyright (C) 2015 Christopher Snowhill. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#undef PI
#define PI 3.1415926535897932384626433832795029

enum { imp_scale = 0x7FFF };
typedef int16_t imp_t;
typedef int32_t imp_off_t; /* for max_res of 512 and impulse width of 32, end offsets must be 32 bits */

#if RESAMPLER_BITS == 16
typedef int32_t intermediate_t;
#elif RESAMPLER_BITS == 32
typedef int64_t intermediate_t;
#endif

static void gen_sinc( double rolloff, int width, double offset, double spacing, double scale,
		int count, imp_t* out )
{
	double angle;

	double const maxh = 256;
	double const step = PI / maxh * spacing;
	double const to_w = maxh * 2 / width;
	double const pow_a_n = pow( rolloff, maxh );
	scale /= maxh * 2;
	angle = (count / 2 - 1 + offset) * -step;

	while ( count-- )
	{
		double w;
		*out++ = 0;
		w = angle * to_w;
		if ( fabs( w ) < PI )
		{
			double rolloff_cos_a = rolloff * cos( angle );
			double num = 1 - rolloff_cos_a -
					pow_a_n * cos( maxh * angle ) +
					pow_a_n * rolloff * cos( (maxh - 1) * angle );
			double den = 1 - rolloff_cos_a - rolloff_cos_a + rolloff * rolloff;
			double sinc = scale * num / den - scale;

			out [-1] = (imp_t) (cos( w ) * sinc + sinc);
		}
		angle += step;
	}
}

enum { width = 32 };
enum { max_res = 512 };
enum { min_width = (width < 4 ? 4 : width) };
enum { adj_width = min_width / 4 * 4 + 2 };
enum { write_offset = adj_width };

enum { buffer_size = 128 };

typedef struct _resampler
{
	int width_;
	int rate_;
	int inptr;
	int infilled;
	int outptr;
	int outfilled;

	int latency;

	imp_t const* imp;
	imp_t impulses [max_res * (adj_width + 2 * (sizeof(imp_off_t) / sizeof(imp_t)))];
	sample_t buffer_in[buffer_size * 2];
	sample_t buffer_out[buffer_size];
} resampler;

void * resampler_create()
{
	resampler *r = (resampler *) malloc(sizeof(resampler));
	if (r) resampler_clear(r);
	return r;
}

void * resampler_dup(const void *_r)
{
    void *_t = (resampler *) malloc(sizeof(resampler));
    if (_t) resampler_dup_inplace(_t, _r);
    return _t;
}

void resampler_dup_inplace(void *_t, const void *_r)
{
	const resampler *r = (const resampler *)_r;
	resampler *t = (resampler *)_t;
	if (r && t)
	{
		memcpy(t, r, sizeof(resampler));
		t->imp = t->impulses + (r->imp - r->impulses);
	}
	else if (t)
	{
		resampler_clear(t);
	}
}

void resampler_destroy(void *r)
{
	free(r);
}

void resampler_clear(void *_r)
{
	resampler * r = (resampler *)_r;
	r->width_ = adj_width;
	r->inptr = 0;
	r->infilled = 0;
	r->outptr = 0;
	r->outfilled = 0;
	r->latency = 0;
	r->imp = r->impulses;

	resampler_set_rate(r, 1.0);
}

void resampler_set_rate( void *_r, double new_factor )
{
	int step; //const
	double filter; //const
	double fraction, pos;
    int n;

	resampler *rs = (resampler *)_r;
    imp_t* out;

	double const rolloff = 0.999;
	double const gain = 1.0;

	/* determine number of sub-phases that yield lowest error */
	double ratio_ = 0.0;
	int res = -1;
	{
		double least_error = 2;
		double pos = 0;
		int r;
		for ( r = 1; r <= max_res; r++ )
		{
			double nearest, error;
			pos += new_factor;
			nearest = floor( pos + 0.5 );
			error = fabs( pos - nearest );
			if ( error < least_error )
			{
				res = r;
				ratio_ = nearest / res;
				least_error = error;
			}
		}
	}
	rs->rate_ = ratio_;

	/* how much of input is used for each output sample */
	step = (int) floor( ratio_ );
	fraction = fmod( ratio_, 1.0 );

	filter = (ratio_ < 1.0) ? 1.0 : 1.0 / ratio_;
	pos = 0.0;
	/*int input_per_cycle = 0;*/
	out = rs->impulses;
	
	for ( n = res; --n >= 0; )
	{
        int cur_step;
        
		gen_sinc( rolloff, (int) (rs->width_ * filter + 1) & ~1, pos, filter,
				(double)(imp_scale * gain * filter), (int) rs->width_, out );
		out += rs->width_;

		cur_step = step;
		pos += fraction;
		if ( pos >= 0.9999999 )
		{
			pos -= 1.0;
			cur_step += 1;
		}

		((imp_off_t*)out)[0] = (cur_step - rs->width_ + 2) * sizeof (sample_t);
		((imp_off_t*)out)[1] = 2 * sizeof (imp_t) + 2 * sizeof (imp_off_t);
		out += 2 * (sizeof(imp_off_t) / sizeof(imp_t));
		/*input_per_cycle += cur_step;*/
	}
	/* last offset moves back to beginning of impulses*/
	((imp_off_t*)out) [-1] -= (char*) out - (char*) rs->impulses;

	rs->imp = rs->impulses;
}

int resampler_get_free(void *_r)
{
	resampler *r = (resampler *)_r;
	return buffer_size - r->infilled;
}

int resampler_get_min_fill(void *_r)
{
	resampler *r = (resampler *)_r;
	const int min_needed = write_offset + 1;
	const int latency = r->latency ? 0 : adj_width;
	int min_free = min_needed - r->infilled - latency;
	return min_free < 0 ? 0 : min_free;
}

void resampler_write_sample(void *_r, sample_t s)
{
	resampler *r = (resampler *)_r;

	if (!r->latency)
	{
	    int i;
		for ( i = 0; i < adj_width / 2; ++i)
		{
			r->buffer_in[r->inptr] = 0;
			r->buffer_in[buffer_size + r->inptr] = 0;
			r->inptr = (r->inptr + 1) % (buffer_size);
			r->infilled += 1;
		}
		r->latency = 1;
	}

	if (r->infilled < buffer_size)
	{
		r->buffer_in[r->inptr] = s;
		r->buffer_in[buffer_size + r->inptr + 0] = s;
		r->inptr = (r->inptr + 1) % (buffer_size);
		r->infilled += 1;
	}
}

#if defined(_MSC_VER) || defined(__GNUC__)
#define restrict __restrict
#endif

static const sample_t * resampler_inner_loop( resampler *r, sample_t** out_,
		sample_t const* out_end, sample_t const in [], int in_size )
{
	in_size -= write_offset;
	if ( in_size > 0 )
	{
		sample_t* restrict out = *out_;
		sample_t const* const in_end = in + in_size;
		imp_t const* imp = r->imp;

		do
		{
			int n;
			/* accumulate in extended precision*/
			int pt = imp [0];
			intermediate_t s = (intermediate_t)pt * (intermediate_t)(in [0]);
			if ( out >= out_end )
				break;
			for ( n = (adj_width - 2) / 2; n; --n )
			{
				pt = imp [1];
				s += (intermediate_t)pt * (intermediate_t)(in [1]);

				/* pre-increment more efficient on some RISC processors*/
				imp += 2;
				pt = imp [0];
				s += (intermediate_t)pt * (intermediate_t)(in [2]);
				in += 2;
			}
			pt = imp [1];
			s += (intermediate_t)pt * (intermediate_t)(in [1]);

			/* these two "samples" after the end of the impulse give the
			 * proper offsets to the next input sample and next impulse */
			in  = (sample_t const*) ((char const*) in  + ((imp_off_t*)(&imp [2]))[0]); /* some negative value */
			imp = (imp_t const*) ((char const*) imp + ((imp_off_t*)(&imp [2]))[1]); /* small positive or large negative */

			out [0] = (sample_t) (s >> 15);
			out += 1;
		}
		while ( in < in_end );

		r->imp = imp;
		*out_ = out;
	}
	return in;
}

#undef restrict

static int resampler_wrapper( resampler *r, sample_t out [], int* out_size,
		sample_t const in [], int in_size )
{
	sample_t* out_ = out;
	long result = resampler_inner_loop( r, &out_, out + *out_size, in, in_size ) - in;

	*out_size = (int)(out_ - out);
	return (int) result;
}

static void resampler_fill( resampler *r )
{
	while (!r->outfilled && r->infilled)
	{
		int inread;

		int writepos = ( r->outptr + r->outfilled ) % (buffer_size);
		int writesize = (buffer_size) - writepos;
		if ( writesize > ( buffer_size - r->outfilled ) )
				writesize = buffer_size - r->outfilled;
		inread = resampler_wrapper(r, &r->buffer_out[writepos], &writesize, &r->buffer_in[buffer_size + r->inptr - r->infilled], r->infilled);
		r->infilled -= inread;
		r->outfilled += writesize;
		if (!inread)
			break;
	}
}

int resampler_get_avail(void *_r)
{
	resampler *r = (resampler *)_r;
	if (r->outfilled < 1 && r->infilled >= r->width_)
	  resampler_fill( r );
	return r->outfilled;
}

static void resampler_read_sample_internal( resampler *r, sample_t *s, int advance )
{
	if (r->outfilled < 1)
	  resampler_fill( r );
	if (r->outfilled < 1)
	{
		*s = 0;
		return;
	}
	*s = r->buffer_out[r->outptr];
	if (advance)
	{
		r->outptr = (r->outptr + 1) % (buffer_size);
		r->outfilled -= 1;
	}
}

void resampler_read_sample( void *_r, sample_t *s )
{
	resampler *r = (resampler *)_r;
	resampler_read_sample_internal(r, s, 1);
}

void resampler_peek_sample( void *_r, sample_t *s )
{
	resampler *r = (resampler *)_r;
	resampler_read_sample_internal(r, s, 0);
}
