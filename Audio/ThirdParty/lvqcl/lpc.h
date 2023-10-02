#ifndef MY_LPC_H
#define MY_LPC_H

/*    data        - beginning of the data
 *    data_len    - length of data (in samples) that are base for extrapolation
 *    nch            - number of (interleaved) channels
 *    lpc_order    - LPC order
 *    extra_bkwd    - number of samples to pre-extrapolate
 *    extra_fwd    - number of samples to post-extrapolate
 *
 *    D = data; N = num_channels; LEN = data_len*N; EX = extra*N
 *
 *    memory layout when invdir == false:
 *
 *    [||||||||||||||||||||||||||||||||][||||||||||||||||||||][
 *    ^ D[0]                            ^ D[LEN]              ^ D[LEN+EX]
 *
 *    memory layout when invdir == true:
 *    ][||||||||||||||||||||][||||||||||||||||||||||||||||||||][
 *                           ^ D[0]                            ^ D[LEN]
 *    ^ D[-1*N-EX]          ^ D[-1*N]
 *
 */

static const size_t LPC_ORDER = 32;

void lpc_extrapolate2(float * const data, const size_t data_len, const int nch, const int lpc_order, const size_t extra_bkwd, const size_t extra_fwd, void ** extrapolate_buffer, size_t * extrapolate_buffer_size);

static inline void lpc_extrapolate_bkwd(float * const data, const size_t data_len, const size_t prime_len, const int nch, const int lpc_order, const size_t extra_bkwd, void ** extrapolate_buffer, size_t * extrapolate_buffer_size)
{
    (void)data_len;
    lpc_extrapolate2(data, prime_len, nch, lpc_order, extra_bkwd, 0, extrapolate_buffer, extrapolate_buffer_size);
}

static inline void lpc_extrapolate_fwd(float * const data, const size_t data_len, const size_t prime_len, const int nch, const int lpc_order, const size_t extra_fwd, void ** extrapolate_buffer, size_t * extrapolate_buffer_size)
{
    lpc_extrapolate2(data + (data_len - prime_len)*nch, prime_len, nch, lpc_order, 0, extra_fwd, extrapolate_buffer, extrapolate_buffer_size);
}

#endif
