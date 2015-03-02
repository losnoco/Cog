#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "resampler.h"

#include "rsp_hle/audio.h"

enum { RESAMPLER_SHIFT = 16 };
enum { RESAMPLER_RESOLUTION = 1 << RESAMPLER_SHIFT };

enum { resampler_buffer_size = 64 * 4 };

typedef struct resampler
{
    int write_pos, write_filled;
    int read_pos, read_filled;
    int phase;
    int phase_inc;
    signed char delay_added;
    signed char delay_removed;
    short buffer_in[2][resampler_buffer_size * 2];
    short buffer_out[resampler_buffer_size * 2];
} resampler;

void * resampler_create(void)
{
    resampler * r = ( resampler * ) malloc( sizeof(resampler) );
    if ( !r ) return 0;

    r->write_pos = 1;
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
    return 4;
}

static int resampler_input_delay(resampler *r)
{
    return 1;
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
    r->write_pos = 1;
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
    r->phase_inc = new_factor * RESAMPLER_RESOLUTION;
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
        r->buffer_in[ 0 ][ r->write_pos ] = ls;
        r->buffer_in[ 0 ][ r->write_pos + resampler_buffer_size ] = ls;

        r->buffer_in[ 1 ][ r->write_pos ] = rs;
        r->buffer_in[ 1 ][ r->write_pos + resampler_buffer_size ] = rs;
        
        ++r->write_filled;

        r->write_pos = ( r->write_pos + 1 ) % resampler_buffer_size;
    }
}

static int resampler_run_cubic(resampler * r, short ** out_, short * out_end)
{
    int in_size = r->write_filled;
    int in_offset = resampler_buffer_size + r->write_pos - r->write_filled;
    short const* inl_ = r->buffer_in[0] + in_offset;
    short const* inr_ = r->buffer_in[1] + in_offset;
    int used = 0;
    in_size -= 4;
    if ( in_size > 0 )
    {
        short* out = *out_;
        short const* inl = inl_;
        short const* inr = inr_;
        short const* const in_end = inl + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;

        do
        {
            int samplel, sampler;
            
            if ( out >= out_end )
                break;

            const int16_t* lut = RESAMPLE_LUT + ((phase & 0xfc00) >> 8);
            
            samplel = ((inl[0] * lut[0]) >> 15) + ((inl[1] * lut[1]) >> 15)
                    + ((inl[2] * lut[2]) >> 15) + ((inl[3] * lut[3]) >> 15);
            sampler = ((inr[0] * lut[0]) >> 15) + ((inr[1] * lut[1]) >> 15)
                    + ((inr[2] * lut[2]) >> 15) + ((inr[3] * lut[3]) >> 15);
            
            if ((samplel + 0x8000) & 0xffff0000) samplel = 0x7fff ^ (samplel >> 31);
            if ((sampler + 0x8000) & 0xffff0000) sampler = 0x7fff ^ (sampler >> 31);
            
            *out++ = (short)samplel;
            *out++ = (short)sampler;

            phase += phase_inc;

            inl += (phase >> 16);
            inr += (phase >> 16);

            phase &= 0xFFFF;
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
        short * out = r->buffer_out + write_pos * 2;
        if ( write_size > ( resampler_buffer_size - r->read_filled ) )
            write_size = resampler_buffer_size - r->read_filled;
        resampler_run_cubic( r, &out, out + write_size * 2 );
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
    if ( r->read_filled < 1 && r->phase_inc )
        resampler_fill_and_remove_delay( r );
    if ( r->read_filled < 1 )
    {
        *ls = 0;
        *rs = 0;
    }
    else
    {
        *ls = r->buffer_out[ r->read_pos * 2 + 0 ];
        *rs = r->buffer_out[ r->read_pos * 2 + 1 ];
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
