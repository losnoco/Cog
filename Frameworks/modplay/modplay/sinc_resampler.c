#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#if (defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__amd64__))
#include <xmmintrin.h>
#define SINC_SSE
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "sinc_resampler.h"

enum { SINC_RESOLUTION = 8192 };
enum { SINC_WIDTH = 16 };
enum { SINC_SAMPLES = SINC_RESOLUTION * SINC_WIDTH };

static float sinc_lut[SINC_SAMPLES + 1];

enum { sinc_buffer_size = SINC_WIDTH * 4 };

static int fEqual(const float b, const float a)
{
    return fabs(a - b) < 1.0e-6;
}

static float sinc(float x)
{
    return fEqual(x, 0.0) ? 1.0 : sin(x * M_PI) / (x * M_PI);
}

#ifdef SINC_SSE
#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
static inline void
__cpuid(int *data, int selector)
{
    asm("cpuid"
        : "=a" (data[0]),
        "=b" (data[1]),
        "=c" (data[2]),
        "=d" (data[3])
        : "a"(selector));
}
#else
#define __cpuid(a,b) memset((a), 0, sizeof(int) * 4)
#endif

static int query_cpu_feature_sse() {
	int buffer[4];
	__cpuid(buffer,1);
	if ((buffer[3]&(1<<25)) == 0) return 0;
	return 1;
}

static int sinc_has_sse = 0;
#endif

void sinc_init(void)
{
    unsigned i;
    float dx = (float)(SINC_WIDTH) / SINC_SAMPLES, x = 0.0;
    for (i = 0; i < SINC_SAMPLES + 1; ++i, x += dx)
    {
        float y = x / SINC_WIDTH;
        float window = 0.42659 - 0.49656 * cos(M_PI + M_PI * y) + 0.076849 * cos(2.0 * M_PI * y);
        sinc_lut[i] = fabs(x) < SINC_WIDTH ? sinc(x) * window : 0.0;
    }
#ifdef SINC_SSE
    sinc_has_sse = query_cpu_feature_sse();
#endif
}

typedef struct sinc_resampler
{
    int write_pos, write_filled;
    int read_pos, read_filled;
    unsigned short phase;
    unsigned int phase_inc;
    float buffer_in[sinc_buffer_size * 2];
    float buffer_out[sinc_buffer_size];
} sinc_resampler;

void * sinc_resampler_create(void)
{
    sinc_resampler * r = ( sinc_resampler * ) malloc( sizeof(sinc_resampler) );
    if ( !r ) return 0;

    r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    r->phase_inc = 0;
    memset( r->buffer_in, 0, sizeof(r->buffer_in) );
    memset( r->buffer_out, 0, sizeof(r->buffer_out) );

    return r;
}

void sinc_resampler_delete(void * _r)
{
    free( _r );
}

void * sinc_resampler_dup(const void * _r)
{
    const sinc_resampler * r_in = ( const sinc_resampler * ) _r;
    sinc_resampler * r_out = ( sinc_resampler * ) malloc( sizeof(sinc_resampler) );
    if ( !r_out ) return 0;

    r_out->write_pos = r_in->write_pos;
    r_out->write_filled = r_in->write_filled;
    r_out->read_pos = r_in->read_pos;
    r_out->read_filled = r_in->read_filled;
    r_out->phase = r_in->phase;
    r_out->phase_inc = r_in->phase_inc;
    memcpy( r_out->buffer_in, r_in->buffer_in, sizeof(r_in->buffer_in) );
    memcpy( r_out->buffer_out, r_in->buffer_out, sizeof(r_in->buffer_out) );

    return r_out;
}

void sinc_resampler_dup_inplace(void *_d, const void *_s)
{
    const sinc_resampler * r_in = ( const sinc_resampler * ) _s;
    sinc_resampler * r_out = ( sinc_resampler * ) _d;

    r_out->write_pos = r_in->write_pos;
    r_out->write_filled = r_in->write_filled;
    r_out->read_pos = r_in->read_pos;
    r_out->read_filled = r_in->read_filled;
    r_out->phase = r_in->phase;
    r_out->phase_inc = r_in->phase_inc;
    memcpy( r_out->buffer_in, r_in->buffer_in, sizeof(r_in->buffer_in) );
    memcpy( r_out->buffer_out, r_in->buffer_out, sizeof(r_in->buffer_out) );
}

int sinc_resampler_get_free_count(void *_r)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;
    return sinc_buffer_size - r->write_filled;
}

int sinc_resampler_ready(void *_r)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;
    return r->write_filled > (SINC_WIDTH * 2);
}

void sinc_resampler_clear(void *_r)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;
    r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
}

void sinc_resampler_set_rate(void *_r, double new_factor)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;
    r->phase_inc = (int)( new_factor * SINC_RESOLUTION );
}

void sinc_resampler_write_sample(void *_r, short s)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;

    if ( r->write_filled < sinc_buffer_size )
    {
        float s32 = s;

        r->buffer_in[ r->write_pos ] = s32;
        r->buffer_in[ r->write_pos + sinc_buffer_size ] = s32;

        ++r->write_filled;

        r->write_pos = ( r->write_pos + 1 ) % sinc_buffer_size;
    }
}

