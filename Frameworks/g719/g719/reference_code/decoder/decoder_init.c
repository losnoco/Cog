/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "state.h"
#include "cnst.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  decoder_reset_tables                                          */
/*  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                                          */
/*                                                                          */
/*  Reset decoder tables                                                    */
/*--------------------------------------------------------------------------*/
/*  decoderState  *d          (i)   state of decoder                        */
/*  short         num_bits    (i)   number of bits                          */
/*--------------------------------------------------------------------------*/
void decoder_reset_tables(DecoderState *d, short num_bits)
{
   d->num_bits_spectrum_stationary = num_bits - 3;
   d->num_bits_spectrum_transient  = num_bits - 1;
   d->num_bits                     = num_bits;
}

/*--------------------------------------------------------------------------*/
/*  Function  decoder_init                                                  */
/*  ~~~~~~~~~~~~~~~~~~~~~~                                                  */
/*                                                                          */
/*  Initialize the state of the decoder                                     */
/*--------------------------------------------------------------------------*/
/*  DecoderState  *d          (i)   state of decoder                        */
/*  short         num_bits    (i)   number of bits                          */
/*--------------------------------------------------------------------------*/
void decoder_init(DecoderState *d, short num_bits)
{
   short i;

   for (i=0 ; i < FRAME_LENGTH ; i++)
   {  
      d->old_out[i] = 0.0f;
   }
     
   d->old_is_transient = 0;

   decoder_reset_tables(d, num_bits);
}
