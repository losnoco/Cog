#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "internal/fir_resampler.h"

enum { fir_width = 16 };

enum { fir_max_res = 1024 };
enum { fir_min_width = (fir_width < 4 ? 4 : fir_width) };
enum { fir_adj_width = fir_min_width / 4 * 4 + 2 };
enum { fir_stereo = 1 }; /* channel count, not boolean value */
enum { fir_write_offset = fir_adj_width * fir_stereo };

enum { fir_buffer_size = fir_width * 2 };

typedef short fir_impulse[fir_adj_width];

/* exp slope to 31/32 of ln(8) */
static const double fir_ratios[32] = {
	1.000, 1.067, 1.139, 1.215, 1.297, 1.384, 1.477, 1.576,
	1.682, 1.795, 1.915, 2.044, 2.181, 2.327, 2.484, 2.650,
	2.828, 3.018, 3.221, 3.437, 3.668, 3.914, 4.177, 4.458,
	4.757, 5.076, 5.417, 5.781, 6.169, 6.583, 7.025, 7.497
};

static fir_impulse fir_impulses[32][fir_max_res];

#undef PI
#define PI 3.1415926535897932384626433832795029

static void gen_sinc( double rolloff, int width, double offset, double spacing, double scale,
		int count, short* out )
{
	double const maxh = 256;
	double const step = PI / maxh * spacing;
	double const to_w = maxh * 2 / width;
	double const pow_a_n = pow( rolloff, maxh );
	
	double angle = (count / 2 - 1 + offset) * -step;

	scale /= maxh * 2;

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
			
			out [-1] = (short) (cos( w ) * sinc + sinc);
		}
		angle += step;
	}
}

typedef struct fir_resampler
{
    int write_pos, write_filled;
    int read_pos, read_filled;
    unsigned short phase;
    unsigned int phase_inc;
	unsigned int ratio_set;
    int buffer_in[fir_buffer_size * 2];
    int buffer_out[fir_buffer_size];
} fir_resampler;

void * fir_resampler_create()
{
	fir_resampler * r = ( fir_resampler * ) malloc( sizeof(fir_resampler) );
    if ( !r ) return 0;

	r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    r->phase_inc = 0;
	r->ratio_set = 0;
    memset( r->buffer_in, 0, sizeof(r->buffer_in) );
    memset( r->buffer_out, 0, sizeof(r->buffer_out) );

	return r;
}

void fir_resampler_delete(void * _r)
{
	free( _r );
}

void * fir_resampler_dup(void * _r)
{
    fir_resampler * r_in = ( fir_resampler * ) _r;
    fir_resampler * r_out = ( fir_resampler * ) malloc( sizeof(fir_resampler) );
    if ( !r_out ) return 0;

    r_out->write_pos = r_in->write_pos;
    r_out->write_filled = r_in->write_filled;
    r_out->read_pos = r_in->read_pos;
    r_out->read_filled = r_in->read_filled;
    r_out->phase = r_in->phase;
    r_out->phase_inc = r_in->phase_inc;
	r_out->ratio_set = r_in->ratio_set;
    memcpy( r_out->buffer_in, r_in->buffer_in, sizeof(r_in->buffer_in) );
    memcpy( r_out->buffer_out, r_in->buffer_out, sizeof(r_in->buffer_out) );

    return r_out;
}

int fir_resampler_get_free_count(void *_r)
{
	fir_resampler * r = ( fir_resampler * ) _r;
    return fir_buffer_size - r->write_filled;
}

int fir_resampler_ready(void *_r)
{
    fir_resampler * r = ( fir_resampler * ) _r;
    return r->write_filled > fir_adj_width;
}

void fir_resampler_clear(void *_r)
{
    fir_resampler * r = ( fir_resampler * ) _r;
    r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    memset( r->buffer_in, 0, sizeof(r->buffer_in) );
}

