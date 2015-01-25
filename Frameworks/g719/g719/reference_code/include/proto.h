/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#ifndef _PROTO_H
#define _PROTO_H

#include "cnst.h"
#include "state.h"

void encoder_init(
  CoderState *c,
  short num_bits
);

void encode_frame(
  float *audio, 
  short *BitStream, 
  CoderState *c
);

short detect_transient(
  float in[],
  CoderState *c
);

void wtda(
  float new_audio[],
  float wtda_audio[],
  float old_wtda[]
);

void direct_transform(
  float *in32,
  float *out32,
  short is_transient
 );

short noise_adjust(
  float *coeffs_norm,
  short *bitalloc
);

void interleave_spectrum(
  float *coefs
);

void flvqenc(
  short **bitstream,
  float *coefs,
  float *coefs_norm,
  short *R,
  short NumSpectumBits,
  short total_bits,
  short is_transient
);

void logqnorm(
  float *x, 
  short *k, 
  short L, 
  short N
);

void reordernorm(
  short *ynrm,
  short *normqlg2,
  short *idxbuf,
  short *normbuf
);

void diffcod(
  short *normqlg2,
  short N,
  short *y,
  short *difidx
);

void normalizecoefs(
  float *coefs,
  short *ynrm,
  short N1,
  short N2,
  short L,
  float *coefs_norm
);

void bitallocsum(
  short *R,
  short nb_sfm,
  short *sum
);

void qcoefs(
  float *coefs,
  short *R,
  short N1,
  short N2,
  short L,
  short *y
);

void lvq1(
  float *x,
  short *k
);

void lvq2(
  short *x,
  short *k,
  short r,
  short R
);

void code2idx(
  short *x,
  short *k,
  short r
);

short huffcheck(
  short *y,
  short *R,
  short N1,
  short N2,
  short L
);

short packingc(
  short *y,
  short *R,
  short *pbits,
  short flag,
  short N1,
  short N2,
  short L
);

void procnobitsbfm(
  float *coefs_norm,
  short *R,
  short *idx,
  short *ycof,
  short **ppbits,
  short nb_sfm,
  short diff
);

void procnf(
  float *coefs,
  short *y,
  short *pbits,
  short nb_vecs
);

void idx2bitsn(
  short x,
  short N,
  short *y
);

void idx2bitsc(
  short *x,
  short N,
  short L,
  short *y
);

void decoder_init(
  DecoderState *d,
  short num_bits
);

void decoder_reset_tables(
  DecoderState *d,
  short num_bits
);

void decode_frame(
  short bitstream[], 
  short bfi, 
  short out16[],
  DecoderState *d
);

void flvqdec(
  short **bitstream,
  float *coefsq,
  float *coefsq_norm,
  short *R,
  short NumSpectumBits,
  short *ynrm,
  short is_transient
);

void hdecnrm(
  short *bitstream,
  short N,
  short *index
);

void hdec2blvq(
  short *bitstream,
  short N,
  short *index
);

void hdec3blvq(
  short *bitstream,
  short N,
  short *index
);

void hdec4blvq(
  short *bitstream,
  short N,
  short *index
);

void hdec5blvq(
  short *bitstream,
  short N,
  short *index
);

short unpackc(
  short *R,
  short *pbits,
  short flag,
  short rv,
  short N1,
  short N2,
  short L,
  short *y
);

void dqcoefs(
  short *y,
  short *idxnrm,
  short *R,
  short N1,
  short N2,
  short L,
  float *coefs,
  float *coefs_norm
);

void dprocnobitsbfm(
  short *R,
  short *idx,
  short *ynrm,
  short *ycof,
  short **ppbits,
  float *coefsq,
  float *coefsq_norm,
  short nb_sfm,
  short diff
);

void dprocnf(
  short *y,
  short *pbits,
  short idxnrm,
  short nb_vecs,
  float *coefs,
  float* coefs_norm
);


void fill_spectrum(
  float *coeff, 
  float *coeff_out, 
  short *R, 
  short is_transient, 
  short norm[],
  short nf_idx
);

void de_interleave_spectrum(
  float *coefs
);

void inverse_transform(
  float *InMDCT,
  float *Out,
  int IsTransient
);

void window_ola(
  float ImdctOut[],
  short auOut[],
  float OldauOut[]
);

void bits2idxn(
  short *y,
  short N,
  short *x
);

void bits2idxc(
  short *y,
  short N,
  short L,
  short *x
);

void dct4_960(
  float v[MLT960_LENGTH],
  float coefs32[MLT960_LENGTH]
);

void dct4_240(
  float v[],
  float coefs32[]
);

void map_quant_weight(
  short normqlg2[],
  short wnorm[],
  short is_transient
);

void recovernorm(
  short *idxbuf,
  short *ynrm,
  short *normqlg2
);

void reordvct(
  short *y,
  short N,
  short *idx
);

void bitalloc(
  short *y,
  short *idx,
  short sum,
  short N,
  short M,
  short *r
);

void codesearch(
  short *x,
  short *C,
  short R
);

void idx2code(
  short *k,
  short *y,
  short R
);

#endif
