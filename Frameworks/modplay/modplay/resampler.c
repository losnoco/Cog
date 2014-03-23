#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#if (defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__amd64__))
#include <xmmintrin.h>
#define RESAMPLER_SSE
#endif

#ifdef _MSC_VER
#define ALIGNED     _declspec(align(16))
#else
#define ALIGNED     __attribute__((aligned(16)))
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "resampler.h"

enum { RESAMPLER_SHIFT = 13 };
enum { RESAMPLER_RESOLUTION = 1 << RESAMPLER_SHIFT };
enum { SINC_WIDTH = 16 };
enum { SINC_SAMPLES = RESAMPLER_RESOLUTION * SINC_WIDTH };
enum { CUBIC_SAMPLES = RESAMPLER_RESOLUTION * 4 };

ALIGNED static float cubic_lut[CUBIC_SAMPLES];

static float sinc_lut[SINC_SAMPLES + 1];

enum { resampler_buffer_size = SINC_WIDTH * 4 };

static int fEqual(const float b, const float a)
{
    return fabs(a - b) < 1.0e-6;
}

static float sinc(float x)
{
    return fEqual(x, 0.0) ? 1.0 : sin(x * M_PI) / (x * M_PI);
}

#ifdef RESAMPLER_SSE
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

static int resampler_has_sse = 0;
#endif

void resampler_init(void)
{
    unsigned i;
    double dx = (float)(SINC_WIDTH) / SINC_SAMPLES, x = 0.0;
    for (i = 0; i < SINC_SAMPLES + 1; ++i, x += dx)
    {
        float y = x / SINC_WIDTH;
#if 1
        // Blackman
        float window = 0.42659 - 0.49656 * cos(M_PI + M_PI * y) + 0.076849 * cos(2.0 * M_PI * y);
#elif 0
        // C.R.Helmrich's 2 term window
        float window = 0.79445 * cos(0.5 * M_PI * y) + 0.20555 * cos(1.5 * M_PI * y);
#elif 0
        // Lanczos
        float window = sinc(y);
#endif
        sinc_lut[i] = fabs(x) < SINC_WIDTH ? sinc(x) * window : 0.0;
    }
    dx = 1.0 / (float)(RESAMPLER_RESOLUTION);
    x = 0.0;
    for (i = 0; i < RESAMPLER_RESOLUTION; ++i, x += dx)
    {
        cubic_lut[i*4]   = (float)(-0.5 * x * x * x +       x * x - 0.5 * x);
        cubic_lut[i*4+1] = (float)( 1.5 * x * x * x - 2.5 * x * x           + 1.0);
        cubic_lut[i*4+2] = (float)(-1.5 * x * x * x + 2.0 * x * x + 0.5 * x);
        cubic_lut[i*4+3] = (float)( 0.5 * x * x * x - 0.5 * x * x);
    }
#ifdef RESAMPLER_SSE
    resampler_has_sse = query_cpu_feature_sse();
#endif
}

typedef struct resampler
{
    int write_pos, write_filled;
    int read_pos, read_filled;
    unsigned short phase;
    unsigned short phase_inc;
    unsigned char quality;
    float buffer_in[resampler_buffer_size * 2];
    float buffer_out[resampler_buffer_size + SINC_WIDTH * 2 - 1];
} resampler;

void * resampler_create(void)
{
    resampler * r = ( resampler * ) malloc( sizeof(resampler) );
    if ( !r ) return 0;

    r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
    r->phase_inc = 0;
    r->quality = RESAMPLER_QUALITY_MAX;
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
    const resampler * r_in = ( const resampler * ) _r;
    resampler * r_out = ( resampler * ) malloc( sizeof(resampler) );
    if ( !r_out ) return 0;

    r_out->write_pos = r_in->write_pos;
    r_out->write_filled = r_in->write_filled;
    r_out->read_pos = r_in->read_pos;
    r_out->read_filled = r_in->read_filled;
    r_out->phase = r_in->phase;
    r_out->phase_inc = r_in->phase_inc;
    r_out->quality = r_in->quality;
    memcpy( r_out->buffer_in, r_in->buffer_in, sizeof(r_in->buffer_in) );
    memcpy( r_out->buffer_out, r_in->buffer_out, sizeof(r_in->buffer_out) );

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
    r_out->quality = r_in->quality;
    memcpy( r_out->buffer_in, r_in->buffer_in, sizeof(r_in->buffer_in) );
    memcpy( r_out->buffer_out, r_in->buffer_out, sizeof(r_in->buffer_out) );
}