void fir_resampler_set_rate(void *_r, double new_factor)
{
    fir_resampler * r = ( fir_resampler * ) _r;
    r->phase_inc = (int)( new_factor * 65536.0 );
	r->ratio_set = 0;
	while ( r->ratio_set < 31 && new_factor > fir_ratios[ r->ratio_set ] ) r->ratio_set++;
}

void fir_resampler_write_sample(void *_r, short s)
{
	fir_resampler * r = ( fir_resampler * ) _r;

    if ( r->write_filled < fir_buffer_size )
	{
        int s32 = s;

        r->buffer_in[ r->write_pos ] = s32;
        r->buffer_in[ r->write_pos + fir_buffer_size ] = s32;

        ++r->write_filled;

		r->write_pos = ( r->write_pos + 1 ) % fir_buffer_size;
	}
}

void fir_init()
{
	double const rolloff = 0.999;
    double const gain = 1.0;

    int const res = fir_max_res;

	int i;

	for (i = 0; i < 32; i++)
	{
	    double const ratio_ = fir_ratios[ i ];
	
		double fraction = 1.0 / (double)fir_max_res;

		double const filter = (ratio_ < 1.0) ? 1.0 : 1.0 / ratio_;
		double pos = 0.0;
		short* out = (short*) fir_impulses[ i ];
	    int n;
		for ( n = res; --n >= 0; )
		{
			gen_sinc( rolloff, (int) (fir_adj_width * filter + 1) & ~1, pos, filter,
				    (double) (0x7FFF * gain * filter), (int) fir_adj_width, out );
			out += fir_adj_width;

			pos += fraction;
		}
	}
}

int fir_resampler_run(void *_r, int ** out_, int * out_end)
{
	fir_resampler * r = ( fir_resampler * ) _r;
    int in_size = r->write_filled;
    int const* in_ = r->buffer_in + fir_buffer_size + r->write_pos - r->write_filled;
	int used = 0;
	in_size -= fir_write_offset;
	if ( in_size > 0 )
	{
		int* out = *out_;
		int const* in = in_;
		int const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;
		int ratio_set = r->ratio_set;
		
		do
		{
			// accumulate in extended precision
            short const* imp = fir_impulses[ratio_set][(phase & 0xFFC0) >> 6];
            int pt = imp [0];
            int s = pt * in [0];
            int n;
			if ( out >= out_end )
				break;
            for ( n = (fir_adj_width - 2) / 2; n; --n )
			{
				pt = imp [1];
				s += pt * in [1];
				
				// pre-increment more efficient on some RISC processors
				imp += 2;
				pt = imp [0];
				in += 2;
				s += pt * in [0];
			}
			pt = imp [1];
			s += pt * in [1];

            phase += phase_inc;

            in += (phase >> 16) - fir_adj_width + 2;

            phase &= 65535;
			
            *out++ = (int) (s >> 7);
		}
		while ( in < in_end );
		
        r->phase = phase;
		*out_ = out;

		used = in - in_;

        r->write_filled -= used;
	}
	
	return used;
}

int fir_resampler_get_sample(void *_r)
{
    fir_resampler * r = ( fir_resampler * ) _r;
    if ( r->read_filled < 1 )
    {
        int write_pos = ( r->read_pos + r->read_filled ) % fir_buffer_size;
        int write_size = fir_buffer_size - write_pos;
        int * out = r->buffer_out + write_pos;
        if ( write_size > ( fir_buffer_size - r->read_filled ) )
            write_size = fir_buffer_size - r->read_filled;
        fir_resampler_run( r, &out, out + write_size );
        r->read_filled += out - r->buffer_out - write_pos;
    }
    if ( r->read_filled < 1 )
        return 0;
    return r->buffer_out[ r->read_pos ];
}

void fir_resampler_remove_sample(void *_r)
{
    fir_resampler * r = ( fir_resampler * ) _r;
    if ( r->read_filled > 0 )
    {
        --r->read_filled;
        r->read_pos = ( r->read_pos + 1 ) % fir_buffer_size;
    }
}