static int sinc_resampler_run(sinc_resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + sinc_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= SINC_WIDTH * 2;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;

        int step = phase_inc > SINC_RESOLUTION ? SINC_RESOLUTION * SINC_RESOLUTION / phase_inc : SINC_RESOLUTION;

        do
        {
            // accumulate in extended precision
            float kernel[SINC_WIDTH * 2], kernel_sum = 0.0;
            int i = SINC_WIDTH;
            int phase_adj = phase * step / SINC_RESOLUTION;
            float sample;

            if ( out >= out_end )
                break;

            for (; i >= -SINC_WIDTH + 1; --i)
            {
                int pos = i * step;
                kernel_sum += kernel[i + SINC_WIDTH - 1] = sinc_lut[abs(phase_adj - pos)];
            }
            for (sample = 0, i = 0; i < SINC_WIDTH * 2; ++i)
                sample += in[i] * kernel[i];
            *out++ = (float)(sample / kernel_sum * (1.0 / 32768.0));

            phase += phase_inc;

            in += phase >> 13;

            phase &= 8191;
        }
        while ( in < in_end );

        r->phase = (unsigned short) phase;
        *out_ = out;

        used = (int)(in - in_);

        r->write_filled -= used;
    }

    return used;
}

#ifdef SINC_SSE
static int sinc_resampler_run_sse(sinc_resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + sinc_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= SINC_WIDTH * 2;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;
        
        int step = phase_inc > SINC_RESOLUTION ? SINC_RESOLUTION * SINC_RESOLUTION / phase_inc : SINC_RESOLUTION;
        
        do
        {
            // accumulate in extended precision
            float kernel_sum = 0.0;
            __m128 kernel[SINC_WIDTH / 2];
            __m128 temp1, temp2;
            __m128 samplex = _mm_setzero_ps();
            float *kernelf = (float*)(&kernel);
            int i = SINC_WIDTH;
            int phase_adj = phase * step / SINC_RESOLUTION;
            
            if ( out >= out_end )
                break;
            
            for (; i >= -SINC_WIDTH + 1; --i)
            {
                int pos = i * step;
                kernel_sum += kernelf[i + SINC_WIDTH - 1] = sinc_lut[abs(phase_adj - pos)];
            }
            for (i = 0; i < SINC_WIDTH / 2; ++i)
            {
                temp1 = _mm_loadu_ps( (const float *)( in + i * 4 ) );
                temp2 = _mm_load_ps( (const float *)( kernel + i ) );
                temp1 = _mm_mul_ps( temp1, temp2 );
                samplex = _mm_add_ps( samplex, temp1 );
            }
            kernel_sum = 1.0 / kernel_sum * (1.0 / 32768.0);
            temp1 = _mm_movehl_ps( temp1, samplex );
            samplex = _mm_add_ps( samplex, temp1 );
            temp1 = samplex;
            temp1 = _mm_shuffle_ps( temp1, samplex, _MM_SHUFFLE(0, 0, 0, 1) );
            samplex = _mm_add_ps( samplex, temp1 );
            temp1 = _mm_set_ss( kernel_sum );
            samplex = _mm_mul_ps( samplex, temp1 );
            _mm_store_ss( out, samplex );
            ++out;
            
            phase += phase_inc;
            
            in += phase >> 13;
            
            phase &= 8191;
        }
        while ( in < in_end );
        
        r->phase = (unsigned short) phase;
        *out_ = out;
        
        used = (int)(in - in_);
        
        r->write_filled -= used;
    }
    
    return used;
}
#endif

static void sinc_resampler_fill(sinc_resampler * r)
{
    while ( r->write_filled > (SINC_WIDTH * 2) &&
            r->read_filled < sinc_buffer_size )
    {
        int write_pos = ( r->read_pos + r->read_filled ) % sinc_buffer_size;
        int write_size = sinc_buffer_size - write_pos;
        float * out = r->buffer_out + write_pos;
        if ( write_size > ( sinc_buffer_size - r->read_filled ) )
            write_size = sinc_buffer_size - r->read_filled;
#ifdef SINC_SSE
        if ( sinc_has_sse )
            sinc_resampler_run_sse( r, &out, out + write_size );
        else
#endif
            sinc_resampler_run( r, &out, out + write_size );
        r->read_filled += out - r->buffer_out - write_pos;
    }
}

int sinc_resampler_get_sample_count(void *_r)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;
    if ( r->read_filled < 1 )
        sinc_resampler_fill( r );
    return r->read_filled;
}

float sinc_resampler_get_sample(void *_r)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;
    if ( r->read_filled < 1 )
        sinc_resampler_fill( r );
    if ( r->read_filled < 1 )
        return 0;
    return r->buffer_out[ r->read_pos ];
}

void sinc_resampler_remove_sample(void *_r)
{
    sinc_resampler * r = ( sinc_resampler * ) _r;
    if ( r->read_filled > 0 )
    {
        --r->read_filled;
        r->read_pos = ( r->read_pos + 1 ) % sinc_buffer_size;
    }
}