void resampler_set_quality(void *_r, int quality)
{
    resampler * r = ( resampler * ) _r;
    if (quality < RESAMPLER_QUALITY_MIN)
        quality = RESAMPLER_QUALITY_MIN;
    else if (quality > RESAMPLER_QUALITY_MAX)
        quality = RESAMPLER_QUALITY_MAX;
    r->quality = (unsigned char)quality;
}

int resampler_get_free_count(void *_r)
{
    resampler * r = ( resampler * ) _r;
    return resampler_buffer_size - r->write_filled;
}

static int resampler_min_filled(resampler *r)
{
    switch (r->quality)
    {
    default:
    case RESAMPLER_QUALITY_ZOH:
        return 1;
            
    case RESAMPLER_QUALITY_LINEAR:
        return 2;
            
    case RESAMPLER_QUALITY_CUBIC:
        return 4;
            
    case RESAMPLER_QUALITY_SINC:
        return SINC_WIDTH * 2;
    }
}

int resampler_ready(void *_r)
{
    resampler * r = ( resampler * ) _r;
    return r->write_filled > resampler_min_filled(r);
}

void resampler_clear(void *_r)
{
    resampler * r = ( resampler * ) _r;
    r->write_pos = 0;
    r->write_filled = 0;
    r->read_pos = 0;
    r->read_filled = 0;
    r->phase = 0;
}

void resampler_set_rate(void *_r, double new_factor)
{
    resampler * r = ( resampler * ) _r;
    r->phase_inc = (int)( new_factor * RESAMPLER_RESOLUTION );
}

void resampler_write_sample(void *_r, short s)
{
    resampler * r = ( resampler * ) _r;

    if ( r->write_filled < resampler_buffer_size )
    {
        float s32 = s;
        s32 *= (1.0 / 32768.0);

        r->buffer_in[ r->write_pos ] = s32;
        r->buffer_in[ r->write_pos + resampler_buffer_size ] = s32;

        ++r->write_filled;

        r->write_pos = ( r->write_pos + 1 ) % resampler_buffer_size;
    }
}

static int resampler_run_zoh(resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + resampler_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= 1;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;
        
        do
        {
            float sample;
            
            if ( out >= out_end )
                break;

            sample = *in;
            *out++ = sample;
            
            phase += phase_inc;
            
            in += phase >> RESAMPLER_SHIFT;
            
            phase &= RESAMPLER_RESOLUTION-1;
        }
        while ( in < in_end );
        
        r->phase = (unsigned short) phase;
        *out_ = out;
        
        used = (int)(in - in_);
        
        r->write_filled -= used;
    }
    
    return used;
}

static int resampler_run_linear(resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + resampler_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= 2;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;
        
        do
        {
            float sample;
            
            if ( out >= out_end )
                break;
            
            sample = in[0] + (in[1] - in[0]) * ((float)phase / RESAMPLER_RESOLUTION);
            *out++ = sample;
            
            phase += phase_inc;
            
            in += phase >> RESAMPLER_SHIFT;
            
            phase &= RESAMPLER_RESOLUTION-1;
        }
        while ( in < in_end );
        
        r->phase = (unsigned short) phase;
        *out_ = out;
        
        used = (int)(in - in_);
        
        r->write_filled -= used;
    }
    
    return used;
}

static int resampler_run_cubic(resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + resampler_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= 4;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;
        
        do
        {
            float * kernel;
            int i;
            float sample;
            
            if ( out >= out_end )
                break;
            
            kernel = cubic_lut + phase * 4;
            
            for (sample = 0, i = 0; i < 4; ++i)
                sample += in[i] * kernel[i];
            *out++ = sample;
            
            phase += phase_inc;
            
            in += phase >> RESAMPLER_SHIFT;
            
            phase &= RESAMPLER_RESOLUTION-1;
        }
        while ( in < in_end );
        
        r->phase = (unsigned short) phase;
        *out_ = out;
        
        used = (int)(in - in_);
        
        r->write_filled -= used;
    }
    
    return used;
}

