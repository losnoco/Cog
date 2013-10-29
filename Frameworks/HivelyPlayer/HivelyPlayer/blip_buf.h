/** \file
Sample buffer that resamples from input clock rate to output sample rate */

#include <stdlib.h>

/* blip_buf 1.1.0 */
#ifndef HVL_BLIP_BUF_H
#define HVL_BLIP_BUF_H

#ifdef __cplusplus
	extern "C" {
#endif

/** First parameter of most functions is blip_t*, or const blip_t* if nothing
is changed. */
typedef struct hvl_blip_t hvl_blip_t;

/** Returns the size of a blip_t object of the given sample count. */
size_t hvl_blip_size( int sample_count );

/** Creates new buffer that can hold at most sample_count samples. Sets rates
so that there are blip_max_ratio clocks per sample. */
void hvl_blip_new_inplace( hvl_blip_t*, int sample_count );

/** Sets approximate input clock rate and output sample rate. For every
clock_rate input clocks, approximately sample_rate samples are generated. */
void hvl_blip_set_rates( hvl_blip_t*, double clock_rate, double sample_rate );

enum { /** Maximum clock_rate/sample_rate ratio. For a given sample_rate,
clock_rate must not be greater than sample_rate*blip_max_ratio. */
hvl_blip_max_ratio = 1 << 20 };

/** Clears entire buffer. Afterwards, blip_samples_avail() == 0. */
void hvl_blip_clear( hvl_blip_t* );

/** Adds positive/negative delta into buffer at specified clock time. */
void hvl_blip_add_delta( hvl_blip_t*, unsigned int clock_time, int delta );

/** Same as blip_add_delta(), but uses faster, lower-quality synthesis. */
void hvl_blip_add_delta_fast( hvl_blip_t*, unsigned int clock_time, int delta );

/** Length of time frame, in clocks, needed to make sample_count additional
samples available. */
int hvl_blip_clocks_needed( const hvl_blip_t*, int sample_count );

enum { /** Maximum number of samples that can be generated from one time frame. */
hvl_blip_max_frame = 4000 };

/** Makes input clocks before clock_duration available for reading as output
samples. Also begins new time frame at clock_duration, so that clock time 0 in
the new time frame specifies the same clock as clock_duration in the old time
frame specified. Deltas can have been added slightly past clock_duration (up to
however many clocks there are in two output samples). */
void hvl_blip_end_frame( hvl_blip_t*, unsigned int clock_duration );

/** Number of buffered samples available for reading. */
int hvl_blip_samples_avail( const hvl_blip_t* );

/** Reads and removes at most 'count' samples and writes them to 'out'. If
'stereo' is true, writes output to every other element of 'out', allowing easy
interleaving of two buffers into a stereo sample stream. Outputs 16-bit signed
samples. Returns number of samples actually read.  */
int hvl_blip_read_samples( hvl_blip_t*, int out [], int count, int gain );

#ifdef __cplusplus
	}
#endif

#endif
