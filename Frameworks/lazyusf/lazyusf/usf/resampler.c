#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "resampler.h"

enum { RESAMPLER_SHIFT = 10 };
enum { RESAMPLER_RESOLUTION = 1 << RESAMPLER_SHIFT };
enum { SINC_WIDTH = 16 };
enum { SINC_SAMPLES = RESAMPLER_RESOLUTION * SINC_WIDTH };

static const float RESAMPLER_BLEP_CUTOFF = 0.90f;
static const float RESAMPLER_BLAM_CUTOFF = 0.93f;
static const float RESAMPLER_SINC_CUTOFF = 0.999f;

static float sinc_lut[SINC_SAMPLES + 1];
static float window_lut[SINC_SAMPLES + 1];

enum { resampler_buffer_size = SINC_WIDTH * 4 };

static int fEqual(const float b, const float a)
{
    return fabs(a - b) < 1.0e-6;
}

static float sinc(float x)
{
    return fEqual(x, 0.0) ? 1.0 : sin(x * M_PI) / (x * M_PI);
}

void resampler_init(void)
{
    unsigned i;
    double dx = (float)(SINC_WIDTH) / SINC_SAMPLES, x = 0.0;
    for (i = 0; i < SINC_SAMPLES + 1; ++i, x += dx)
    {
        float y = x / SINC_WIDTH;
#if 0
        // Blackman
        float window = 0.42659 - 0.49656 * cos(M_PI + M_PI * y) + 0.076849 * cos(2.0 * M_PI * y);
#elif 1
        // Nuttal 3 term
        float window = 0.40897 + 0.5 * cos(M_PI * y) + 0.09103 * cos(2.0 * M_PI * y);
#elif 0
        // C.R.Helmrich's 2 term window
        float window = 0.79445 * cos(0.5 * M_PI * y) + 0.20555 * cos(1.5 * M_PI * y);
#elif 0
        // Lanczos
        float window = sinc(y);
#endif
        sinc_lut[i] = fabs(x) < SINC_WIDTH ? sinc(x) : 0.0;
        window_lut[i] = window;
    }
}

typedef struct resampler
{
    int write_pos, write_filled;
    int read_pos, read_filled;
    float phase;
    float phase_inc;
    signed char delay_added;
    signed char delay_removed;
    float buffer_in[2][resampler_buffer_size * 2];
    float buffer_out[resampler_buffer_size * 2];
} resampler;

void * resampler_create(void)
{
    resampler * r = ( resampler * ) malloc( sizeof(resampler) );
    if ( !r ) return 0;

    r->write_pos = SINC_WIDTH - 1;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    r->phase_inc = 0;
    r->delay_added = -1;
    r->delay_removed = -1;
    memset( r->buffer_in, 0, sizeof(r->buffer_in) );
    memset( r->buffer_out, 0, sizeof(r->buffer_out) );

    return r;
}

void resampler_delete(void * _r)
{
    free( _r );
}

void * resampler_dup(const void * _r)
{
    void * r_out = malloc( sizeof(resampler) );
    if ( !r_out ) return 0;

    resampler_dup_inplace(r_out, _r);

    return r_out;
}

void resampler_dup_inplace(void *_d, const void *_s)
{
    const resampler * r_in = ( const resampler * ) _s;
    resampler * r_out = ( resampler * ) _d;

    r_out->write_pos = r_in->write_pos;
    r_out->write_filled = r_in->write_filled;
    r_out->read_pos = r_in->read_pos;
    r_out->read_filled = r_in->read_filled;
    r_out->phase = r_in->phase;
    r_out->phase_inc = r_in->phase_inc;
    r_out->delay_added = r_in->delay_added;
    r_out->delay_removed = r_in->delay_removed;
    memcpy( r_out->buffer_in, r_in->buffer_in, sizeof(r_in->buffer_in) );
    memcpy( r_out->buffer_out, r_in->buffer_out, sizeof(r_in->buffer_out) );
}

int resampler_get_free_count(void *_r)
{
    resampler * r = ( resampler * ) _r;
    return resampler_buffer_size - r->write_filled;
}

static int resampler_min_filled(resampler *r)
{
    return SINC_WIDTH * 2;
}

static int resampler_input_delay(resampler *r)
{
    return SINC_WIDTH - 1;
}

static int resampler_output_delay(resampler *r)
{
    return 0;
}

int resampler_ready(void *_r)
{
    resampler * r = ( resampler * ) _r;
    return r->write_filled > resampler_min_filled(r);
}

void resampler_clear(void *_r)
{
    resampler * r = ( resampler * ) _r;
    r->write_pos = SINC_WIDTH - 1;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    r->delay_added = -1;
    r->delay_removed = -1;
}

void resampler_set_rate(void *_r, double new_factor)
{
    resampler * r = ( resampler * ) _r;
    r->phase_inc = new_factor;
}

