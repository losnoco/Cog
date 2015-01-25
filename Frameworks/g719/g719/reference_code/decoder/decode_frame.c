/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "state.h"
#include "cnst.h"
#include "proto.h"

/*--------------------------------------------------------------------------*/
/*  Function  decode_frame                                                  */
/*  ~~~~~~~~~~~~~~~~~~~~~~                                                  */
/*                                                                          */
/*  Decodes a single frame                                                  */
/*--------------------------------------------------------------------------*/
/*  short         bitstream[]  (i)    bitstream to decode                   */
/*  short         bfi          (i)    bad frame indicator                   */
/*  short         out16[]      (o)    decoded audio                         */
/*  DecoderState  *d           (i/o)  state of decoder                      */
/*--------------------------------------------------------------------------*/
void decode_frame(short bitstream[], 
                  short bfi, 
                  short out16[],
                  DecoderState *d)
{
   short is_transient;
   float t_audio_q[FRAME_LENGTH];
   float wtda_audio[2*FRAME_LENGTH];
   short bitalloc[NB_SFM];   
   short ynrm[NB_SFM];
   short i;
   float audio_q_norm[FREQ_LENGTH];
   short nf_idx;
   short **pbitstream;
   short *tbitstream;

   if (bfi) 
   {      
      for (i=0; i < FRAME_LENGTH; i++) 
      {
         t_audio_q[i] = d->old_coeffs[i];
         d->old_coeffs[i] = d->old_coeffs[i]/2;
      }
      is_transient = d->old_is_transient;
   }
   else
   {
      if (*bitstream == G192_BIT1) 
      {
         is_transient = 1;
      } 
      else 
      {
         is_transient = 0;
      }   

      bitstream++;

      tbitstream = bitstream;
      pbitstream = &bitstream;
      if (is_transient) 
      {
         flvqdec(pbitstream, t_audio_q, audio_q_norm, bitalloc, (short)d->num_bits_spectrum_transient, ynrm, is_transient); 
         nf_idx = 0;
      }
      else
      {
         flvqdec(pbitstream, t_audio_q, audio_q_norm, bitalloc, (short)d->num_bits_spectrum_stationary, ynrm, is_transient); 
         bits2idxn(bitstream, 2, &nf_idx);
         bitstream += 2;
      }
      
      for (i = FREQ_LENGTH; i < FRAME_LENGTH; i++) 
      {
         t_audio_q[i] = 0.0f;
      }

      fill_spectrum(audio_q_norm, t_audio_q,  bitalloc, is_transient, ynrm, nf_idx);

      if (is_transient) 
      {
         de_interleave_spectrum(t_audio_q);
      }

      for (i=0; i < FRAME_LENGTH; i++) 
      {
         d->old_coeffs[i] = t_audio_q[i];

      }
      d->old_is_transient = is_transient;
   }
   
   inverse_transform(t_audio_q, wtda_audio, is_transient);
   window_ola(wtda_audio, out16, d->old_out);
}