#ifdef RESAMPLER_SSE
static int resampler_run_cubic_sse(resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + resampler_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= 4;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;
        
        do
        {
            __m128 temp1, temp2;
            __m128 samplex = _mm_setzero_ps();
            
            if ( out >= out_end )
                break;
            
            temp1 = _mm_loadu_ps( (const float *)( in ) );
            temp2 = _mm_load_ps( (const float *)( cubic_lut + phase * 4 ) );
            temp1 = _mm_mul_ps( temp1, temp2 );
            samplex = _mm_add_ps( samplex, temp1 );
            temp1 = _mm_movehl_ps( temp1, samplex );
            samplex = _mm_add_ps( samplex, temp1 );
            temp1 = samplex;
            temp1 = _mm_shuffle_ps( temp1, samplex, _MM_SHUFFLE(0, 0, 0, 1) );
            samplex = _mm_add_ps( samplex, temp1 );
            _mm_store_ss( out, samplex );
            ++out;
            
            phase += phase_inc;
            
            in += phase >> RESAMPLER_SHIFT;
            
            phase &= RESAMPLER_RESOLUTION - 1;
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

static int resampler_run_sinc(resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + resampler_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= SINC_WIDTH * 2;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;

        int step = phase_inc > RESAMPLER_RESOLUTION ? RESAMPLER_RESOLUTION * RESAMPLER_RESOLUTION / phase_inc : RESAMPLER_RESOLUTION;

        do
        {
            float kernel[SINC_WIDTH * 2], kernel_sum = 0.0;
            int i = SINC_WIDTH;
            int phase_adj = phase * step / RESAMPLER_RESOLUTION;
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
            *out++ = (float)(sample / kernel_sum);

            phase += phase_inc;

            in += phase >> RESAMPLER_SHIFT;

            phase &= RESAMPLER_RESOLUTION-1;
        }
        while ( in < in_end );

        r->phase = (unsigned short) phase;
        *out_ = out;

        used = (int)(in - in_);

        r->write_filled -= used;
    }

    return used;
}

#ifdef RESAMPLER_SSE
static int resampler_run_sinc_sse(resampler * r, float ** out_, float * out_end)
{
    int in_size = r->write_filled;
    float const* in_ = r->buffer_in + resampler_buffer_size + r->write_pos - r->write_filled;
    int used = 0;
    in_size -= SINC_WIDTH * 2;
    if ( in_size > 0 )
    {
        float* out = *out_;
        float const* in = in_;
        float const* const in_end = in + in_size;
        int phase = r->phase;
        int phase_inc = r->phase_inc;
        
        int step = phase_inc > RESAMPLER_RESOLUTION ? RESAMPLER_RESOLUTION * RESAMPLER_RESOLUTION / phase_inc : RESAMPLER_RESOLUTION;
        
        do
        {
            // accumulate in extended precision
            float kernel_sum = 0.0;
            __m128 kernel[SINC_WIDTH / 2];
            __m128 temp1, temp2;
            __m128 samplex = _mm_setzero_ps();
            float *kernelf = (float*)(&kernel);
            int i = SINC_WIDTH;
            int phase_adj = phase * step / RESAMPLER_RESOLUTION;
            
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
            kernel_sum = 1.0 / kernel_sum;
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
            
            in += phase >> RESAMPLER_SHIFT;
            
            phase &= RESAMPLER_RESOLUTION - 1;
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

static void resampler_fill(resampler * r)
{
    int min_filled = resampler_min_filled(r);
    int quality = r->quality;
    while ( r->write_filled > min_filled &&
            r->read_filled < resampler_buffer_size )
    {
        int write_pos = ( r->read_pos + r->read_filled ) % resampler_buffer_size;
        int write_size = resampler_buffer_size - write_pos;
        float * out = r->buffer_out + write_pos;
        if ( write_size > ( resampler_buffer_size - r->read_filled ) )
            write_size = resampler_buffer_size - r->read_filled;
        switch (quality)
        {
        case RESAMPLER_QUALITY_ZOH:
            resampler_run_zoh( r, &out, out + write_size );
            break;
                
        case RESAMPLER_QUALITY_LINEAR:
            resampler_run_linear( r, &out, out + write_size );
            break;
                
        case RESAMPLER_QUALITY_CUBIC:
#ifdef RESAMPLER_SSE
            if ( resampler_has_sse )
                resampler_run_cubic_sse( r, &out, out + write_size );
            else
#endif
                resampler_run_cubic( r, &out, out + write_size );
            break;
                
        case RESAMPLER_QUALITY_SINC:
#ifdef RESAMPLER_SSE
            if ( resampler_has_sse )
                resampler_run_sinc_sse( r, &out, out + write_size );
            else
#endif
                resampler_run_sinc( r, &out, out + write_size );
            break;
        }
        r->read_filled += out - r->buffer_out - write_pos;
    }
}

int resampler_get_sample_count(void *_r)
{
    resampler * r = ( resampler * ) _r;
    if ( r->read_filled < 1 )
        resampler_fill( r );
    return r->read_filled;
}

float resampler_get_sample(void *_r)
{
    resampler * r = ( resampler * ) _r;
    if ( r->read_filled < 1 )
        resampler_fill( r );
    if ( r->read_filled < 1 )
        return 0;
    return r->buffer_out[ r->read_pos ];
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