void resampler_write_sample(void *_r, short ls, short rs)
{
    resampler * r = ( resampler * ) _r;

    if ( r->delay_added < 0 )
    {
        r->delay_added = 0;
        r->write_filled = resampler_input_delay( r );
    }
    
    if ( r->write_filled < resampler_buffer_size )
    {
        float s32 = ls;

        r->buffer_in[ 0 ][ r->write_pos ] = s32;
        r->buffer_in[ 0 ][ r->write_pos + resampler_buffer_size ] = s32;

        s32 = rs;
        
        r->buffer_in[ 1 ][ r->write_pos ] = s32;
        r->buffer_in[ 1 ][ r->write_pos + resampler_buffer_size ] = s32;
        
        ++r->write_filled;

        r->write_pos = ( r->write_pos + 1 ) % resampler_buffer_size;
    }
}

static int resampler_run_sinc(resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    int in_offset = resampler_buffer_size + r->write_pos - r->write_filled;
    float const* inl_ = r->buffer_in[0] + in_offset;
    float const* inr_ = r->buffer_in[1] + in_offset;
    int used = 0;
    in_size -= SINC_WIDTH * 2;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* inl = inl_;
        float const* inr = inr_;
        float const* const in_end = inl + in_size;
        float phase = r->phase;
        float phase_inc = r->phase_inc;

        int step = phase_inc > 1.0f ? (int)(RESAMPLER_RESOLUTION / phase_inc * RESAMPLER_SINC_CUTOFF) : (int)(RESAMPLER_RESOLUTION * RESAMPLER_SINC_CUTOFF);
        int window_step = RESAMPLER_RESOLUTION;

        do
        {
            float kernel[SINC_WIDTH * 2], kernel_sum = 0.0;
            int i = SINC_WIDTH;
            int phase_reduced = (int)(phase * RESAMPLER_RESOLUTION);
            int phase_adj = phase_reduced * step / RESAMPLER_RESOLUTION;
            float samplel, sampler;

            if ( out >= out_end )
                break;

            for (; i >= -SINC_WIDTH + 1; --i)
            {
                int pos = i * step;
                int window_pos = i * window_step;
                kernel_sum += kernel[i + SINC_WIDTH - 1] = sinc_lut[abs(phase_adj - pos)] * window_lut[abs(phase_reduced - window_pos)];
            }
            for (samplel = 0, sampler = 0, i = 0; i < SINC_WIDTH * 2; ++i)
            {
                samplel += inl[i] * kernel[i];
                sampler += inr[i] * kernel[i];
            }
            kernel_sum = 1.0f / kernel_sum;
            *out++ = (float)(samplel * kernel_sum);
            *out++ = (float)(sampler * kernel_sum);

            phase += phase_inc;

            inl += (int)phase;
            inr += (int)phase;

            phase = fmod(phase, 1.0f);
        }
        while ( inl < in_end );

        r->phase = phase;
        *out_ = out;

        used = (int)(inl - inl_);

        r->write_filled -= used;
    }

    return used;
}

static void resampler_fill(resampler * r)
{
    int min_filled = resampler_min_filled(r);
    while ( r->write_filled > min_filled &&
            r->read_filled < resampler_buffer_size )
    {
        int write_pos = ( r->read_pos + r->read_filled ) % resampler_buffer_size;
        int write_size = resampler_buffer_size - write_pos;
        float * out = r->buffer_out + write_pos * 2;
        if ( write_size > ( resampler_buffer_size - r->read_filled ) )
            write_size = resampler_buffer_size - r->read_filled;
        resampler_run_sinc( r, &out, out + write_size * 2 );
        r->read_filled += ( out - r->buffer_out - write_pos * 2 ) / 2;
    }
}

static void resampler_fill_and_remove_delay(resampler * r)
{
    resampler_fill( r );
    if ( r->delay_removed < 0 )
    {
        int delay = resampler_output_delay( r );
        r->delay_removed = 0;
        while ( delay-- )
            resampler_remove_sample( r );
    }
}

int resampler_get_sample_count(void *_r)
{
    resampler * r = ( resampler * ) _r;
    if ( r->read_filled < 1 )
        resampler_fill_and_remove_delay( r );
    return r->read_filled;
}

void resampler_get_sample(void *_r, short * ls, short * rs)
{
    resampler * r = ( resampler * ) _r;
    if ( r->read_filled < 1 && r->phase_inc)
        resampler_fill_and_remove_delay( r );
    if ( r->read_filled < 1 )
    {
        *ls = 0;
        *rs = 0;
    }
    else
    {
        int samplel = (int)r->buffer_out[ r->read_pos * 2 + 0 ];
        int sampler = (int)r->buffer_out[ r->read_pos * 2 + 1 ];
        if ( ( samplel + 0x8000 ) & 0xffff0000 ) samplel = ( samplel >> 31 ) ^ 0x7fff;
        if ( ( sampler + 0x8000 ) & 0xffff0000 ) sampler = ( sampler >> 31 ) ^ 0x7fff;
        *ls = (short)samplel;
        *rs = (short)sampler;
    }
}

void resampler_remove_sample(void *_r)
{
    resampler * r = ( resampler * ) _r;
    if ( r->read_filled > 0 )
    {
        --r->read_filled;
        r->read_pos = ( r->read_pos + 1 ) % resampler_buffer_size;
    }
}
