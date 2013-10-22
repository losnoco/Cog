/** \file
Sample buffer that resamples from input clock rate to output sample rate */

/* blip_buf 1.1.0 */
#ifndef BLIP_BUF_H 
#define BLIP_BUF_H

#include <limits.h>

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

typedef int buf_t;

/** Sample buffer that resamples to output rate and accumulates samples
until they're read out */
struct blip_t
{
  fixed_t offset;
  int index;
  int avail;
  int size;
  int integrator;
  int last_value;
  buf_t samples[128];
};

#ifdef __cplusplus
	extern "C" {
#endif

/** First parameter of most functions is blip_t*, or const blip_t* if nothing
is changed. */
typedef struct blip_t blip_t;

/** Clears entire buffer. Afterwards, blip_samples_avail() == 0. */
void ptm_blip_clear( blip_t* );

/** Adds positive/negative delta into buffer at specified clock time. */
void ptm_blip_add_delta( blip_t*, float clock_time, int delta );

/** Number of buffered samples available for reading. */
int ptm_blip_samples_avail( const blip_t* );

/** Reads and removes at most 'count' samples and writes them to 'out'. If
'stereo' is true, writes output to every other element of 'out', allowing easy
interleaving of two buffers into a stereo sample stream. Outputs 16-bit signed
samples. Returns number of samples actually read.  */
int ptm_blip_read_sample( blip_t* );


/* Deprecated */
typedef blip_t blip_buffer_t;

#ifdef __cplusplus
	}
#endif

#endif
